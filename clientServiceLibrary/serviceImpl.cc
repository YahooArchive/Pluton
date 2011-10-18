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

//////////////////////////////////////////////////////////////////////
// The main source module for all of the pluton::service processing.
//////////////////////////////////////////////////////////////////////

#include <string>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_SELECT
#include <sys/select.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/un.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"
#include "netString.h"

#include "misc.h"
#include "processExitReason.h"
#include "faultImpl.h"
#include "serviceImpl.h"
#include "serviceAttributes.h"
#include "decodePacket.h"

static bool GInsideJVM = false;

int
pluton::serviceImpl::setInternalAttributes(int attributes)
{
  switch (attributes) {
  case pluton::serviceAttributes::insideLinuxJVM:
    GInsideJVM = true;
    break;

  default:
    break;
  }

  return -1;
}

pluton::serviceImpl::serviceImpl() :
  _mode(initializing),
  _acceptSocket(-1), _reportingSocket(-1),
  _myPid(0),
  _recorderIndex(0), _recorderCycle(0),
  _affinityTimeout(0), _maximumRequests(0),
  _requestCount(0), _responseCount(0), _faultCount(0),
  _defaultOwner(0), _defaultRequest(0),
  _oldSIGURGHandler(0), _packetTraceFlag(false), _debugFlag(false), _pollProxy(0)
{
  _myPid = getpid();
  _evRequests.pid = _myPid;
  _evRequests.type = reportingChannel::performanceReport;
  _evRequests.U.pd.entries = 0;

  ////////////////////////////////////////////////////////////
  // Check for environment variable settings
  ////////////////////////////////////////////////////////////

  if (getenv("plutonPacketTrace")) _packetTraceFlag = true;
  if (getenv("plutonServiceDebug")) _debugFlag = true;
}


pluton::serviceImpl::~serviceImpl()
{
  terminate();
}


pluton::poll_handler_t
pluton::serviceImpl::setPollProxy(pluton::poll_handler_t newHandler)
{
  pluton::poll_handler_t oldHandler = _pollProxy;

  _pollProxy = newHandler;		// Can be NULL to revert to the system call

  return oldHandler;
}


//////////////////////////////////////////////////////////////////////
// Depending on the mode in which the service is started, it may have
// to open its own accept socket.
//////////////////////////////////////////////////////////////////////

static int
openAcceptSocket(const char* path)
{
  struct sockaddr_un su;
  if (strlen(path) >= sizeof(su.sun_path)) {
    errno = ENAMETOOLONG;
    return -1;
  }

  memset((void*) &su, '\0', sizeof(su));
  su.sun_family = AF_UNIX;
  strcpy(su.sun_path, path);

  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock == -1) return -1;

  if (unlink(path) == -1) if (errno != ENOENT) return -1;
  if (bind(sock, (struct sockaddr*) &su, sizeof(su)) == -1) return -1;
  if (listen(sock, 20) == -1) return -1;

  return sock;
}


//////////////////////////////////////////////////////////////////////
// Track how long each request takes so that it can be reported back
// to the manager and the scoreboard.
//////////////////////////////////////////////////////////////////////

void
pluton::serviceImpl::startResponseTimer(pluton::perCallerService* owner)
{
  gettimeofday(&owner->_requestStartTime, 0);		// Start the clock on the service
  if (_mode == managerMode) _shmService.startResponseTimer(owner->_requestStartTime,
							   owner->_affinityFlag);
}


//////////////////////////////////////////////////////////////////////
// The request has been completed - for some reason - record
// associated information in shm and the reporting Socket.
//////////////////////////////////////////////////////////////////////

void
pluton::serviceImpl::stopResponseTimer(pluton::perCallerService* owner,
				       reportingChannel::reportReason rr,
				       int requestLength, int responseLength,
				       const std::string& function)
{
  gettimeofday(&owner->_requestEndTime, 0);		// Stop the clock on the service
  if (rr == reportingChannel::ok) {
    ++_responseCount;
    _shmService.setProcessResponseCount(_responseCount);
  }
  else {
    ++_faultCount;
    _shmService.setProcessFaultCount(_faultCount);
  }

  //////////////////////////////////////////////////////////////////////
  // Add a report to the event array and if it is full, write the
  // events back to the manager.
  //////////////////////////////////////////////////////////////////////

  if (_reportingSocket != -1) {
    _evRequests.U.pd.rqL[_evRequests.U.pd.entries].reason = rr;
    _evRequests.U.pd.rqL[_evRequests.U.pd.entries].durationMicroSeconds =
      util::timevalDiffuS(owner->_requestEndTime, owner->_requestStartTime);
    _evRequests.U.pd.rqL[_evRequests.U.pd.entries].requestLength = requestLength;
    _evRequests.U.pd.rqL[_evRequests.U.pd.entries].responseLength = responseLength;

    strncpy(_evRequests.U.pd.rqL[_evRequests.U.pd.entries].function,
	    function.data(),
	    sizeof(_evRequests.U.pd.rqL[_evRequests.U.pd.entries].function)-1);
    _evRequests.U.pd.rqL[_evRequests.U.pd.entries].function[sizeof(_evRequests.U.pd.rqL[_evRequests.U.pd.entries].function)-1] = '\0';

    if (++_evRequests.U.pd.entries == reportingChannel::maximumPerformanceEntries) sendReports();
  }

  if (_mode == managerMode) _shmService.stopResponseTimer(owner->_requestEndTime);
}


