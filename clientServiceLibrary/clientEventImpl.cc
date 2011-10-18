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

#include "config.h"

#include <assert.h>

#include <iostream>
#include <sstream>


#include "global.h"
#include "misc.h"
#include "util.h"
#include "faultImpl.h"
#include "clientImpl.h"

using namespace pluton;


//////////////////////////////////////////////////////////////////////
// Event based interfaces
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Iterate through the request Queue, progressing them to completion
// or to the point that one has a pending I/O event.
//
// Fill in the I/O event structure and return it to the caller. The
// request is removed from the inProgress list and is not returned
// until one of the *Event() routines are called. For this reason, the
// callers *must* ultimately return an event rather than lose it,
// otherwise the request is lost.
//
// If the optional clientRequest is supplied, that is used to
// constrain returned events to that request only. All requests upto
// the matching request are serendipitously progressed as far as
// possible.
//////////////////////////////////////////////////////////////////////

const pluton::clientEvent::eventWanted*
pluton::clientImpl::getNextEventWanted(perCallerClient* owner, const struct timeval* now,
				       const pluton::clientRequest* onlyThisR)
{
  //  assert(thread_self() == 0);   // Can't have threads and events

  struct pollfd fds;
  pluton::clientRequestImpl* R;

  pluton::clientRequestImpl* nextR = _todoQueue.getFirst();
  pluton::clientEvent::eventWanted* ewp = 0;

  DBGPRT << "getNextEventWanted Entry ow=" << owner << " nextR= " << nextR << std::endl;

  while (nextR) {

    R = nextR;				// Get next now as R can be deleted in
    nextR = _todoQueue.getNext(R); 	// the body of this loop

    if (!progressOrTerminate(owner, R, &fds, 0)) continue;

    //////////////////////////////////////////////////////////////////////
    // This request is waiting on a blocking I/O. Remove from our
    // queue and prepare for returning to the caller. Constrain to
    // only returning "onlyThisR" request if supplied by the caller.
    //////////////////////////////////////////////////////////////////////

    if (onlyThisR && (onlyThisR != R->getClientRequestPtr())) continue;

    _todoQueue.deleteRequest(R);
    ewp = R->getEventWanted();

    //////////////////////////////////////////////////////////////////////
    // Start the timeout clock if this is the first time that an event
    // is queued for this request since it's been added.
    //////////////////////////////////////////////////////////////////////

    if (!R->getClock().isRunning()) {
      DBGPRT << "Starting clock for " << R->getRequestID() << " n=" << now->tv_sec
	     << " toMS=" << R->getTimeoutMS() << std::endl;
      R->getClock().start(now, R->getTimeoutMS());
    }

    //////////////////////////////////////////////////////////////////////
    // If the request has timed out, terminate it. Otherwise set the
    // maximum amount of time that the event handler can wait in
    // ewp->timeout.
    //////////////////////////////////////////////////////////////////////

    int remainingMS = R->getClock().getMSremaining(now, &ewp->timeout);
    DBGPRT << "Remaining MS for " << R->getRequestID() << " ms=" << remainingMS << std::endl;
    if (remainingMS <= 0) {
      DBGPRT << "Setting timeout fault for " << R->getRequestID() << std::endl;
      R->setFault(pluton::serviceTimeout, "service timeout");
      ewp = 0;		// Termination makes this invalid
      terminateRequest(R, false);
      continue;
    }

    // Have a request that is blocked, return it to caller

    owner->sizePollArray(R->_socket);		// Sizes the requestIndex array to fit
    owner->setRequestIndex(R->_socket, R);	// this calls entry.
    if (fds.events & POLLIN) {
      ewp->type = pluton::clientEvent::wantRead;
    }
    else {
      ewp->type = pluton::clientEvent::wantWrite;
    }

    ewp->fd = fds.fd;
    ewp->clientRequestPtr = R->getClientRequestPtr();
    R->setEventTypeWanted(ewp->type);

    //////////////////////////////////////////////////////////////////////
    // If the caller is being selective, we progress all requests on
    // the basis that they are likely to select on just the one
    // request and inadvertently starve the others. Conversely if the
    // caller is not selective, then return the first request that is
    // blocked, on the basis that they will be programming according
    // to the docs and accumulating all requests.
    //////////////////////////////////////////////////////////////////////

    if (!onlyThisR) break;
  }

  DBGPRT << "getNextEventWanted return=" << ewp << std::endl;

  return ewp;
}


