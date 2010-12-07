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

#include <iostream>
#include <sstream>
#include <string>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <st.h>

#include "debug.h"
#include "logging.h"
#include "util.h"
#include "global.h"
#include "commandPort.h"
#include "listenInterface.h"
#include "listenBacklog.h"
#include "shmLookup.h"
#include "pidMap.h"
#include "manager.h"
#include "process.h"
#include "service.h"
#include "serviceKey.h"

using namespace std;


//////////////////////////////////////////////////////////////////////
// The manager is the controlling state-thread. It establishes all the
// necessary framework to start service processes and log results. The
// main role of the manager thread while running is to distribute
// child exit signals and test for config changes.
//////////////////////////////////////////////////////////////////////

manager::manager()
  : _commandPortInterface(0), _configurationDirectory("."), _rendezvousDirectory("."),
    _lookupMapFile("./lookup.map"),
    _emergencyExitDelay(30),
    _statisticsLogInterval(600), _defaultUID(-1), _defaultGID(-1), _stStackSize(0),
    _logStatsFlag(false),
    _configurationReloadFlag(false), _configurationReloadAfter(0),
    _reapChildrenFlag(false),
    _quitMessage(0), _commandAcceptSocket(-1),
    _serviceCount(0),
    _activeProcessCount(0), _maximumProcessCount(0), _childCount(0), _zombieCount(0),
    _processAdded(0), _requestsReported(0),
    _serviceMap(new serviceMapType), _shmLookupPtr(new shmLookup),
    _startTime(time(0)), _forkLimiter(3)
{
  zeroPeriodicCounts();
}


//////////////////////////////////////////////////////////////////////

manager::~manager()
{
  if (_shmLookupPtr) delete _shmLookupPtr;
  if (_serviceMap) delete _serviceMap;
}


//////////////////////////////////////////////////////////////////////

bool
manager::initialize()
{
  setThreadID(st_thread_self());

  if (!_LB.initialize(_errorMessage)) return false;	// Listen Backlog
  if (!checkRendezvousDirectory()) return false;

  return true;
}


//////////////////////////////////////////////////////////////////////
// Make sure - as best we can - that the Rendezvous directory is
// accessible. Also, make it absolute if it's a relative path as it
// forms part of the path in the lookupMap.
//////////////////////////////////////////////////////////////////////

bool
manager::checkRendezvousDirectory()
{
  struct stat sb;
  const char*	rvPath = _rendezvousDirectory.c_str();
  if (stat(rvPath, &sb) == -1) {
    util::messageWithErrno(_errorMessage, "Could not stat() rendezvousDirectory", rvPath);
    return false;
  }

  if (!(sb.st_mode & S_IFDIR)) {
    errno = ENOTDIR;
    util::messageWithErrno(_errorMessage, "rendezvousDirectory is not a directory", rvPath);
    return false;
  }

  if (access(rvPath, R_OK | W_OK | X_OK | F_OK) == -1) {
    util::messageWithErrno(_errorMessage, "rendezvousDirectory inaccessible", rvPath);
    return false;
  }

  // If the path is absolute we're done

  if (*rvPath == '/') return true;

  // Convert relative to absolute - as best we can. Note the the RHEL
  // manpage warns against realpath(3) - but I think the rationale is
  // largely specious.

  char workBuffer[MAXPATHLEN * 4];
  workBuffer[MAXPATHLEN * 4 - 3] = '\3';	// Put up a small picket fence
  workBuffer[MAXPATHLEN * 4 - 2] = '\2';	// just for the parnoid RHEL folk
  workBuffer[MAXPATHLEN * 4 - 1] = '\1';

  _rendezvousDirectory = realpath(rvPath, workBuffer);

  assert(workBuffer[MAXPATHLEN * 4 - 3] == '\3');	// Check that the fence
  assert(workBuffer[MAXPATHLEN * 4 - 2] == '\2');	// is still in good
  assert(workBuffer[MAXPATHLEN * 4 - 1] == '\1');	// standing.

  LOGPRT << "Manager Rendezvous realpath: " << _rendezvousDirectory << endl;

  return true;
}


//////////////////////////////////////////////////////////////////////////
// Initialize the listening socket for the command port. Return false
// on any failure with an error message.
//////////////////////////////////////////////////////////////////////////

bool
manager::initializeCommandPort(std::string& em)
{
  if (!_commandPortInterface) return true;

  listenInterface li;
  if (li.openAndListen(_commandPortInterface)) {
    util::messageWithErrno(em, 0, _commandPortInterface);
    return false;
  }

  _commandAcceptSocket = li.transferFD();		// Take ownership of the FD

  if (util::setCloseOnExec(_commandAcceptSocket) == -1) {
    util::messageWithErrno(em, "Could not set FD_CLOEXEC on Command Accept Socket",
			   _commandPortInterface);
    close(_commandAcceptSocket);
    _commandAcceptSocket = -1;
    return false;
  }

  if (!st_thread_create(commandPort::listen, static_cast<void*>(this), 0, _stStackSize)) {
    util::messageWithErrno(em, "st_thread_create(commandPort::listen) failed");
    close(_commandAcceptSocket);
    _commandAcceptSocket = -1;
    return false;
  }

  return true;
}


//////////////////////////////////////////////////////////////////////

void
manager::initiateShutdownSequence(const char* reason)
{
  if (debug::manager()) DBGPRT << "manager::initiateShutdownSequence() " << _logID << endl;

  if (shutdownInProgress()) return;
  LOGPRT << "Manager shutdown: " << reason << endl;

  baseInitiateShutdownSequence();

  for (serviceMapIter mi=_serviceMap->begin(); mi!=_serviceMap->end(); ++mi) {
    mi->second->initiateShutdownSequence(reason);
  }
}


