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

#include <iostream>
#include <sstream>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/un.h>

#include <unistd.h>
#include <errno.h>

#include "misc.h"
#include "util.h"
#include "clientImpl.h"
#include "perCallerClient.h"
#include "clientRequestImpl.h"

using namespace pluton;


//////////////////////////////////////////////////////////////////////

pluton::clientRequestImpl::clientRequestImpl()
  : _tryCount(0), _socket(-1), _packetIn(4096*4), _decoder(this),
    _next(0),
    _state(withCaller), _affinity(false),
    _owner(0), _timeoutMS(0), _clientRequestPtr(0),
    _clientHandle(0)
{
  _packetIn.disableCompaction();	// Ensure pointers to response data are stable
}

pluton::clientRequestImpl::~clientRequestImpl()
{
  if (_debugFlag) DBGPRT << "~clientRequestImpl() " << (void*) this << std::endl;

  deleteFromQueues();
  setState("destructor", withCaller);

  if (_socket != -1) close(_socket);
}


//////////////////////////////////////////////////////////////////////
// A request can be destroyed or reset while still on the
// perCallerClient or the clientImpl request list. This routine
// removes the request from those queue if it's present.
//
// This situation is particularly likely if the request is allocated
// on the stack and a caller routine returns prior to waiting for
// completion. This is the caller's error, but the consequences are
// pretty hard to diagnose - a random SIGSEGV in the middle of this
// library.
//////////////////////////////////////////////////////////////////////

void
pluton::clientRequestImpl::deleteFromQueues()
{
  if ((_state != withCaller) && _owner) {
    _owner->getMyThreadImpl()->deleteRequest(this);
    _owner->deleteCompletedRequest(this);
  }
}


const char*
pluton::clientRequestImpl::stateToEnglish(pluton::clientRequestImpl::state st)
{
  switch (st) {
  case withCaller: return "withCaller";
  case openConnection: return "openConnection";
  case bypassAffinityOpen: return "bypassAffinityOpen";
  case connecting: return "connecting";
  case opportunisticWrite: return "opportunisticWrite";
  case subsequentWrites: return "subsequentWrites";
  case reading: return "reading";
  case done: return "done";
  default: break;
  }
  return "??? unknown clientRequestImpl::state";
}


//////////////////////////////////////////////////////////////////////
// Debug routine: Write the iovec to clog
//////////////////////////////////////////////////////////////////////

void
pluton::clientRequestImpl::debugLogIOV(int iovc, struct iovec* iov, int maxBytes)
{
  for (int ix=0; ix < iovc; ++ix) {
    int bytes = iov->iov_len;
    if (bytes > maxBytes) bytes = maxBytes;
    DBGPRTMORE << std::string((char*) iov->iov_base, bytes);
    maxBytes -= bytes;
    if (maxBytes <= 0) break;
    ++iov;
  }
}

//////////////////////////////////////////////////////////////////////
// Set the request back to the initial state. Should there be a public
// abort() method that lets a caller terminate a request mid-flight?
// If so, it would mostly just call reset().
//////////////////////////////////////////////////////////////////////

void
pluton::clientRequestImpl::reset()
{
  DBGPRT << "reset " << this << " state=" << stateToEnglish(_state)
	 << " owner=" << _owner << std::endl;

  deleteFromQueues();

  _tryCount = 0;
  if (_socket != -1) {
    close(_socket);
    _socket = -1;
  }

  resetRequestValues();
  resetResponseValues();

  setState("reset", withCaller);

  _affinity = false;

  _owner = 0;
  _timeoutMS = 0;
  _clientRequestPtr = 0;
  _clientHandle = 0;
}


//////////////////////////////////////////////////////////////////////
// Decode the inbound response, in particular, make sure it matches
// the request we sent. This protects against services doing the wrong
// thing and sending out a response destined for another client. This
// of course should never happen, but it's a pretty serious security
// issue if it does, so be paranoid.
//
// "Request matching" is bypassed for tools that exchange
// requests. But this does not present a risk because the ultimate
// client *is* making the test, so even if transfer-tools get it
// wrong, the final recipient will detect the error.
//////////////////////////////////////////////////////////////////////

