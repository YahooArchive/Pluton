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

#include <string>
#include <list>

#include <iostream>

using namespace std;

#include <arpa/inet.h>		// htons
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>

#include <st.h>

#include "util.h"
#include "global.h"
#include "netString.h"
#include "decodePacket.h"
#include "requestImpl.h"
#include "serviceImpl.h"


static const char* myName = "plTransmitter";

static const char* usage =
"\n"
"Usage: plTransmitter [-dhC] [-c connectsPerHost] [-l serviceLifeSecs]\n"
"                     [-s maxRequestDurationSecs] hosts...\n"
"\n"
"Transmit Pluton Requests to plReceivers running on remote hosts\n"
"\n"
"Where:\n"
" -C   Connect to all resolved addresses immediately rather than on-demand\n"
"\n"
" -c   Number of connections per resolved address per host (default: 1)\n"
" -d   Turn on debugging output\n"
" -h   Print this usage message on STDOUT and exit(0)\n"
" -l   The service should exit after this many seconds to enable a fresh\n"
"       set of gethostbyname() results. Zero disables this feature\n"
"       (default: 3600)\n"
" -s   Maximum seconds allowed per request (default: 30)\n"
"\n"
" hosts are a list of hostname[:port] values that are resolved\n"
"      with gethostbyname() (default port: 14099)\n"
"\n"
"See also: http://localhost/docs/pluton/\n"
"\n";


//////////////////////////////////////////////////////////////////////
// Each host on the command line expands to a set of host entry as
// gethostbyname() can return multiple answers per query.
//////////////////////////////////////////////////////////////////////



class remoteHost {
public:
  remoteHost(const string& setHostName, int setPort, const char* setAddr)
    : _hostName(setHostName), _port(setPort), _fd(0)
  {
    memcpy((void*) &_addr, (void*) setAddr, sizeof(_addr));
    _packetIn.disableCompaction();	// So we can point at the response data
  }

  ~remoteHost() { disconnect(); }

  const char*	getName() const { return _hostName.c_str(); }

  bool		connect(int timeoutUS=10000000);
  void		disconnect();

  int		sendRequest(const char* cp, int len, unsigned int timeoutuS) const;
  void		transferFileDescriptor(int fd) { close(fd); }
  int		readPacket(unsigned long timeoutuS);
  void		getInboundPacketData(const char*& p, int& l) const { p = _packetPtr; l = _packetLen; }

private:
  string 	_hostName;
  int		_port;
  in_addr_t 	_addr;
  st_netfd_t	_fd;

  netStringFactoryManaged 	_packetIn;
  pluton::decodeRequestPacket	_decoder;

  int		_packetOffset;
  const char*	_packetPtr;
  int		_packetLen;
};

typedef list<remoteHost*>		remoteHostList;
typedef list<remoteHost*>::iterator	remoteHostListIter;


//////////////////////////////////////////////////////////////////////
// These statics are used to communicate command-line parameters to
// the threads.
//////////////////////////////////////////////////////////////////////

static remoteHostList 	freeHosts;
static	st_cond_t	freeHostsCvar;

static void*	handler(void*);


static	int	connectsPerHost = 1;
static  int	serviceLifeSecs = 3600;
static  bool	debugFlag = false;
static  bool	connectImmediatelyFlag = false;
static	int	activeServices = 0;
static  int     maxRequestDurationSecs = 30;


//////////////////////////////////////////////////////////////////////

class handlerData {
public:
  handlerData(pluton::serviceImpl* initS, int initTI)
    : S(initS), threadIndex(initTI) {}
  pluton::serviceImpl* 	S;
  int			threadIndex;
};