//////////////////////////////////////////////////////////////////////
// Pass events back into the clientImpl. These are mostly wrappers to
// check that the supplied event makes sense and can be mapped back to
// a particular request (ie, callers may get it wrong so don't trust
// them).
//////////////////////////////////////////////////////////////////////

int
pluton::clientImpl::sendCanReadEvent(pluton::perCallerClient* owner, int fd)
{
  DBGPRT << "sendCanReadEvent(" << owner << ", " << fd << ")" << std::endl;

  pluton::clientRequestImpl* R = owner->getRequestByIndex(fd);
  if (!R) return -1;
  if (R->_socket != fd) return -2;
  if (!R->getEventTypeWanted(pluton::clientEvent::wantRead)) return -3;

  R->clearEventTypeWanted();

  progress pr = readEvent(R, 0);
  return progressEvent(owner, R, pr);
}


int
pluton::clientImpl::sendCanWriteEvent(pluton::perCallerClient* owner, int fd)
{
  DBGPRT << "sendCanWriteEvent(" << owner << ", " << fd << ")" << std::endl;

  pluton::clientRequestImpl* R = owner->getRequestByIndex(fd);
  if (!R) return -1;
  if (R->_socket != fd) return -2;
  if (!R->getEventTypeWanted(pluton::clientEvent::wantWrite)) return -3;

  R->clearEventTypeWanted();

  progress pr = writeEvent(R, 0);
  return progressEvent(owner, R, pr);
}


//////////////////////////////////////////////////////////////////////
// If a timeout is returned, it may not be the limit for this request
// so simply add it back into the todo queue and the next iteration of
// getNextEventWanted will determine which ones have truly timed out.
//////////////////////////////////////////////////////////////////////

int
pluton::clientImpl::sendTimeoutEvent(pluton::perCallerClient* owner, int fd, bool abortFlag)
{
  DBGPRT << "sendTimeoutEvent(" << owner << ", " << fd << ", " << abortFlag << ")" << std::endl;

  pluton::clientRequestImpl* R = owner->getRequestByIndex(fd);
  if (!R) return -1;
  if (R->_socket != fd) return -2;
  if (R->getEventTypeWanted(pluton::clientEvent::wantNothing)) return -3;

  R->clearEventTypeWanted();

  // Do they want to force a timeout regardless of what the initial
  // timeout may have been?

  if (abortFlag) {
    DBGPRT << "forced service timeout" << std::endl;
    R->setFault(pluton::serviceTimeout, "Event forced service timeout");
    terminateRequest(R, false);
    R->setState("sendTimeoutEvent", pluton::clientRequestImpl::withCaller);
  }
  else {
    _todoQueue.addToHead(R);
  }

  return 0;
}


//////////////////////////////////////////////////////////////////////
// An internal routine to progress the request based on the returned
// event. Return the count of completed events. This code is virtually
// a duplicate of the progression logic in the second half of
// pluton::clientImpl::progressRequests. The main difference - and the
// reason for the close duplication - is that requests are not on the
// queue when they have an event pending.
//////////////////////////////////////////////////////////////////////

int
pluton::clientImpl::progressEvent(pluton::perCallerClient* owner, pluton::clientRequestImpl* R,
				  pluton::clientImpl::progress pr)
{
  switch (pr) {

  case needPoll:
    _todoQueue.addToHead(R);
    return 0;

  case done:
    terminateRequest(R, true);
    break;

  case retryMaybe:
    if (retryRequest(R)) {
      _todoQueue.addToHead(R);
      return 0;
    }
    terminateRequest(R, false);
    break;

  case failed:
    terminateRequest(R, false);
    break;
  }

  return 1;
}


//////////////////////////////////////////////////////////////////////
// Return the first completed request to the caller. Once returned,
// the request is no longer our responsibility.
//////////////////////////////////////////////////////////////////////

pluton::clientRequest*
pluton::clientImpl::getCompletedRequest(pluton::perCallerClient* owner)
{
  pluton::clientRequestImpl* R = owner->popCompletedRequest();
  DBGPRT << "getCompletedRequest ow=" << owner << " R=" << R << std::endl;
  if (!R) return 0;

  R->setState("getCompletedRequest", pluton::clientRequestImpl::withCaller);	// Handed back

  DBGPRT << "Return " << R << "=" << R->getClientRequestPtr() << "/"
	 << R->getRequestID() << std::endl;

  return R->getClientRequestPtr();	// Return wrapper pointer, not impl pointer
}
