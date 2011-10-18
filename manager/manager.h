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

#ifndef P_MANAGER_H
#define P_MANAGER_H 1

#include <string>

#include "hash_mapWrapper.h"
#include "hashString.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <time.h>

#include <st.h>


class service;
class shmLookup;

#include "threadedObject.h"
#include "listenBacklog.h"
#include "processExitReason.h"
#include "rateLimit.h"

////////////////////////////////////////////////////////////////////////////////
// This class is a mega-class for plutonManager. It encapsulates the
// variables shares amongst various routines and contains the daemon
// state information.
////////////////////////////////////////////////////////////////////////////////

class manager : public threadedObject {
 public:
  manager();
  ~manager();

  bool	initialize();			// Main initialization
  bool	initializeCommandPort(std::string& errorMessage);	// Optionally successful init
  void	preRun() {};			// Called by owning thread
  bool	run();
  void	runUntilIdle();
  void	initiateShutdownSequence(const char* reason);
  void	completeShutdownSequence();

 public:
  const char* 	getType() const { return "manager"; }

  void	parseCommandLineOptions(int argc, char** argv);
  bool 	loadConfigurations(time_t since);

  void		setQuitMessage(const char* message);
  const char*	getQuitMessage() const { return _quitMessage; }

  void	addProcessCount();
  void	subtractProcessCount(processExit::reason);
  int	getProcessCount() const { return _activeProcessCount; }

  void	addRequestReported(int c) { _requestsReported += c; }

  bool	checkForkLimiter(time_t now) { return _forkLimiter.allowed(now); }
  void	addChild() { ++_childCount; ++_processAdded; }
  void	subtractChild() { --_childCount; }
  int	getChildCount() const { return _childCount; }

  void	addZombie() { ++_zombieCount; }
  void	subtractZombie() { --_zombieCount; }
  int	getZombieCount() const { return _zombieCount; }

  void	setConfigurationReload(bool newState, time_t after);

  void	startCommandThread(st_netfd_t);

  int	getStatisticsLogInterval() const { return _statisticsLogInterval; }
  int	getDefaultUID() const { return _defaultUID; }
  int	getDefaultGID() const { return _defaultGID; }
  int	getStStackSize() const { return _stStackSize; }
  int	getEmergencyExitDelay() const { return _emergencyExitDelay; }
  int	getCommandAcceptSocket() const { return _commandAcceptSocket; }

  void	periodicStatisticsReport(time_t now);
  void	getStatistics(std::string&, int interval=0, time_t now=0, bool periodicFlag=false);
  void	zeroPeriodicCounts();

  // Child Management

  void	setReapChildren(bool ns) { _reapChildrenFlag = ns; }

  // Global routines

  int	getListenBacklog(st_netfd_t sFD) { return _LB.queueLength(sFD); }

  ////////////////////////////////////////
  ////////////////////////////////////////

 private:
  manager&	operator=(const manager& rhs);	// Assign not ok
  manager(const manager& rhs);		// Copy not ok

  service*	createService(const std::string& name, const std::string& path,
			      struct stat&);
  bool		checkRendezvousDirectory();
  const char*	getConfigurationDirectory() const { return _configurationDirectory; }
  const char*	rebuildLookup();
  service*	findServiceInServiceMap(const std::string&);
  void		destroyOffspring(threadedObject* to=0, const char* reason="");

  // Configuration parameters from command line

  const char* 	_commandPortInterface;
  const char* 	_configurationDirectory;
  std::string	_rendezvousDirectory;
  const char* 	_lookupMapFile;

  int	_emergencyExitDelay;
  int   _statisticsLogInterval;
  int	_defaultUID;
  int	_defaultGID;
  int   _stStackSize;

  // Signal controlled and variable control of server state

  bool		_logStatsFlag;
  bool		_configurationReloadFlag;
  time_t	_configurationReloadAfter;
  bool		_reapChildrenFlag;

  const char*	_quitMessage;
  int		_commandAcceptSocket;

  // Counters and stats

  int		_serviceCount;
  int		_activeProcessCount;
  int		_maximumProcessCount;
  int		_childCount;
  int		_zombieCount;

  int		_exitReasonCount[processExit::maxReasonCount];

  // Rate of change counters (for periodic reporting)

  int		_processAdded;
  int		_requestsReported;

  typedef P_STLMAP<std::string, service*, hashString>	serviceMapType;
  typedef serviceMapType::iterator		serviceMapIter;
  typedef serviceMapType::const_iterator	serviceMapConstIter;

  serviceMapType*	_serviceMap;	// Name to service lookup
  shmLookup*		_shmLookupPtr;

  // Misc

  time_t		_startTime;
  util::rateLimit	_forkLimiter;

  listenBacklog	_LB;
};

#endif
