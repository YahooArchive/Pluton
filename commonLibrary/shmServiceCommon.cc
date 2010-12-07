#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "pluton/fault.h"

#include "global.h"
#include "processExitReason.h"
#include "shmService.h"
#include "util.h"


#ifndef MAP_NOSYNC
#define MAP_NOSYNC 0
#endif

pluton::shmServiceHandler::shmServiceHandler()
  : _shmServicePtr(0), _shmProcessPtr(0), _shmThreadPtr(0),
    _mapSize(0),
    _maxProcesses(0), _maxThreads(0),
    _myPid(0), _myTid(0)
{
  _myLastStart.tv_sec = 0;
  _myLastStart.tv_usec = 0;
}

pluton::shmServiceHandler::~shmServiceHandler()
{
  if (_shmServicePtr) munmap(static_cast<void*>(_shmServicePtr), _mapSize);
}

//////////////////////////////////////////////////////////////////////
// Map the serviceMap for the service-side. This should be the first
// call to this class as the mapping is needed for most of the other
// methods to do something useful. A tid of -1 indicates a
// non-threaded service. In either case, the threadedness of the
// service must match the config.
//////////////////////////////////////////////////////////////////////

pluton::faultCode
pluton::shmServiceHandler::mapService(int fd, pid_t pid, bool threadedFlag)
{
  // Need to know the size to map

  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    close(fd);
    return pluton::shmFstatFailed;
  }

  _mapSize = sb.st_size;

  void* p = mmap(0, _mapSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NOSYNC, fd, 0);
  if (p == MAP_FAILED) return pluton::shmMmapFailed;

  shmService* tmpSP = static_cast<shmService*>(p);

  if (tmpSP->_header._version != plutonGlobal::shmServiceVersion) return pluton::shmVersionMisMatch;

  _shmServicePtr = static_cast<shmService*>(p);
  _myConfig = _shmServicePtr->_config;

  if (!threadedFlag) {					// Non-threaded service but a threaded
    if (_myConfig._maximumThreads > 1) return pluton::shmThreadConfigNotService;	// config
  }
  else {						// Threaded service but not a threaded
    if (_myConfig._maximumThreads == 1) return pluton::shmThreadServiceNotConfig;	// config
  }

  _myPid = pid;

  return pluton::noFault;
}


bool
pluton::shmServiceHandler::setProcessReady(const struct timeval& now)
{
  if (!_shmProcessPtr) if (!resolvePointers()) return false;

  shmTimeVal tv;
  tv.tv_sec = now.tv_sec;
  tv.tv_usec = now.tv_usec;
  _shmProcessPtr->_lastActive = _shmThreadPtr->_lastActive = tv;

  return true;
}


//////////////////////////////////////////////////////////////////////
// Active transitions are recorded in shm so that the manager can
// understand the true state of the service. Whether it is stalled,
// idle or very busy. Concurrency of service is determined by the
// amount of active time vs idle time as measured by the service via
// this method.
//
// Because these counters are in shared memory that are not locked,
// they will *sometimes* be wrong due to update collisions, but that
// is likely to be very rare and the occassional collision doesn't
// really have much bearing as they are routinely reset to zero and
// re-sampled.
//////////////////////////////////////////////////////////////////////

void
pluton::shmServiceHandler::startResponseTimer(const struct timeval& now, bool affinity)
{
  if (!_shmProcessPtr) if (!resolvePointers()) return;

  _myLastStart = now;

  shmTimeVal tv;
  tv.tv_sec = now.tv_sec;
  tv.tv_usec = now.tv_usec;
  _shmProcessPtr->_lastActive = _shmThreadPtr->_lastActive = tv;
  _shmThreadPtr->_affinityFlag = affinity;

  //////////////////////////////////////////////////////////////////////
  // Opportunistically count the current service occupancy for the
  // manager since their sampling may never catch peaks.
  //////////////////////////////////////////////////////////////////////

  unsigned int maxActive = 0;
  for (int pidx=0; pidx < _myConfig._maximumProcesses; ++pidx) {
    if (_shmServicePtr->_process[pidx]._pid == 0) continue;
    for (int tidx=0; tidx < _myConfig._maximumThreads; ++tidx) {
      if (_shmServicePtr->_process[pidx]._thread[tidx]._activeFlag) ++maxActive;
    }
  }

  if (maxActive > _shmServicePtr->_aggregateCounters._maximumActiveCount) {
    _shmServicePtr->_aggregateCounters._maximumActiveCount = maxActive;
  }
}


//////////////////////////////////////////////////////////////////////
// Calculate duration of request and transfer state to shm
//////////////////////////////////////////////////////////////////////