int
pluton::clientRequestImpl::decodeResponse(std::string& em)
{
  char nsType;
  const char* nsDataPtr;
  int nsDataLength;
  int nsOffset;

  _packetIn.getNetString(nsType, nsDataPtr, nsDataLength, &nsOffset);

  if (!_decoder.addType(nsType, nsDataPtr, nsDataLength, nsOffset, em)) {
    return -1;
  }

  //////////////////////////////////////////////////////////////////////
  // Is this now a complete response? If not, indicate to caller that
  // they need to read in more data.
  //////////////////////////////////////////////////////////////////////

  if (!_decoder.haveCompleteRequest()) return 0;

  //////////////////////////////////////////////////////////////////////
  // There is a complete response. Make sure that it is in response to
  // the particular request we sent. A service may have a bug which
  // gets the request and responses out of order and passing that onto
  // a user may not be a good thing from at least a privacy
  // perspective. Bypassing these checks is allowed for tools that are
  // acting as proxies. Normal clients have no interface to request a
  // bypass. The test is a simple clientname + unique requestID match.
  //////////////////////////////////////////////////////////////////////

  if (! byPassIDCheck()) {
    std::string cn;
    getClientName(cn);
    if ((cn != getOwner()->getClientName()) || (getRequestID() != _requestIDSent)) {
      std::ostringstream os;
      os << "Service routing to wrong clients. Expected=" << getOwner()->getClientName()
	 << "/" << _requestIDSent
	 << " Got=" << cn << "/" << getRequestID();

      resetResponseValues();	// Don't let wrong data thru to client
      setFault(pluton::seriousInternalRoutingError, os.str());
      DBGPRT << "Hib = " << os.str() << std::endl;
      return -1;
    }
  }

  DBGPRT << "Complete packet: " << getRequestID() << std::endl;

  //////////////////////////////////////////////////////////////////////
  // The decoder has recorded the buffer offsets of various parts of
  // the packet, now that parsing is complete and the managed
  // netString will no longer relocate, convert those offsets into
  // pointers.
  //////////////////////////////////////////////////////////////////////

  adjustOffsets(_packetIn.getBasePtr(), _packetIn.getRawOffset());

  return 1;
}


//////////////////////////////////////////////////////////////////////
// Prepare the request for sending to a service. This routine has to
// set anything that can change as a result of a request progressing
// then being retried.
//////////////////////////////////////////////////////////////////////

void
pluton::clientRequestImpl::prepare(bool needAffinity)
{
  DBGPRT << "clientRequest: prepare needAffinity=" << needAffinity
	 << " haveAffinity=" << getAffinity() << std::endl;

  if (needAffinity || getAffinity()) {
    setState("prepare", bypassAffinityOpen);
  }
  else {
    setState("prepare", openConnection);
  }

  _sendDataPtr[0] = _packetOutPre.data();
  _sendDataLength[0] = _packetOutPre.length();
  getRequestData(_sendDataPtr[1], _sendDataLength[1]);
  _sendDataPtr[2] = _packetOutPost.data();
  _sendDataLength[2] = _packetOutPost.length();

  resetResponseValues();
  _packetIn.reset();
  _decoder.reset();
  _decoder.setRequest(this);
  setPacketOffset(_packetIn.getRawOffset());
}


//////////////////////////////////////////////////////////////////////
// The main interest in state changes is that this is the way in which
// the owner's counters of outstanding requests are managed.
//////////////////////////////////////////////////////////////////////

void
pluton::clientRequestImpl::setState(const char* who, state newState)
{
  DBGPRT << "setState " << this << "/" << getRequestID()
	 << ", from " << stateToEnglish(_state) << " to " << stateToEnglish(newState)
	 << " by " << who << std::endl;
  if (newState == _state) return;

  if (newState == reading) getOwner()->addReadingCount();	// Reading count is solely for the
  if (_state == reading) getOwner()->subtractReadingCount();	// executeAndWaitSent completion

  _state = newState;
}


