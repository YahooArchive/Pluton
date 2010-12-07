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

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <time.h>
#include <signal.h>

#include <st.h>

#include "debug.h"
#include "logging.h"
#include "util.h"
#include "processExitReason.h"
#include "pidMap.h"
#include "process.h"
#include "reportingChannel.h"
#include "shmService.h"
#include "service.h"
#include "manager.h"

using namespace std;


//////////////////////////////////////////////////////////////////////
// A process object and state thread are created for each real Unix
// process associated with a service. The main functions of module are
// to monitor the Unix process for stall/idle/stderr messages.
//////////////////////////////////////////////////////////////////////


int process::currentObjectCount = 0;
int process::maximumObjectCount = 0;
process::trackMap process::processTracker;


static string
signalToEnglish(int sig)
{
  switch (sig) {
  case SIGHUP: return "SIGHUP";
  case SIGINT: return "SIGINT";
  case SIGABRT: return "SIGABRT - may have exceeded Memory ulimits";
  case SIGSEGV: return "SIGSEGV";
  case SIGPIPE: return "SIGPIPE";
  case SIGTERM: return "SIGTERM";
  case SIGXCPU: return "SIGXCPU - may have exceeded CPU ulimits";
  case SIGUSR1: return "SIGUSR1";
  case SIGUSR2: return "SIGUSR2";
  case SIGKILL: return "SIGKILL - someone did kill -9 or maybe you're out of swap! (ask Okan)";
  default: break;
  }

  ostringstream os;
  os << sig;

  return os.str();
}

const char*
process::reasonCodeToEnglish(processExit::reason er)
{
  switch (er) {
  case processExit::noReason: return "noreason";
  case processExit::serviceShutdown: return "serviceShutdown";
  case processExit::unresponsive: return "unresponsive";
  case processExit::abnormalExit: return "abnormalExit";
  case processExit::lostIO: return "lostIO";
  case processExit::runawayChild: return "runawayChild";
  case processExit::maxRequests: return "maxRequests";
  case processExit::excessProcesses: return "excessProcesses";
  case processExit::idleTimeout: return "idleTimeout";
  case processExit::acceptFailed: return "acceptFailed";
  default:break;
  }

  return "??? weird processExit::reason";
}

process::process(service* setS, int setID, const std::string& setName,
		 int acceptSocket, int shmServiceFD, int reportingSocket,
		 pluton::shmServiceHandler* setShmService)
  : threadedObject(setS),
    _S(setS), _id(setID), _name(setName), _shmService(setShmService),
    _exitReason(processExit::noReason), _sdReason(processExit::noReason), _backoffReason(0),
    _acceptSocket(acceptSocket), _shmServiceFD(shmServiceFD), _reportingSocket(reportingSocket),
    _stErrorNetFD(0),
    _repeatLogEntryCount(0),
    _pid(-1), _lastSignalSent(0), _childHasExitedFlag(false), _childExitStatus(0),
    _totalTimeInProcess(0), _requestsCompleted(0),
    _totalRequestLength(0), _totalResponseLength(0),
    _maximumRequestLength(0), _maximumResponseLength(0),
    _readErrors(0), _writeErrors(0), _faultsReported(0)
{
  ++currentObjectCount;
  if (currentObjectCount > maximumObjectCount) maximumObjectCount = currentObjectCount;
  processTracker[this] = this;
  _fildesSTDERR[0] = -1;
  _fildesSTDERR[1] = -1;
  
  for (int ix=0; ix < _mtipSize; ++ix) _maximumTimeInProcess[ix] = 0;
}


process::~process()
{
  if (_stErrorNetFD) st_netfd_free(_stErrorNetFD);
  if (_fildesSTDERR[0] != -1) close(_fildesSTDERR[0]);
  if (_fildesSTDERR[1] != -1) close(_fildesSTDERR[1]);
  _fildesSTDERR[0] = _fildesSTDERR[1] = -1;

  --currentObjectCount;
  processTracker.erase(this);
}