//////////////////////////////////////////////////////////////////////
// Report on residual object count. They should all be zero if the
// classes are destroying their objects correctly.
//////////////////////////////////////////////////////////////////////

void
manager::completeShutdownSequence()
{
  LOGPRT << "Final Objects:"
	 << " Map=" << _serviceMap->size()
	 << " Services=" << service::getCurrentObjectCount()
	 << " Process=" << process::getCurrentObjectCount()
	 << " Children=" << _childCount
	 << " Zombies=" << _zombieCount
	 << endl;
}


//////////////////////////////////////////////////////////////////////
// When destroing a service, if it has an entry in the serviceMap then
// the serviceMap entry and the corresponding acceptPath are removed
// otherwise the service has been replaced and the Map and path
// remain. This ensures that a service is always available to clients,
// even when it is being replaced due to an updated config.
//////////////////////////////////////////////////////////////////////

void
manager::destroyOffspring(threadedObject* to, const char* reason)
{
  service* S = dynamic_cast<service*>(to);

  const service* matchingS = findServiceInServiceMap(S->getName());
  if (!matchingS || (matchingS == S)) {
    if (matchingS) _serviceMap->erase(S->getName());
    S->removeAcceptPath();
    S->removeServiceMap();
  }

  --_serviceCount;
  delete S;

  notifyOwner("manager", "destroyOffspring");	// Tell owner so it can recalibrate/exit
}


//////////////////////////////////////////////////////////////////////

void
manager::addProcessCount()
{
  ++_activeProcessCount;
  _maximumProcessCount = max(_maximumProcessCount, _activeProcessCount);
}


void
manager::subtractProcessCount(processExit::reason res)
{
  --_activeProcessCount;
  ++_exitReasonCount[res];
}

void
manager::zeroPeriodicCounts()
{
  for (int ix=processExit::noReason; ix<processExit::maxReasonCount; ++ix) {
    _exitReasonCount[ix] = 0;
  }
}


//////////////////////////////////////////////////////////////////////

void
manager::setQuitMessage(const char* m)
{
  _quitMessage = m;
  
  notifyOwner("manager", m);
}


void
manager::setConfigurationReload(bool newState, time_t changesSince)
{
  _configurationReloadFlag = newState;
  _configurationReloadAfter = changesSince;
}


//////////////////////////////////////////////////////////////////////
// Rebuild the shared memory lookup structure based on the service
// map. The Service Key and the Search Key are *not* the
// same. serviceKey() knows how to construct the Search Key.
//
// The keys are first constructed in a temporary hash so that the size
// of the shm can be determined once all keys are processed. With the
// known size, the new shm is created and the temporary hashmap
// details are transferred to shm.
//////////////////////////////////////////////////////////////////////

const char*
manager::rebuildLookup()
{
  shmLookup::mapType keysIn;		// Temporary key holder

  for (serviceMapConstIter ix=_serviceMap->begin(); ix != _serviceMap->end(); ++ix) {
    service* sm = ix->second;
    pluton::serviceKey SK;		// Construct the Search Key from the Service Key
    SK.parse(sm->getName(), false);
    string sk;
    SK.getSearchKey(sk);
    keysIn[sk] = sm->getAcceptPath();	// and add it into the temporary hash.
  }

  const char* res = _shmLookupPtr->buildMap(_lookupMapFile, keysIn);

  if (debug::oneShot()) _shmLookupPtr->dumpMap();

  return res;
}


//////////////////////////////////////////////////////////////////////
// The main loop of the manager:
// 	o Periodically make status reports
//	o Notice child exits and tell the controlling thread
//	o Notice config change signals and reload configurations
//////////////////////////////////////////////////////////////////////

bool
manager::run()
{
  time_t nextReport = 0;
  while (!_quitMessage) {
    if (_configurationReloadFlag) {
      LOGPRT << "Manager Signal: Configuration Reload" << endl;
      if (!loadConfigurations(_configurationReloadAfter)) break;
      _configurationReloadFlag = false;
    }
    if (_reapChildrenFlag) {
      _reapChildrenFlag = false;
      pidMap::reapChildren("manager::run");
    }

    time_t now = st_time();
    if (nextReport <= now) {
      periodicStatisticsReport(now);
      nextReport = now + _statisticsLogInterval;
    }

    LOGFLUSH;
    enableInterrupts();			// Wait for an interrupt or
    int res = st_usleep(util::MICROSECOND/4);	// Could do the self-pipe trick to get notified
    if (debug::manager()) DBGPRT << "manager::st_usleep=" << res << " errno=" << errno << endl;
    disableInterrupts();
  }

  return false;
}


//////////////////////////////////////////////////////////////////////.
// Give the services a chance to shutdown
//////////////////////////////////////////////////////////////////////

void
manager::runUntilIdle()
{
  if (debug::manager()) DBGPRT << "manager::runUntilIdle() " << _logID << endl;

  enableInterrupts();
  while (_serviceCount > 0) {
    st_sleep(1);
    if (_reapChildrenFlag) {
      _reapChildrenFlag = false;
      pidMap::reapChildren("manager::runUntilIdle");
    }
    if ((_serviceCount > 0) && debug::manager()) {
      DBGPRT << "Manager exit: waiting on " << _serviceCount << endl;
      ostringstream os;
      process::list(os, 0);
      DBGPRT << os.str() << endl;
    }
  }
}

//////////////////////////////////////////////////////////////////////

service*
manager::findServiceInServiceMap(const std::string& name)
{
  serviceMapIter mi = _serviceMap->find(name);

  if (mi == _serviceMap->end()) return 0;

  return mi->second;
}
