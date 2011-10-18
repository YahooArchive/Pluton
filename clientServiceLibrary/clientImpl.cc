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

//////////////////////////////////////////////////////////////////////
// The main source module for all the pluton::client processing.
//////////////////////////////////////////////////////////////////////

#include "config.h"

#include <iostream>
#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "global.h"
#include "misc.h"
#include "util.h"
#include "timeoutClock.h"
#include "faultImpl.h"
#include "clientImpl.h"

using namespace pluton;

static pluton::poll_handler_t	staticPollProxy = 0;

//////////////////////////////////////////////////////////////////////
// The clientImpl is a container for *all* outstanding requests. It is
// defined as a singleton by clientWrapper. Primarily the job of
// clientImpl instance is to hold the queue of requests.
//////////////////////////////////////////////////////////////////////

pluton::clientImpl::clientImpl()
  : _oneAtATimePerThread(false),
    _debugFlag(false), _requestID(100), _useCount(0), _todoQueue("todo")
{
  if (getenv("plutonClientDebug")) _debugFlag = true;
  DBGPRT << "clientImpl created" << std::endl;

  signal(SIGPIPE, SIG_IGN);				// Ignore these
}


pluton::clientImpl::~clientImpl()
{
  deleteOwner(0);		// Called once at destruction of program
}


//////////////////////////////////////////////////////////////////////
// Serialization protection methods. Neither the perCallerClient nor
// the clientImpl can be shared between threads. While this is well
// documented (and should be a default assumption of any object by all
// programmers), we "trust but verify".
//////////////////////////////////////////////////////////////////////

void
pluton::clientImpl::assertMutexFreeThenLock(pluton::perCallerClient* owner)
{
  owner->assertMutexFreeThenLock();
  assert(_oneAtATimePerThread == false);
  _oneAtATimePerThread = true;
}

void
pluton::clientImpl::unlockMutex(pluton::perCallerClient* owner)
{
  _oneAtATimePerThread = false;
  owner->unlockMutex();
}


const char*
pluton::clientImpl::getProgressEnglish(pluton::clientImpl::progress pr)
{
  switch (pr) {
  case needPoll: return "needPoll";
  case done: return "done";
  case retryMaybe: return "retryMaybe";
  case failed: return "failed";
  default: break;
  }
  return "?? progress ??";
}

//////////////////////////////////////////////////////////////////////
// A caller can replace the system poll() call with their own. This is
// primarily of interest to users of state threads.
//////////////////////////////////////////////////////////////////////

pluton::poll_handler_t
pluton::clientImpl::setPollProxy(pluton::poll_handler_t newHandler)
{
  pluton::poll_handler_t oldHandler = staticPollProxy;

  staticPollProxy = newHandler;		// Can be NULL to revert to the system call

  return oldHandler;
}


pluton::completionCondition::~completionCondition()
{
}


//////////////////////////////////////////////////////////////////////
// Prepare a socket for connect()
//////////////////////////////////////////////////////////////////////

static int
openSocket()
{
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock == -1) return -1;

  // Make sure the socket can't be used by child processes and that
  // it's non-blocking

  if (fcntl(sock, F_SETFL, FD_CLOEXEC | O_NONBLOCK) == -1) {
    int saveErrno = errno;
    close(sock);
    errno = saveErrno;
    return -1;
  }

  //////////////////////////////////////////////////////////////////////
  // Depending on the OS, there are different ways of stopping
  // SIGPIPEs, some allow a socket option, others all an option on the
  // send() system call and older ones require turning off the signal.
  //////////////////////////////////////////////////////////////////////

#ifdef SO_NOSIGPIPE
  int trueFlag = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &trueFlag, sizeof(trueFlag)) == -1) {
    int saveErrno = errno;
    close(sock);
    errno = saveErrno;
    return -1;
  }
#endif

  return sock;
}


//////////////////////////////////////////////////////////////////////
// Initiate a non-blocking connect(), which may or may not complete.
//
// Return <0 == error, 0 == inprogress, >0 == done
//////////////////////////////////////////////////////////////////////

static int
connectSocket(int sock, const char* path)
{
  struct sockaddr_un su;
  memset((void*) &su, '\0', sizeof(su));
  su.sun_family = AF_UNIX;

  if (strlen(path) >= sizeof(su.sun_path)) {
    errno = ENAMETOOLONG;
    return -1;
  }
  strcpy(su.sun_path, path);

  if (connect(sock, (struct sockaddr*) &su, sizeof(su)) == -1) {
    if (errno != EINPROGRESS) return -1;
    return 0;
  }

  util::increaseOSBuffer(sock, 64 * 1024, 64 * 1024);

  return 1;
}


//////////////////////////////////////////////////////////////////////
// Assemble the request into a set of write pointers. All packet data
// must be stored in per-request unique data structures since multiple
// requests can be in-progress at the same time.
//////////////////////////////////////////////////////////////////////

void
pluton::clientImpl::assembleRequest(clientRequestImpl* R)
{
  if (_requestID > 10000000) _requestID = 100;
  R->setRequestID(_requestID);
  R->_requestIDSent = _requestID;	// Remember what we sent for checking purposes
  ++_requestID;

  //////////////////////////////////////////////////////////////////////
  // Construct an iovec for writev to avoid copying the request data
  // which can slice about 10% off of CPU costs for large requests.
  //////////////////////////////////////////////////////////////////////

  R->_packetOutPre.clear();
  R->_packetOutPost.clear();
  R->assembleRequestPacket(R->_packetOutPre, R->_packetOutPost);
}


//////////////////////////////////////////////////////////////////////
// A raw request is one that is pre-assembled (or perhaps read in from
// a raw exchange with a tool) and does not need re-assembly. This is
// a tools-only interface.
//////////////////////////////////////////////////////////////////////

void
pluton::clientImpl::assembleRawRequest(clientRequestImpl* R, const char* rawPtr, int rawLength)
{
  R->_sendDataPtr[0] = rawPtr;
  R->_sendDataLength[0] = rawLength;
  R->_sendDataPtr[1] = 0;
  R->_sendDataLength[1] = 0;
  R->_sendDataPtr[2] = 0;
  R->_sendDataLength[2] = 0;
  DBGPRT << "assembleRaw: L=" <<  R->_sendDataLength[0] << std::endl;
}


//////////////////////////////////////////////////////////////////////
// Establish a connection with a service.
//////////////////////////////////////////////////////////////////////