//////////////////////////////////////////////////////////////////////
// Read netStrings until there is a complete packet or an
// error. _fault is set if there is an error.
//
// Return: 0 no request, client error _fault set
//	   <0 no request, unexpect serviceImpl error _fault set
//	   >0 have request
//
// The packet it read into a the owner's netStringFactoryManaged which
// produces complete netStrings for decoding.
//
// Normally the netStringFactoryManaged class will opportunisitically
// garbage collect to minimize the memory requirement, however this
// module requires access to all netStrings even after the factory has
// produced them as the serviceRequest points directly into the
// factory.
//
// In this case the factory has been set to *not* garbage collect by
// our constructor so this routine has to do it manually otherwise the
// factory could grow to an unbounded size.
//////////////////////////////////////////////////////////////////////

int
pluton::serviceImpl::readRequestPacket(pluton::perCallerService* owner,
				       requestImpl* R, unsigned int requestTimeoutSecs)
{
  if (_mode != fauxSTDIOMode) {
    owner->_packetIn.reset();			// Clear out possible previous read errors
  }
  else {
    owner->_packetIn.reclaimByCompaction();	// Opportunitistic garbage collection
  }
  owner->_decoder.reset();
  owner->_decoder.setRequest(R);
  R->setPacketOffset(owner->_packetIn.getRawOffset());

  std::string em;
  int recorderFD = -1;

  if (!_recorderPrefix.empty()) {
    recorderFD = recordPacketInStart();
  }

  if (_packetTraceFlag) write(2, "PktIn: ", 7);

  // Loop on getting each netString until the decoder says we have a
  // complete packet.

  do {
    const char* parseError = 0;
    while (!owner->_packetIn.haveNetString(parseError)) {
      if (parseError) {
	if (_packetTraceFlag) write(2, "\n", 1);
	if (recorderFD != -1) close(recorderFD);
	owner->_fault.set(requestNetStringParseError, __FUNCTION__, __LINE__, 0, 0, 0,
			  parseError);
	return 0;
      }

      if (!handleRead(owner, requestTimeoutSecs)) {
	if (_packetTraceFlag) write(2, "\n", 1);
	if (recorderFD != -1) close(recorderFD);
	return 0;	// Eof or error (handleRead sets _fault)
      }
    }

    //////////////////////////////////////////////////////////////////////
    // Write out this netString as part of the recorder file
    //////////////////////////////////////////////////////////////////////

    const char* nsDataPtr;
    int nsDataLength;
    if (recorderFD != -1) {
      owner->_packetIn.getRawString(nsDataPtr, nsDataLength);
      write(recorderFD, nsDataPtr, nsDataLength);
    }

    //////////////////////////////////////////////////////////////////////
    // Extract the netString and check that it is a type we want
    //////////////////////////////////////////////////////////////////////

    char nsType;
    int nsDataOffset;
    owner->_packetIn.getNetString(nsType, nsDataPtr, nsDataLength, &nsDataOffset);

    //////////////////////////////////////////////////////////////////////
    // The packet decoder does all the type checking for us.
    //////////////////////////////////////////////////////////////////////

    if (!owner->_decoder.addType(nsType, nsDataPtr, nsDataLength, nsDataOffset, em)) {
      if (_packetTraceFlag) write(2, "\n", 1);
      if (recorderFD != -1) close(recorderFD);
      owner->_fault.set(requestDecodeFailed, __FUNCTION__, __LINE__, 0, 0, em.c_str());
      return 0;
    }
  } while (!owner->_decoder.haveCompleteRequest());

  //////////////////////////////////////////////////////////////////////
  // Once the packet has been completely read in, the netString buffer
  // will not move, so we can convert offset values to absolute
  // pointers.
  //////////////////////////////////////////////////////////////////////

  R->adjustOffsets(owner->_packetIn.getBasePtr(), owner->_packetIn.getRawOffset());

  if (_packetTraceFlag) write(2, "\n", 1);
  if (recorderFD != -1) close(recorderFD);

  return 1;
}


//////////////////////////////////////////////////////////////////////
// Write the outbound packet back to the client. The socket is
// non-blocking so this routine does an opportunistic write as it's
// most likely to succeed. If that write is incomplete, it goes
// through the usual poll()/write() loop to send the complete
// packet. Note that no timeouts are necessary as the manager and the
// client are completely responsible for this. The manager will kill
// me if I stall for too long and the client times out after a caller
// specified time and closes the socket.
//
// The write is done as as set of iovecs because the response data is
// supplied as a set of (upto) three pointer, length pairs.
//////////////////////////////////////////////////////////////////////