int
main(int argc, char** argv)
{
  char optionChar;

  while ((optionChar = getopt(argc, argv, "Cc:dhl:s:")) != EOF) {
    switch (optionChar) {
    case 'c':
      connectsPerHost = atoi(optarg);
      if (connectsPerHost <= 0) {
	clog << "Error: connectsPerHost must be greater than zero" << endl;
	clog << usage;
	exit(1);
      }
      break;

    case 'C':
      connectImmediatelyFlag = true;
      break;

    case 'd':
      debugFlag = true;
      break;

      
    case 'l':
      serviceLifeSecs = atoi(optarg);
      if (serviceLifeSecs < 0) {
        clog << "Error: serviceLifeSecs must not be negative" << endl;
        clog << usage;
        exit(1);
      }
      break;

    case 'h':
      cout << usage;
      exit(0);

    case 's':
      maxRequestDurationSecs = atoi(optarg);
      if (maxRequestDurationSecs <= 0) {
	cerr << "Error: maxRequestDurationSecs must be greater than zero" << endl << usage;
	exit(1);
      }
      break;
	
    default:
      clog << usage;
      exit(1);
    }
  }

  argc -= optind;
  argv += optind;
  if (argc == 0) {
    clog << "Error: Must supply at least one host on the command line" << endl;
    clog << usage;
    exit(1);
  }

  st_init();

  //////////////////////////////////////////////////////////////////////
  // Build the list of hosts to establish connections with
  //////////////////////////////////////////////////////////////////////

  int resolveCount = 0;
  while (argc-- > 0) {
    string hostName;
    int port;
    util::splitInterface(*argv, hostName, port, "", plutonGlobal::plutonPort);
    struct hostent* hep = gethostbyname(hostName.c_str());
    if (!hep) {
      herror(hostName.c_str());
      exit(1);
    }

    //////////////////////////////////////////////////////////////////////
    // Iterate down the h_addr_list to get all addresses for this
    // host. If asked via the command-line, establish the connection
    // to each address now.
    //////////////////////////////////////////////////////////////////////

    char** hostArgv = hep->h_addr_list;
    while (*hostArgv) {
      ++resolveCount;
      for (int ix=0; ix < connectsPerHost; ++ix) {
	remoteHost* hp = new remoteHost(string(*argv), port, *hostArgv);
	freeHosts.push_front(hp);
	if (connectImmediatelyFlag) hp->connect();
      }
      hostArgv++;
    }

    ++argv;
  }

  if (freeHosts.empty()) {
    clog << "Error: Unable to resolve any host names via gethostbyname()" << endl;
    exit(1);
  }

  clog << "Unique resolved host addresses: " << resolveCount << endl;
 
  freeHostsCvar = st_cond_new();
  if (!freeHostsCvar) {
    clog << "Error: Unable to create st_cond_t" << endl;
    exit(1);
  }

  //////////////////////////////////////////////////////////////////////
  // Create there base serviceImpl used by all threads.
  //////////////////////////////////////////////////////////////////////

  pluton::perCallerService owner("plTranmitter", 0);
  pluton::serviceImpl S;

  if (!S.initialize(&owner, true)) {
    clog << myName << owner.getFault().getMessage() <<endl;
    exit(1);
  }
  S.setPollProxy(st_poll);

  //////////////////////////////////////////////////////////////////////
  // The number of threads (and thus concurrenct accepts) allowed is
  // configure for each service and made available via the shm config.
  //////////////////////////////////////////////////////////////////////

  int maxAccepts = S.getMaximumThreads();	// From shm Config
  activeServices = maxAccepts;

  // The "new" of handlerData is totally lossy, but it's a one-off
  // transfer of parameters to each thread, so no harm.

  for (int ix=1; ix < maxAccepts; ++ix) {
    if (!st_thread_create(handler, (void*) new handlerData(&S, ix), 0, 0)) {
      clog << "Error: " << myName << " could not create a state thread" << endl;
      exit(1);
    }
  }

  clog << "Accept threads enabled: " << maxAccepts << endl;

  handler((void*) new handlerData(&S, 0));	// The main thread is zero

  return(0);
}


//////////////////////////////////////////////////////////////////////
// Get the next free host to use. If none are available, wait for one.
//////////////////////////////////////////////////////////////////////

static remoteHost*
getHostFromFreePool()
{
  while (freeHosts.empty()) {
    st_cond_wait(freeHostsCvar);
  }

  remoteHost* H = freeHosts.front();
  freeHosts.pop_front();

  return H;
}