//////////////////////////////////////////////////////////////////////

bool
process::initialize()
{
  gettimeofday(&_startTime, 0);
  _shmService->resetProcess(_id, &_startTime);

  if (!forkExecChild(_acceptSocket, _shmServiceFD, _reportingSocket)) return false;

  ostringstream os;
  os << _name << "/" << _id << "-" << _pid;
  _logID = os.str();

  if (logging::processStart()) LOGPRT << "Process Start: " << _logID
				      << " (" << _S->getActiveProcessCount() << " of "
				      << _S->getMaximumProcesses()
				      << ")" << endl;

  // Every 20th process gets an early exit so that leakage trends
  // can be detected (not yet implemented)

  //	if ((_pid % 20) == 0) _maximumRequests /= 2;

  if (!process::startThread(this)) {			// If this fails we'll lose children
    LOGPRT << "Process Error: " << _logID << " process::startThread() failed" << endl;
    return false;
  }

  return true;
}


//////////////////////////////////////////////////////////////////////
// Keep a record of costs for this process for reporting and analysis
// purposes.
//////////////////////////////////////////////////////////////////////

void
process::trackCosts(const char* function, const pluton::reportingChannel::performanceDetails& rd)
{
  _totalTimeInProcess += rd.durationMicroSeconds;
  ++_requestsCompleted;

  if (rd.durationMicroSeconds < _maximumTimeInProcess[0]) return;

  _totalRequestLength += rd.requestLength;
  _totalResponseLength += rd.responseLength;

  if (rd.requestLength > _maximumRequestLength) _maximumRequestLength = rd.requestLength;
  if (rd.responseLength > _maximumResponseLength) _maximumResponseLength = rd.responseLength;

  switch (rd.reason) {
  case pluton::reportingChannel::readError: _readErrors++; break;
  case pluton::reportingChannel::writeError: _writeErrors++; break;
  case pluton::reportingChannel::fault: _faultsReported++; break;
  default: break;
  }

  // Always insert the first duration see - the later loops rely on
  // this.

  if (_maximumTimeInProcess[_mtipSize-1] == 0) {
    _maximumTimeInProcess[_mtipSize-1] = rd.durationMicroSeconds;
    return;
  }

  // Insert the durationMicroSeconds in order in the array

  int insertAt = 0;
  for (int ix=0;  ix < _mtipSize; ++ix) {
    if (rd.durationMicroSeconds <= _maximumTimeInProcess[ix]) break;
    insertAt = ix;
  }

  for (int ix=0; ix < insertAt; ++ix) {
    _maximumTimeInProcess[ix] = _maximumTimeInProcess[ix+1];
  }

  _maximumTimeInProcess[insertAt] = rd.durationMicroSeconds;
}


//////////////////////////////////////////////////////////////////////
// Tell the process thread that the child has exited and record the
// exit status and resource costs for logging and analysis. This
// routine is called by the main thread that catches SIGCHLD.
//////////////////////////////////////////////////////////////////////

void
process::notifyChildExit(int status, struct rusage& ru)
{
  if (debug::child()) DBGPRT << "Child Exit: " << _pid << " st=" << status
			     << " ec=" << WEXITSTATUS(status) << endl;

  _childHasExitedFlag = true;
  _childExitStatus = status;
  _resourcesUsed = ru;

  getSERVICE()->subtractChild();
  getSERVICE()->getMANAGER()->subtractChild();

  notifyOwner("process", "Child Exit");
}


//////////////////////////////////////////////////////////////////////
// Wait for the manager thread to reap the child and tell us.  If that
// doesn't occur in a timely fashion, make repeatedly stronger efforts
// to kill the child directly. Give up if all reasonable efforts fail.
//
// Return true if the child exited, otherwise return false.
//////////////////////////////////////////////////////////////////////

typedef struct {
  int	signal;
  const char* name;
  int	sleepTime;
  const char* characterization;
} escalationType;