int
pluton::serviceImpl::writeResponsePacket(pluton::perCallerService* owner,
					 const char* firstPtr, int firstLen,
					 const char* middlePtr, int middleLen,
					 const char* lastPtr, int lastLen,
					 unsigned int timeoutSecs, int traceFD)
{
  struct iovec iov[3];
  int iovCount = 0;

  if (firstPtr) {
    iov[iovCount].iov_base = (char*) firstPtr;
    iov[iovCount++].iov_len = firstLen;
  }

  if (middlePtr) {
    iov[iovCount].iov_base = (char*) middlePtr;
    iov[iovCount++].iov_len = middleLen;
  }

  if (lastPtr) {
    iov[iovCount].iov_base = (char*) lastPtr;
    iov[iovCount++].iov_len = lastLen;
  }

  struct iovec* iovPtr = iov;	// Iterate through if write is repeated

  if ((traceFD >= 0) && (iovCount > 0)) {
    write(traceFD, "PktOut: ", 8);
    writev(traceFD, iovPtr, iovCount);
  }

  int bytesWritten = 0;
  while (iovCount > 0) {
    int res = writev(owner->_sockOut, iovPtr, iovCount);
    if (res <= 0) {
      if ((errno == EAGAIN) || (errno == EINTR)) {
	struct pollfd fds;
	fds.fd = owner->_sockOut;
	fds.events = POLLOUT;
	fds.revents = 0;
	if (_pollProxy) {
	  res = (_pollProxy)(&fds, 1,
			     timeoutSecs > 0 ? timeoutSecs * util::MICROSECOND : (unsigned) -2);
	}
	else {
	  res = poll(&fds, 1, -1);
	}
	if (_debugFlag) std::clog << "poll write=" << res << " fd=" << fds.fd
				  << " rev=" << fds.revents << std::endl;
	if (res == 1) continue;
      }
      return res;
    }

    // Adjust the iovecs as the write may have been partially completed.

    bytesWritten += res;
    while (res > 0) {
      int sub = std::min(res, (int) iovPtr->iov_len);	// How much of this vec is consumed?
      iovPtr->iov_len -= sub;				// Adjust residual vec to suit
      {				// Mumble Linux has iovecs as void*
	char* p = static_cast<char*>(iovPtr->iov_base);
	p += sub;
	iovPtr->iov_base = p;
      }
      res -= sub;
      if (iovPtr->iov_len == 0) {			// If vec is completely consumed
	--iovCount;					// Move to next
	++iovPtr;
      }
    }	
  }

  return bytesWritten;
}


//////////////////////////////////////////////////////////////////////
// Do post-contruction initialization. This routine mainly exists so
// that we can communicate back to the caller without raising
// exceptions.
//
// A service can start and run in one of three modes; either it is
// passed an accept socket, it is passed an accept path or it reads
// fd3 and writes to fd4. The way it distinguishes between the
// different modes is:
//
// 1) If the environment variable 'plutonAcceptSocket' exists it
//    is the accept path to create and use.
//
// 2) If inherited fds all exist and are of the right type, assume we
//    are executed by the manager and use the inherited fds as the
//    acceptSocket and communication channels.
//
// 3) Otherwise read requests from fd3 and write response to fd4.
//
// Special pluton-tools have direct access to Impl and can set their
// threadID to a positive value to indicate they will support
// concurrent requests. Impl checks for thread-count consistency with
// the config as that affects the way the shared memory scoreboard is
// updated.
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////            
// Breaking out of an accept() call is hard. The only way to do it is
// to close the accept fd. Thus we track the accept fd container in a
// static so that the signal handler can get at it. And of course I
// want to avoid selecting the accept fd as there is a timing window
// between select and accept when multiple services are sitting on the
// one accept socket.
//////////////////////////////////////////////////////////////////////            

static	int	staticAcceptFD = -1;
static void
ourSIGURGHandler(int sig)
{
  if (staticAcceptFD != -1) {
    close(staticAcceptFD);		// force myself off of accept() with EBADF
    staticAcceptFD = -1;		// Quite the hack, but how else to get off accept()?
  }
}


