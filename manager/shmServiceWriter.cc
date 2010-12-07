#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "pluton/fault.h"

#include "global.h"
#include "util.h"
#include "shmService.h"

using namespace std;

#ifndef MAP_NOSYNC
#define MAP_NOSYNC 0
#endif


//////////////////////////////////////////////////////////////////////
// Map and initialize the shmService memory for the manager.
//////////////////////////////////////////////////////////////////////

pluton::faultCode
pluton::shmServiceHandler::mapManager(int fd, int processCount, int threadCount)
{
  _mapSize = sizeof(shmService) + sizeof(shmProcess) * processCount
    + sizeof(shmThread) * processCount * threadCount;

  _maxProcesses = processCount;
  _maxThreads = threadCount;

  char buf = 0;
  int res = pwrite(fd, &buf, 1, _mapSize);	// Allocate out to _mapSize
  if (res != 1) return pluton::shmPreWriteFailed;

  void* p = mmap(0, _mapSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NOSYNC, fd, 0);
  if (p == MAP_FAILED) return pluton::shmMmapFailed;

  // Initialize the memory

  _shmServicePtr = static_cast<shmService*>(p);

  bzero((void*) _shmServicePtr, _mapSize);
  _shmServicePtr->_header._version = plutonGlobal::shmServiceVersion;

  return pluton::noFault;
}


//////////////////////////////////////////////////////////////////////
// Initialize the shmService entry for this process
//////////////////////////////////////////////////////////////////////

void
pluton::shmServiceHandler::resetProcess(int id, struct timeval* forkTime)
{
  shmProcess* sP = &_shmServicePtr->_process[id];

  sP->_pid = 0;
  sP->_activeThreadCount = 0;
  sP->_requestCount = 0;
  sP->_responseCount = 0;
  sP->_faultCount = 0;
  sP->_activeuSecs = 0;
  if (forkTime) {
    sP->_firstActive.tv_sec = forkTime->tv_sec;
    sP->_firstActive.tv_usec = forkTime->tv_usec;
  }
  else {
    sP->_firstActive.tv_sec = 0;
    sP->_firstActive.tv_usec = 0;
  }

  sP->_shutdownRequest = processExit::noReason;
  sP->_exitReason = processExit::noReason;

  for (int tidx=0; tidx < _maxThreads; ++tidx) {
    sP->_thread[tidx]._activeFlag = false;
    sP->_thread[tidx]._firstActive = sP->_firstActive;
    sP->_thread[tidx]._lastActive = sP->_lastActive;
    sP->_thread[tidx]._clientRequestID = 0;
    sP->_thread[tidx]._clientRequestStartTime.tv_sec = 0;
    sP->_thread[tidx]._clientRequestStartTime.tv_usec = 0;
    sP->_thread[tidx]._clientRequestEndTime.tv_sec = 0;
    sP->_thread[tidx]._clientRequestEndTime.tv_usec = 0;
    sP->_thread[tidx]._clientRequestName[0] = '\0';
  }
}


//////////////////////////////////////////////////////////////////////
// The manager sets the pid and per-pid parameters so the the service
// can find it's _process[] entry.
//////////////////////////////////////////////////////////////////////

void
pluton::shmServiceHandler::setProcessPID(int id, pid_t pid)
{
  _shmServicePtr->_process[id]._pid = pid;
}

void
pluton::shmServiceHandler::setProcessShutdownRequest(int id, processExit::reason er)
{
  _shmServicePtr->_process[id]._shutdownRequest = er;
}

processExit::reason
pluton::shmServiceHandler::getProcessExitReason(int id) const
{
  return _shmServicePtr->_process[id]._exitReason;
}

bool
pluton::shmServiceHandler::getProcessActiveFlag(int id) const
{
  return _shmServicePtr->_process[id]._activeThreadCount > 0;
}

void
pluton::shmServiceHandler::getProcessLastRequestTime(int id, struct timeval& tv) const
{
  tv.tv_sec = _shmServicePtr->_process[id]._lastActive.tv_sec;
  tv.tv_usec = _shmServicePtr->_process[id]._lastActive.tv_usec;
}


int
pluton::shmServiceHandler::getProcessRequestCount(int id) const
{
  return _shmServicePtr->_process[id]._requestCount;
}

int
pluton::shmServiceHandler::getProcessResponseCount(int id) const
{
  return _shmServicePtr->_process[id]._responseCount;
}

int
pluton::shmServiceHandler::getProcessFaultCount(int id) const
{
  return _shmServicePtr->_process[id]._faultCount;
}

unsigned int
pluton::shmServiceHandler::getThreadClientRequestID(int id, int tid) const
{
  return _shmServicePtr->_process[id]._thread[tid]._clientRequestID;
}

const char*
pluton::shmServiceHandler::getThreadClientRequestName(int id, int tid) const
{
  return _shmServicePtr->_process[id]._thread[tid]._clientRequestName;
}

void
pluton::shmServiceHandler::getThreadClientRequestStartTime(int id, int tid,
							   struct timeval& tv) const
{
  tv.tv_sec = _shmServicePtr->_process[id]._thread[tid]._clientRequestStartTime.tv_sec;
  tv.tv_usec = _shmServicePtr->_process[id]._thread[tid]._clientRequestStartTime.tv_usec;
}

long
pluton::shmServiceHandler::getProcessActiveuSecs(int id) const
{
  return _shmServicePtr->_process[id]._activeuSecs;
}

pid_t
pluton::shmServiceHandler::getProcessPID(int id) const
{
  return _shmServicePtr->_process[id]._pid;
}


void
pluton::shmServiceHandler::updateManagerHeartbeat(time_t now) const
{
  _shmServicePtr->_managerHeartbeat = now;
}

int
pluton::shmServiceHandler::getMaximumActiveCount() const
{
  return _shmServicePtr->_aggregateCounters._maximumActiveCount;
}

long long
pluton::shmServiceHandler::getActiveuSecs() const
{
  return _shmServicePtr->_aggregateCounters._activeuSecs;
}

long
pluton::shmServiceHandler::getRequestCount() const
{
  return _shmServicePtr->_aggregateCounters._requestCount;
}

void
pluton::shmServiceHandler::resetAggregateCounters()
{
  _shmServicePtr->_aggregateCounters._maximumActiveCount = 0;
  _shmServicePtr->_aggregateCounters._activeuSecs = 0;
  _shmServicePtr->_aggregateCounters._requestCount = 0;
}


//////////////////////////////////////////////////////////////////////
// Search for the oldest (nee best) process to kill. Oldest means the
// process that has handled the most requests. Return the index of the
// process if one is found or -1 if none found.
//////////////////////////////////////////////////////////////////////

int
pluton::shmServiceHandler::getOldestProcess() const
{
  unsigned int	highestRequestCount = 0;
  int	iXofHighestRequestCount = -1;

  for (int ix=0; ix < _shmServicePtr->_config._maximumProcesses; ++ix) {
    if ((_shmServicePtr->_process[ix]._pid != 0) &&
	(_shmServicePtr->_process[ix]._requestCount > highestRequestCount)) {
      iXofHighestRequestCount = ix;
      highestRequestCount = _shmServicePtr->_process[ix]._requestCount;
    }
  }

  return iXofHighestRequestCount;
}
