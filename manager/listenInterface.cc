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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <netinet/in.h>		// FBSD4 needs this for getaddrinfo()

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "util.h"
#include "listenInterface.h"


listenInterface::listenInterface(const char* defaultInterface, int defaultPort)
  : _interface(defaultInterface), _port(defaultPort), _addrInfo(0), _fd(-1)
{
}


//////////////////////////////////////////////////////////////////////

listenInterface::~listenInterface()
{
  if (_addrInfo) freeaddrinfo(_addrInfo);
  if (_fd != -1) close(_fd);
}


//////////////////////////////////////////////////////////////////////
// Given an interface address and port, resolve to an IP address.
//////////////////////////////////////////////////////////////////////

const char*
listenInterface::resolve(const char* interface, int port)
{
  struct addrinfo hints;

  bzero((void*) &hints, sizeof(hints));
  hints.ai_flags = AI_PASSIVE;
  hints.ai_flags |= AI_CANONNAME;
  hints.ai_family = PF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  //////////////////////////////////////////////////////////////////////
  // getaddrinfo dynamically allocates saip for us on success. Our
  // destructor free this resource.
  //////////////////////////////////////////////////////////////////////

  _addrInfo = 0;
  int gae = getaddrinfo(interface, 0, &hints, &_addrInfo);
  if (gae) return gai_strerror(gae);

  // Check that there is a result and that it has an address.

  if (!_addrInfo) return "No addrinfo associated with interface";

  if (!_addrInfo->ai_addr) return "No ai_addr associated with interface";

  // Force our designated port onto the getaddrinfo results

  ((struct sockaddr_in *)_addrInfo->ai_addr)->sin_port = htons((unsigned short) port);

  return 0;
}


//////////////////////////////////////////////////////////////////////
// Open a listening socket
//////////////////////////////////////////////////////////////////////

const char*
listenInterface::openAndListen(const char* interfaceAndPort, int queueDepth)
{ 
  std::string newInterface;
  int newPort;

  util::splitInterface(interfaceAndPort, newInterface, newPort, _interface.c_str(), _port);

  const char* err = resolve(newInterface.c_str(), newPort);
  if (err) return err;

  _fd = socket(_addrInfo->ai_family, _addrInfo->ai_socktype, _addrInfo->ai_protocol);
  if (_fd == -1) return strerror(errno);

  ////////////////////////////////////////////////////////////
  // A server can reuse an address so we don't have to wait 2 minutes
  // for all the TTLs to expire. But don't worry if it fails as the
  // bind will tell us whether it really failed or not.
  ////////////////////////////////////////////////////////////

  int trueFlag = 1;
  setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, (void *) &trueFlag, sizeof(trueFlag));

  if (bind(_fd, _addrInfo->ai_addr, _addrInfo->ai_addrlen) == -1) return strerror(errno);
  if (listen(_fd, queueDepth) == -1) return strerror(errno);

  return 0;
}


//////////////////////////////////////////////////////////////////////
// The caller take responsibility for the listening on this fd
//////////////////////////////////////////////////////////////////////

int
listenInterface::transferFD()
{
  int returnFD = _fd;
  _fd = -1;		// We can no longer close it

  return returnFD;
}
