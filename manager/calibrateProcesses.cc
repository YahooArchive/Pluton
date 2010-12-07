#include <iostream>

#include <sys/time.h>
#include <sys/types.h>

#include <signal.h>

#include "debug.h"
#include "logging.h"
#include "util.h"
#include "pidMap.h"
#include "processExitReason.h"
#include "manager.h"
#include "service.h"
#include "process.h"

using namespace std;


//////////////////////////////////////////////////////////////////////
// Determine how many processes should be running for a given service
// and adjust accordingly.
//
// Calibration is called for the following reasons:
//
// o) A serviceSocket has a connect request and no processes are
//    running (the service is in the initial state).
//
// o) A process has sent a report via the reportingChannel
//
// o) The service has timed out waiting for a report
//
// o) A process has exited
//////////////////////////////////////////////////////////////////////

void
service::calibrateProcesses(const char* why, int listenBacklog,
			    bool decreaseOkFlag, bool increaseToMinimumOkFlag)
{
  if (debug::calibrate()) DBGPRT << "calibrate: " << _name << ":" << why
				 << " do=" << decreaseOkFlag << " io=" << increaseToMinimumOkFlag
				 << endl;

  //////////////////////////////////////////////////////////////////////
  // If we need to ramp up to minimum, do that and return
  //////////////////////////////////////////////////////////////////////

  if (increaseToMinimumOkFlag && (_activeProcessCount < _config.minimumProcesses)) {
    int pc = _config.minimumProcesses - _activeProcessCount;
    while (pc-- > 0) createProcess("createMinimum", false);
    return;
  }

  ++_calibrateSamples;
  _currListenBacklog += listenBacklog;

  //////////////////////////////////////////////////////////////////////
  // Only check for algorithmic changes periodically, so that activity
  // times and counters aren't too granular.
  //////////////////////////////////////////////////////////////////////

  struct timeval now;
  gettimeofday(&now, 0);
  bool doneOne = false;
  int samplePeriod = now.tv_sec - _prevCalibrationTime;

  // First the occupancy-base tests which roll counters, etc.

  if (_nextCalibrationTime <= now.tv_sec) {
    _currListenBacklog /= _calibrateSamples;
    _calibrateSamples = 0;


    // Always do "Up" as that does calibration calcs

    doneOne = calibrateOccupancyUp(why, now, samplePeriod);


    // Only do "Down" if "Up" didn't create a process

    if (!doneOne && decreaseOkFlag && (_activeProcessCount > _config.minimumProcesses)) {
      doneOne = calibrateOccupancyDown(why, now, samplePeriod);
    }

    // Roll all the values from current -> previous

    _prevListenBacklog = _currListenBacklog;
    _currListenBacklog = 0;
    if (doneOne) {
      _prevOccupancyByTime = _config.occupancyPercent;	// That's our guess
    }
    else {
      _prevOccupancyByTime = _currOccupancyByTime;
    }
    _prevCalibrationTime = now.tv_sec;
    _nextCalibrationTime = now.tv_sec + minimumSamplePeriod;

    _shmService.resetAggregateCounters();
    _timeInProcess = 0;
    _requestsCompleted = 0;
  }

  // Idle timeout is done every call if nothing else happened.

  if (!doneOne && decreaseOkFlag) doneOne = calibrateIdleDown(why, now, samplePeriod);
}


//////////////////////////////////////////////////////////////////////
// Ramp up processes based on occupancy. Return true if at least one
// process was started.
//////////////////////////////////////////////////////////////////////

