/*
Copyright (c) 2010, Yahoo! Inc. All rights reserved.

Redistribution and use of this software in source and binary forms, with or
without modification, are permitted provided that the following conditions are
met: 

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation and/or 
other materials provided with the distribution.

* Neither the name of Yahoo! Inc. nor the names of its contributors may be used 
to endorse or promote products derived from this software without specific prior 
written permission of Yahoo! Inc.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <stack>
#include <list>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "debug.h"
#include "logging.h"
#include "util.h"
#include "shmLookup.h"
#include "processExitReason.h"
#include "reportingChannel.h"
#include "pidMap.h"
#include "process.h"
#include "service.h"
#include "manager.h"

using namespace std;


//////////////////////////////////////////////////////////////////////
// A service object and controlling state thread is created for each
// valid service configuration found. The main purpose is to create
// the associated sockets and shm, manage the number of processes and
// accept reports from the Reporting Channel.
//////////////////////////////////////////////////////////////////////

int service::currentObjectCount = 0;
int service::maximumObjectCount = 0;
service::trackMap service::serviceTracker;


service::service(const std::string& setName, manager* setM)
  : threadedObject(setM),
    _CFInode(0), _CFSize(0), _CFMtime(0),
    _acceptSocket(-1), _shmServiceFD(-1), _stAcceptingFD(0),
    _stReportingFD(0), _lastPerformanceReport(st_time()),
    _name(setName), _M(setM), _type(isLocalService),
    _shutdownFlag(false), _nextStartAttempt(0), _shutdownReason(""),
    _childCount(0), _activeProcessCount(0),
    _prevCalibrationTime(time(0)),
    _nextCalibrationTime(_prevCalibrationTime+minimumSamplePeriod),
    _prevOccupancyByTime(0), _currOccupancyByTime(0),
    _prevListenBacklog(0), _currListenBacklog(0),
    _calibrateSamples(0),
    _timeInProcess(0), _requestsCompleted(0)
{
  ++currentObjectCount;
  if (currentObjectCount > maximumObjectCount) maximumObjectCount = currentObjectCount;
  serviceTracker[this] = this;
  _reportingChannelPipes[0] = -1;
  _reportingChannelPipes[1] = -1;
}

//////////////////////////////////////////////////////////////////////

service::~service()
{
  if (_stReportingFD) st_netfd_free(_stReportingFD);
  if (_stAcceptingFD) st_netfd_free(_stAcceptingFD);
  if (_acceptSocket != -1) close(_acceptSocket);
  if (_shmServiceFD != -1) close(_shmServiceFD);

  if (_reportingChannelPipes[0] != -1) close(_reportingChannelPipes[0]);
  if (_reportingChannelPipes[1] != -1) close(_reportingChannelPipes[1]);

  --currentObjectCount;
  serviceTracker.erase(this);
}


//////////////////////////////////////////////////////////////////////
// Initialize the service and start the controlling thread.
//
// The notify channel is a pipe thru which the processes send reports
// about themselves to the service thread. It is watched with a
// st_thread and assumes a consistent write size by the writer.
//////////////////////////////////////////////////////////////////////

bool
service::initialize()
{
  _logID = _name;

  if (!loadConfiguration(_configurationPath)) return false;
  if (!createAcceptSocket(_rendezvousDirectory)) return false;
  if (!createServiceMap(_rendezvousDirectory)) return false;

  if (pipe(_reportingChannelPipes) == -1) {
    util::messageWithErrno(_errorMessage, "Could not create pipes for Reporting Channel");
    return false;
  }

  // Don't let children inherit the reporting channel directly (they
  // get it via dup2())

  if (util::setCloseOnExec(_reportingChannelPipes[0]) == -1) {
    util::messageWithErrno(_errorMessage, "Could not set FD_CLOEXEC on ReportingChannelReader");
    return false;
  }

  if (util::setCloseOnExec(_reportingChannelPipes[1]) == -1) {
    util::messageWithErrno(_errorMessage, "Could not set FD_CLOEXEC on ReportingChannelWriter");
    return false;
  }

  _stReportingFD = st_netfd_open(getReportingChannelReaderSocket());
  if (!_stReportingFD) {
    util::messageWithErrno(_errorMessage, "st_netfd_open() failed for Reporting Channel");
    return false;
  }

  _stAcceptingFD = st_netfd_open(_acceptSocket);
  if (!_stAcceptingFD) {
    util::messageWithErrno(_errorMessage, "st_netfd_open() failed for Accepting Socket");
    return false;
  }

  //////////////////////////////////////////////////////////////////////
  // Create the shared memory used to communicate with the children
  //////////////////////////////////////////////////////////////////////

  pluton::faultCode fc = _shmService.mapManager(_shmServiceFD, _config.maximumProcesses,
						_config.maximumThreads);
  if (fc != pluton::noFault) {
    util::messageWithErrno(_errorMessage, "mmap() failed for serviceMap", 0, (int) fc);
    return false;
  }

  //////////////////////////////////////////////////////////////////////
  // Populate the shmService config parameters as needed. Not all
  // parameters are copied across.
  //////////////////////////////////////////////////////////////////////

  shmService* p = _shmService.getServicePtr();

  p->_config._maximumProcesses = _config.maximumProcesses;
  p->_config._maximumThreads = _config.maximumThreads;
  p->_config._maximumRequests = _config.maximumRequests;
  p->_config._maximumRetries = _config.maximumRetries;
  p->_config._idleTimeout = _config.idleTimeout;
  p->_config._affinityTimeout = _config.affinityTimeout;
  p->_config._recorderCycle = _config.recorderCycle;

  strncpy(p->_config._recorderPrefix,
	  _config.recorderPrefix.c_str(), sizeof(p->_config._recorderPrefix)-1);
  p->_config._recorderPrefix[sizeof(p->_config._recorderPrefix)-1] = '\0';	// Ensure a C str

  // Initialize per-service and per-instance counters

  _shmService.resetAggregateCounters();
  for (int pid=0; pid < _config.maximumProcesses; ++pid) {
    _shmService.resetProcess(pid);
  }

  //////////////////////////////////////////////////////////////////////
  // Finally, start the control thread for this service
  //////////////////////////////////////////////////////////////////////

  for (int id=0; id < _config.maximumProcesses; ++id) _unusedIds.push_back(id);

  if (!service::startThread(this)) {
    util::messageWithErrno(_errorMessage, " service::startThread() failed");
    return false;
  }

  return true;
}


//////////////////////////////////////////////////////////////////////
// Test to see if a process can be started
//////////////////////////////////////////////////////////////////////

bool
service::createAllowed(bool applyRateLimiting, bool justTesting)
{
  if (_unusedIds.empty()) return false;			// Maxed out already
  time_t now = st_time();
  if (_nextStartAttempt > now) return false;	// If forking is failing, throttle
  if (applyRateLimiting && !_startLimiter.allowed(now, justTesting)) return false;

  return true;
}


//////////////////////////////////////////////////////////////////////
// Indicate how long before a new process can be created. Normally
// this should be zero, but rate limiting and fork() errors can
// constraint the rate so that we don't flog the OS with a boat-load
// of failing forks (or should that be flailing forks?)
//////////////////////////////////////////////////////////////////////

int
service::secondsToNextCreation(int upperLimit)
{
  if (_unusedIds.empty()) return upperLimit;	// Maxed out already
  time_t now = st_time();
  if (_nextStartAttempt > now) {		// If forking is failing, throttle
    return min(upperLimit, (int) (_nextStartAttempt - now));
  }

  if (!_startLimiter.allowed(now, true)) {	// Too many per second?
    return min(upperLimit, _startLimiter.getTimeUnit());
  }

  return 0;					// Go for it, Big Buddy!
}


//////////////////////////////////////////////////////////////////////
// preRun() is "in-thread" initialization. In this case, start the
// pre-started minimum number of services.
//////////////////////////////////////////////////////////////////////

void
service::preRun()
{
  if (_config.prestartProcessesFlag) {
    int startedCount = 0;
    for (int ix=0; ix < _config.minimumProcesses; ++ix) {
      if (createProcess("preStart", false)) ++startedCount;
    }
    if (startedCount != _config.minimumProcesses) {
      LOGPRT << "Service Warning: " << _logID
	     << ": Could not start minimumProcesses of " << _config.minimumProcesses
	     << " only started " << startedCount << endl;
    }
  }
  if (logging::serviceStart()) LOGPRT << "Service Ready: " << _logID << endl;
}


//////////////////////////////////////////////////////////////////////
// The main loop of the service thread serves three main purposes, it:
//
// o listens to the accept socket if _activeProcessCount is zero
// o accepts reports from processes via the reporting channel
// o monitors and adjusts the number of processes
//////////////////////////////////////////////////////////////////////

bool
service::run()
{
  if (debug::service()) DBGPRT << "service::run " << _logID << endl;

  while (!shutdownInProgress()) {

    _shmService.updateManagerHeartbeat(st_time());

    //////////////////////////////////////////////////////////////////////
    // If there are no processes running, listen on the accept socket
    // ourselves and delay creating a service when the first connect()
    // comes is.
    //////////////////////////////////////////////////////////////////////

    if (_activeProcessCount == 0) {
      if (debug::service()) DBGPRT << "service::run " << _logID << " listen for accept" << endl;
      listenForAccept();
      continue;
    }

    //////////////////////////////////////////////////////////////////////
    // There are some processes run, check for reports. Note that the
    // sleep interval has a bearing on how often the calibration
    // routine is called if the report rate is very low.
    //////////////////////////////////////////////////////////////////////

    enableInterrupts();
    int res = st_netfd_poll(_stReportingFD, POLLIN, pollInterval * util::MICROSECOND);
    disableInterrupts();

    if (debug::service()) DBGPRT << "service::run " << _logID << " reporting=" << res << endl;
    if (res == 0) {
      readReportingChannel();
      continue;
    }
      
    ////////////////////////////////////////////////////////////
    // No reports, consider calibrating down
    ////////////////////////////////////////////////////////////

    int backlog = getMANAGER()->getListenBacklog(_stAcceptingFD);
    if (!shutdownInProgress()) calibrateProcesses("offTimer", backlog, true, false);
  }

  if (debug::service()) DBGPRT << "service::run " << _logID << " shutdownInProgress" << endl;

  return false;
}


//////////////////////////////////////////////////////////////////////
// No processes are running so listen for an inbound connection and
// then start a process to handle it. This situation can occur if the
// service is configured to allow zero minimum-processes or all the
// service instances are exiting non-zero due to some catastrophic
// error. In the latter case, the connection is simply closed so that
// the client gets an immediate timeout.
//////////////////////////////////////////////////////////////////////

void
service::listenForAccept()
{
  int secsToCreate = secondsToNextCreation(pollInterval);
  if (debug::service()) DBGPRT << "Service: " << _logID
			       << " PC=0, STNC=" << secsToCreate << endl;

  // If it's too soon to accept new request, just drain them

  if (secsToCreate) {
    enableInterrupts();
    int res = st_netfd_poll(_stAcceptingFD, POLLIN, secsToCreate * util::MICROSECOND);
    if (res == 0) {
      st_netfd_t afd = st_accept(_stAcceptingFD, 0, 0, 0);
      if (afd) st_netfd_close(afd);
      if (debug::service()) DBGPRT << "Service: " << _logID << " draining Accept Queue for "
				   << _nextStartAttempt - st_time()
				   << endl;
    }
    disableInterrupts();
    return;
  }

  //////////////////////////////////////////////////////////////////////
  // Not too soon now. Can create a new instance once a connection is
  // pending.
  //////////////////////////////////////////////////////////////////////

  enableInterrupts();
  int res = st_netfd_poll(_stAcceptingFD, POLLIN, pollInterval * util::MICROSECOND);
  disableInterrupts();
  if (debug::service()) DBGPRT << "Service: " << _stAcceptingFD
			       << " " << _logID << " accept=" << res
			       << " errno=" << errno
			       << endl;
  if (res == 0) {				// A connect() is present on the accepting FD
    if (debug::process()) DBGPRT << _logID << " Process: Create due to accept traffic" << endl;
    createProcess("listen", false);		// Accept socket is readable
  }
}


//////////////////////////////////////////////////////////////////////
// Handle traffic on the reporting channel
//////////////////////////////////////////////////////////////////////

void
service::readReportingChannel()
{
  pluton::reportingChannel::record ev;

  if (debug::reportingChannel()) DBGPRT << "reportingChannel: " << _logID << " readFully" << endl;
  ssize_t bytes = st_read_fully(_stReportingFD, static_cast<void*>(&ev), sizeof(ev), 0);
  if (debug::reportingChannel()) {
    DBGPRT << "reportingChannel: " << _logID << " bytes " << bytes
	   << " pid=" << ev.pid << " t=" << ev.type
	   << " ec=" << ev.U.pd.entries << endl;
  }
  if ((bytes == -1) && (errno == ETIME)) return;

  if (bytes != sizeof(ev)) {
    LOGPRT << "Service Warning: " << _logID
	   << ": Unexpected byte count from Reporting Channel. Expected "
	   << sizeof(ev) << ", got " << bytes << " errno=" << errno << endl;
    return;
  }

  ////////////////////////////////////////////////////////////
  // Got the right sized lump of data, who sent it?
  ////////////////////////////////////////////////////////////

  process* P = pidMap::find(ev.pid);
  if (!P) {
    LOGPRT << "Service Warning: " << _logID
	   << ": Reporting Event from unknown pid "
	   << ev.pid << " ignored. (length=(" << bytes << ")" << endl;
    return;
  }

  bool fullReport = ev.U.pd.entries == pluton::reportingChannel::maximumPerformanceEntries;

  if (fullReport) _lastPerformanceReport = st_time();	// For idle timeout purposes

  //////////////////////////////////////////////////////////////////////
  // Tracking events occurs at Process, Service and Manager.
  //////////////////////////////////////////////////////////////////////

  switch (ev.type) {
  case pluton::reportingChannel::performanceReport:
    for (int ix=0;
	 (ix < ev.U.pd.entries) && (ix < pluton::reportingChannel::maximumPerformanceEntries);
	 ++ix) { 
      P->trackCosts(ev.U.pd.rqL[ix].function, ev.U.pd.rqL[ix]);
      trackCosts(ev.U.pd.rqL[ix].durationMicroSeconds);
    }
    _M->addRequestReported(ev.U.pd.entries);
    {
      int backlog = getMANAGER()->getListenBacklog(_stAcceptingFD);
      calibrateProcesses("report", backlog, true, fullReport);	// calibrate after report
    }
    break;

  default:
    LOGPRT << "Service Warning: " << _logID
	   << ": Unrecognized reporting event " << ev.type
	   << " from pid " << ev.pid << " ignored." << endl;
    break;
  }
}


//////////////////////////////////////////////////////////////////////
// Accumulate costs from reporting channel for calibration purposes.
//////////////////////////////////////////////////////////////////////

void
service::trackCosts(unsigned int durationMicroSeconds)
{
  _timeInProcess += durationMicroSeconds;
  ++_requestsCompleted;
}

//////////////////////////////////////////////////////////////////////
// "in-thread" start of shutdown. Basically the same as ::run except
// the wait is on the accept socket draining instead of the shutdown
// request - which has already been started.
//
// The file associated with the accept socket has been removed from
// the file system so no new connections are possible. This routine
// lets the current crop of connections finish prior to tell the
// processes to shut down.
//////////////////////////////////////////////////////////////////////

void
service::runUntilIdle()
{
  if (debug::service()) DBGPRT << "service::runUntilIdle: " << _logID
			       << " childCount=" << _childCount << endl;

  // Don't wait forever as they could wedge here

  time_t stopAfter = getUnresponsiveTimeout();
  if (stopAfter <= 0) stopAfter = getMANAGER()->getEmergencyExitDelay() / 2;
  if (stopAfter <= 0) stopAfter = 2;
  stopAfter += st_time();
  int backlog = 0;
  while ((_childCount > 0) && (st_time() < stopAfter)) {

    // Check for reports

    enableInterrupts();
    int res = st_netfd_poll(_stReportingFD, POLLIN, pollInterval * util::MICROSECOND);
    disableInterrupts();

    if (debug::service()) DBGPRT << "service::runUntilIdle: " << _logID
				 << " reportingChannel1=" << res << endl;
    if (res == 0) readReportingChannel();
      
    // Probe the accept socket for pending count.

    backlog = getMANAGER()->getListenBacklog(_stAcceptingFD);
    if (debug::service()) DBGPRT << "service::runUntilIdle: " << _logID
				 << " backlog=" << backlog
				 << " stopAfter=" << stopAfter << endl;
    if (backlog <= 0) break;
  }

  // Notify if unable to shutdown cleanly

  if ((backlog > 0) && logging::processExit()) {
    LOGPRT << "Service Warning: " << _logID << " Residual backlog at shutdown=" << backlog << endl;
  }

  //////////////////////////////////////////////////////////////////////
  // Now that all requests are in progress or done, notify the
  // per-process threads.
  //////////////////////////////////////////////////////////////////////

  for (processMapIter mi=_processMap.begin(); mi!=_processMap.end(); ++mi) {
    mi->second->setShutdownReason(processExit::serviceShutdown, true);
  }

  if (debug::service()) DBGPRT << "service processes notified " << _logID << endl;

  //////////////////////////////////////////////////////////////////////
  // Now complete the draining of the reporting channel until all
  // children disappear.
  //////////////////////////////////////////////////////////////////////

  while (_childCount > 0) {
    enableInterrupts();
    int res = st_netfd_poll(_stReportingFD, POLLIN, pollInterval * util::MICROSECOND);
    disableInterrupts();

    if (debug::service()) DBGPRT << "service::runUntilIdle: " << _logID
				 << " reportingChannel2=" << res << endl;
    if (res == 0) readReportingChannel();
  }
}


//////////////////////////////////////////////////////////////////////
// A process has shutdown completely - destroy the instance. 
//////////////////////////////////////////////////////////////////////

void
service::destroyOffspring(threadedObject* to, const char* backoffReason)
{
  process* P = dynamic_cast<process*>(to);

  if (debug::service()) DBGPRT << "service::destroyOffspring(" << P->getLogID() << ")"
			       << " backoff=" << (backoffReason ? backoffReason : "NULL") << endl;

  if (P->error())
    LOGPRT << "Process Error: " << P->getLogID() << " " << P->getErrorMessage() << endl;
  _unusedIds.push_front(P->getID());
  _processMap.erase(P);
  _M->subtractProcessCount(P->getExitReason());
  processExit::reason er = P->getExitReason();
  delete P;

  notifyOwner("service", "destroyOffspring");	// Tell owner so it can recalibrate/exit

  if (shutdownInProgress()) return;	// Ignore backoff if shutting down

  //////////////////////////////////////////////////////////////////////
  // Starting new service instances are likely to be deleterious to
  // the system since they are failing, however this can be
  // over-ridden if "exec-failure-backoff" is set to zero. Note that
  // if the actually exec() call fails, process::forkExecChild() will
  // sleep for a second so a completely bad exec is somewhat mitigated.
  //////////////////////////////////////////////////////////////////////

  if (backoffReason) {
    if (_config.execFailureBackoff == 0) {
      LOGPRT << "Service Warning: " << _logID
	     << ": prefer to backoff restarts due to: " << backoffReason << endl;
    }
    else {
      LOGPRT << "Service Warning: " << _logID
	     << ": backoff restarts for "
	     << _config.execFailureBackoff << "s: " << backoffReason << endl;
      _nextStartAttempt = st_time() + _config.execFailureBackoff;
      return;
    }
  }

  if (er == processExit::maxRequests) createProcess("maxRequests");
}


//////////////////////////////////////////////////////////////////////
// Based on the starting time, return the oldest process entry in the
// _processMap. Return NULL if none found.
//////////////////////////////////////////////////////////////////////

process*
service::findOldestProcess()
{
  process* P = 0;

  for (processMapIter mi=_processMap.begin(); mi!=_processMap.end(); ++mi) {
    if (!P) {
      P = mi->second;
      continue;
    }
    if (mi->second->getStartTime() < P->getStartTime()) P = mi->second;
  }

  return P;
}

//////////////////////////////////////////////////////////////////////
// On OS/X the close-on-exec bit does not appear to impact the ulimits
// checks which appear to be tested when a file is first opened. As a
// work-around, once a process is forked it directly closes all the
// files associated with other processes that should normally get
// closed at the point of exec anyway.
//////////////////////////////////////////////////////////////////////

#ifdef __APPLE__
void
service::closeAllfdsExcept(const service* exceptThis, int aboveFD)
{
  for (trackMap::const_iterator ix=serviceTracker.begin(); ix != serviceTracker.end(); ++ix) {
    const service* S = ix->second;
    if (S != exceptThis) S->closeMyfds(aboveFD);
  }
}

void
service::closeMyfds(int aboveFD) const
{
  if ((_acceptSocket != -1) && (_acceptSocket > aboveFD)) close(_acceptSocket);
  if ((_shmServiceFD != -1) && (_shmServiceFD > aboveFD)) close(_shmServiceFD);
  if ((_reportingChannelPipes[0] != -1) && (_reportingChannelPipes[0] > aboveFD)) {
    close(_reportingChannelPipes[0]);
  }
  if ((_reportingChannelPipes[1] != -1)  && (_reportingChannelPipes[1] > aboveFD)) {
    close(_reportingChannelPipes[1]);
  }
}

#endif


//////////////////////////////////////////////////////////////////////
// Static access routine for the command port
//////////////////////////////////////////////////////////////////////

void
service::list(ostringstream& os, const char* name)
{
  os
    << setw(25)			// Name
    << " " << setw(8)		// Active
    << " " << setw(8)		// Spare
    << " " << setw(6)		// Occupancy
    << " " << setw(6)		// Listen Queue
    << " " << setw(8)		// Next Start attempt
    << endl;

  os
    << setw(25) << setiosflags(ios::left) << "Name" << resetiosflags(ios::left)
    << " " << setw(8) << "Active"
    << " " << setw(8) << "Spare"
    << " " << setw(6) << "Occ%"
    << " " << setw(6) << "Listen"
    << " " << setw(8) << "BackOff"
    << endl;

  time_t now = time(0);

  for (trackMap::const_iterator ix=serviceTracker.begin(); ix != serviceTracker.end(); ++ix) {
    service* SM = ix->second;
    if (name && (SM->_name != name)) continue;

    os
      << setw(25) << setiosflags(ios::left) << SM->_name << resetiosflags(ios::left)
      << setw(0) << " " << setw(8) << SM->getActiveProcessCount()
      << setw(0) << " " << setw(8) << SM->_unusedIds.size()
      << setw(0) << " " << setw(6) << (int) SM->_currOccupancyByTime
      << setw(0) << " " << setw(6) << SM->getMANAGER()->getListenBacklog(SM->_stAcceptingFD);

    if (SM->_nextStartAttempt > now) {
      os << " " << setw(8) << SM->_nextStartAttempt - now;
    }
    else {
      os << " " << setw(8) << "no";
    }

    os << endl;
  }
}


//////////////////////////////////////////////////////////////////////

void
service::initiateShutdownSequence(const char* reason)
{
  if (debug::service()) {
    DBGPRT << "service::initiateShutdownSequence() " << _logID << ": " << reason << endl;
  }

  if (shutdownInProgress()) return;
  LOGPRT << "Service Shutdown: " << _logID << " " << reason << endl;

  _shutdownReason = reason;

  baseInitiateShutdownSequence();

  notifyOwner("service", reason);
}


//////////////////////////////////////////////////////////////////////
// Complete the process of shutting down a service, only return once
// all resources are returned so that this object can be deleted.
//////////////////////////////////////////////////////////////////////

void
service::completeShutdownSequence()
{
  if (debug::service()) DBGPRT << "service::completeShutdownSequence() "
			       << _logID << " AP=" << _activeProcessCount << endl;

  enableInterrupts();
  while (_activeProcessCount > 0) {
    st_sleep(5);
    if ((_activeProcessCount > 0) && logging::serviceExit()) {
      LOGPRT << "Service Exit: "
	     << _logID << ": waiting on " << _activeProcessCount << endl;
    }
  }
  disableInterrupts();
}


//////////////////////////////////////////////////////////////////////
// Create the common accept socket used by all the service
// processes. The socket is created then renamed to the correct name
// so that the name is always visible to calling programs - even when
// a service is been replaced due to a configuration change. This
// should make service restarts transparent to clients.
//////////////////////////////////////////////////////////////////////

bool
service::createAcceptSocket(const std::string& socketDirectory)
{
  string finalPath = socketDirectory;
  finalPath += "/";
  finalPath += _name;
  finalPath += ".socket";

  string tempPath(finalPath);
  tempPath += ".tmp";

  struct sockaddr_un su;
  bzero((void*) &su, sizeof(su));
  su.sun_family = AF_UNIX;

  if (tempPath.length() >= sizeof(su.sun_path)) {
    errno = ENAMETOOLONG;
    util::messageWithErrno(_errorMessage, "Create sockaddr", tempPath);
    return false;
  }

  if ((unlink(tempPath.c_str()) == -1) && (errno != ENOENT)) {
    util::messageWithErrno(_errorMessage, "unlink()", tempPath);
    return false;
  }

  _acceptSocket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (_acceptSocket == -1) {
    util::messageWithErrno(_errorMessage, "socket()", tempPath);
    return false;
  }

  if (util::setCloseOnExec(_acceptSocket) == -1) {
    util::messageWithErrno(_errorMessage, "fcntl(FD_CLOEXEC)", tempPath);
    return false;
  }

  strcpy(su.sun_path, tempPath.c_str());
  if (bind(_acceptSocket, (struct sockaddr*) &su, sizeof(su)) == -1) {
    util::messageWithErrno(_errorMessage, "bind()", tempPath);
    return false;
  }

  //////////////////////////////////////////////////////////////////////
  // Arbitrarily let the listen queue be deeper than max concurrency
  // plus a margin. A very low-latency, high use function may require
  // that the multiple be great than the hard-coded value
  // here. Ultimately this may need to be a config value if the guess
  // proves to be a problem.
  //////////////////////////////////////////////////////////////////////

  if (listen(_acceptSocket, 128 + getMaximumProcesses() * 3) == -1) {
    util::messageWithErrno(_errorMessage, "listen()", tempPath);
    return false;
  }

  //////////////////////////////////////////////////////////////////////
  // Make sure everyone can access the socket. The parent directory is
  // responsible for determining access rights to the services run by
  // this manager.
  //////////////////////////////////////////////////////////////////////

  if (chmod(tempPath.c_str(), S_IRUSR|S_IWUSR | S_IRGRP|S_IWGRP | S_IROTH|S_IWOTH) == -1) {
    util::messageWithErrno(_errorMessage, "fchmod()", tempPath);
    return false;
  }

  //////////////////////////////////////////////////////////////////////
  // The final step is the rename, which the manpage promises is
  // atomic.
  //////////////////////////////////////////////////////////////////////

  if (rename(tempPath.c_str(), finalPath.c_str()) == -1) {
    util::messageWithErrno(_errorMessage, "rename()", finalPath);
    return false;
  }

  _acceptPath = finalPath;

  return true;
}


void
service::removeAcceptPath()
{
  if (!_acceptPath.empty() && (unlink(_acceptPath.c_str()) == -1)) {
    util::messageWithErrno(_errorMessage, "unlink()", _acceptPath);
  }
}


//////////////////////////////////////////////////////////////////////
// Create the shmService file
//////////////////////////////////////////////////////////////////////

bool
service::createServiceMap(const std::string& mapDirectory)
{
  string finalPath = mapDirectory;
  finalPath += "/";
  finalPath += _name;
  finalPath += ".mmap";

  string tempPath(finalPath);
  tempPath += ".tmp";

  const char* path = tempPath.c_str();
  _shmServiceFD = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (_shmServiceFD == -1) {
    util::messageWithErrno(_errorMessage, "open(serviceMap)", tempPath);
    return false;
  }

  if (util::setCloseOnExec(_shmServiceFD) == -1) {
    util::messageWithErrno(_errorMessage, "fcntl(FD_CLOEXEC)", finalPath);
    return false;
  }

  // The final step is the rename, which the manpage promises is
  // atomic.

  if (rename(path, finalPath.c_str()) == -1) {
    util::messageWithErrno(_errorMessage, "rename()", finalPath);
    return false;
  }

  _shmPath = finalPath;

  return true;
}


void
service::removeServiceMap()
{
  if (!_shmPath.empty() && (unlink(_shmPath.c_str()) == -1)) {
    util::messageWithErrno(_errorMessage, "unlink()", _shmPath);
  }
}
