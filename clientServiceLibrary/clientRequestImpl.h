#ifndef _CLIENTREQUESTIMPL_H
#define _CLIENTREQUESTIMPL_H

#include <sys/time.h>
#include <sys/types.h>

#include "netString.h"

#include "pluton/clientEvent.h"

#include "timeoutClock.h"
#include "faultImpl.h"
#include "decodePacket.h"
#include "requestImpl.h"


//////////////////////////////////////////////////////////////////////
// The clientRequestImpl shepards a client supplied request through
// the stages of exchanging a request. Two major areas are connection
// management and execution flow.
//
// Connection management is about establishing the named socket
// connection, exchanging the request and connection retention based
// on affinity settings.
//
// Execution flow is transitioning a request through the stages as
// seen by the client, particularly:
// 
// o Accepting a request into the queue
//
// o Sending the request to the service
//
// o Marking the request as sent
//
// o Reading the response from the service
//
// o Placing the completed request on the _availableForWaitQueue;
//
// o Releasing a request to the caller once it has been identified by
//   a *wait* or getNext* call.
//////////////////////////////////////////////////////////////////////


namespace pluton {

  class clientRequest;
  class perCallerClient;

  class clientRequestImpl : public requestImpl {
  public:
    clientRequestImpl();
    ~clientRequestImpl();

    void 	reset();

    void	prepare(bool haveAffinity);
    int 	issueWrite(int timeoutMS);
    int 	issueRead(int timeoutMS);
    int		decodeResponse(std::string& errorMessage);

    enum state { withCaller, openConnection, bypassAffinityOpen, connecting,
		 opportunisticWrite, subsequentWrites, reading,
		 done };
    static const char*  stateToEnglish(pluton::clientRequestImpl::state);

    bool	inProgress() const { return (_state != withCaller) && (_state != done); }
    bool	withMeAlready() const { return _state != withCaller; }

    void	setState(const char* who, state ns);
    state	getState() const { return _state; }
    const char*	getStateEnglish() const { return stateToEnglish(_state); }

    void	setAffinity(bool tf) { _affinity = tf; }
    bool	getAffinity() const { return _affinity; }

    pluton::timeoutClock& getClock() { return _clock; }
    pluton::clientEvent::eventWanted*	getEventWanted() { return &_eventWanted; }

    void	setClientHandle(void* h) { _clientHandle = h; }
    void*	getClientHandle() const { return _clientHandle; }

    clientRequestImpl*	getNext() const { return _next; }
    void		setNext(clientRequestImpl* n) { _next = n; }

    ////////////////////////////////////////

    void		setOwner(perCallerClient* newOwner) { _owner = newOwner; }
    perCallerClient*	getOwner() const { return _owner; }

    void		setPollIndex(int pi) { _pollIndex = pi; }
    int			getPollIndex() const { return _pollIndex; }

    void		setClientRequestPtr(pluton::clientRequest* setH) { _clientRequestPtr = setH; }
    pluton::clientRequest*	getClientRequestPtr() const { return _clientRequestPtr; }

    ////////////////////////////////////////

    std::string		_rendezvousID;

    int			_tryCount;
    int 		_socket;
    unsigned int	_requestIDSent;

    netStringGenerate	_packetOutPre;		// Output packet is assembled in
    netStringGenerate	_packetOutPost;		// these netStrings

    static const int	maxIOVECs = 3;

    const char* 	_sendDataPtr[maxIOVECs];	// Point back to the assembled netStrings
    int 		_sendDataLength[maxIOVECs];	// used by writev() et al.
    int			_sendDataResidual;

    netStringFactoryManaged 	_packetIn;
    decodeResponsePacket 	_decoder;
    int				_bytesRead;

  private:
    void 		deleteFromQueues();
    void		debugLogIOV(int, struct iovec*, int maxBytes=400);

    //////////////////////////////////////////////////////////////////////
    // The requests are moved within and between two queues as they
    // progress. Rather than use external data structures to manage
    // the queue, such as <list>, the queue is managed as a single
    // link-list with an internal pointer - thus avoiding allocators
    // at the expense of hand-coding queue management.
    //////////////////////////////////////////////////////////////////////

    clientRequestImpl*	_next;

    state		_state;
    bool		_affinity;
    int			_pollIndex;			// Index to pending pollfds array

    //////////////////////////////////////////////////////////////////////
    // In the case of the libevent interface, the eventWanted
    // structure is used to exchanged desired event information.
    //////////////////////////////////////////////////////////////////////

    pluton::clientEvent::eventWanted	_eventWanted;

    pluton::perCallerClient*	_owner;
    unsigned int		_timeoutMS;
    pluton::timeoutClock	_clock;
    pluton::clientRequest*      _clientRequestPtr;


    //////////////////////////////////////////////////////////////////////
    // A general container for the client to help track requests. It
    // contains whatever goop they want to put there.
    //////////////////////////////////////////////////////////////////////

    void*		_clientHandle;
  };
}

#endif