void
pluton::shmServiceHandler::stopResponseTimer(const struct timeval& now)
{
  if (!_shmProcessPtr) if (!resolvePointers()) return;

  // setProcessActive may not have resolvedPointers due to Manager
  // tardiness, so only unwind the setProcessActive actions if it
  // took any.

  if (_myLastStart.tv_sec > 0) {
    long uSecs = util::timevalDiffuS(now, _myLastStart);
    _shmProcessPtr->_activeuSecs += uSecs;
    _shmServicePtr->_aggregateCounters._activeuSecs += uSecs;
    ++_shmServicePtr->_aggregateCounters._requestCount;
  }
}


//////////////////////////////////////////////////////////////////////
// The true definition of an inactive service is when a service/thread
// is sitting on an accept() waiting for a request. This is the
// definition the manager need to distinguish between an idle and
// stalled service.
//////////////////////////////////////////////////////////////////////

void
pluton::shmServiceHandler::setProcessAcceptingRequests(bool tf)
{
  if (!_shmProcessPtr) if (!resolvePointers()) return;

  if (tf) {
    _shmThreadPtr->_activeFlag = false;
    --_shmProcessPtr->_activeThreadCount;
    if (--_shmProcessPtr->_activeThreadCount < 0) _shmProcessPtr->_activeThreadCount = 0;
  }
  else {
    _shmThreadPtr->_activeFlag = true;
    ++_shmProcessPtr->_activeThreadCount;
  }
}


//////////////////////////////////////////////////////////////////////
// Various counter transfer routines. All of these *set* their values
// rather than increment to minimize non-lock issues with shm.
//////////////////////////////////////////////////////////////////////

void
pluton::shmServiceHandler::setProcessRequestCount(int c)
{
  if (!_shmProcessPtr) if (!resolvePointers()) return;

  _shmProcessPtr->_requestCount = c;
}

void
pluton::shmServiceHandler::setProcessResponseCount(int c)
{
  if (!_shmProcessPtr) if (!resolvePointers()) return;

  _shmProcessPtr->_responseCount = c;
}

void
pluton::shmServiceHandler::setProcessFaultCount(int c)
{
  if (!_shmProcessPtr) if (!resolvePointers()) return;

  _shmProcessPtr->_faultCount = c;
}


//////////////////////////////////////////////////////////////////////
// Transfer details to shared memory so others can share the joy.
//////////////////////////////////////////////////////////////////////

void
pluton::shmServiceHandler::setProcessClientDetails(unsigned int requestID,
						   struct timeval startTime,
						   const char* cnamePtr, int cnameLength)
{
  if (!_shmProcessPtr) if (!resolvePointers()) return;

  _shmThreadPtr->_clientRequestID = requestID;

  shmTimeVal tv;
  tv.tv_sec = startTime.tv_sec;
  tv.tv_usec = startTime.tv_usec;
  _shmThreadPtr->_clientRequestStartTime = tv;

  unsigned int cpLen = cnameLength;
  if (cpLen >= sizeof(_shmThreadPtr->_clientRequestName)) {
    cpLen = sizeof(_shmThreadPtr->_clientRequestName)-1;
  }

  strncpy(_shmThreadPtr->_clientRequestName, cnamePtr, cpLen);
  _shmThreadPtr->_clientRequestName[cpLen] = '\0';
}


void
pluton::shmServiceHandler::setProcessExitReason(processExit::reason er)
{
  if (!_shmProcessPtr) if (!resolvePointers()) return;

  _shmProcessPtr->_exitReason = er;
}


void
pluton::shmServiceHandler::setProcessTerminated()
{
  if (!_shmProcessPtr) if (!resolvePointers()) return;

  _shmProcessPtr->_pid = 0;
}


processExit::reason
pluton::shmServiceHandler::getProcessShutdownRequest()
{
  if (!_shmProcessPtr) if (!resolvePointers()) return processExit::noReason;

  return _shmProcessPtr->_shutdownRequest;
}


time_t
pluton::shmServiceHandler::getManagerHeartbeat()
{
  if (!_shmServicePtr) if (!resolvePointers()) return 0;

  return _shmServicePtr->_managerHeartbeat;
}


//////////////////////////////////////////////////////////////////////
// Find the shmProcess and shmThread that matches this service
// handler. Since the pid in that structure is set independantly by
// the manager after the child is forked, there is a timing window
// whereby it's possible that it may not be set for the first few
// moments. Accordingly, not finding the entry is treated pretty
// passively by callers.
//////////////////////////////////////////////////////////////////////

bool
pluton::shmServiceHandler::resolvePointers()
{
  if (!_shmServicePtr) return false;		// If not mapped at all, can't search

  for (int pidx=0; pidx < _myConfig._maximumProcesses; ++pidx) {
    if (_shmServicePtr->_process[pidx]._pid == _myPid) {
      _shmProcessPtr = &_shmServicePtr->_process[pidx];
      _shmThreadPtr = &_shmProcessPtr->_thread[_myTid];
      return true;
    }
  }

  return false;
}