bool
service::calibrateOccupancyUp(const char* why, const struct timeval& now, int samplePeriod)
{
  if (debug::calibrate()) DBGPRT << "calibrateOccUp: " << _name << ":" << why << endl;

  //////////////////////////////////////////////////////////////////////
  // Get active uSeconds and request count for this sample period.
  //////////////////////////////////////////////////////////////////////

  long long activeuSecs = _shmService.getActiveuSecs();
  long requestCount = _shmService.getRequestCount();

  //////////////////////////////////////////////////////////////////////
  // Sanity check shared counters and timers. They can get trashed due
  // to lock-less updates or memory stomping services.
  //////////////////////////////////////////////////////////////////////

  long activeSecs = activeuSecs / util::MICROSECOND;
  if (	(activeSecs < 0) ||
	(activeSecs > (minimumSamplePeriod * 2 * _activeProcessCount))) {
    if (debug::calibrate()) DBGPRT << "no calibrate: " << _name
				   << " " << activeSecs << " " << _activeProcessCount << endl;
    return false;
  }

  //////////////////////////////////////////////////////////////////////
  // Average out the backlog across the sample period
  //////////////////////////////////////////////////////////////////////

  if (_activeProcessCount > 0) {
    _currOccupancyByTime = activeuSecs;
    _currOccupancyByTime /= _activeProcessCount;
    _currOccupancyByTime *= 100;
    _currOccupancyByTime /= static_cast<float>(samplePeriod * util::MICROSECOND);
  }
  else {
    _currOccupancyByTime = 0;
  }

  //////////////////////////////////////////////////////////////////////
  // Use a moving average occupancy to smooth spikes
  //////////////////////////////////////////////////////////////////////

  _currOccupancyByTime = _currOccupancyByTime * 0.75 + _prevOccupancyByTime * 0.25;
  _currListenBacklog = _currListenBacklog * 0.75 + _prevListenBacklog * 0.25;

  if (debug::calibrate()) {
    DBGPRT << "calibrateUp: " << _name << ":" << why << " sample=" << samplePeriod
	   << " uSecs=" << activeuSecs << "/" << _timeInProcess
	   << " reqs=" << requestCount << "/" << _requestsCompleted
	   << " apc=" << _activeProcessCount << "/" << _childCount
	   << " occ=" << (int) _currOccupancyByTime << "/" << (int) _prevOccupancyByTime
	   << " LQ=" << _currListenBacklog << "/" << _prevListenBacklog
	   << " targetO=" << _config.occupancyPercent
	   << endl;
  }

  //////////////////////////////////////////////////////////////////////
  // If more are needed, calculate how many more to get to the desired
  // occupancy and start them up.
  //////////////////////////////////////////////////////////////////////

  if (_currOccupancyByTime <= _config.occupancyPercent) return false;

  int shortFall = (int) _currOccupancyByTime - _config.occupancyPercent;
  shortFall *= _activeProcessCount;
  shortFall += 99;				// Round % up
  shortFall /= 100;
  if (shortFall == 0) ++shortFall;		// Make sure it's at least one
  if (_currListenBacklog >= 1) ++shortFall;	// These may overshoot since
  if ((_prevListenBacklog >= 10) && (_prevListenBacklog >= 10)) {
    shortFall += 5;
  }

  if (shortFall > (_config.maximumProcesses - _activeProcessCount)) {
    shortFall = _config.maximumProcesses - _activeProcessCount;
  }

  if (logging::calibrate()) {
    LOGPRT << "Calibrate Up: " << _name
	   << " apc=" << _activeProcessCount
	   << " currO=" << (int) _currOccupancyByTime
	   << " prevO=" << (int) _prevOccupancyByTime
	   << " currLQ=" << (int) _currListenBacklog
	   << " prevLQ=" << (int) _prevListenBacklog
	   << " shortfall=" << shortFall
	   << endl;
  }

  while (shortFall-- > 0) {
    if (!createProcess("createIncrease", false)) break;
  }

  return true;					// Say we started some
}


//////////////////////////////////////////////////////////////////////
// Ramp down processes based on occupancy. Return true if at least one
// process was told to exit.
//
// Calculate what the new occupancy would be if a process were to be
// removed. The idea is that a process will only be removed if the new
// occupancy is still below the threshold. In other words, don't
// remove a process if the result is that next time through this code
// a process is started up.
//
// n=new, o=old, C=count, O=occupancy
// nO = oO * nC / (nC-1)
//////////////////////////////////////////////////////////////////////