bool
pluton::clientImpl::openConnection(pluton::clientRequestImpl* R)
{
  DBGPRT << "openConnection " << R->_rendezvousID << std::endl;

  R->_socket = openSocket();
  if (R->_socket == -1) {
    std::string em;
    util::messageWithErrno(em, "System Error: socket() failed", R->_rendezvousID.c_str());
    R->setFault(pluton::openSocketFailed, em);
    R->setState("openConnection::open socket failed", pluton::clientRequestImpl::done);
    return false;
  }

  R->setState("openConnection", pluton::clientRequestImpl::connecting);

  int res = connectSocket(R->_socket, R->_rendezvousID.c_str());

  //////////////////////////////////////////////////////////////////////
  // There are three possible returns from the non-blocking
  // connect. It may have errored (<0), it may have completed (>0) or
  // it may still be in progress (==0).
  //////////////////////////////////////////////////////////////////////

  if (res < 0) {		// Errored
    std::string em;
    util::messageWithErrno(em, "System Error: connect() failed", R->_rendezvousID.c_str());
    R->setFault(pluton::connectFailed, em);
    R->setState("openConnection::res < 0", pluton::clientRequestImpl::done);
    return false;
  }

  if (res > 0) {		// Completed - change state
    R->setState("openConnection::res > 0", pluton::clientRequestImpl::opportunisticWrite);
  }

  return true;
}


//////////////////////////////////////////////////////////////////////
// Only do the initialization once per process, regarless of whether
// there are threads or not. Since initialization is all about the
// lookup map, threaded-ness is irrelevant as all threads still get
// the same lookup.
//
// If this is the first call, establish the lookup information needed
// to discover service socket paths. If we eliminate wildcards from
// the lookup mechanism, then most of this code disappears.
//
// In a thread environment, this routine is protected by a mutex as it
// needs to potentially do a number of one-time things, such as map
// memory and so on.
//////////////////////////////////////////////////////////////////////

static bool		S_initializedFlag = false;	// Only act on the first initialize() call
static std::string	S_lookupPath;
static shmLookup	S_lookupMap;


bool
pluton::clientImpl::initialize(pluton::perCallerClient* owner, const char* setLookupPath)
{
  pluton::faultInternal* fault = owner->getFaultPtr();
  fault->clear("pluton::client::initialize");

  if (S_initializedFlag) return true;	// Already done - first caller always wins

  if (setLookupPath && *setLookupPath) S_lookupPath = setLookupPath;

  // Map preference order: 1. Supplied path, 2. env variable, 3. default

  if (S_lookupPath.empty()) {
    const char* envPath = getenv("plutonLookupMap");
    if (envPath) {
      S_lookupPath = envPath;
    }
    else {
      S_lookupPath = pluton::lookupMapPath;		// Default
    }
  }

  DBGPRT << "Initialize " << S_lookupPath << std::endl;

  pluton::faultCode fc;
  int res = S_lookupMap.mapReader(S_lookupPath.c_str(), fc);
  if (res < 0) {
    fault->set(fc, __FUNCTION__, __LINE__, res, errno, S_lookupPath.c_str());
    DBGPRT << "mapReader Failed " << S_lookupPath << " fc = " << fc << std::endl;
    S_lookupPath.erase();	// Don't remember a failed path
    return false;
  }

  S_initializedFlag = true;

  return true;
}


//////////////////////////////////////////////////////////////////////
// Find the Rendezvous End Point for a request being added to the
// list. If there is an error, set _fault and return false. Otherwise
// return true.
//////////////////////////////////////////////////////////////////////

bool
pluton::clientImpl::setRendezvousID(pluton::faultInternal* fault,
				    pluton::clientRequestImpl* R, const char* serviceKey)
{
  const char* err = R->setServiceKey(serviceKey);
  if (err) {
    fault->set(pluton::serviceKeyBad, __FUNCTION__, __LINE__, 0, 0, serviceKey, err);
    R->setFault(pluton::serviceKeyBad, fault->getMessage());
    return false;
  }

  //////////////////////////////////////////////////////////////////////
  // Find the Rendezvous End Point ID for the service. The lookup uses
  // the "searchKey" not the serviceKey. The searchKey is formated
  // differently to simplify the wildcard function name matching.
  //////////////////////////////////////////////////////////////////////

  std::string rendezvousID;
  std::string sk;
  pluton::faultCode fc;
  R->getSearchKey(sk);
  int res = S_lookupMap.findService(sk.data(), sk.length(), rendezvousID, fc);
  if (res <= 0) {
    fault->set(fc, __FUNCTION__, __LINE__, res, 0, serviceKey);
    R->setFault(fc, fault->getMessage());
    return false;
  }

  DBGPRT << "Rendezvous " << rendezvousID << " for " << R->getServiceKey() << std::endl;

  R->_rendezvousID = rendezvousID;

  return true;
}


//////////////////////////////////////////////////////////////////////
// Add a request to the singleton list of requests this perCaller. All
// of the preparation is done in this routine so that the state
// machine in progressRequests() can pick up and continue from any
// point.
//////////////////////////////////////////////////////////////////////

bool
pluton::clientImpl::addRequest(pluton::perCallerClient* owner,
			       const char* serviceKey, pluton::clientRequestImpl* R,
			       const char* rawPtr, int rawLength)
{
  DBGPRT << "addRequest: oW=" << owner
	 << " R=" << R << "=" << R->getClientRequestPtr()
	 << "  " << serviceKey
	 << " ch=" << R->getClientHandle()
	 << " oTodo=" << owner->getTodoCount()
	 << " oDone=" << owner->getCompletedQueueCount()
	 << " state=" << R->getStateEnglish()
	 << " withme=" << (R->withMeAlready() ? "yes" : "no")
	 << std::endl;

  pluton::faultInternal* fault = owner->getFaultPtr();
  fault->clear("pluton::client::addRequest");

  if (!S_initializedFlag) {
    fault->set(pluton::notInitialized, __FUNCTION__, __LINE__);
    return false;
  }

  if (R->withMeAlready()) {
    fault->set(pluton::requestAlreadyAdded, __FUNCTION__, __LINE__); 
    return false;
  }

  if (R->getRequestDataLength() < 0) {
    fault->set(pluton::badRequestLength, __FUNCTION__, __LINE__);
    return false;
  }

  R->setDebug(_debugFlag);

  //////////////////////////////////////////////////////////////////////
  // Check for valid affinity constraints. If the request has an
  // affinity connection from a previous request, skip over the lookup
  // for the connection path and prepare() the request in the "already
  // connected" state (aka opportunisticWrite).
  //////////////////////////////////////////////////////////////////////

  if (R->getAttribute(pluton::keepAffinityAttr)) {
    DBGPRT << "addRequest: keepAffinityAttr" << std::endl;
    if (R->getAttribute(pluton::noWaitAttr)) {
      fault->set(pluton::noWaitNotAllowed, __FUNCTION__, __LINE__);
      R->setFault(pluton::noWaitNotAllowed, fault->getMessage());
      return false;
    }

    if (!R->getAttribute(pluton::noRetryAttr)) {
      fault->set(pluton::needNoRetry, __FUNCTION__, __LINE__);
      R->setFault(pluton::needNoRetry, fault->getMessage());
      return false;
    }
  }

  if (R->getAttribute(pluton::needAffinityAttr)) {
    if (!R->getAffinity()) {
      fault->set(pluton::noAffinity, __FUNCTION__, __LINE__);
      R->setFault(pluton::noAffinity, fault->getMessage());
      return false;
    }
  }
  else {
    if (!setRendezvousID(fault, R, serviceKey)) return false;
  }

  //////////////////////////////////////////////////////////////////////
  // Phew! Got all that affinity stuff out of the way, now prepare the
  // request and set the state machine up.
  //////////////////////////////////////////////////////////////////////

  R->_tryCount = 0;
  R->setOwner(owner);		// Request is now *owned* by this perCaller
  owner->addTodoCount();	// perCaller tracks current request for condition tests

  //////////////////////////////////////////////////////////////////////
  // Special case raw requests as they are supplied by tools that are
  // allowed to by-pass some of the checks this API normally imposes
  // on callers.
  //////////////////////////////////////////////////////////////////////

  if (rawPtr) {
    R->setByPassIDCheck(true);
    assembleRawRequest(R, rawPtr, rawLength);
  }
  else {
    R->setByPassIDCheck(false);
    assembleRequest(R);
  }

  R->prepare(R->getAttribute(pluton::needAffinityAttr));
  R->getClock().stop();			// Events use per-request timers

  assertMutexFreeThenLock(owner);	// No other thread better be here
  _todoQueue.addToHead(R);		// Ready for processing
  unlockMutex(owner);

  return true;
}


