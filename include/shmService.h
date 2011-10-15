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

#ifndef P_SHMSERVICE_H
#define P_SHMSERVICE_H

//////////////////////////////////////////////////////////////////////
// shmService is a shared memory segment that contains details about a
// particular service. It is used to communicate config information to
// child processes and for child processes to communicate their state
// back to the manager.
//////////////////////////////////////////////////////////////////////

#include <sys/time.h>
#include <sys/types.h>

#include "stdintWrapper.h"

#include "pluton/fault.h"
#include "processExitReason.h"


//////////////////////////////////////////////////////////////////////
// There are three main structures in shared memory for each service.
//
// shmService contains configuration and aggregate values calculated
// by the services. It also contains an array of shmProcess structures.
//
// There is a unique shmProcess for each process that can be
// forked. It contains response-time and activity data that is used to
// calibrate the number of processes.
//
// Normally there is just one thread per service, but special cases
// exist for things like pluton tools, particularly plTransmitter-type
// programs. For this reason, the shmThread structure exists to
// identify each client connection and request.
//
// The shm class does *not* provide any locking between the manager
// and the service processes, nor does it lock for any concurrent
// thread access by the services.
//
// It is important that all variables are defined with specific size
// types as generic types such as int and long may vary between 32bit
// and 64bit executables running on the same CPU. It is assumed that
// system-defined structures take this into account; particularly
// timeval. One risky case are enums. Is it possible for them to be
// different sizes for 32/64 bit compiles? Theoretically,
// maybe. Practically, not.
//////////////////////////////////////////////////////////////////////

class shmConfig {
 public:
  int32_t	_maximumProcesses;		// 4
  int32_t	_maximumThreads;		// 8 *
  int32_t	_maximumRequests;		// 12
  int32_t	_maximumRetries;		// 16 *
  int32_t	_idleTimeout;			// 20
  int32_t	_affinityTimeout;		// 24 *
  int32_t	_recorderCycle;			// 28
  int32_t	_align1;			// 32 *
  char		_recorderPrefix[256];		// 32+64 *
};


//////////////////////////////////////////////////////////////////////
// All shared structures have to be the same for both 32 and 64
// programs, so we cannot use any syste defined structures directly.
//////////////////////////////////////////////////////////////////////

typedef struct {
  uint64_t	tv_sec;				// 8 *
  uint64_t	tv_usec;			// 16 *
} shmTimeVal;

class shmThread {
 public:
  uint32_t		_activeFlag;		// 4
  uint32_t		_affinityFlag;		// 8 *
  shmTimeVal		_firstActive;		// 24 *
  shmTimeVal		_lastActive;		// 40 *

  uint32_t		_clientRequestID;	// 44
  uint32_t		_align2;		// 48 *
  shmTimeVal		_clientRequestStartTime; // 64 *
  shmTimeVal		_clientRequestEndTime;	// 80 *
  char			_clientRequestName[128]; // 80 + 16 *
};


class shmProcess {
 public:
  int32_t	_pid;			// 4
  int32_t	_activeThreadCount;	// 8 * Leave as signed to detect/repair -ve values
  uint32_t	_requestCount;		// 12
  uint32_t	_responseCount;		// 16 *
  uint32_t	_faultCount;		// 20
  uint32_t	_activeuSecs;		// 24 * Aggregate for all threads

  shmTimeVal	_firstActive;		// 40 *
  shmTimeVal	_lastActive;		// 56 *

  processExit::reason	_shutdownRequest;	// 60
  processExit::reason	_exitReason;	// 64 *

  shmThread	_thread[1];		// 64 + 80 + 16 *
};


class shmService {
 public:
  struct {
    uint32_t	_version;		// Set from global.h
    int32_t	_align1;
  } _header;				// 8 * Header is only set by manager

  uint64_t	_managerHeartbeat;	// 16 * Manager ticks this to keep services alive
  shmConfig	_config;		// * Set by Manager, copied by service

  struct {
    uint32_t	_maximumActiveCount;
    uint32_t	_requestCount;
    uint64_t	_activeuSecs;
  } _aggregateCounters;			// * + 16

  shmProcess	_process[1];	// Set by Manager and service
};