bool
pluton::serviceImpl::initialize(pluton::perCallerService* owner, bool threadedFlag)
{
  if (_debugFlag) std::clog << "SIDebug: initialize mode=" << _mode << std::endl;

  owner->_fault.clear("pluton::service::initialize");

  if (_mode != initializing) {
    owner->_fault.set(alreadyInitialized, __FUNCTION__, __LINE__);
    return false;
  }

  ////////////////////////////////////////////////////////////
  // Check for environment variable over-rides
  ////////////////////////////////////////////////////////////

  const char* socketName = getenv("plutonAcceptSocket");
  if (socketName) {
    _acceptSocket = openAcceptSocket(socketName);
    if (_acceptSocket == -1) {
      owner->_fault.set(openAcceptSocketFailed, __FUNCTION__, __LINE__, 0, errno, socketName);
      return false;
    }
    if (_debugFlag) std::clog << "SIDebug: Accept Mode" << std::endl;
    _mode = acceptMode;
    return true;
  }

  //////////////////////////////////////////////////////////////////////
  // All conditions must be exactly right to set manager mode.
  //////////////////////////////////////////////////////////////////////

  struct stat sb;
  int res;

  res = fstat(plutonGlobal::inheritedAcceptFD, &sb);
  if ((res == 0) && (sb.st_mode & S_IFSOCK)) {
    res = fstat(plutonGlobal::inheritedShmServiceFD, &sb);
    if ((res == 0) && (sb.st_mode & S_IFREG)) {
      res = fstat(plutonGlobal::inheritedReportingFD, &sb);
      if ((res == 0) && (sb.st_mode & S_IFIFO)) {

	pluton::faultCode fc = _shmService.mapService(plutonGlobal::inheritedShmServiceFD, _myPid,
					     threadedFlag);
	if (fc != pluton::noFault) {
	  owner->_fault.set(fc, __FUNCTION__, __LINE__, 0, errno);
	  return false;
	}

	//////////////////////////////////////////////////////////////////////
	// The planets are aligned (all 8 of them), set up for manager mode.
	//////////////////////////////////////////////////////////////////////

	if (_debugFlag) std::clog << "SIDebug: Manager Mode" << std::endl;
	_mode = managerMode;

	close(plutonGlobal::inheritedShmServiceFD);		// No longer need this once mapped

	// Copy relevant shm config values into a local copy. Never
	// trust writable shm for too long...

	_affinityTimeout = _shmService.getServicePtr()->_config._affinityTimeout;
	_maximumRequests = _shmService.getServicePtr()->_config._maximumRequests;
	_recorderCycle = _shmService.getServicePtr()->_config._recorderCycle;
	_recorderPrefix.assign(_shmService.getServicePtr()->_config._recorderPrefix);

	_acceptSocket = plutonGlobal::inheritedAcceptFD;
	util::increaseOSBuffer(_acceptSocket, 64*1024, 64*1024);	// These get inherited

	_reportingSocket = plutonGlobal::inheritedReportingFD;

	// Notify the manager via shm that this service is now active

	struct timeval now;
	gettimeofday(&now, 0);
	_shmService.setProcessReady(now);
	staticAcceptFD = _acceptSocket;

	//////////////////////////////////////////////////////////////////////
	// Remember whether the caller has registered a SIGURG
	// handler. If they have, it will be re-instated around calls
	// to the service API.
	//////////////////////////////////////////////////////////////////////

	_oldSIGURGHandler = signal(SIGURG, ourSIGURGHandler);
	if (_oldSIGURGHandler) signal(SIGURG, _oldSIGURGHandler);	// re-instate existing

	return true;
      }
      else if (_debugFlag) std::clog << "SIDebug: inheritedReportingFD=" << res
				     << " mode=" << sb.st_mode << " errno=" << errno << std::endl;
    }
    else if (_debugFlag) std::clog << "SIDebug: inheritedShmServiceFD=" << res
				   << " mode=" << sb.st_mode << " errno=" << errno << std::endl;
  }
  else if (_debugFlag) std::clog << "SIDebug: inheritedAcceptFD=" << res
				 << " mode=" << sb.st_mode << " errno=" << errno << std::endl;

  //////////////////////////////////////////////////////////////////////
  // At various stages during the read of the request, the actual
  // number of bytes to read is unknown so the input fd is set
  // non-blocking and the read code selects and reads so that maximum
  // number of bytes can be transferred per call, yet not block on the
  // end of the request. The client *could* shutdown() the socket and
  // then we could avoid all the non-blocking goop.
  //////////////////////////////////////////////////////////////////////

  if (util::setNonBlocking(FAUX_STDIN) == -1) {
    owner->_fault.set(pluton::noFauxSTDIN, __FUNCTION__, __LINE__);
    return false;
  }

  if (_debugFlag) std::clog << "SIDebug: fauxSTDIO Mode" << std::endl;
  _mode = fauxSTDIOMode;

  return true;
}


//////////////////////////////////////////////////////////////////////
// Read the request in. Return false if caller should exit. This code
// is mostly about deciding *how* to get the request data based on the
// mode and affinity settings. Also, this routine normally hides
// client errors such unexpected EOF and bad packets.
//////////////////////////////////////////////////////////////////////

bool
pluton::serviceImpl::getRequest(pluton::perCallerService* owner, requestImpl* R,
				unsigned int acceptTimeoutSecs, unsigned int requestTimeoutSecs,
				bool exposeClientErrors)
{
  if (_debugFlag) std::clog << "SIDebug: getRequest mode=" << _mode << std::endl;

  owner->_fault.clear("pluton::service::getRequest");
  if (owner->_state != pluton::perCallerService::canGetRequest) {
    owner->_fault.set(getRequestNotNext, __FUNCTION__, __LINE__);
    if (_debugFlag) std::clog << "SIDebug: getRequest ret=wrongState" << _mode << std::endl;
    return false;
  }

  if (_mode == initializing) {
    owner->_fault.set(notInitialized, __FUNCTION__, __LINE__);
    if (_debugFlag) std::clog << "SIDebug: getRequest not initialized" << std::endl;
    return false;
  }

  if (_mode == managerMode) {			// Reached config limit?
    if ((_maximumRequests > 0) && (_requestCount >= _maximumRequests) && !owner->_affinityFlag) {
      owner->_state = pluton::perCallerService::mustShutdown;
      _shmService.setProcessExitReason(processExit::maxRequests);
      if (_debugFlag) std::clog << "SIDebug: getRequest ret=no more" << std::endl;
      return false;		// Not a fault!
    }
  }

  //////////////////////////////////////////////////////////////////////
  // Iterate around trying to get an inbound connection and
  // request. Most non-persistent errors are likely to be client
  // errors which are not normally exposed to the service. In these
  // cases, simply loop around and try again.
  //////////////////////////////////////////////////////////////////////

  while (true) {
    int res = getOneConnection(owner, acceptTimeoutSecs);
    if (_debugFlag) std::clog << "SIDebug: getOneConnection=" << res << std::endl;
    if (res < 0) {			// Persistent error or closure of socket?
      closeConnection(owner);
      owner->_state = pluton::perCallerService::mustShutdown;
      return false;			// _fault may be set by getOneConnection
    }
    if (res == 0) {			// Timeout
      closeConnection(owner);
      continue;				// Loop around for client type errors
    }

    R->resetRequestValues();		// Completely reset the request data
    R->resetResponseValues();		// and the response data
    R->setFault(pluton::noFault);	// Start with no fault
    startResponseTimer(owner);

    //////////////////////////////////////////////////////////////////////
    // Adjust the timeout down to the affinity limit for affinity
    // connections
    //////////////////////////////////////////////////////////////////////

    if (owner->_affinityFlag) {
      if (_debugFlag) std::clog << "SIDebug: Affinity timeout is " << _affinityTimeout
				<< " rto=" << requestTimeoutSecs << std::endl;
      if (_affinityTimeout > 0) {
	if ((requestTimeoutSecs == 0) || (_affinityTimeout < (signed) requestTimeoutSecs)) {
	  requestTimeoutSecs = _affinityTimeout;
	  if (_debugFlag) std::clog << "SIDebug: request t/o = " << requestTimeoutSecs << std::endl;
	}
      }
    }

    res = readRequestPacket(owner, R, requestTimeoutSecs);
    if (res > 0) break;

    //////////////////////////////////////////////////////////////////////
    // Eof or error. Cannot continue unless in manager mode and if
    // client errors are not to be exposed (tools ask for this, but
    // regular services will not be exposed).
    //////////////////////////////////////////////////////////////////////

    stopResponseTimer(owner, reportingChannel::readError, 0, 0, "");
    closeConnection(owner);
    if (exposeClientErrors || _mode != managerMode) return false;
  }

  //////////////////////////////////////////////////////////////////////
  // We have a request - update shm scoreboard and return to caller.
  //////////////////////////////////////////////////////////////////////

  owner->_state = pluton::perCallerService::canSendResponse;
  unsigned int requestID = owner->_decoder.getRequestID();
  R->setRequestID(requestID);

  std::string cname;
  R->getClientName(cname);
  ++_requestCount;
  _shmService.setProcessRequestCount(_requestCount);
  _shmService.setProcessClientDetails(requestID, owner->_requestStartTime,
				      cname.data(), cname.length());

  //////////////////////////////////////////////////////////////////////
  // Remember the attributes for when the caller sends the response.
  //////////////////////////////////////////////////////////////////////

  owner->_affinityFlag = false;
  owner->_noWaitFlag = R->getAttribute(pluton::noWaitAttr);
  if (owner->_noWaitFlag) {
    closeConnection(owner);
  }
  else {
    owner->_affinityFlag = R->getAttribute(pluton::keepAffinityAttr);
  }

  if (_debugFlag) std::clog << "SIDebug: get ok _noWait=" << owner->_noWaitFlag
			    << " _affinity=" << owner->_affinityFlag << std::endl;

  return true;
}