//////////////////////////////////////////////////////////////////////
// Reset the perCallerClient to the initial state. This is mainly
// about removing outstanding requests.
//////////////////////////////////////////////////////////////////////

void
pluton::clientImpl::reset(pluton::perCallerClient* owner)
{
  if (!S_initializedFlag) {
    owner->getFaultPtr()->set(pluton::notInitialized, __FUNCTION__, __LINE__);
  }
  else {
    owner->getFaultPtr()->clear("pluton::client::reset");
  }

  deleteOwner(owner);
}


//////////////////////////////////////////////////////////////////////
// Remove all requests associated with a perCallerClient/owner. This
// could be due to imminent destruction or simply a client timeout.
//////////////////////////////////////////////////////////////////////

void
pluton::clientImpl::deleteOwner(pluton::perCallerClient* owner,
				pluton::faultCode faultCode, const char* faultText)
{
  DBGPRT << "deleteOwner: " << owner << " " << faultText << std::endl;

  pluton::clientRequestImpl* nextR = _todoQueue.getFirst();

  while (nextR) {
    pluton::clientRequestImpl* R = nextR;
    nextR = _todoQueue.getNext(nextR);
    if (!owner || (R->getOwner() == owner)) {
      _todoQueue.deleteRequest(R);
      R->setFault(faultCode, faultText);
      terminateRequest(R, false);
      R->setState("deleteOwner", pluton::clientRequestImpl::withCaller);
    }
  }
}


//////////////////////////////////////////////////////////////////////
// A clientRequestImpl is being destroyed - erase() this request from
// the singleton, if it's present. This is pretty inefficient if the
// request is in progress as it performs a serial search and rebuilds
// the queue, however callers should *not* be destroying requests
// while they are in progress so they kinda get what they pay for
// here...
//////////////////////////////////////////////////////////////////////

void
pluton::clientImpl::deleteRequest(pluton::clientRequestImpl* deleteR)
{
  if (_todoQueue.deleteRequest(deleteR)) deleteR->getOwner()->subtractTodoCount();
}


//////////////////////////////////////////////////////////////////////
// Release all resources associated with the connection and handle
// retry requirements.
//
// Return: true if the request can be retried.
//////////////////////////////////////////////////////////////////////

bool
pluton::clientImpl::retryRequest(pluton::clientRequestImpl* R)
{
  DBGPRT << R->getRequestID() << " retryRequest " << R->getFaultText() << std::endl;


  if (R->_socket != -1) {	// A retry always closes the current socket
    close(R->_socket);
    R->_socket = -1;
  }

  // Some requests cannot be retried .. for obvious reasons.

  if (R->getAttribute(pluton::noRetryAttr) || R->getAttribute(pluton::needAffinityAttr)) {
    return false;
  }

  // And other requests may have been retried enough already

  if (R->_tryCount >= MAXIMUM_TRY_COUNT) return false;

  // Ok, this request is good to retry

  R->prepare(false);

  return true;				// Tell caller that retrying is ok
}


//////////////////////////////////////////////////////////////////////
// A request has completed, cleanup resources depending on affinity
// settings. The connection can only be retained for affinity on a
// successful request. The supplied request has been removed from the
// _todoQueue and this routine moves it to the owners completed queue.
//////////////////////////////////////////////////////////////////////

void
pluton::clientImpl::terminateRequest(pluton::clientRequestImpl* R, bool ok)
{
  DBGPRT << "terminateRequest: " << R << " " << R->getRequestID()
	 << " ok=" << ok
	 << " af=" << R->getAttribute(pluton::keepAffinityAttr)
	 << " oTodo=" << R->getOwner()->getTodoCount()
	 << " oDone=" << R->getOwner()->getCompletedQueueCount()
	 << std::endl;

  if (R->getAttribute(pluton::keepAffinityAttr) && ok) {
    R->setAffinity(true);
    DBGPRT << "Affinity set true: " << R->getRequestID() << std::endl;
  }
  else {
    if (R->_socket != -1) {
      close(R->_socket);
      R->_socket = -1;
    }
    R->setAffinity(false);
  }

  R->setState("terminateRequest", pluton::clientRequestImpl::done);
  R->getOwner()->subtractTodoCount();
  R->getOwner()->addToCompletedQueue(R);
}


//////////////////////////////////////////////////////////////////////
// The executeAndWait*() set of methods provide a variety of ways for
// the caller to manage the asynchronous responses. A sucessful return
// is given only if the executeAndWait*() methods are able to meet the
// desired condition; All, Any, Sent, One or Blocked. If the wait
// condition is met, then the corresponding conditions have been met;
// specifically:
//
// All:		All remaining requests have completed (could be zero)
// Any:		At least one request has completed
// One:		The passed-in request has completed
// Blocked:	At least one request has completed or any further
//		progress risks blocking on I/O
//
// The caller can then reliably check for a fault for the
// corresponding request(s) because as far as this routine is
// concerned, the corresponding request(s) require no further
// processing.
//
// The type of failure depends on the wait-type and is documented at
// each method.
//
// The way the different executeAndWait*() methods work is to provide
// a callback routine that checks for the desired condition. The
// progressRequests() method does all the actual work, including
// calling the callback to test for a successful caller condition.
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// waitAll
// -------
//
// Success: return >= 0
//	All outstanding requests completed (could be zero)
//
// Timeout: return 0
//
// Failure: return < 0
//	Systemic error - such as unexpected return from poll()
//////////////////////////////////////////////////////////////////////