//////////////////////////////////////////////////////////////////////
// The handler for the shm structures
//////////////////////////////////////////////////////////////////////

namespace pluton {
  class shmServiceHandler {
  public:
    shmServiceHandler();
    ~shmServiceHandler();

    //////////////////////////////////////////////////////////////////////
    // Mapping methods
    //////////////////////////////////////////////////////////////////////

    pluton::faultCode	mapManager(int fd, int processCount, int threadCount);
    pluton::faultCode	mapService(int fd, pid_t pid, bool threadedFlag);

    bool	ifMapped() const { return _shmServicePtr != 0; }

    shmService*	getServicePtr() { return _shmServicePtr; }
    shmProcess*	getProcessPtr() { return _shmProcessPtr; }
    shmThread*	getThreadPtr() { return _shmThreadPtr; }

    //////////////////////////////////////////////////////////////////////
    // Config and limit methods
    //////////////////////////////////////////////////////////////////////

    int		getSize() { return _mapSize; }
    int		getMaximumProcess() const { return _myConfig._maximumProcesses; }
    int		getMaximumThreads() const { return _myConfig._maximumThreads; }
    int		getMaximumRetries() const { return _myConfig._maximumRetries; }
    int		getMaximumRequests() const { return _myConfig._maximumRequests; }
    int		getIdleTimeout() const { return _myConfig._idleTimeout; }

    //////////////////////////////////////////////////////////////////////
    // Aggregate Service methods
    //////////////////////////////////////////////////////////////////////

    int		getOldestProcess() const;
    int		getMaximumActiveCount() const;
    long long	getActiveuSecs() const;
    long	getRequestCount() const;
    void	resetAggregateCounters();

    //////////////////////////////////////////////////////////////////////
    // Manager to Process methods
    //////////////////////////////////////////////////////////////////////

    void	updateManagerHeartbeat(time_t now) const;

    void	resetProcess(int id, struct timeval* forkTime=0);
    void	setProcessPID(int id, pid_t pid);
    void	setProcessShutdownRequest(int id, processExit::reason);

    bool	getProcessActiveFlag(int id) const;
    void	getProcessLastRequestTime(int id, struct timeval&) const;
    int		getProcessRequestCount(int id) const;
    int		getProcessResponseCount(int id) const;
    int		getProcessFaultCount(int id) const;

    unsigned int getThreadClientRequestID(int id, int tid) const;
    const char* getThreadClientRequestName(int id, int tid) const;
    void	getThreadClientRequestStartTime(int id, int tid, struct timeval&) const;
    long	getProcessActiveuSecs(int id) const;

    pid_t	getProcessPID(int id) const;
    void	resetProcessCalibration(int id);
    processExit::reason	getProcessExitReason(int id) const;

    ////////////////////
    // Process methods
    ////////////////////

    bool	setProcessReady(const struct timeval& now);
    void	startResponseTimer(const struct timeval& now, bool affinity);
    void	stopResponseTimer(const struct timeval& now);
    void	setProcessAcceptingRequests(bool);
    void	setProcessRequestCount(int);
    void	setProcessResponseCount(int);
    void	setProcessFaultCount(int);
    void	setProcessClientDetails(unsigned int requestID, struct timeval,
					const char* cnamePtr, int cnameLength);
    void	setProcessExitReason(processExit::reason);
    void	setProcessTerminated();

    processExit::reason	getProcessShutdownRequest();
    time_t	getManagerHeartbeat();

  private:
    bool	resolvePointers();	// Search for pid and set _shmProcessPtr, _shmThreadPtr

    shmService*		_shmServicePtr;
    shmProcess*		_shmProcessPtr;
    shmThread*		_shmThreadPtr;

    //////////////////////////////////////////////////////////////////////
    // Take copies of essential values - never rely on the contents of
    // shm to remain valid.
    //////////////////////////////////////////////////////////////////////

    int			_mapSize;
    int			_maxProcesses;	// Manager only
    int			_maxThreads;	// Manager only

    // Service copies these

    pid_t		_myPid;
    int			_myTid;
    struct timeval	_myLastStart;
    shmConfig		_myConfig;	// Copy to protect against corruption
  };
}

#endif