static escalationType escalate[] =
  {
    { SIGINT, "SIGINT", 3, "recalcitrant" },
    { SIGTERM, "SIGTERM", 4, "intransigent" },
    { SIGKILL, "SIGKILL", 5, "irredeemable" },
    { 0 }
  };


bool
process::waitForChildExit()
{
  pidMap::reapChildren("process::waitForChildren::1");
  if (_childHasExitedFlag) return true;

  if (kill(_pid, SIGURG) == -1) {
      util::messageWithErrno(_errorMessage, " kill SIGURG disallowed", 0);
      LOGPRT << "Process Error: " << _logID << " " << _errorMessage << endl;
      return false;				// Leave it as a zombie
  }

  st_sleep(3);				// Reaper notifies us if the child exits
  pidMap::reapChildren("process::waitForChildren::2");
  if (_childHasExitedFlag) return true;
  LOGPRT << "Process Status: " << _logID << ": Waiting for exit()" << endl;

  //////////////////////////////////////////////////////////////////////
  // Time to get tough on the process, we really do want it to go away
  // because if we don't kill it, then a service process stays around
  // for ever listening on an accept sock for a file that no longer
  // exists in the file system.
  //
  // Loop through the escalation procedures until it goes away or we
  // run out of options.
  //////////////////////////////////////////////////////////////////////

  escalationType* ep = escalate;
  while (ep->signal) {
    _lastSignalSent = ep->signal;
    if (kill(_pid, ep->signal) == -1) {	// Make a suggestion
      util::messageWithErrno(_errorMessage, " kill disallowed", 0);
      LOGPRT << "Process Error: " << _logID << " " << ep->name << _errorMessage << endl;
      return false;				// Leave it as a zombie
    }

    LOGPRT << "Process Warning: " << _logID << " " << ep->name
	   << " sent to " << ep->characterization << " child" << endl;

    st_sleep(ep->sleepTime);
    pidMap::reapChildren("process::waitForChildren::3");
    if (_childHasExitedFlag) return true;	// Good boy Jonny
    ++ep;
  }

  //////////////////////////////////////////////////////////////////////
  // We give up on waiting for this child. Note that the reaper will
  // catch this child if/when it finally exits. It just would have
  // been nice if it behaved...
  //////////////////////////////////////////////////////////////////////

  LOGPRT << "Process Error: " << _logID << " One child left behind" << endl;

  return false;
}


//////////////////////////////////////////////////////////////////////
// The process thread serves two purposes:
//
// o Reads their stderr for logging traffic
// o Checks that the process isn't stalled on a request
//////////////////////////////////////////////////////////////////////

bool
process::run()
{
  if (debug::process()) DBGPRT << "process::run " << _id << endl;

  long unresponsiveTimeout = getSERVICE()->getUnresponsiveTimeout();
  time_t startTime = st_time();

  while (!shutdownInProgress()) {
    int res = readTheirStderr(2 * util::MICROSECOND);

    if (shutdownInProgress()) break;
    if (res <= 0) {
      setShutdownReason(processExit::lostIO);
      _shmService->setProcessShutdownRequest(_id, processExit::lostIO);
      break;
    }

    //////////////////////////////////////////////////////////////////////
    // Has the service instance taken too long on a request?
    //////////////////////////////////////////////////////////////////////

    struct timeval lastActive;
    _shmService->getProcessLastRequestTime(_id, lastActive);
    time_t now = st_time();
    if (debug::process()) DBGPRT << "Last Active Sec=" << lastActive.tv_sec
				 << " startTime=" << startTime
				 << " isactive=" << _shmService->getProcessActiveFlag(_id)
				 << " now=" << now
				 << " unr=" << unresponsiveTimeout
				 << endl;
    if (lastActive.tv_sec == 0) lastActive.tv_sec = startTime;	// No traffic? XX Is this needed?
    if (_shmService->getProcessActiveFlag(_id)) {
      if ((unresponsiveTimeout > 0) && ((lastActive.tv_sec + unresponsiveTimeout) < now)) {
	if (debug::process()) DBGPRT << "Closing due to unresponsiveness" << endl;
	setShutdownReason(processExit::unresponsive, true);
	_shmService->setProcessShutdownRequest(_id, processExit::unresponsive);
	break;
      }
    }
  }

  return false;
}