class waitAllCondition : public pluton::completionCondition {
  public:
  waitAllCondition(pluton::perCallerClient* setOwner) : _owner(setOwner) {}
  bool	satisfied(const pluton::clientRequestImpl* R)
  {
    return (_owner->getTodoCount() == 0);
  }
  const char* type() const { return "waitAll"; }
private:
  pluton::perCallerClient*	_owner;
};

int
pluton::clientImpl::executeAndWaitAll(pluton::perCallerClient* owner)
{
  DBGPRT << "executeAndWaitAll() oTodo=" << owner->getTodoCount()
	 << " oDone=" << owner->getCompletedQueueCount()
	 << std::endl;

  if (!S_initializedFlag) {
    owner->getFaultPtr()->set(pluton::notInitialized, __FUNCTION__, __LINE__);
    return -1;
  }

  assertMutexFreeThenLock(owner);	// No other thread better be here
  owner->getFaultPtr()->clear("pluton::client::executeAndWaitAll");

  int res = 0;
  if (owner->getTodoCount() == 0) {
    res = owner->getCompletedQueueCount();
  }
  else {
    waitAllCondition waitAll(owner);
    res = progressRequests(owner, waitAll);
    if (res > 0) res = owner->getCompletedQueueCount();
  }

  owner->eraseCompletedQueue(pluton::clientRequestImpl::withCaller); // All done
  unlockMutex(owner);

  DBGPRT << "E&WAll Return=" << res << std::endl;
  return res;
}


//////////////////////////////////////////////////////////////////////
// waitSent
// --------
//
// Success: return > 0
//	All outstanding have been sent to a service (could be zero)
//
// Timeout: return 0
//
// Failure: return < 0
//	Systemic error - such as unexpected return from poll()
//////////////////////////////////////////////////////////////////////

class waitSentCondition : public pluton::completionCondition {
  public:
  waitSentCondition(pluton::perCallerClient* setOwner) : _owner(setOwner) {}
  bool	satisfied(const pluton::clientRequestImpl* R)
  {
    return _owner->getTodoCount() == _owner->getReadingCount();
  }
  const char* type() const { return "waitSent"; }
private:
  pluton::perCallerClient*	_owner;
};

int
pluton::clientImpl::executeAndWaitSent(pluton::perCallerClient* owner)
{
  DBGPRT << "executeAndWaitSent() oTodo=" << owner->getTodoCount()
	 << " oDone=" << owner->getCompletedQueueCount()
	 << std::endl;

  if (!S_initializedFlag) {
    owner->getFaultPtr()->set(pluton::notInitialized, __FUNCTION__, __LINE__);
    return -1;
  }

  assertMutexFreeThenLock(owner);	// No other thread better be here
  owner->getFaultPtr()->clear("pluton::client::executeAndWaitSent");

  int res = 0;
  if (owner->getTodoCount() > 0) {
    waitSentCondition waitSent(owner);
    res = progressRequests(owner, waitSent);
  }
  unlockMutex(owner);

  DBGPRT << "E&WSent Return=" << res << std::endl;
  return res;
}


//////////////////////////////////////////////////////////////////////
// waitBlocked
// -----------
//
// This wait method doesn't fit as cleanly with the original
// completionCondition model, mainly because it needs to interact with
// the poll() call which is not something any of the others are
// interested in. But we wedged it in anyhow...
//
// Success: return > 0
//	All outstanding requests have progressed to a blocking I/O
//
// Timeout: return 0
//
// Failure: return < 0
//	Systemic error - such as unexpected return from poll()
//////////////////////////////////////////////////////////////////////

class waitBlockedCondition : public pluton::completionCondition {
  public:
  waitBlockedCondition(pluton::perCallerClient* setOwner, unsigned int timeBudgetMS)
  : _owner(setOwner)
  {
    struct timeval now;
    gettimeofday(&now, 0);
    _clock.start(&now, timeBudgetMS);
  }
  bool	satisfied(const pluton::clientRequestImpl* R)
  {
    return !_owner->completedQueueEmpty();
  }
  bool getTimeBudget(const struct timeval& now, int& to)
  {
    int newTo = _clock.getMSremaining(&now, 0);
    if (newTo < to) {
      to = newTo;
      return true;
    }

    return false;
  }
  const char* type() const { return "waitBlocked"; }
private:
  const pluton::perCallerClient*	_owner;
  pluton::timeoutClock			_clock;
};

int
pluton::clientImpl::executeAndWaitBlocked(pluton::perCallerClient* owner,
					  unsigned int timeoutMilliSeconds)
{
  DBGPRT << "executeAndWaitBlocked() oTodo=" << owner->getTodoCount()
	 << " oDone=" << owner->getCompletedQueueCount()
	 << " toMS=" << timeoutMilliSeconds
	 << std::endl;

  if (!S_initializedFlag) {
    owner->getFaultPtr()->set(pluton::notInitialized, __FUNCTION__, __LINE__);
    return -1;
  }

  assertMutexFreeThenLock(owner);	// No other thread better be here
  owner->getFaultPtr()->clear("pluton::client::executeAndWaitBlocked");

  int res = 0;
  if (owner->getTodoCount() > 0) {
    waitBlockedCondition waitBlocked(owner, timeoutMilliSeconds);
    res = progressRequests(owner, waitBlocked);
    
  }
  unlockMutex(owner);

  DBGPRT << "E&WBlocked Return=" << res << std::endl;
  return res;
}


//////////////////////////////////////////////////////////////////////
// waitAny
// -------
//
// This variant waits until at least one request is on the perCaller's
// return queue.
//
// Success: return address of first completed from the perCaller's
// queue.
//
// Failure: return NULL
//	Systemic error or no requests available for waiting
//////////////////////////////////////////////////////////////////////

class waitAnyCondition : public pluton::completionCondition {
  public:
  waitAnyCondition(pluton::perCallerClient* setOwner)
    : _owner(setOwner)
  {
  }
  bool	satisfied(const pluton::clientRequestImpl* R)
  {
    return !_owner->completedQueueEmpty();
  }
  const char* type() const { return "waitAny"; }

private:
  const pluton::perCallerClient*	_owner;
};


