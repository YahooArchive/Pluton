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

#ifndef _PERCALLERCLIENT_H
#define _PERCALLERCLIENT_H

#include <string>

#include <sys/time.h>

#ifndef NO_INCLUDE_POLL
#include <poll.h>
#endif
#include "signal.h"

#include "timeoutClock.h"
#include "requestImpl.h"
#include "clientRequestImpl.h"
#include "requestQueue.h"


//////////////////////////////////////////////////////////////////////
// This wrapper class contains unique per-caller state. In particular,
// timeout limits, name, fault and request management details. Most of
// the actual work is done in the clientImpl singleton. This class is
// particularly important for libevent and state thread support and
// true thread support as well as nested callers.
//////////////////////////////////////////////////////////////////////

namespace pluton {

  class clientImpl;

  class perCallerClient {
  public:
    perCallerClient(const char* yourName, unsigned int defaultTimeoutMilliSeconds);
    ~perCallerClient();

    void		assertMutexFreeThenLock();
    void		unlockMutex() { _oneAtATimePerCaller = false; }

    void		setMyThreadImpl(pluton::clientImpl* n) { _myThreadImpl = n; }
    pluton::clientImpl*	getMyThreadImpl() const { return _myThreadImpl; }

    unsigned int	getTimeoutMilliSeconds() const { return _timeoutMilliSeconds; }
    void		setTimeoutMilliSeconds(unsigned int defaultTimeoutMilliSeconds);
    const std::string&	getClientName() { return _clientName; }

    void		addTodoCount();
    void		subtractTodoCount() { --_todoCount; }
    int			getTodoCount() const { return _todoCount; }

    void		addReadingCount() { ++_readingCount; }
    void		subtractReadingCount() { --_readingCount; }
    int			getReadingCount() const { return _readingCount; }
    struct pollfd*	sizePollArray(unsigned int entriesNeeded);

    pluton::timeoutClock&	getClock() { return _clock; }

    void			setRequestIndex(unsigned int ix, pluton::clientRequestImpl*);
    pluton::clientRequestImpl*	getRequestByIndex(unsigned int ix);

    // Completed Request Queue management

    bool			completedQueueEmpty() const { return _completedQueue.empty(); }
    void			addToCompletedQueue(clientRequestImpl* R)
    { _completedQueue.addToHead(R); }

    pluton::clientRequestImpl*	popCompletedRequest();
    void			dumpQueue() const { _completedQueue.dump("completed"); }

    pluton::clientRequestImpl*	firstCompletedRequest() const
      { return _completedQueue.getFirst(); }

    pluton::clientRequestImpl*	nextCompletedRequest(pluton::clientRequestImpl* R) const
      { return _completedQueue.getNext(R); }

    bool	deleteCompletedRequest(pluton::clientRequestImpl* R)
    { return _completedQueue.deleteRequest(R); }

    void	eraseCompletedQueue(pluton::clientRequestImpl::state);
    int		getCompletedQueueCount() const { return _completedQueue.count(); };

    void		blockSIGPIPE();
    void		restoreSIGPIPE();

    bool			hasFault() const { return _fault.hasFault(); }
    const pluton::fault&	getFault() const { return _fault; }
    pluton::faultInternal*	getFaultPtr() { return &_fault; }

    void	setThreadedApplication(bool, pluton::thread_t);
    bool	getThreadedApplication() const { return _threadedApplication; }
    bool	threadOwnerUnchanged(pluton::thread_t id) const
    {
      return !_threadedApplication || (id == _owningThreadID);
    }

  private:
    bool		_oneAtATimePerCaller;	// Make sure threads serialize around me
    bool		_threadedApplication;
    pluton::thread_t	_owningThreadID;
    std::string		_clientName;
    unsigned int	_timeoutMilliSeconds;
    int			_todoCount;
    int			_readingCount;
    pluton::timeoutClock	_clock;

    sig_t		_originalHandler;

    pluton::clientImpl*	_myThreadImpl;

    //////////////////////////////////////////////////////////////////////
    // Map fds to requests so returned events can find the
    // corresponding request.
    //////////////////////////////////////////////////////////////////////

    unsigned int		_fdsSize;
    struct pollfd*		_fds;
    pluton::clientRequestImpl**	_fdsToRequests;

    pluton::requestQueue	_completedQueue;
    pluton::faultInternal	_fault;
  };
}

#endif
