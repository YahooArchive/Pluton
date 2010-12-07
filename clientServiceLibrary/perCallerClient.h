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