pluton::clientRequest*
pluton::clientImpl::executeAndWaitAny(pluton::perCallerClient* owner)
{
  DBGPRT << "executeAndWaitAny() oTodo=" << owner->getTodoCount()
	 << " oDone=" << owner->getCompletedQueueCount()
	 << std::endl;

  if (!S_initializedFlag) {
    owner->getFaultPtr()->set(pluton::notInitialized, __FUNCTION__, __LINE__);
    return 0;
  }

  assertMutexFreeThenLock(owner);	// No other thread better be here
  owner->getFaultPtr()->clear("pluton::client::executeAndWaitAny");
  waitAnyCondition waitAny(owner);
  progressRequests(owner, waitAny);

  pluton::clientRequestImpl* R = owner->popCompletedRequest();
  unlockMutex(owner);

  if (!R) return 0;

  DBGPRT << "executeAndWaitAny() returning " << R << " ch=" << R->getClientHandle() << std::endl;

  R->setState("executeAndWaitAny", pluton::clientRequestImpl::withCaller);	// Handed back
  return R->getClientRequestPtr();
}


//////////////////////////////////////////////////////////////////////
// waitOne
// -------
//
// Wait for the specific request to transition to ::done
//
// Success: return > 0
//	Supplied request has completed
//
// Timeout: return 0
//
// Failure: return < 0
//	Systemic error - such as unexpected return from poll()
//////////////////////////////////////////////////////////////////////

class waitOneCondition : public pluton::completionCondition {
  public:
  waitOneCondition(pluton::clientRequestImpl* setRequest)
    : _request(setRequest)
  {
  }
  bool	satisfied(const pluton::clientRequestImpl* R)
  {
    return _request->getState() == pluton::clientRequestImpl::done;
  }
  const char* type() const { return "waitOne"; }

private:
  pluton::clientRequestImpl*	_request;
};

int
pluton::clientImpl::executeAndWaitOne(pluton::perCallerClient* owner, pluton::clientRequestImpl* R)
{
  DBGPRT << "executeAndWaitOne() oTodo=" << owner->getTodoCount()
    	 << " oDone=" << owner->getCompletedQueueCount()
	 << std::endl;

  if (!S_initializedFlag) {
    owner->getFaultPtr()->set(pluton::notInitialized, __FUNCTION__, __LINE__);
    return -1;
  }

  assertMutexFreeThenLock(owner);	// No other thread better be here
  owner->getFaultPtr()->clear("pluton::client::executeAndWaitOne");
  if (!R->withMeAlready()) {
    R->setFault(pluton::requestNotAdded, "Not in progress");
    owner->getFaultPtr()->set(pluton::requestNotAdded, __FUNCTION__, __LINE__);
    unlockMutex(owner);
    return 0;
  }
  waitOneCondition waitOne(R);

  int res = progressRequests(owner, waitOne);

  DBGPRT << "E&WOne=" << res << " oDone=" << owner->getCompletedQueueCount() << std::endl;
  owner->deleteCompletedRequest(R);
  DBGPRT << "E&WOne now oDone=" << owner->getCompletedQueueCount() << std::endl;
  R->setState("executeAndWaitOne", pluton::clientRequestImpl::withCaller);	// Handed back

  unlockMutex(owner);

  DBGPRT << "E&WOne Return=" << res << std::endl;
  return res;
}


//////////////////////////////////////////////////////////////////////
// Progress all requests through their transitions until the caller's
// condition is met, the _todoQueue is empty or we run out of time.
//
// The state machine is a two part loop. The first part prepares for a
// poll() and the second part processes the results of the poll() and
// generates a single event for each request.
//
// The primary reason for this structure is to accomodate the
// clientEvent interface that is targeted at libevent-type
// programs. The transitioning of a request actually occurs in the
// event handlers.
//
// The completionCondition is a generalized way of providing the
// different completion tests. Rather than embed the different types
// of tests into this loop the conditions are provided as a callback.
//
// Return: < 0 = systemic error, 0 = condition cannot be met/timeout,
// > 0 = condition met
//////////////////////////////////////////////////////////////////////