//////////////////////////////////////////////////////////////////////
// Run down the process until it exits
//////////////////////////////////////////////////////////////////////

void
process::runUntilIdle()
{
  time_t tryUntil = st_time() + getSERVICE()->getUnresponsiveTimeout();

  if (debug::process()) DBGPRT << "process::runUntilIdle " << _id << " until " << tryUntil << endl;

  while (!_childHasExitedFlag && (tryUntil > st_time())) {
    readTheirStderr(1 * util::MICROSECOND);
  }

  if (debug::process()) DBGPRT << "process::runUntilIdle done" << _childHasExitedFlag << endl;
}


//////////////////////////////////////////////////////////////////////

void
process::setShutdownReason(processExit::reason newReason, bool issueKillFlag)
{
  if (_sdReason == processExit::noReason) _sdReason = newReason;
  if (shutdownInProgress()) return;

  initiateShutdownSequence(reasonCodeToEnglish(newReason));
  _shmService->setProcessShutdownRequest(_id, newReason);

  if (!issueKillFlag) return;

  _lastSignalSent = SIGURG;
  if (kill(_pid, _lastSignalSent) == -1) {
    string em;
    util::messageWithErrno(em, "SIGURG disallowed");

    //////////////////////////////////////////////////////////////////////
    // There is a timing window between setting the shutdown request
    // and sending the kill. A service could beat us to the punch, so
    // adjust the message if this is possible.
    //////////////////////////////////////////////////////////////////////

    if (errno == ESRCH) {
      LOGPRT << "Process Warning: " << _logID << " " << em
	     << " (May have exited quickly though)" << endl;
    }
    else {
      LOGPRT << "Process Error: " << _logID << " " << em << endl;
    }
  }
}


//////////////////////////////////////////////////////////////////////
// Read the log stream from the process that comes in via their
// stderr.
//
// Error: return -1, set _errorMessage
// Eof: return 0
// Good: return > 0
//////////////////////////////////////////////////////////////////////

int
process::readTheirStderr(st_utime_t waitTime)
{
  if (debug::process()) DBGPRT << "readTheirStderr for " << waitTime << endl;
  if (!_stErrorNetFD) {
    enableInterrupts();
    int res = st_usleep(waitTime);
    disableInterrupts();
    if (debug::process()) DBGPRT << "readTheirStderr st_usleep="
				 << res << " errno=" << errno << endl;
    return 0;
  }

  enableInterrupts();
  int res = st_netfd_poll(_stErrorNetFD, POLLIN, waitTime);
  disableInterrupts();
  if (debug::process()) DBGPRT << "readTheirStderr st_netfd_poll="
			       << res << " errno=" << errno << endl;
  if (res != 0) return 1;

  char	buffer[1000];
  int readBytes = st_read(_stErrorNetFD, (void*) buffer, sizeof(buffer), 0);
  if (debug::process()) DBGPRT << "readTheirStderr st_read=" << readBytes
			       << " errno=" << errno << endl;
  if (readBytes <= -1) {
    if ((errno == ETIME) || (errno == EINTR)) return 1;
    util::messageWithErrno(_errorMessage, "Unexpected errno from st_read(STDERR)");
    return -1;
  }

  // On eof, discard the st file handle to indicate eof for subsequent
  // calls.

  if (readBytes == 0) {
    if (debug::process()) DBGPRT << "readTheirStderr eof" << endl;
    st_netfd_free(_stErrorNetFD);
    _stErrorNetFD = 0;
    return 0;		// Eof
  }

  //////////////////////////////////////////////////////////////////////
  // The read data has to be buffered in a line-assembly string until
  // a \n is seen as both the C and C++ I/O libraries embedded in
  // typical services can write partial lines.
  //////////////////////////////////////////////////////////////////////

  if (!logging::processLog()) return readBytes;
  _pendingLogEntry.append(buffer, readBytes);

  string::size_type ix;
  while ((ix = _pendingLogEntry.find_first_of("\n")) != string::npos) {
    string completeLine(_pendingLogEntry, 0, ix);

    if (completeLine == _lastLogLine) {
      ++_repeatLogEntryCount;
    }
    else {
      if (_repeatLogEntryCount == 1) {
	LOGPRT << "Log: " << _logID << " " << _lastLogLine << endl;
	_repeatLogEntryCount = 0;
      }

      if (_repeatLogEntryCount > 0) {
	LOGPRT << "Log: " << _logID << " previous message repeated "
	       << _repeatLogEntryCount << " times " << endl;
	_repeatLogEntryCount = 0;
      }
      LOGPRT << "Log: " << _logID << " " << completeLine << endl;
      _lastLogLine = completeLine;
      if (cout.bad()) {
	cerr << "Warning: " << _logID << " Logging output failed for line="
	     << completeLine.length() << " pendingSz=" << _pendingLogEntry.length() << endl;
	cerr.flush();
	cout.clear();
	write(2, "Marker: thr\n", 12);	// To determine if Warning disappears too!
      }
    }
    ++ix;	// Skip over NL
    _pendingLogEntry.erase(0, ix);
  }

  if (debug::oneShot()) DBGPRT << "Log: " << _logID
			       << " bytes=" << readBytes
			       << " Pending=" << _pendingLogEntry.length() << endl;

  return readBytes;
}