bool
service::calibrateOccupancyDown(const char* why, const struct timeval& now, int samplePeriod)
{
  if (debug::calibrate()) DBGPRT << "calibrateOccDown: " << _name << ":" << why << endl;

  float new1Occupancy = _currOccupancyByTime;
  new1Occupancy *= _activeProcessCount;
  new1Occupancy /= (_activeProcessCount - 1);

  float new2Occupancy = _prevOccupancyByTime;
  new2Occupancy *= _activeProcessCount;
  new2Occupancy /= (_activeProcessCount - 1);

  if (debug::calibrate()) DBGPRT << "calibrateDown: " << _name
				 << " new1Occ=" << new1Occupancy
				 << " new2Occ=" << new2Occupancy
				 << endl;

  int lowerThreshold = _config.occupancyPercent - 5;		// XX Hack

  if ((new1Occupancy >= lowerThreshold) && (new2Occupancy >= lowerThreshold)) return false;

  if (logging::calibrate()) {
    LOGPRT << "Calibrate Down: " << _name
	   << " apc=" << _activeProcessCount
	   << " currO=" << (int) _currOccupancyByTime
	   << " prevO=" << (int) _prevOccupancyByTime
	   << " propCO=" << (int) new1Occupancy
	   << " propPO=" << (int) new2Occupancy
	   << endl;
  }

  removeOldestProcess(processExit::excessProcesses);

  return true;
}


//////////////////////////////////////////////////////////////////////
// Idle timeout calibration only applies when no reports have been
// seen for the idle-timeout period and there is more than the
// configured minimum processes still running.
//////////////////////////////////////////////////////////////////////

bool
service::calibrateIdleDown(const char* why, const struct timeval& now, int samplePeriod)
{
  if (debug::calibrate()) DBGPRT << "calibrateIdle: " << _config.idleTimeout
				 << " apc=" << _activeProcessCount
				 << " lp=" << _lastPerformanceReport
				 << " now=" << now.tv_sec
				 << endl;

  if ((_config.idleTimeout == 0) || (_activeProcessCount <= _config.minimumProcesses)) {
    return false;
  }

  if ((_lastPerformanceReport + _config.idleTimeout) > now.tv_sec) return false;

  removeOldestProcess(processExit::idleTimeout);


  return true;
}


//////////////////////////////////////////////////////////////////////
// Create a new process for this service. Return true if the service
// started. The fork/exec is done here so that we *know* we have a
// pidMap entry prior to returning to the main loop and possibly
// reaping children.
//
// Return: true if a process was started.
//////////////////////////////////////////////////////////////////////

bool
service::createProcess(const char* reason, bool applyRateLimiting)
{
  if (debug::service()) DBGPRT << "service::createProcess: " << _logID
			       << " R=" << reason
			       << " UU=" << _unusedIds.size()
			       << " apc=" << _activeProcessCount
			       << " nextStart " << _nextStartAttempt
			       << endl;

  if (!createAllowed(applyRateLimiting, false)) return false;

  int id = _unusedIds.front();				// Get an unused id
  _unusedIds.pop_front();

  process* P = new process(this, id, _name,
			   _acceptSocket, _shmServiceFD, getReportingChannelWriterSocket(),
			   &_shmService);
  _processMap[P] = P;

  _M->addProcessCount();		// Add to aggregate totals
  ++_activeProcessCount;		// It's also active until shutdown

  if (!P->initialize()) {
    destroyOffspring(P, "initialize failed");
    return false;
  }

  if (debug::service()) DBGPRT << "Service: createProcess " << P->getID() << endl;

  return true;
}


//////////////////////////////////////////////////////////////////////
// Find the oldest process and initiate a shutdown.
//////////////////////////////////////////////////////////////////////

bool
service::removeOldestProcess(processExit::reason why)
{
  process* P = findOldestProcess();
  if (!P) {
    LOGPRT << "Warning: " << _logID << " Could not find oldest process" << endl;
    return false;
  }

  P->setShutdownReason(why, true);

  return true;
}