static void
returnHostToFreePool(remoteHost* H, bool hostFailedFlag)
{
  if (hostFailedFlag) {
    freeHosts.push_back(H);
  }
  else {
    freeHosts.push_front(H);
  }
}


//////////////////////////////////////////////////////////////////////
// Each thread runs the handler
//////////////////////////////////////////////////////////////////////

static void*
handler(void* voidH)
{
  handlerData* hd = (handlerData*) voidH;

  if (debugFlag) clog << myName << " " << hd->threadIndex
		      << " accept thread started " << st_thread_self() << endl;

  time_t endTime = 0;
  if (serviceLifeSecs > 0) endTime = st_time() + serviceLifeSecs;

  //////////////////////////////////////////////////////////////////////
  // Use a per-thread instance of pluton::service, so that unique
  // service state is maintained per thread.
  //////////////////////////////////////////////////////////////////////

  pluton::perCallerService owner("plTransmitter", hd->threadIndex);
  owner.initialize();

  //////////////////////////////////////////////////////////////////////
  // For each request, establish (or re-use) a connection to a remote
  // host and exchange the request with the remote end.
  //////////////////////////////////////////////////////////////////////

  pluton::serviceRequestImpl localR;

  while (true) {
    int remaining = 0;
    if (endTime > 0) {				// Have we got to shutdown eventually?
      remaining = endTime - st_time();		// Yes - calculate time remaining and
      if (remaining <= 0) break;		// see if we're outta here.
    }

    if (debugFlag) clog << hd->threadIndex << " on getRequest()" << endl;
    if (!hd->S->getRequest(&owner, &localR, remaining, maxRequestDurationSecs)) break;
    if (localR.getAttribute(pluton::noRemoteAttr)) {
      localR.setFault(pluton::remoteNotAllowed);
      hd->S->sendResponse(&owner, &localR, maxRequestDurationSecs);
      continue;
    }
    if (debugFlag) clog << hd->threadIndex << " have Request" << endl;
    remoteHost* H = getHostFromFreePool();
    if (!H->connect()) {
      string em = "Cannot connect to ";
      em += H->getName();
      localR.setFault(pluton::remoteConnectFailed, em.c_str());
      hd->S->sendResponse(&owner, &localR, maxRequestDurationSecs);
      returnHostToFreePool(H, true);
      continue;
    }

    ////////////////////////////////////////////////////////////
    // All we do is transfer the raw bits straight through!
    ////////////////////////////////////////////////////////////

    const char* rawPtr;
    int rawRequestLength;
    localR.getInboundPacketData(rawPtr, rawRequestLength);
    //    if (debugFlag) clog << "Relay Out: L=" << rawRequestLength << " "
    //			<< string(rawPtr, rawRequestLength) << endl;

    if (H->sendRequest(rawPtr, rawRequestLength, localR.getTimeoutuS()) != rawRequestLength) {
      string em = "Write failed to ";
      em += H->getName();
      localR.setFault(pluton::remoteTransferFailed, em.c_str());
      hd->S->sendResponse(&owner, &localR, maxRequestDurationSecs);
      H->disconnect();
      returnHostToFreePool(H, true);
      continue;
    }

    //////////////////////////////////////////////////////////////////////
    // The tricky bit. Transfer the file descriptor, if present.
    //////////////////////////////////////////////////////////////////////

    if (localR.hasFileDescriptor()) H->transferFileDescriptor(localR.getFileDescriptor());

    //////////////////////////////////////////////////////////////////////
    // Oh... that wasn't so tricky after all. Goodo.
    //////////////////////////////////////////////////////////////////////

    if (H->readPacket(localR.getTimeoutuS()) == -1) {
      if (debugFlag) clog << "readPacket() failed" << endl;
      string em = "Read failed from ";
      em += H->getName();
      localR.setFault(pluton::remoteTransferFailed, em.c_str());
      hd->S->sendResponse(&owner, &localR, maxRequestDurationSecs);
      H->disconnect();
      returnHostToFreePool(H, true);
      continue;
    }

    //////////////////////////////////////////////////////////////////////
    // Transfer results from remoteR to localR
    //////////////////////////////////////////////////////////////////////

    int rawResponseLength;
    H->getInboundPacketData(rawPtr, rawResponseLength);
    //    if (debugFlag) clog << "Relay In: L=" << rawResponseLength << " "
    //			<< string(rawPtr, rawResponseLength) << endl;
    if (!hd->S->sendRawResponse(&owner, rawRequestLength, rawPtr, rawResponseLength,
				myName, maxRequestDurationSecs)) {
      clog << "Warning: pluton::serviceImpl::sendRawResponse() failed: "
	   << owner.getFault().getMessage() << endl;
    }
    returnHostToFreePool(H, false);
  }

  hd->S->terminate();

  return 0;
}