//////////////////////////////////////////////////////////////////////
// The child is in the process of exiting (or may already have
// exited), slurp in any final stderr that it may have generated.
//////////////////////////////////////////////////////////////////////

void
process::consumeFinalStderr()
{
  for (int countDown = 4; countDown > 0; --countDown) {
    if (!_stErrorNetFD) break;			// readTheirStderr will close this
    if (readTheirStderr(2 * util::MICROSECOND) <= 0) break;
  }

  // They've had plenty of time to finish up - log whatever we have

  if (! _pendingLogEntry.empty()) {
    LOGPRT << "Log: " << _logID << " " << _pendingLogEntry << "<<" << endl;
  }
}


//////////////////////////////////////////////////////////////////////
// A child can exit for a variety of reasons. Report why this child
// exited - particularly if the exit was unexpected. The processExit
// reason is set here for the manager to aggregate.
//////////////////////////////////////////////////////////////////////

static void
decodeExitReason(processExit::reason er, string& reasons, bool& abnormal, const char*& backoff)
{

  switch (er) {
  case processExit::maxRequests:
    if (!reasons.empty()) reasons += ", ";
    reasons += "maximum requests";
    return;

  case processExit::excessProcesses:
    if (!reasons.empty()) reasons += ", ";
    reasons += "excess processes";
    return;

  case processExit::idleTimeout:
    if (!reasons.empty()) reasons += ", ";
    reasons += "idle timeout";
    return;

  case processExit::acceptFailed:
    if (!reasons.empty()) reasons += ", ";
    reasons += "accept failed";
    abnormal = true;
    backoff = "accept failed";
    return;

  case processExit::serviceShutdown:
    if (!reasons.empty()) reasons += ", ";
    reasons += "service shutdown";
    return;

  case processExit::unresponsive:
    if (!reasons.empty()) reasons += ", ";
    reasons += "unresponsive";
    abnormal = true;
    return;

  case processExit::abnormalExit:
    if (!reasons.empty()) reasons += ", ";
    reasons += "abnormal Exit";
    backoff = "accept failed";
    abnormal = true;
    return;

  case processExit::lostIO:
    if (!reasons.empty()) reasons += ", ";
    reasons += "lost STDIO";
    abnormal = true;
    return;

  case processExit::runawayChild:
    if (!reasons.empty()) reasons += ", ";
    reasons += "runaway Child";
    abnormal = true;
    backoff = "runaway child";
    return;

  case processExit::noReason:
  case processExit::maxReasonCount:
    return;
  }
}