int
pluton::clientImpl::progressRequests(pluton::perCallerClient* owner,
				   pluton::completionCondition& callerCondition)
{
  DBGPRT << "progressRequests() oW="  << owner
	 << " type=" << callerCondition.type()
	 << " todo=" << _todoQueue.count()
	 << " oDone=" << owner->getCompletedQueueCount()
	 << " crun=" << owner->getClock().isRunning()
	 << std::endl;

  //////////////////////////////////////////////////////////////////////
  // If this is the first executeAndWait* call since adding requests,
  // start the timeout clock ticking for the owner of the requests.
  //////////////////////////////////////////////////////////////////////

  struct timeval now;
  bool nowIsCurrent = false;

  if (!owner->getClock().isRunning()) {
    gettimeofday(&now, 0);			// Set base for timeout calcs
    nowIsCurrent = true;
    owner->getClock().start(&now, owner->getTimeoutMilliSeconds());
  }

  //////////////////////////////////////////////////////////////////////
  // Make sure the poll() array is big enough to fit all the requests.
  // This sizing is for the owner/thread so it cannot change during
  // iterations here so it only needs to be sized on entry.
  //////////////////////////////////////////////////////////////////////

  int queueCount = _todoQueue.count();
  struct pollfd* fds = owner->sizePollArray(queueCount);

  //////////////////////////////////////////////////////////////////////
  // Loop until we timeout, meet a condition or run out of requests.
  //////////////////////////////////////////////////////////////////////

  while (!_todoQueue.empty()) {

    //////////////////////////////////////////////////////////////////////
    // Calculate time remaining for this batch of requests.
    //////////////////////////////////////////////////////////////////////

    if (!nowIsCurrent) {
      gettimeofday(&now, 0);
      nowIsCurrent = true;
    }

    int timeBudgetMS = owner->getClock().getMSremaining(&now);
    DBGPRT << "timeBudgetMS=" << timeBudgetMS
	   << " global to=" << owner->getTimeoutMilliSeconds() << std::endl;

    //////////////////////////////////////////////////////////////////////
    // constructPollList progresses everything up until to a blocking
    // I/O is needed.
    //////////////////////////////////////////////////////////////////////

    int pollSubmitCount = constructPollList(owner, fds, queueCount, timeBudgetMS);

    if (checkConditions(owner, callerCondition)) {
      DBGPRT << "Condition Pre I/O true: " << callerCondition.type() << std::endl;
      return 1;
    }

    //////////////////////////////////////////////////////////////////////
    // Retries and failures can cause zero remaining requests, so if
    // there is nothing left to progress, get out of here.
    //////////////////////////////////////////////////////////////////////

    if (pollSubmitCount == 0) continue;		

    if (timeBudgetMS <= 0) {
      deleteOwner(owner, pluton::serviceTimeout, "service timeout");
      DBGPRT << "E&W timeout 1" << std::endl;
      return 0;
    }

    //////////////////////////////////////////////////////////////////////
    // Adjust the time budget depending on the condition. It can never
    // be zero at this point, so if the callerCondition adjusts it
    // down to zero, we use this information for to determine if this
    // progress is constrained by a smaller timebudget.
    //////////////////////////////////////////////////////////////////////

    bool timeBudgetAdjusted = callerCondition.getTimeBudget(now, timeBudgetMS);

    //////////////////////////////////////////////////////////////////////
    // Issue the poll either directly to the OS or via the caller
    // supplied poll() proxy. In the case of a caller supplied poll()
    // proxy, it's important that the state of the _todoQueue is
    // stable at this point as another "thread" can enter this code
    // while we're waiting on the pollProxy. "Stable" means you can
    // walk the queue and that all outstanding requests are on the
    // queue.
    //
    // (We could eliminate this constraint by removing the pollProxy
    // approach in favor of a thread-only approach, but at this stage
    // it's tolerable and doesn't burden this implementation that
    // much.)
    //////////////////////////////////////////////////////////////////////

    int fdsAvailable;
    if (staticPollProxy) {
      DBGPRT << "E&W pollProxy(" << pollSubmitCount << ", " << timeBudgetMS << ")" << std::endl;
      _oneAtATimePerThread = false;
      fdsAvailable = (staticPollProxy)(fds, pollSubmitCount, timeBudgetMS * util::MICROMILLI);
      assert(_oneAtATimePerThread == false);
      _oneAtATimePerThread = true;
    }
    else {
      DBGPRT << "E&W poll(" << pollSubmitCount << ", " << timeBudgetMS << ")" << std::endl;
      fdsAvailable = poll(fds, pollSubmitCount, timeBudgetMS);
    }
    DBGPRT << "E&W poll returned=" << fdsAvailable << " errno=" << errno
	   << " tba=" << timeBudgetAdjusted
	   << std::endl;

    nowIsCurrent = false;	// We have no idea what time it is after a poll()

    //////////////////////////////////////////////////////////////////////
    // Rather than encode waitBlocked logic into the callerCondition
    // class, we test it explicitly here.
    //////////////////////////////////////////////////////////////////////

    if (timeBudgetAdjusted && (fdsAvailable == 0)) {
      DBGPRT << "E&W waitBlocked Q=" << owner->getCompletedQueueCount() << std::endl;
      return owner->getCompletedQueueCount();	// Say how many have completed
    }

    //////////////////////////////////////////////////////////////////////
    // Decode the results of the poll() and turn them into events.
    //////////////////////////////////////////////////////////////////////

    if (fdsAvailable == 0) {			// Timeout means all requests are dead
      deleteOwner(owner, pluton::serviceTimeout, "service timeout");
      DBGPRT << "E&W timeout 2" << std::endl;
      return 0;
    }

    if (fdsAvailable == -1) {		// Serious and possibly unexpected error
      if (util::retryNonBlockIO(errno)) continue;
      owner->getFaultPtr()->set(pluton::seriousInternalOSError, __FUNCTION__, __LINE__,
				0, errno, "poll()", "returned -1");
      deleteOwner(0, pluton::seriousInternalOSError,
		  owner->getFaultPtr()->getMessage(0, false).c_str());
      DBGPRT << "E&W serious error" << std::endl;
      return -1;
    }

    //////////////////////////////////////////////////////////////////////
    // Dispatch the events. Each request on the queue has a poll entry
    // identified by the request's pollIndex. A pollIndex is necessary
    // as the _todoQueue can get changed by another thread across the
    // poll() call.
    //////////////////////////////////////////////////////////////////////

    if (_debugFlag) {
      DBGPRT << "Poll List:";
      for (int ix=0; ix < pollSubmitCount; ++ix) {
	DBGPRTMORE << " (" << fds[ix].fd << "," << fds[ix].revents << ")";
      }
      DBGPRTMORE << std::endl;
    }

    pluton::clientRequestImpl* nextR = _todoQueue.getFirst();

    while (nextR) {
      pluton::clientRequestImpl* R = nextR;
      nextR = _todoQueue.getNext(R);

      if (staticPollProxy && (owner != R->getOwner())) continue;	// Not ours to touch

      int ix = R->getPollIndex();	// Which poll entry corresponds to this request?
      assert(ix != -1);			// Just to be sure of no bugs here
      R->setPollIndex(-1);		// Insurance

      if (!fds[ix].revents) continue;	// No I/O available for this request

      //////////////////////////////////////////////////////////////////////
      // poll() indicates that I/O is ok for this request. Call the
      // read/write handler to issue the I/O then dispatch on the
      // results of that call.
      //////////////////////////////////////////////////////////////////////

      progress pr = needPoll;
      if (fds[ix].revents & (POLLIN|POLLRDNORM)) {	// proxyio returns POLLRDNORM
	pr = readEvent(R, timeBudgetMS);
      }
      if (fds[ix].revents & (POLLOUT|POLLWRNORM)) {
	pr = writeEvent(R, timeBudgetMS);
      }

      DBGPRT << "postEvent "
	     << ix << " " << fds[ix].fd << "," << fds[ix].revents
	     << " " << getProgressEnglish(pr) << std::endl;

      switch (pr) {

      case needPoll:
	break;

      case done:
	_todoQueue.deleteRequest(R);
	terminateRequest(R, true);
	break;

      case retryMaybe:
	if (!retryRequest(R)) {
	  _todoQueue.deleteRequest(R);
	  terminateRequest(R, false);
	}
	break;

      case failed:
	_todoQueue.deleteRequest(R);
	terminateRequest(R, false);
	break;
      }
    }

    //////////////////////////////////////////////////////////////////////
    // Check for met conditions after processing all I/O events
    //////////////////////////////////////////////////////////////////////

    if (checkConditions(owner, callerCondition)) {
      DBGPRT << "Condition Post I/O true: " << callerCondition.type() << std::endl;
      return 1;
    }
  }

  DBGPRT << "Fell out of wait " << callerCondition.type()
	 << " oTodo=" << owner->getTodoCount()
	 << " oDone=" << owner->getCompletedQueueCount()
	 << std::endl;

  return 0;	// Did not meet their condition
}


//////////////////////////////////////////////////////////////////////
// Handle the write event and progress the request.
//////////////////////////////////////////////////////////////////////