//////////////////////////////////////////////////////////////////////
// Establish a network connection to the remote host
//////////////////////////////////////////////////////////////////////

bool
remoteHost::connect(int timeoutuS)
{
  if (_fd) return true;

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("socket()");
    return false;
  }

  _fd = st_netfd_open_socket(sock);
  if (!_fd) {
    perror("st_netfd_open_socket()");
    close(sock);
    return false;
  }

  struct sockaddr_in saddr;
  bzero((void*) &saddr, sizeof(saddr));
  saddr.sin_family = AF_INET;
  memcpy((void*) &saddr.sin_addr, (void*) &_addr, sizeof(saddr.sin_addr));
  saddr.sin_port = htons(_port);
  if (st_connect(_fd, (sockaddr*) &saddr, sizeof(saddr), timeoutuS) == -1) {
    perror("st_connect()");
    st_netfd_close(_fd);
    return false;
  }

  return true;
}


void
remoteHost::disconnect()
{
  if (_fd) st_netfd_close(_fd);
  _fd = 0;
}


int
remoteHost::sendRequest(const char* cp, int len, unsigned int timeoutuS) const
{
  return st_write(_fd, cp, len, timeoutuS);
}


//////////////////////////////////////////////////////////////////////
// Reading netStrings from the remote connection until we have a
// complete packet or error.
//
// Return: -1 = error otherwise _packetPtr and _packetLen.
//////////////////////////////////////////////////////////////////////

int
remoteHost::readPacket(unsigned long timoutuS)
{
  _packetIn.reclaimByCompaction();
  _decoder.reset();
  _packetOffset = _packetIn.getRawOffset();	// Need to determine packet location

  while (true) {
    char* bufPtr;
    int minRequired, maxAllowed;
    if (_packetIn.getReadParameters(bufPtr, minRequired, maxAllowed) == -1) {
      errno = E2BIG;
      return -1;
    }

    int bytes = st_read(_fd, bufPtr, maxAllowed, timoutuS);
    //    if (debugFlag) clog << "st_read=" << bytes << " errno=" << errno
    //			<< " RTO=" << timoutuS << endl;
    if (bytes <= 0) {
      errno = EIO;
      return -1;
    }

    //////////////////////////////////////////////////////////////////////
    // Got some data, decode it and look for a completed response.
    //////////////////////////////////////////////////////////////////////

    _packetIn.addBytesRead(bytes);
    const char* parseError = 0;
    string em;
    while (_packetIn.haveNetString(parseError)) {
      char nsType;
      const char* nsDataPtr;
      int nsDataLength;
      int nsOffset;
      _packetIn.getNetString(nsType, nsDataPtr, nsDataLength, &nsOffset);
      if (!_decoder.addType(nsType, nsDataPtr, nsDataLength, nsOffset, em)) {
	return -1;
      }

      //////////////////////////////////////////////////////////////////////
      // If this is a complete request, set the packet pointer details
      // and return success.
      //////////////////////////////////////////////////////////////////////

      if (_decoder.haveCompleteRequest()) {
	_packetPtr = _packetIn.getBasePtr() + _packetOffset;
	_packetLen = _packetIn.getRawOffset() -  _packetOffset;
	return 0;
      }
    }
    if (parseError) return -1;
  }
}