//////////////////////////////////////////////////////////////////////
// This routine does most of the work to establish an inbound
// connection. It is in a separate routine so the parent can easily
// iterate on non-fatal errors.
//
// Accept the next connection from the acceptSocket if such a socket
// is open. If a socket is already open due to a previous keepAffinity
// request or non-manager mode, do nothing.
//
// Return: 0 no connection
//	   <0 no connection, unexpect serviceImpl error _fault set
//	   >0 have connection
//////////////////////////////////////////////////////////////////////

int
pluton::serviceImpl::getOneConnection(pluton::perCallerService* owner, unsigned int timeoutSecs)
{
  if (_debugFlag) std::clog << "SIDebug: getOneConnection mode=" << _mode
		       << " _affinity=" << owner->_affinityFlag << std::endl;

  if ((_mode == acceptMode) || (_mode == managerMode)) {
    if (!owner->_affinityFlag) {
      if (_mode == managerMode) _shmService.setProcessAcceptingRequests(true);
      int sock = acceptConnection(owner, timeoutSecs);
      if (_mode == managerMode) _shmService.setProcessAcceptingRequests(false);
      if (sock == -1) return -1;			// Unrecoverable
      if (sock == -2) return 0;				// Timeout
      owner->_sockIn = owner->_sockOut = sock;
    }
  }
  else {
    owner->_sockIn = FAUX_STDIN;
    owner->_sockOut = FAUX_STDOUT;
  }

  return 1;
}


//////////////////////////////////////////////////////////////////////
// Use select to check for accept. There is a timing window between
// the select and the accept where another service could have grabbed
// the connection. In this case we will block and if an attempt by the
// manager to signal us occurs then, then we will ultimately
// abnormally terminate as we will stay blocked until the Manager
// SIGINTs us.
//////////////////////////////////////////////////////////////////////

#ifdef __linux__
int
pluton::serviceImpl::linuxAccept(int acceptFD, struct sockaddr *sa, socklen_t *salen)
{
  while (true) {
    fd_set fds;
    FD_ZERO(&fds);
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    FD_SET(acceptFD, &fds);
    int res = select(acceptFD+1, &fds, 0, 0, &tv);
    if (_debugFlag) std::clog << "SIDebug: select=" << res << " errno=" << errno << std::endl;

    if (res < 0) return -1;
    if (res == 1) break;
  }

  return accept(acceptFD, sa, salen);
}
#endif

//////////////////////////////////////////////////////////////////////
// Accept a new connection from the socket. If we're in manager mode,
// we may get a signal from the manager telling us to check for a
// shutdown request. The timeout only applies if a proxy Poll handler
// has been registered. _fault is set on unexpected failure.
//
// If the caller has a SIGURG handler, wrap our signal handler around
// accept() and then restore the caller's handler. If they remove
// their SIGURG handler, this code assumes that it has control of that
// signal for good. This is not perfectly correct behaviour as the
// caller *might* re-instate a new handler later on, but given the
// structure that services are meant to use, this is an exceedingly
// unlikely event.
//
// The best thing for the caller is to not have a SIGURG handler -
// something a service would not normally need.
//////////////////////////////////////////////////////////////////////

