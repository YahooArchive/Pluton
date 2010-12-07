#ifndef _SERVICE_H
#define _SERVICE_H

#include <string>
#include <sstream>
#include "ostreamWrapper.h"
#include <list>
#include <vector>

#include "hash_mapWrapper.h"

#include <sys/types.h>
#include <time.h>

class manager;
class process;

#include "hashString.h"
#include "hashPointer.h"
#include "rateLimit.h"
#include "serviceConfig.h"
#include "shmService.h"
#include "threadedObject.h"


//////////////////////////////////////////////////////////////////////
// A service exists for each service configuration filex. A service
// manages the process instances. The service is also responsible for
// managing rogue services - ie, those crashing a lot, don't start up
// properly or don't exist.
//////////////////////////////////////////////////////////////////////

class service : public threadedObject {
 public:
  service(const std::string& name, manager* M);
  ~service();

  bool	initialize();			// Called by parent thread
  void	preRun();			// Called by owning thread
  bool	run();
  void	runUntilIdle();
  void	initiateShutdownSequence(const char* reason);
  void	completeShutdownSequence();
  void	destroyOffspring(threadedObject* t=0, const char* reason="");

  const char* 	getType() const { return "service"; }

  static bool	startThread(service*);


  const std::string&	getName() const { return _name; }
  const std::string&	getAcceptPath() const { return _acceptPath; }

  bool			loadConfiguration(const std::string& path);
  serviceConfig&	getConfig() { return _config; }

  manager*	getMANAGER() const { return _M; }

  // Config and other accessors

  const char*	getCD() const { return _config.cd.c_str(); }
  const char*	getEXEC() const { return _config.exec.c_str(); }

  long		getIdleTimeout() const { return _config.idleTimeout; }
  long		getUnresponsiveTimeout() const { return _config.unresponsiveTimeout; }

  long		getUlimitCPUMilliSeconds() const { return _config.ulimitCPUMilliSeconds; }
  long		getUlimitDATAMemory() const { return _config.ulimitDATAMemory; }
  long		getUlimitOpenFiles() const { return _config.ulimitOpenFiles; }
  long		getMaximumRequests() const { return _config.maximumRequests; }
  long		getMinimumProcesses() const { return _config.minimumProcesses; }
  long		getMaximumProcesses() const { return _config.maximumProcesses; }

  int		getActiveProcessCount() const { return _activeProcessCount; }
  void		subtractActiveProcessCount() { --_activeProcessCount; }

  void		addChild() { ++_childCount; }
  void		subtractChild() { --_childCount; }
  int		getChildCount() const { return _childCount; }

  void		removeAcceptPath();
  void		removeServiceMap();

  // Track object usage so that leakage can be detected

  static void	list(std::ostringstream&, const char* name=0);
  static int	getCurrentObjectCount() { return currentObjectCount; }
  static int	getMaximumObjectCount() { return maximumObjectCount; }

  // Special OS/X support

#ifdef __APPLE__
  static void	closeAllfdsExcept(const service*, int aboveFD);
  void		closeMyfds(int aboveFD) const;
#endif

  // process controls

  int	getReportingChannelReaderSocket() const { return _reportingChannelPipes[0]; }
  int	getReportingChannelWriterSocket() const { return _reportingChannelPipes[1]; }


  void	setConfigurationPath(const std::string& sPath) { _configurationPath = sPath; }
  void	setRendezvousDirectory(const std::string& rDir) { _rendezvousDirectory = rDir; }

  // Config change detection is via fstat() values

  ino_t		getCFInode() const { return _CFInode; }
  void		setCFInode(ino_t s) { _CFInode = s; }

  off_t		getCFSize() const { return _CFSize; }
  void		setCFSize(off_t s) { _CFSize = s; }

  time_t	getCFMtime() const { return _CFMtime; }
  void		setCFMtime(time_t s) { _CFMtime = s; }

  ////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////

 private:
  service&	operator=(const service& rhs);	// Assign not ok
  service(const service& rhs);			// Copy not ok

  static const int minimumSamplePeriod = 11;	// For calibrating services

  int	secondsToNextCreation(int upperLimit);
  bool	createAcceptSocket(const std::string& socketDirectory);
  bool	createServiceMap(const std::string& mapDirectory);
  bool	createAllowed(bool applyRateLimiting=true, bool justTesting=true);
  bool	createProcess(const char* reason="", bool applyRateLimiting=true);

  void	trackCosts(unsigned int durationMicroSeconds);
  void	calibrateProcesses(const char* why, int acceptQueueLength,
			   bool decreaseOkFlag, bool increaseToMinimumOkFlag);

  bool	calibrateOccupancyUp(const char* why, const struct timeval& now, int samplePeriod);
  bool	calibrateOccupancyDown(const char* why, const struct timeval& now, int samplePeriod);
  bool	calibrateIdleDown(const char* why, const struct timeval& now, int samplePeriod);

  process*	findOldestProcess();
  bool		removeOldestProcess(processExit::reason why);

  void	listenForAccept();
  void	readReportingChannel();

  static const int pollInterval = 5;

  static int		currentObjectCount;
  static int		maximumObjectCount;

  typedef		hash_map<const service*, service*, hashPointer>	trackMap;
  static trackMap	serviceTracker;

  std::string	_configurationPath;
  std::string	_rendezvousDirectory;

  ino_t		_CFInode;
  off_t		_CFSize;
  time_t	_CFMtime;

  int				_acceptSocket;
  int				_shmServiceFD;
  pluton::shmServiceHandler	_shmService;
  st_netfd_t			_stAcceptingFD;

  std::string		_acceptPath;
  std::string		_shmPath;

  int		_reportingChannelPipes[2];
  st_netfd_t	_stReportingFD;
  time_t	_lastPerformanceReport;

  serviceConfig	_config;

  std::string 	_name;
  manager*	_M;

  enum { isLocalService, isRemoteService } _type;

  bool		_shutdownFlag;
  time_t	_nextStartAttempt;
  const char*	_shutdownReason;

  ////////////////////////////////////////////////////////////

  std::list<int>	_unusedIds;		// Pre-allocated for ID

  hash_map<const process*, process*, hashPointer> 	_processMap;
  typedef hash_map<const process*, process*, hashPointer>::iterator	processMapIter;
  typedef hash_map<const process*, process*, hashPointer>::const_iterator processMapConstIter;

  int		_childCount;			// Forked but not exited
  int		_activeProcessCount;		// Forked and prior to shutdown

  //////////////////////////////////////////////////////////////////////
  // Manage the process count via occupancy percentages and backlog
  // queue lengths.
  //////////////////////////////////////////////////////////////////////

  time_t	_prevCalibrationTime;
  time_t	_nextCalibrationTime;
  float		_prevOccupancyByTime;
  float		_currOccupancyByTime;
  float		_prevListenBacklog;
  float		_currListenBacklog;
  int		_calibrateSamples;

  long		_timeInProcess;			// uSecs from reporting Channel
  long		_requestsCompleted;

  //////////////////////////////////////////////////////////////////////
  // In many cases, apart from occupancy, the creation of new
  // processes is purposely limited in the event that a service is
  // exit(0)ing unreasonably.
  //////////////////////////////////////////////////////////////////////

  util::rateLimit	_startLimiter;		// One start per second
};

#endif
