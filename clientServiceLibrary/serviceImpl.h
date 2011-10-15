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

#ifndef P_SERVICEIMPL_H
#define P_SERVICEIMPL_H 1

#include <string>

#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "netString.h"


#include "global.h"
#include "faultImpl.h"
#include "decodePacket.h"
#include "reportingChannel.h"
#include "requestImpl.h"
#include "shmService.h"

namespace pluton {

  typedef int (*poll_handler_t)(struct pollfd *fds, int nfds, unsigned long long timeout);

  class serviceRequestImpl : public requestImpl {
  };

  class perCallerService;

  class serviceImpl {
  public:
    serviceImpl();
    ~serviceImpl();

    static int 			setInternalAttributes(int);

    pluton::poll_handler_t 	setPollProxy(pluton::poll_handler_t);

    bool	initialize(pluton::perCallerService* owner, bool threadedFlag=false);

    int		getMaximumThreads() const { return _shmService.getMaximumThreads(); }

    bool	getRequest(pluton::perCallerService* owner, pluton::requestImpl*,
			   unsigned int acceptTimeoutSecs=0, unsigned int requestTimeoutSecs=0,
			   bool exposeClientErrors=false);

    int		getOneConnection(pluton::perCallerService* owner, unsigned int acceptTimeoutSecs);

    bool	sendResponse(pluton::perCallerService* owner, const pluton::requestImpl*,
			     unsigned int timeoutSecs=0);
    void	terminate();

    // Tools only

    bool	sendRawResponse(pluton::perCallerService* owner, int requestLength,
				const char* responsePtr, int responseLength,
				const std::string& serviceFunction, unsigned int timeoutSecs);

    void	setDefaultRequest(pluton::serviceRequestImpl* newR) { _defaultRequest = newR; }
    pluton::serviceRequestImpl*	getDefaultRequest() const { return _defaultRequest; }

    void	setDefaultOwner(pluton::perCallerService* newO) { _defaultOwner = newO; }
    pluton::perCallerService*	getDefaultOwner() const { return _defaultOwner; }

  private:
    serviceImpl&	operator=(const serviceImpl& rhs);	// Assign not ok
    serviceImpl(const serviceImpl& rhs);			// Copy not ok

    // The exchange mechanism depends on how the service is started.

    enum { initializing, acceptMode, managerMode, fauxSTDIOMode } _mode;
    static const int	FAUX_STDIN = 3;
    static const int	FAUX_STDOUT = 4;

    int		_acceptSocket;
    int		_reportingSocket;
    pid_t	_myPid;
    int		_recorderIndex;
    int		_recorderCycle;

    reportingChannel::record	_evRequests;
    shmServiceHandler		_shmService;

    // Config parameters copied from shmService when in manager mode

    int		_affinityTimeout;
    int		_maximumRequests;
    std::string	_recorderPrefix;

    // Counters transferred to shm so others can see them

    int		_requestCount;
    int		_responseCount;
    int		_faultCount;

    // Misc

    perCallerService*		_defaultOwner;		// These two are used by
    serviceRequestImpl*		_defaultRequest;	// standard services

    sig_t			_oldSIGURGHandler;
    bool			_packetTraceFlag;
    bool			_debugFlag;
    pluton::poll_handler_t	_pollProxy;
    time_t			_lastHeartbeat;

    //////////////////////////////////////////////////////////////////////

    int		acceptConnection(pluton::perCallerService* owner, unsigned int timeoutSecs);
    void	closeConnection(pluton::perCallerService* owner, bool canKeepaffinity=false);
#ifdef __linux__
    int		linuxAccept(int acceptFD, struct sockaddr *sa, socklen_t *salen);
#endif
    void	startResponseTimer(pluton::perCallerService* owner);
    void	stopResponseTimer(pluton::perCallerService* owner,
				  reportingChannel::reportReason,
				  int requestLength, int responseLength,
				  const std::string& function);

    int		readRequestPacket(pluton::perCallerService* owner,
				  requestImpl* R, unsigned int timeoutSecs);
    int		writeResponsePacket(pluton::perCallerService* owner,
				    const char*, int, const char*, int, const char*, int,
				    unsigned int timeoutSecs, int traceFD);

    bool	handleRead(pluton::perCallerService* owner, unsigned int timeoutSecs);
    int		handleWrite(const char*, int);

    void	notifyManager();
    void	sendReports(bool forceWriteFlag=false);

    int		recordPacketInStart();
    void	recordPacketOut(const char*, int, const char* p1=0, int=0, const char* p2=0, int=0);

    bool	checkManagerHeartbeat();
  };


  //////////////////////////////////////////////////////////////////////
  // This perCallerService wrapper contains unique per-caller
  // state. It is primarily intended for multi-threaded services that
  // can concurrently call serviceImpl. Non multi-threaded services
  // use a default instance of this class.
  //////////////////////////////////////////////////////////////////////

  class perCallerService {
  public:
    perCallerService(const char* name, int threadID=0);
    ~perCallerService();

    void			initialize();
    bool			hasFault() const { return _fault.hasFault(); }
    const pluton::fault&	getFault() const { return _fault; }

    pluton::faultInternal	_fault;

    enum { initializing, canGetRequest, canSendResponse, mustShutdown } _state;
    bool			_noWaitFlag;
    bool			_affinityFlag;

    int		_sockIn;
    int		_sockOut;
    int		_myTid;

    netStringFactoryManaged	_packetIn;
    decodeRequestPacket		_decoder;
    std::string			_name;

    struct timeval		_requestStartTime;
    struct timeval		_requestEndTime;
  };
}

#endif