int
pluton::serviceImpl::acceptConnection(pluton::perCallerService* owner, unsigned int timeoutSecs)
{
  struct sockaddr sa;
  socklen_t salen = sizeof(sa);

  if (_oldSIGURGHandler) _oldSIGURGHandler = signal(SIGURG, ourSIGURGHandler);

  if (_debugFlag) {
    std::clog << "SIDebug: accept(" << _acceptSocket << ") URG=" << _oldSIGURGHandler << std::endl;
  }

  int sock = -1;
  if (_pollProxy) {
    struct pollfd fds;
    fds.fd = _acceptSocket;
    fds.events = POLLIN;
    fds.revents = 0;
    if ((_pollProxy)(&fds, 1, timeoutSecs > 0 ? timeoutSecs * util::MICROSECOND : (unsigned) -2) == 1) {
      sock = accept(_acceptSocket, &sa, &salen);
    }
    else {
      return -2;
    }
  }
  else {

    //////////////////////////////////////////////////////////////////////
    // In Linux with Java we have to select() for accept() as accept()
    // never returns after a SIGURG. Consequently we need to break out
    // of accept() periodically to test for an exit signal. Sigh.
    //////////////////////////////////////////////////////////////////////

#ifdef __linux__
    if (GInsideJVM) {
      if (_debugFlag) std::clog << "SIDebug: JVM select" << std::endl;
      sock = linuxAccept(_acceptSocket, &sa, &salen);
    }
    else {
      sock = accept(_acceptSocket, &sa, &salen);
    }
#else
    sock = accept(_acceptSocket, &sa, &salen);
#endif
  }

  if (_debugFlag) {
    std::clog << "SIDebug: A accept=" << sock << " URG=" << _oldSIGURGHandler << std::endl;
  }
  if (_oldSIGURGHandler) {
    int saveErrno = errno;
    signal(SIGURG, _oldSIGURGHandler);
    errno = saveErrno;
  }

  // If accept() gave us a socket, set it up and return to caller

  if (sock != -1) {
    util::setNonBlocking(sock);
    return sock;
  }

  //////////////////////////////////////////////////////////////////////
  // Accept failed, complain if it wasn't a signal from the manager.
  //////////////////////////////////////////////////////////////////////

  if (_mode != managerMode) {
    if (_debugFlag) std::clog << "SIDebug: accept=-1 non-manager errno=" << errno << std::endl;
    owner->_fault.set(acceptFailed, __FUNCTION__, __LINE__, 0, errno);
    return -1;
  }

  //////////////////////////////////////////////////////////////////////
  // If it is from the manager exit with their reason.
  //////////////////////////////////////////////////////////////////////

  if ((errno == EBADF) && (staticAcceptFD == -1)) {
    if (_debugFlag) std::clog << "SIDebug: accept=-1 Manager EBADF/-1 errno=" << errno << std::endl;
    processExit::reason er = _shmService.getProcessShutdownRequest();
    _shmService.setProcessExitReason(er);
    return -1;
  }

  if (_debugFlag) std::clog << "SIDebug: accept=-1 manager fault" << std::endl;
  owner->_fault.set(acceptFailed, __FUNCTION__, __LINE__, 0, errno);
  _shmService.setProcessExitReason(processExit::acceptFailed);
  return -1;
}


//////////////////////////////////////////////////////////////////////
// General routine for sending fault/response back to the client. The
// caller is responsible for deciding which of fault or response data
// is sent based on noFault and responseLength. They can choose to
// send both however the client is not coded to deal with this
// ambiguity.
//////////////////////////////////////////////////////////////////////

bool
pluton::serviceImpl::sendResponse(pluton::perCallerService* owner,
				  const requestImpl* R, unsigned int timeoutSecs)
{
  owner->_fault.clear("pluton::service::sendResponse");
  if (owner->_state != pluton::perCallerService::canSendResponse) {
    owner->_fault.set(sendResponseNotNext, __FUNCTION__, __LINE__);
    return false;
  }

  //////////////////////////////////////////////////////////////////////
  // If the client didn't stick around, short-cut the response
  // processing.
  //////////////////////////////////////////////////////////////////////

  if (owner->_noWaitFlag) {
    stopResponseTimer(owner, R->getFaultCode() == pluton::noFault ?
		      reportingChannel::ok : reportingChannel::fault,
		      R->getRequestDataLength(), 0, R->getServiceFunction());
    owner->_state = pluton::perCallerService::canGetRequest;
    return true;
  }

  //////////////////////////////////////////////////////////////////////
  // Assemble as a three-part writev to avoid copying the response
  // data set by the caller.
  //////////////////////////////////////////////////////////////////////

  netStringGenerate packetOutPre;
  netStringGenerate packetOutPost;
  R->assembleResponsePacket(owner->_name, packetOutPre, packetOutPost);

  const char* resP;
  int resL;
  R->getResponseData(resP, resL);
  if (!_recorderPrefix.empty()) {
    recordPacketOut(packetOutPre.data(), packetOutPre.length(),
		    resP, resL,
		    packetOutPost.data(), packetOutPost.length());
  }

  int writeBytes = writeResponsePacket(owner,
				       packetOutPre.data(), packetOutPre.length(),
				       resP, resL,
				       packetOutPost.data(), packetOutPost.length(),
				       timeoutSecs,
				       _packetTraceFlag ? 2 : -1);

  //////////////////////////////////////////////////////////////////////
  // It's pretty hard to determine the reason for a failed write as
  // the client may have exited, stopped reading, got suspended, who
  // knows? We accept EPIPE and truncated writes as client errors,
  // otherwise assume it's an error at our end.
  //////////////////////////////////////////////////////////////////////

  if (_debugFlag) std::clog << "SIDebug: writeResponsePacket=" << writeBytes
		       << " errno=" << errno << std::endl;

  if ((writeBytes == -1) && (errno != EPIPE)) {
    owner->_fault.set(socketWriteFailed, __FUNCTION__, __LINE__, 0, errno);
    closeConnection(owner);
    owner->_state = pluton::perCallerService::mustShutdown;
    _shmService.setProcessExitReason(processExit::lostIO);
    return false;
  }

  closeConnection(owner, true);
  owner->_state = pluton::perCallerService::canGetRequest;

  stopResponseTimer(owner, R->getFaultCode() == pluton::noFault ?
		    reportingChannel::ok : reportingChannel::fault,
		    R->getRequestDataLength(), R->getResponseDataLength(),
		    R->getServiceFunction());

  return true;
}


