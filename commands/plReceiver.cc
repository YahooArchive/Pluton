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

#include <string>

#include <string>
#include <iostream>

#include <stdio.h>


using namespace std;

#include <st.h>

#include "global.h"
#include "util.h"
#include "netString.h"
#include "listenInterface.h"
#include "decodePacket.h"
#include "requestImpl.h"

#include "pluton/client.h"
#include "clientImpl.h"
#include "serviceImpl.h"

static const char* myName = "plReceiver";
static int maxConcurrency = 0;
static int concurrency = 0;


static const char* usage =
"\n"
"Usage: plReceiver [-dh] [-a maxAccepts] [-k ServiceKey]\n"
"                  [-l lookupMapPath] [-s maxRequestDurationSecs]\n"
"                  [[Interface][:port]]\n"
"\n"
"Accept and submit Pluton Requests sent by remote plTransmitters\n"
"\n"
"Where:\n"
"\n"
" -a   Maximum number of concurrent accepted requests (default: 10)\n"
" -d   Turn on debugging output\n"
" -h   Print this usage message on STDOUT and exit(0)\n"
" -k   Replace requested ServiceKey with this value. This is useful\n"
"       when running plReceiver on the same system as plTransmitter.\n"
" -l   The Lookup Map used to connect to the local service (default: '')\n"
" -s   Maximum seconds allowed per request (default: 30)\n"
"\n"
" Interface and Port define the listening address (default: localhost:14099)\n"
"\n"
"See also: " PACKAGE_URL "\n"
"\n";

//////////////////////////////////////////////////////////////////////
// These statics are used to communicate command-line parameters to
// the threads.
//////////////////////////////////////////////////////////////////////

static void*	handler(void*);

static  int	maxAccepts = 10;
static  bool	debugFlag = false;
static  char*	lookupMapPath = "";
static	bool	shutdownFlag = false;
static	char*	serviceKey = 0;
static	int	maxRequestDurationSecs = 30;

//////////////////////////////////////////////////////////////////////

int
main(int argc, char** argv)
{
  char optionChar;

  while ((optionChar = getopt(argc, argv, "a:dhk:l:s:")) != EOF) {
    switch (optionChar) {
    case 'a':
      maxAccepts = atoi(optarg);
      if (maxAccepts <= 0) {
	cerr << "Error: maxAccepts must be greater than zero" << endl << usage;
	exit(1);
      }
      break;

    case 'd':
      debugFlag = true;
      break;

    case 'h':
      cout << usage;
      exit(0);

    case 'k':
      serviceKey = optarg;
      break;

    case 'l':
      lookupMapPath = optarg;
      break;

    case 's':
      maxRequestDurationSecs = atoi(optarg);
      if (maxRequestDurationSecs <= 0) {
	cerr << "Error: maxRequestDurationSecs must be greater than zero" << endl << usage;
	exit(1);
      }
      break;
	
    default:
      cerr << usage;
      exit(1);
    }
  }

  argc -= optind;
  argv += optind;
  const char* interface = "localhost:14099";

  if (argc > 1) {
    cerr << "Error: Only one interface parameter is allowed on the command line" << endl;
    cerr << usage;
    exit(1);
  }

  if (argc == 1) {
    interface = argv[0];
    --argc; ++argv;
  }

  listenInterface li;
  const char *err = li.openAndListen(interface, maxAccepts * 2 + 20);
  if (err) {
    cerr << "Error: Could not listen on " << interface << ": " << err << endl;
    exit(1);
  }
 
  st_netfd_t acceptFDs = st_netfd_open_socket(li.transferFD());
  if (!acceptFDs) {
    perror("Error: Could not st_netfdopen_socket");
    exit(1);
  }

  pluton::client C(myName);
  C.initialize(lookupMapPath);
  //  C.setDebug(debugFlag);
  st_init();
  C.setPollProxy(st_poll);

  //////////////////////////////////////////////////////////////////////
  // The thread structure is to have a single thread per allowable
  // accept instance.
  //////////////////////////////////////////////////////////////////////

  for (int ix=1; ix < maxAccepts; ++ix) {
    if (!st_thread_create(handler, (void*) acceptFDs, 0, 0)) {
      clog << "Error: " << myName << " could not create a state thread" << endl;
      exit(1);
    }
  }

  clog << "Accept threads enabled: " << maxAccepts << endl;

  handler((void*) acceptFDs);	// I am thread zero

  st_netfd_close(acceptFDs);

  return(0);
}


//////////////////////////////////////////////////////////////////////
// Loop on reading data until we have a complete packet or error.
// Return: -1 = error
//////////////////////////////////////////////////////////////////////