//////////////////////////////////////////////////////////////////////
// Evaluate the exit status and log the reason. If a service exits
// under its own control it will set an exitReason in shm. If the
// service exits due to various signals, intuit the reason (the
// expectation is that it has exceeded resource limits).
//
// In all cases, give as much data as possible for non-zero exits to
// assist in diagnosing a broken service.
//////////////////////////////////////////////////////////////////////

void
process::reportChildExit()
{
  if (!_childHasExitedFlag) {
    LOGPRT << "Child Exit: abnormal " << _logID << " child will not exit" << endl;
    _exitReason = processExit::runawayChild;
    return;
  }

  string reasons;	// Accumulate reasons here
  bool abnormal = false;


  //////////////////////////////////////////////////////////////////////
  // Did the child make a non-zero exit?
  //////////////////////////////////////////////////////////////////////

  if (WIFEXITED(_childExitStatus) && WEXITSTATUS(_childExitStatus)) {
    ostringstream os;
    os << "exit(" << WEXITSTATUS(_childExitStatus) << ")";
    if (!reasons.empty()) reasons += ", ";
    reasons += os.str();
    abnormal = true;
    _backoffReason = process::exitCodeToEnglish(WEXITSTATUS(_childExitStatus));
  }

  //////////////////////////////////////////////////////////////////////
  // Did it exit because of a signal? If so, does it match the one we
  // sent it? If so, presume our signal causes it to exit.
  //////////////////////////////////////////////////////////////////////

  if (WIFSIGNALED(_childExitStatus)) {
    if (WTERMSIG(_childExitStatus) == _lastSignalSent) {
      ostringstream os;
      os << "responded to kill(" << signalToEnglish(_lastSignalSent) << ")";
      if (!reasons.empty()) reasons += ", ";
      reasons += os.str();
    }
    else {
      ostringstream os;
      os << "signal " << signalToEnglish(WTERMSIG(_childExitStatus));
      if (!reasons.empty()) reasons += ", ";
      reasons += os.str();
      if (WTERMSIG(_childExitStatus) != SIGTERM) {	// Assume SIGTERM is sysadmin
	abnormal = true;
	_backoffReason = "Died from signal";
      }
    }
  }

  //////////////////////////////////////////////////////////////////////
  // Did the service record an exit reason?
  //////////////////////////////////////////////////////////////////////

  processExit::reason er = _shmService->getProcessExitReason(_id);
  if (er != processExit::noReason) {
    decodeExitReason(er, reasons, abnormal, _backoffReason);
    _exitReason = er;
  }
  else {
    er = _sdReason;			// Did we try and shut it down?
    if (er != processExit::noReason) {
      decodeExitReason(er, reasons, abnormal, _backoffReason);
      _exitReason = er;
    }
  }

  if (abnormal) {
    LOGPRT << "Child Exit: abnormal " << _logID << " " << reasons << endl;
  }
  else if (logging::processExit()) {
    LOGPRT << "Child Exit: normal " << _logID << " " << reasons << endl;
  }
}


//////////////////////////////////////////////////////////////////////
// A process has completely shutdown and the associated child will
// normally have exited. Analyze the resources used and produce a
// summary report.
//////////////////////////////////////////////////////////////////////