//////////////////////////////////////////////////////////////////////
// A tools-only interface for sending a raw packet back instead of
// having the service assemble it from a request. This method is only
// available via the Impl and not visible via pluton::service.
//////////////////////////////////////////////////////////////////////


bool
pluton::serviceImpl::sendRawResponse(pluton::perCallerService* owner,
				     int requestLength, const char* p, int l,
				     const std::string& serviceFunction,
				     unsigned int timeoutSecs)
{
  owner->_fault.clear("pluton::service::sendRawResponse");
  if (owner->_state != pluton::perCallerService::canSendResponse) {
    owner->_fault.set(sendRawResponseNotNext, __FUNCTION__, __LINE__);
    return false;
  }

  if (!_recorderPrefix.empty()) recordPacketOut(p, l);

  int writeBytes = writeResponsePacket(owner,
				       p, l,		// Data one
				       0, 0,		// Data two
				       0, 0,		// Data three
				       timeoutSecs,
				       _packetTraceFlag ? 2 : -1);

  //////////////////////////////////////////////////////////////////////
  // Write failures can occur for numerous reasons. Most are assumed
  // to be clients disappearing - perhaps due to timing out.
  //////////////////////////////////////////////////////////////////////

  if ((writeBytes == -1) && (errno != EPIPE)) {
    owner->_fault.set(socketWriteFailed, __FUNCTION__, __LINE__, 0, errno);
    closeConnection(owner);
    owner->_state = pluton::perCallerService::canGetRequest;
    stopResponseTimer(owner, reportingChannel::writeError, requestLength, 0, serviceFunction);
    return false;
  }

  closeConnection(owner, true);
  owner->_state = pluton::perCallerService::canGetRequest;
  stopResponseTimer(owner, reportingChannel::ok, requestLength, writeBytes, serviceFunction);

  return true;
}


//////////////////////////////////////////////////////////////////////
// Release system resources associated with a completed request. If
// this connection can keep affinity, then leave the socket open for
// the next getRequest() call.
//////////////////////////////////////////////////////////////////////

void
pluton::serviceImpl::closeConnection(pluton::perCallerService* owner, bool canKeepAffinity)
{
  if ((_mode == acceptMode) || (_mode == managerMode)) {
    if (canKeepAffinity && owner->_affinityFlag) return;

    if (owner->_sockIn != -1) close(owner->_sockIn);
    if ((owner->_sockIn != owner->_sockOut) && (owner->_sockOut != -1)) close(owner->_sockOut);
    owner->_sockIn = owner->_sockOut = -1;
  }

  owner->_affinityFlag = false;
}


void
pluton::serviceImpl::terminate()
{
  if (_acceptSocket != -1) {
    close(_acceptSocket);
    _acceptSocket = -1;
    _shmService.setProcessTerminated();		// Tell manager we're outta here
  }
  if (_reportingSocket != -1) {
    notifyManager();
    sleep(1);					// close(2) says "queued data are
    close(_reportingSocket);			// discarded (for a socket(2)), so
    _reportingSocket = -1;			// increase the chances for the read
  }
}


//////////////////////////////////////////////////////////////////////
// Tell the manager that there may be something interesting to look at
// in the shmService structure. This is simply done by sending
// anything we have to the Reporting Channel which will wake up the
// monitoring thread.
//////////////////////////////////////////////////////////////////////

void
pluton::serviceImpl::notifyManager()
{
  sendReports(true);
}


//////////////////////////////////////////////////////////////////////
// Send all outstanding reports to the manager via the reporting
// channel.
//////////////////////////////////////////////////////////////////////

void
pluton::serviceImpl::sendReports(bool forceWriteFlag)
{
  if (_reportingSocket == -1) return;

  if ((_evRequests.U.pd.entries > 0) || forceWriteFlag) {
    int bytes = write(_reportingSocket, (char*) &_evRequests, sizeof(_evRequests));
    if (bytes != sizeof(_evRequests)) {
      close(_reportingSocket);
      _reportingSocket = -1;
    }
    _evRequests.U.pd.entries = 0;
  }
}


//////////////////////////////////////////////////////////////////////
// Read whatever is on offer into the netString. Return false on error
// with _fault set.
//////////////////////////////////////////////////////////////////////

