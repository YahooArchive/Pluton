#ifndef _MANAGER_H
#define _MANAGER_H

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

  typedef hash_map<std::string, service*, hashString>	serviceMapType;
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