int
readPacket(st_netfd_t fd, netStringFactoryManaged& packetIn, pluton::decodePacket& decoder,
	   int maxDuration, string& em)
{
  time_t endTime = st_time() + maxDuration;	// Make sure we don't wedge forever

  while (true) {
    char* bufPtr;
    int minRequired, maxAllowed;
    if (packetIn.getReadParameters(bufPtr, minRequired, maxAllowed) == -1) {
      em = "packet bigger than maximum buffer size";
      return -1;
    }

    if (st_time() > endTime) {
      em = "Request read time exceeded maxRequestDurationSecs";
      return -1;
    }

    int bytes = st_read(fd, bufPtr, maxAllowed, maxRequestDurationSecs * util::MICROSECOND);
#ifdef NO
    if (debugFlag) cout << "st_read() = " << bytes << " errno=" << errno
			<< " for " << maxAllowed << endl;
#endif
    if (bytes == 0) return 0;
    if (bytes < 0) {
      if (errno == EAGAIN) continue;
      em = "Socket read failed or timed out";
      return -1;
    }

    //////////////////////////////////////////////////////////////////////
    // Got some data, decode it and look for a completed response.
    //////////////////////////////////////////////////////////////////////

    packetIn.addBytesRead(bytes);
    const char* parseError = 0;
    while (packetIn.haveNetString(parseError)) {
      char nsType;
      const char* nsDataPtr;
      int nsDataLength;
      int nsOffset;
      packetIn.getNetString(nsType, nsDataPtr, nsDataLength, &nsOffset);
      if (!decoder.addType(nsType, nsDataPtr, nsDataLength, nsOffset, em)) return -1;
      if (decoder.haveCompleteRequest()) return 1;	// Have one packet
    }
    if (parseError) {
      em = parseError;
      return -1;
    }
  }
}


//////////////////////////////////////////////////////////////////////
// Each accept instance is managed by a separate thread.
//////////////////////////////////////////////////////////////////////

static void*
handler(void *voidFD)
{
  if (debugFlag) clog << myName << " accept thread started " << st_thread_self() << endl;

  st_netfd_t acceptFDs = (st_netfd_t) voidFD;


  //////////////////////////////////////////////////////////////////////
  // Use a per-thread instance of the pluton::client so that it can
  // maintain a separate timeout relationship between the thread and
  // the request.
  //////////////////////////////////////////////////////////////////////

  pluton::client C;
  C.initialize("");

  netStringFactoryManaged packetIn;
  pluton::decodeRequestPacket decoder;
  pluton::clientRequest R;

  packetIn.disableCompaction();	// Stop internal data from moving

  //////////////////////////////////////////////////////////////////////
  // Take responsibility for a single inbound connection and iterate
  // on requests coming in on that connection. If the connection dies,
  // re-accept.
  //////////////////////////////////////////////////////////////////////

  while (!shutdownFlag) {
    st_netfd_t fd = st_accept(acceptFDs, 0, 0, (unsigned) -2);
    if (!fd) {
      if (errno == ETIME) continue;
      perror("st_accept()");
      break;
    }

    ++concurrency;
    if (concurrency > maxConcurrency) maxConcurrency = concurrency;

    if (debugFlag) clog << myName << " accepted by " << st_thread_self()
			<< " MaxC=" << maxConcurrency << endl;

    while (true) {
      packetIn.reclaimByCompaction();
      decoder.reset();
      decoder.setRequest(R.getImpl());
      R.getImpl()->setPacketOffset(packetIn.getRawOffset());

      string em;
      int packetCount = readPacket(fd, packetIn, decoder, maxRequestDurationSecs, em);
      if (packetCount == 0) break;		// Eof
      if (packetCount < 0) {
	if (debugFlag) clog << "ReadPacket error: " << em << endl;
	break;
      }
      R.getImpl()->adjustOffsets(packetIn.getBasePtr(), packetIn.getRawOffset());

#ifdef NO
      if (debugFlag) clog << myName << " have request "
			  << " Rto=" << R.getImpl()->getTimeoutMS()
			  << endl;
#endif

      //////////////////////////////////////////////////////////////////////
      // Have request from remote end, issue it locally and...
      //////////////////////////////////////////////////////////////////////

      const char* rawPtr;
      int rawLength;
      R.getImpl()->getInboundPacketData(rawPtr, rawLength);
      if (!C.addRawRequest(serviceKey ? serviceKey : R.getImpl()->getServiceKey().c_str(), &R,
			   rawPtr, rawLength)) {
	if (debugFlag) clog << "addRequest failed " << R.getFaultText() << endl;
	continue;
      }
      if (C.executeAndWaitOne(R) == -1) {
	if (debugFlag) clog << "executeAndWait failed" << endl;
	break;
      }

      //////////////////////////////////////////////////////////////////////
      // ... relay the response back to the original sender.
      //////////////////////////////////////////////////////////////////////

      R.getImpl()->getInboundPacketData(rawPtr, rawLength);
#ifdef NO
      if (debugFlag) clog << myName << "Request completed: Len="
			  << rawLength
			  << string(rawPtr, rawLength) << endl;
#endif
      int wBytes = st_write(fd, rawPtr, rawLength, R.getImpl()->getTimeoutuS());
      if (wBytes != rawLength) {
	if (debugFlag) perror("st_write");
	break;
      }
    }
    --concurrency;
    st_netfd_close(fd);
  }

  return 0;
}