void
process::reportChildCosts()
{
  if (!_childHasExitedFlag) return;		// Haven't got any exit data
  if (!logging::processUsage()) return;
  if (_requestsCompleted == 0) return;

  float maxMsecs[_mtipSize];
  for (int ix=0; ix < _mtipSize; ++ix) {
    maxMsecs[ix] = _maximumTimeInProcess[ix];
    maxMsecs[ix] /= 1000;
  }

  float utime = _resourcesUsed.ru_utime.tv_sec + (float) _resourcesUsed.ru_utime.tv_usec
    / util::MICROSECOND;
  float stime = _resourcesUsed.ru_stime.tv_sec + (float) _resourcesUsed.ru_stime.tv_usec
    / util::MICROSECOND;

  utime *= 1000;		// Convert to millisecs
  stime *= 1000;

  float avgMsecs = _totalTimeInProcess;

  std::string sReqAvg;
  std::string sReqMax;
  std::string sResAvg;
  std::string sResMax;

  utime /= _requestsCompleted;	// And reduce to a per-request value
  stime /= _requestsCompleted;
  avgMsecs /= _requestsCompleted;
  avgMsecs /= 1000;

  _totalRequestLength /= _requestsCompleted;	// Totals become averages
  _totalResponseLength /= _requestsCompleted;

  util::intToEnglish((int) _totalRequestLength, sReqAvg);
  util::intToEnglish(_maximumRequestLength, sReqMax);
  util::intToEnglish((int) _totalRequestLength, sResAvg);
  util::intToEnglish(_maximumResponseLength, sResMax);


  std::string activeStr;
  util::durationToEnglish(_totalTimeInProcess / util::MICROSECOND, activeStr);

  //////////////////////////////////////////////////////////////////////
  // The _totalRequestLength value is derived from reports whereas
  // _startTime and _endTime are the manager's view of when things
  // occurred. It's possible for these numbers to differ by small
  // margins over time, simple due to the slightly different sampling
  // points. Assume this incongruence if idleTimeuS is negative.
  //////////////////////////////////////////////////////////////////////

  std::string idleStr;
  long long idleTimeuS = util::timevalDiffuS(_endTime, _startTime);
  idleTimeuS -= _totalTimeInProcess;
  if (idleTimeuS < 0) idleTimeuS = 0;
  idleTimeuS /= util::MICROSECOND;
  util::durationToEnglish(idleTimeuS, idleStr);

  std::string sRequests;
  util::intToEnglish(_requestsCompleted, sRequests);

  // There is a bug in the FreeBSD 6.0 libc snprintf() for converting
  // floating points on 64bit.
  //
  //  1025824 Jun 14 19:53 /lib/libc.so.6

#if defined(__x86_64__) && (__FreeBSD__ == 6)
#define	CONVERTOR (int)
#else
#define CONVERTOR
#endif

  LOGPRT << "Usage: " << _logID
	 << " Active=" << activeStr
	 << " Idle=" << idleStr
	 << " Reqs=" << sRequests << "/" << _faultsReported
	 << "/" << _readErrors << "/" << _writeErrors
	 << " Sz=" << sReqAvg << "/" << sReqMax << " " << sResAvg << "/" << sResMax
	 << " mSecs=" << (int) avgMsecs << " " << (int) maxMsecs[0];

  for (int ix=1; ix < _mtipSize; ++ix) LOGPRT << "/" << (int) maxMsecs[ix];

  LOGPRT << " CPUms/rq=" << CONVERTOR utime << "/" << CONVERTOR stime
	 << " Mem=" << _resourcesUsed.ru_maxrss / 1024
	 << "/"	 << _resourcesUsed.ru_idrss / 1024
	 << "/" << _resourcesUsed.ru_isrss / 1024
	 << endl;
}


//////////////////////////////////////////////////////////////////////
// Send a signal to the child to encourage it to shutdown
//////////////////////////////////////////////////////////////////////

void
process::initiateShutdownSequence(const char* reason)
{
  if (debug::process()) {
    DBGPRT << "process::initiateShutdownSequence() " << _logID << ": " << reason
	   << (shutdownInProgress() ? " Already - ignored" : " first") << endl;
  }

  if (shutdownInProgress()) return;

  baseInitiateShutdownSequence();

  notifyOwner("process", reason);
}


//////////////////////////////////////////////////////////////////////
// Take all steps necessary to complete finish up with this process so
// that the process object can be deleted.
//////////////////////////////////////////////////////////////////////