//////////////////////////////////////////////////////////////////////
// Read the request data into a Managed netString
//////////////////////////////////////////////////////////////////////

int
pluton::clientRequestImpl::issueRead(int timeoutMS)
{
  char* bufPtr;
  int minRequired, maxAllowed;
  if (_packetIn.getReadParameters(bufPtr, minRequired, maxAllowed) == -1) {
    errno = E2BIG;
    return -1;
  }

  int bytes = recv(_socket, bufPtr, maxAllowed, 0);
  DBGPRT << "recv(" << _socket << ") max=" << maxAllowed
	 << " read=" << bytes << " errno=" << errno;
  if (bytes > 0) DBGPRTMORE << " S=" << std::string(bufPtr, bytes > 300 ? 300 : bytes);
  DBGPRTMORE << std::endl;

  // If errno indicates a retry, returned with an request to re-poll

  if (bytes == -1) {
    if (util::retryNonBlockIO(errno)) return -2;
    return -1;
  }

  if (bytes > 0) {
    _packetIn.addBytesRead(bytes);	// Tell factory about the new data
    _bytesRead += bytes;
  }

  return bytes;
}


//////////////////////////////////////////////////////////////////////
// Depending on the OS options, write the request data to the service
// socket. The conniptions are about avoiding SIGPIPE if the service,
// per chance, should die mid-way through writing the reponhse. A
// SIGPIPE handler might be easier but complex clients may already be
// using that signal and these conniptions are compile-time, which is
// cheaper.
//////////////////////////////////////////////////////////////////////

int
pluton::clientRequestImpl::issueWrite(int timeoutMS)
{
  struct iovec iov[maxIOVECs];
  int ix;

  _sendDataResidual = 0;
  int iovCount = 0;
  for (ix=0; ix < maxIOVECs; ++ix) {
    if (_sendDataLength[ix] > 0) {
      iov[iovCount].iov_base = (char*)_sendDataPtr[ix];
      iov[iovCount].iov_len = _sendDataLength[ix];
      _sendDataResidual += _sendDataLength[ix];
      ++iovCount;
    }
  }

  if (iovCount == 0) return 0;

#if !defined(SO_NOSIGPIPE) && defined(MSG_NOSIGNAL)
  struct msghdr mh;
  mh.msg_name = 0;
  mh.msg_namelen = 0;
  mh.msg_control = 0;
  mh.msg_control = 0;
  mh.msg_controllen = 0;
  mh.msg_flags = 0;
  mh.msg_iov = iov;
  mh.msg_iovlen = iovCount;
  int bytesSent = sendmsg(_socket, &mh, MSG_NOSIGNAL);
  if (_debugFlag) {
    DBGPRT << "sendmsg(" << _socket << ", " << bytesSent << "/" << _sendDataResidual
	   << ") errno=" << errno << std::endl;
    DBGPRT << "S=";
    debugLogIOV(iovCount, iov);
    DBGPRTMORE << std::endl;
  }
#else
  int bytesSent = writev(_socket, iov, iovCount);
  if (_debugFlag) {
    DBGPRT << "writev(" << _socket << ", " << bytesSent << "/" << _sendDataResidual
	   << ") errno=" << errno << std::endl;
    DBGPRT << "S=";
    debugLogIOV(iovCount, iov);
    DBGPRTMORE << std::endl;
  }
#endif

  if (bytesSent <= 0) return bytesSent;

  int bytesSentReturned = bytesSent;
  _sendDataResidual -= bytesSent;

  for (ix=0; ix<iovCount; ++ix) {
    if (bytesSent <= 0) break;
    if (_sendDataLength[ix] == 0) continue;	// Ignore previously written vecs

    int sub = std::min(_sendDataLength[ix], bytesSent);
    _sendDataLength[ix] -= sub;
    _sendDataPtr[ix] += sub;
    bytesSent -= sub;
  }

  return bytesSentReturned;
}