pluton::clientImpl::progress
pluton::clientImpl::writeEvent(pluton::clientRequestImpl* R, int timeBudgetMS)
{
  DBGPRT << "writeEvent " << R << "/" << R->getRequestID()
	 << " st=" << R->getStateEnglish() << std::endl;

  //////////////////////////////////////////////////////////////////////
  // Internal consistency check: there is only one acceptable state
  // for a writeEvent
  //////////////////////////////////////////////////////////////////////

  if (R->getState() != pluton::clientRequestImpl::subsequentWrites) {
    R->setFault(pluton::writeEventWrongState);
    R->getOwner()->getFaultPtr()->set(pluton::writeEventWrongState, __FUNCTION__, __LINE__);
    return failed;
  }

  int res = R->issueWrite(timeBudgetMS);
  if (res < 0) {
    R->setFault(pluton::socketWriteFailed, R->getOwner()->getFaultPtr()->getMessage().c_str());
    return retryMaybe;
  }

  //////////////////////////////////////////////////////////////////////
  // Is this request done with writing and now ready for reading?
  //
  // At this transition point, request faults are no longer used to
  // indicate incomplete transitions. They are only used for failed
  // requests; thus the setting to noFault - to clear the incomplete
  // transition fault. This is not really a big deal as callers should
  // not be checking for faults prior to completion anyway. Early
  // fault setting is simply an attempt to help faulty clients get
  // noticed.
  //////////////////////////////////////////////////////////////////////

  if (R->_sendDataResidual == 0) {
    R->setState("writeEvent::dataResidual==0", pluton::clientRequestImpl::reading);
    R->setFault(pluton::noFault);
  }

  return needPoll;
}


//////////////////////////////////////////////////////////////////////
// Handle the read event and progress the request.
//////////////////////////////////////////////////////////////////////

pluton::clientImpl::progress
pluton::clientImpl::readEvent(pluton::clientRequestImpl* R, int timeBudgetMS)
{
  DBGPRT << "readEvent " << R  << "/" << R->getRequestID()
	 << " st=" << R->getStateEnglish() << std::endl;

  //////////////////////////////////////////////////////////////////////
  // Only two states are valid for a read event, get the easiest state
  // out of the way first.
  //////////////////////////////////////////////////////////////////////

  if (R->getState() == pluton::clientRequestImpl::connecting) {
    R->setState("readEvent::connected", pluton::clientRequestImpl::opportunisticWrite);
    return needPoll;
  }

  if (R->getState() != pluton::clientRequestImpl::reading) {
    R->setFault(pluton::readEventWrongState);
    R->getOwner()->getFaultPtr()->set(pluton::readEventWrongState, __FUNCTION__, __LINE__);
    return failed;
  }

  int res = R->issueRead(timeBudgetMS);
  if (res == -2) return needPoll;

  if (res == -1) {
    std::string em;
    util::messageWithErrno(em, "System Error: recv() failed", R->_rendezvousID.c_str());
    R->setFault(pluton::socketReadFailed, em);
    return retryMaybe;
  }

  //////////////////////////////////////////////////////////////////////
  // If read returns an EOF *and* it's a noWait request *and* zero
  // bytes have been read in thus far, then treat it as a sucessful
  // request. Otherwise it's an error.
  //////////////////////////////////////////////////////////////////////

  if (res == 0) {
    if ((R->_bytesRead == 0) && R->getAttribute(pluton::noWaitAttr)) {
      R->setFault(pluton::noFault);
      return done;
    }

    R->setFault(pluton::incompleteResponse, "Socket closed with incomplete response");
    return retryMaybe;
  }

  //////////////////////////////////////////////////////////////////////
  // Got some data, decode it and look for a completed response.
  //////////////////////////////////////////////////////////////////////

  const char* parseError = 0;
  std::string em;
  while (R->_packetIn.haveNetString(parseError)) {
    int res = R->decodeResponse(em);
    if (res < 0) {
      R->setFault(pluton::responsePacketFormatError, em);
      return retryMaybe;
    }

    //////////////////////////////////////////////////////////////////////
    // This is the point at which a successful request is
    // complete. Yeah - party time!
    //////////////////////////////////////////////////////////////////////

    if (res > 0) return done;
  }

  if (parseError) {
    R->setFault(pluton::responsePacketFormatError, parseError);
    return retryMaybe;
  }

  return needPoll;
}


//////////////////////////////////////////////////////////////////////
// Scan the _todoQueue Queue and progress them towards completion or
// to the need for a potentially blocking I/O. Blocking I/Os are added
// to array of file descriptors returned to the caller.
//
// All I/O events for all requests are constructed *unless* the caller
// has provided a poll proxy routine. The presence of this proxy is
// indication that the poll calls should be done on a perCallerClient
// basis in which case only the subset of requests owned by that
// perCallerClient are assembled into the poll list. The same applies
// on a thread basis too.
//////////////////////////////////////////////////////////////////////

int
pluton::clientImpl::constructPollList(pluton::perCallerClient* owner,
				      struct pollfd* fds, int fdsSize,
				      int timeBudgetMS)
{
  int fdsInUse = 0;
  pluton::clientRequestImpl* nextR = _todoQueue.getFirst();

  while (nextR) {
    pluton::clientRequestImpl* R = nextR;
    nextR = _todoQueue.getNext(R);	// Grab next now as R might be moved

    DBGPRT << "Construct PollList: oW=" << owner << "/" << R->getOwner() << " R="
	   << R << "/" << R->getRequestID() << " st=" << R->getState() << std::endl;

    //////////////////////////////////////////////////////////////////////
    // In pollProxy mode, only requests owned by the caller are
    // processed. This is particularly important for state thread
    // support.
    //////////////////////////////////////////////////////////////////////

    if (staticPollProxy && (owner != R->getOwner())) continue;	// Cannot touch non-owned requests

    if (progressOrTerminate(owner, R, fds+fdsInUse, timeBudgetMS)) {
      R->setPollIndex(fdsInUse);	// Say it *is* on the poll list
      ++fdsInUse;			// This request has progressed to poll
      assert(fdsInUse <= fdsSize);	// Just to be sure
    }
    else {
      R->setPollIndex(-1);		// Say it's not on the poll list
    }
  }

  return fdsInUse;
}


//////////////////////////////////////////////////////////////////////
// Wrapper around progressTowardsPoll to encapsulate some of the
// retry/terminate logic.
//
// Return: true if the request has progressed to a block I/O and thus
// needs a poll.
//
// Return: false if the request has terminated and returned to the
// owner. The request must not be on any queue when this routine is
// called.
//////////////////////////////////////////////////////////////////////

bool
pluton::clientImpl::progressOrTerminate(pluton::perCallerClient* owner,
					pluton::clientRequestImpl* R, struct pollfd* fds,
					int timeBudgetMS)
{
  DBGPRT << "progressOrTerminate: oW=" << owner << "/" << R->getOwner()
	 << " R=" << R << "/" << R->getRequestID()
	 << " st=" << R->getStateEnglish()
	 << " budget=" << timeBudgetMS
	 << std::endl;

  while (true) {
    progress pr = progressTowardsPoll(R, fds, timeBudgetMS);
    DBGPRT << "return progressTowardsPoll=" << getProgressEnglish(pr) << std::endl;

    switch (pr) {

    case needPoll: return true;

    case done:
      _todoQueue.deleteRequest(R);
      terminateRequest(R, true);
      return false;

    case retryMaybe:
      if (retryRequest(R)) continue;

      _todoQueue.deleteRequest(R);
      terminateRequest(R, false);
      return false;

    case failed:
      _todoQueue.deleteRequest(R);
      terminateRequest(R, false);
      return false;
    }
  }
}


