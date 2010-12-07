#ifndef _CLIENTIMPL_H
#define _CLIENTIMPL_H

#include <string>

#include <sys/time.h>

#include "netString.h"

#include "pluton/client.h"
#include "pluton/clientEvent.h"

#include "faultImpl.h"
#include "decodePacket.h"
#include "requestImpl.h"
#include "perCallerClient.h"
#include "clientRequestImpl.h"
#include "shmLookup.h"


//////////////////////////////////////////////////////////////////////
// That actual implementation of a client is separate from the client
// definition to protect the callers from binary incompatibility. The
// clientImpl is instantiated as a singleton as only one controlling
// entity should be used so that all requests are visible to
// a single clientImpl.
//////////////////////////////////////////////////////////////////////


namespace pluton {

  //////////////////////////////////////////////////////////////////////
  // The execution of requests is controlled by a completionCondition
  // handler. The satisfied() method is called whenever a request
  // changes state.
  //////////////////////////////////////////////////////////////////////

  class completionCondition {
  public:
    virtual 		~completionCondition() = 0;
    virtual bool	satisfied(const pluton::clientRequestImpl*) = 0;
    virtual const char* type() const = 0;

    virtual bool	needRequests() { return false; }	// True if need to view requests
    virtual bool 	getTimeBudget(const struct timeval& now, int& to) { return false; }
  };


  class clientImpl {
  public:
    clientImpl();
    ~clientImpl();

    static const int	MAXIMUM_TRY_COUNT = 2;	// Config is currently ignored!

    //////////////////////////////////////////////////////////////////////
    // A number of helpers progress a request and this enum is a
    // common way of communicating how far they get before being
    // stopped by something.
    //////////////////////////////////////////////////////////////////////

    enum progress { needPoll, done, retryMaybe, failed };
    static const char*	getProgressEnglish(pluton::clientImpl::progress);

    void		assertMutexFreeThenLock(pluton::perCallerClient* owner);
    void		unlockMutex(pluton::perCallerClient* owner);

    void	reset(pluton::perCallerClient*);
    bool	initialize(pluton::perCallerClient*, const char* lookupMapPath);

    pluton::poll_handler_t setPollProxy(pluton::poll_handler_t);

    bool	addRequest(pluton::perCallerClient* owner,
			   const char* serviceKey, pluton::clientRequestImpl*,
			   const char* rawPtr=0, int rawLength=0);

    int 			executeAndWaitSent(pluton::perCallerClient*);
    int 			executeAndWaitAll(pluton::perCallerClient*);
    pluton::clientRequest*      executeAndWaitAny(pluton::perCallerClient*);
    int 			executeAndWaitOne(pluton::perCallerClient*,
						  pluton::clientRequestImpl*);
    int				executeAndWaitBlocked(pluton::perCallerClient*,
						      unsigned int timeoutMilliSeconds);

    void	deleteOwner(pluton::perCallerClient* owner,
			    pluton::faultCode faultCode=pluton::noFault, const char* faultText="");
    void	deleteRequest(pluton::clientRequestImpl*);
    void	setDebug(bool tf) { _debugFlag = tf; }

    ////////////////////////////////////////
    // Event based interface
    ////////////////////////////////////////

    const pluton::clientEvent::eventWanted*
      getNextEventWanted(pluton::perCallerClient* owner,
			 const struct timeval* now,
			 const pluton::clientRequest* onlyThisR);

    int	sendCanReadEvent(pluton::perCallerClient* owner, int fd);
    int	sendCanWriteEvent(pluton::perCallerClient* owner, int fd);
    int	sendTimeoutEvent(pluton::perCallerClient* owner, int fd, bool abortFlag);

    pluton::clientRequest*	getCompletedRequest(pluton::perCallerClient* owner);


    //////////////////////////////////////////////////////////////////////
    // Track how many pluton::client instances are using this Impl.
    //////////////////////////////////////////////////////////////////////

    void 	incrementUseCount() { ++_useCount; }
    int 	decrementUseCount() { --_useCount; return _useCount; }

  private:
    clientImpl&	operator=(const clientImpl& rhs);	// Assign not ok
    clientImpl(const clientImpl& rhs);			// Copy not ok

    ////////////////////////////////////////
    // Helper methods
    ////////////////////////////////////////

    void	addToQueue(pluton::clientRequestImpl* R);
    int		getTodoQueueCount() const;
    bool	setRendezvousID(pluton::faultInternal*,
				pluton::clientRequestImpl*, const char* serviceKey);
    bool	openConnection(pluton::clientRequestImpl*);
    void	assembleRequest(pluton::clientRequestImpl*);
    void	assembleRawRequest(pluton::clientRequestImpl*, const char* rawPtr, int rawLength);

    void	terminateRequest(pluton::clientRequestImpl*, bool ok);
    bool	retryRequest(pluton::clientRequestImpl*);

    // The main request progression routines

    int		progressRequests(pluton::perCallerClient* owner, pluton::completionCondition&);

    int		constructPollList(pluton::perCallerClient* owner, struct pollfd* fds, int fdsSize,
				  int timeBudgetMS);
    bool	progressOrTerminate(pluton::perCallerClient* owner, pluton::clientRequestImpl*,
				    struct pollfd* fds, int timeBudgetMS);
    progress	progressTowardsPoll(pluton::clientRequestImpl* R, struct pollfd* fds,
				    int timeBudgetMS);

    progress	writeEvent(pluton::clientRequestImpl*, int timeBudgetMS);
    progress	readEvent(pluton::clientRequestImpl*, int timeBudgetMS);

    bool	checkConditions(pluton::perCallerClient* owner, pluton::completionCondition&);

    int		progressEvent(pluton::perCallerClient*, pluton::clientRequestImpl*, progress);

    ////////////////////////////////////////

    bool			_oneAtATimePerThread;	// Make sure thread serialize around me
    bool			_debugFlag;
    unsigned int		_requestID;		// Unique ID for each request
    int				_useCount;		// pluton::client instances pointing to me

    ////////////////////////////////////////
    // Queue of outstanding requests
    ////////////////////////////////////////

    pluton::requestQueue        _todoQueue;
  };
}

#endif