void
process::completeShutdownSequence()
{
  if (debug::process()) DBGPRT << "process::completeShutdownSequence() " << _logID << endl;

  if (!_childHasExitedFlag && (_pid != -1)) {
    if (debug::child()) DBGPRT << "Debug: kill(" << _pid << ")" << endl;
    _lastSignalSent = SIGURG;
    if (kill(_pid, _lastSignalSent) == -1) {
      string em;
      util::messageWithErrno(em, "SIGURG disallowed");
      LOGPRT << "Process Error: " << _logID << " " << em << endl;
    }
  }

  // Give the child a while to generate any final stderr messages

  enableInterrupts();
  consumeFinalStderr();

  getSERVICE()->getMANAGER()->addZombie();	// It's a zombie until
						// we get the exit

  //////////////////////////////////////////////////////////////////////
  // Wait for the child to exit (or forcably kill it, if it refused to
  // go). If the child doesn't disappear after all efforts, force the
  // removal from the pidmap as that indicates that there is no longer
  // an associated process object.
  //////////////////////////////////////////////////////////////////////

  if (waitForChildExit()) getSERVICE()->getMANAGER()->subtractZombie();	// Good exit

  pidMap::remove(getPID());		// Remove regardless
  gettimeofday(&_endTime, 0);

  reportChildExit();			// Log the reason for the exit
  reportChildCosts();			// and log the resource costs

  if (logging::processExit()) LOGPRT << "Process Exit: " << _logID << endl;

  _S->subtractActiveProcessCount();	// Tell the service we're gone

  _shmService->resetProcess(_id);
}


//////////////////////////////////////////////////////////////////////
// A process doesn't have an offspring to destroy, but the base class
// requires a definition.
//////////////////////////////////////////////////////////////////////

void
process::destroyOffspring(threadedObject* to, const char* reason)
{
}


//////////////////////////////////////////////////////////////////////
// Static access routine for the command port
//////////////////////////////////////////////////////////////////////

void
process::list(ostringstream& os, const char* name)
{
  os << "					       Last  Active" << endl;
  os << "Log ID				    Up For   Active  P/cent Requests  Responses	 Faults Client Name/Request#" << endl;

  struct timeval now;
  gettimeofday(&now, 0);

  for (trackMap::const_iterator ix=processTracker.begin(); ix != processTracker.end(); ++ix) {
    process* P = ix->second;
    if (name && (P->_name != name)) continue;
    string upForStr;
    int upForMS = util::timevalDiffMS(now, P->_startTime);
    util::durationToEnglish(upForMS / util::MILLISECOND, upForStr);
    struct timeval lastRequestTime;
    P->_shmService->getProcessLastRequestTime(P->_id, lastRequestTime);

    string lastActiveStr = "-";
    float activePercent = 0;
    long activeuSecs = P->_shmService->getProcessActiveuSecs(P->_id);

    if (P->_shmService->getProcessRequestCount(P->_id) > 0) {
      util::durationToEnglish(now.tv_sec - lastRequestTime.tv_sec, lastActiveStr);

      if (activeuSecs > 0) {
	activePercent = activeuSecs;
	activePercent /= (util::MICROSECOND / util::MILLISECOND);		// Convert to MS
	activePercent /= upForMS;
	activePercent *= 100;
      }
    }

    os
      << setw(30) << setiosflags(ios::left) << P->_logID << resetiosflags(ios::left)
      << setw(0) << " "
      << setiosflags(ios::right) << setw(10) << upForStr << resetiosflags(ios::right)
      << setw(0) << " " << setw(8) << lastActiveStr
      << setw(0) << " " << setw(6) << (int) activePercent << "%"
      << setw(0) << " " << setw(8) << P->_shmService->getProcessRequestCount(P->_id)
      << setw(0) << " " << setw(10) << P->_shmService->getProcessResponseCount(P->_id)
      << setw(0) << " " << setw(7) << P->_shmService->getProcessFaultCount(P->_id)
      // << " " << P->_shmService->getProcessClientRequestName(P->_id)
      // << "/" << P->_shmService->getProcessClientRequestID(P->_id)
      << endl;
  }
}