//////////////////////////////////////////////////////////////////////
// This routine in conjunction with the event routines, is responsible
// for progressing a request through the states to completion. This
// routine does as much as it can to progress a request up until the
// point that a non-blocking I/O is needed.
//
// One slightly interesting thing is that the first write of the
// request to the service socket does not wait for the poll() to say
// the socket is writable. This optimization almost certainly has to
// work for newly opened sockets, so, for modest sized requests, we
// may get done with writing the full request without ever making a
// call to poll(). Since the socket is non-blocking, it's safe if this
// assumption is wrong.
//////////////////////////////////////////////////////////////////////

pluton::clientImpl::progress
pluton::clientImpl::progressTowardsPoll(pluton::clientRequestImpl* R, struct pollfd* fds,
					int timeBudgetMS)
{
  DBGPRT << "progressTowardsPoll: R=" << R << "/" << R->getRequestID()
	 << " st=" << R->getStateEnglish()
	 << " budget=" << timeBudgetMS
	 << std::endl;

  switch (R->getState()) {

  case pluton::clientRequestImpl::bypassAffinityOpen:
  case pluton::clientRequestImpl::openConnection:
    R->_tryCount++;
    if (R->getState() == pluton::clientRequestImpl::openConnection) {
      if (!openConnection(R)) return failed;		// No point retrying this
    }

    R->setFault(pluton::requestInProgress, "Request incomplete");	// default fault
    R->_bytesRead = 0;

    DBGPRT << R->getRequestID() << " COPE2 " << R << " " << R->getState() << std::endl;
    if (R->getState() == pluton::clientRequestImpl::connecting) {
      fds->fd = R->_socket;
      fds->events = POLLIN;
      fds->revents = 0;
      return needPoll;
    }
    R->setState("progressTowardsPoll::Fall thru connect",
		pluton::clientRequestImpl::opportunisticWrite);

    // Fall thru if connect() has completed

    // FALL THRU

  case pluton::clientRequestImpl::opportunisticWrite: // Do a non-blocking write
    {
      int res = R->issueWrite(timeBudgetMS);
      std::string em;
      if (res <= 0) {
	util::messageWithErrno(em, "System Error: sendmsg() failed",
			       R->_rendezvousID.c_str());
	R->setFault(pluton::socketWriteFailed, em);
	return retryMaybe;
      }

      if (R->_sendDataResidual == 0) {		// Opportunistic write complete?
	R->setState("progressTowardsPoll::residual==0", pluton::clientRequestImpl::reading);
	R->setFault(pluton::noFault);	// At this point all faults are from the service
	fds->fd = R->_socket;
	fds->events = POLLIN;
	fds->revents = 0;
	return needPoll;
      }
    }

  R->setState("progressTowardsPoll::residual>0", pluton::clientRequestImpl::subsequentWrites);

  // FALLTHRU

  case pluton::clientRequestImpl::subsequentWrites:
    fds->fd = R->_socket;
    fds->events = POLLOUT;
    fds->revents = 0;
    return needPoll;

    // Note that a state could exist called opportunisticRead, which
    // is where a completed write transitions to. The reason for not
    // doing this is the assumption that this loop will block before a
    // service can do the read/write sequence.

  case pluton::clientRequestImpl::connecting:
  case pluton::clientRequestImpl::reading:
    fds->fd = R->_socket;
    fds->events = POLLIN;
    fds->revents = 0;
    return needPoll;

  case pluton::clientRequestImpl::done:
    assert(pluton::clientRequestImpl::done);

  case pluton::clientRequestImpl::withCaller:
    assert(pluton::clientRequestImpl::withCaller);
  }

  return failed;
}


//////////////////////////////////////////////////////////////////////
// Check if any conditions have been met. There are two types of
// condition checking callbacks. Ones which need to examine every
// request and ones which only need to know owner or singleton state.
//
// A condition checker tells us what type it is by the needRequests()
// method. If the condition checker needs to examine each request,
// then they are called with the _todo queue as well as the
// "completed" requests on the owner queue. If needRequests() is
// false, then the condition checker is called just once without any
// supplied request.
//
// As it happens, none of the current condition checkers return true
// for needRequests() but the code is here anyway as it was needed
// previously...
//////////////////////////////////////////////////////////////////////

bool
pluton::clientImpl::checkConditions(pluton::perCallerClient* owner,
				    pluton::completionCondition& callerCondition)
{
  DBGPRT << "CheckCondition: " << callerCondition.type()
	 << " " << callerCondition.needRequests()
	 << " oTodo=" << owner->getTodoCount()
	 << " oDone=" << owner->getCompletedQueueCount()
	 << " _todoQ=" << _todoQueue.getFirst() << std::endl;

  //////////////////////////////////////////////////////////////////////
  // Do we need to scan all requests or is global state sufficient for
  // the needs of the condition checker?
  //////////////////////////////////////////////////////////////////////

  if (!callerCondition.needRequests()) {
    bool res = callerCondition.satisfied(0);
    DBGPRT << "Global condition: " << callerCondition.type() << "=" << res << std::endl;
    return res;
  }

  //////////////////////////////////////////////////////////////////////
  // Bother. This condition needs to examine all requests.
  //////////////////////////////////////////////////////////////////////

  for (pluton::clientRequestImpl* R=_todoQueue.getFirst(); R; R=_todoQueue.getNext(R)) {
    DBGPRT << "CheckCondition: " << callerCondition.type() << " "
	   << R << "/" << R->getRequestID()
	   << " st=" << R->getState() << std::endl;

    if (owner != R->getOwner()) continue;	// only check owner's requests
    if (callerCondition.satisfied(R)) return true;
  }

  //////////////////////////////////////////////////////////////////////
  // No luck with the condition on the _todoQueue. Let's try the
  // "done" queue.
  //////////////////////////////////////////////////////////////////////


  DBGPRT << "No _todo Conditions met" << std::endl;

  for (pluton::clientRequestImpl* R=owner->firstCompletedRequest(); R;
       owner->nextCompletedRequest(R)) {
    DBGPRT << "CheckCondition: ow " << R << "/" << R->getRequestID()
	   << " st=" << R->getState() << std::endl;

    if (owner != R->getOwner()) continue;	// only check owner's requests

    if (callerCondition.satisfied(R)) return true;
  }

  DBGPRT << "No _done Conditions met" << std::endl;

  return false;
}