bool
pluton::serviceImpl::handleRead(pluton::perCallerService* owner, unsigned int requestTimeoutSecs)
{
  char* bufPtr;
  int minRequired, maxAllowed;		// How much is needed?
  if (owner->_packetIn.getReadParameters(bufPtr, minRequired, maxAllowed) == -1) {
    owner->_fault.set(netStringTooLarge, __FUNCTION__, __LINE__, maxAllowed);
    return false;
  }

  //////////////////////////////////////////////////////////////////////          
  // Wait until there is data to read. The read fd has been set
  // non-blocking as this caller doesn't know how many bytes are
  // pending so it typically supplies a larger than necessary
  // buffer. The downside of this is that we have to poll prior to
  // reading.
  //////////////////////////////////////////////////////////////////////          

  struct pollfd fds;
  fds.fd = owner->_sockIn;
  fds.events = POLLIN;
  fds.revents = 0;

  int res;
  if (_pollProxy) {
    if (requestTimeoutSecs) {
      res = (_pollProxy)(&fds, 1, requestTimeoutSecs * util::MICROSECOND);
    }
    else {
      res = (_pollProxy)(&fds, 1, (unsigned) -2);
    }
  }
  else {
    if (requestTimeoutSecs) {
      res = poll(&fds, 1, requestTimeoutSecs * util::MILLISECOND);
    }
    else {
      res = poll(&fds, 1, -1);
    }
  }

  if (_debugFlag) std::clog << "SIDebug: poll returns " << res
			    << " fd=" << owner->_sockIn
			    << " rev=" << fds.revents
			    << " from " << requestTimeoutSecs << std::endl;

  if (res == 0) return false;		// Timeout is a failure without a fault

  if (res != 1) {			// Eh?
    owner->_fault.set(errno == EINTR ? pollInterrupted : seriousInternalOSError,
		      __FUNCTION__, __LINE__, owner->_sockIn, errno);
    return false;
  }

  //////////////////////////////////////////////////////////////////////
  // Read is indicated - slurp in as much as the netString allows.
  //////////////////////////////////////////////////////////////////////

  int bytesRead = read(owner->_sockIn, bufPtr, maxAllowed);

  if (_debugFlag) std::clog << "SIDebug: read=" << owner->_sockIn
		       << " maxAllowed=" << maxAllowed
		       << " readres=" << bytesRead << " errno=" << errno << std::endl;

  if (bytesRead == 0) return false;

  if (bytesRead > 0) {
    if (_packetTraceFlag) write(2, bufPtr, bytesRead);
    owner->_packetIn.addBytesRead(bytesRead);	// Tell the factory about the new data
    return true;
  }

  //////////////////////////////////////////////////////////////////////          
  // Since it's a non-blocking fd, ignore the "retriable" errors. If              
  // they recurr we'll eventually come off of the loop due to the                 
  // timeout.                                                                     
  //////////////////////////////////////////////////////////////////////          

  if (util::retryNonBlockIO(errno)) return true;

  owner->_fault.set(socketReadFailed, __FUNCTION__, __LINE__, maxAllowed, errno);
  return false;
}


//////////////////////////////////////////////////////////////////////
// Helper routine for the recording methods.
//////////////////////////////////////////////////////////////////////

static void
writeRecord(const char* fname,
	    const char* p1=0, int l1=0,
	    const char* p2=0, int l2=0,
	    const char* p3=0, int l3=0)
{
  int fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0640);
  if (fd != -1) {
    if (p1) write(fd, p1, l1);
    if (p2) write(fd, p2, l2);
    if (p3) write(fd, p3, l3);
    close(fd);
  }
}

//////////////////////////////////////////////////////////////////////
// If the service is configured to record packets, then the input and
// output packets are written to a round-robin set of files. The
// round-robin count - if set non-zero - is used purely to constrain
// the disk space consumed by a debug config that is accidently left
// enabled.
//
// Almost no error checking or reporting occurs in this code -
// recording is a "best effort", passive service.
//
// Recording of inbound has to be done piece-meal because that's the
// way it is parsed in. This routine opens the file for recording the
// inbound packet and returns the fd for subsequent writes.
//////////////////////////////////////////////////////////////////////

int
pluton::serviceImpl::recordPacketInStart()
{
  ++_recorderIndex;
  if (_recorderCycle > 0) _recorderIndex %= _recorderCycle;

  std::ostringstream os;
  os << _recorderPrefix << "in."
     << _myPid << "."
     << std::setw(5) << std::setfill('0') << _recorderIndex;
  std::string fname = os.str();

  return open(fname.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0640);
}

void
pluton::serviceImpl::recordPacketOut(const char* firstPtr, int firstLen,
				     const char* middlePtr, int middleLen,
				     const char* lastPtr, int lastLen)
{
  std::ostringstream os;
  os << _recorderPrefix << "out."
     << _myPid << "."
     << std::setw(5) << std::setfill('0') << _recorderIndex;
  std::string fname = os.str();

  writeRecord(fname.c_str(), firstPtr, firstLen, middlePtr, middleLen, lastPtr, lastLen);
}


//////////////////////////////////////////////////////////////////////
// perCallerService manages state unique to parallel service
// handlers. It only applies to tools built with the rest of pluton
// and is not accessible to regular services.
//////////////////////////////////////////////////////////////////////

pluton::perCallerService::perCallerService(const char* setName, int threadID)
  : _state(canGetRequest),
    _noWaitFlag(false), _affinityFlag(false),
    _sockIn(-1), _sockOut(-1), _myTid(threadID), _packetIn(16 * 1024)
{
  util::IA ia;
  _name = setName;
  _name.append(":");
  _name.append(util::ltoa(ia, static_cast<int>(getpid())));
  _name.append("/");
  _name.append(util::ltoa(ia, _myTid));

  _packetIn.disableCompaction();
}


pluton::perCallerService::~perCallerService()
{
}

void
pluton::perCallerService::initialize()
{
}
