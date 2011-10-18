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
#include <iomanip>
#include <string>

#include <sys/time.h>

#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "util.h"
#include "pluton/client.h"

using namespace std;


static const char* usage =
"Usage: plPing [-dhKkNnow] [-c count] [-C contextValue] [-s packetsize]\n"
"                          [-L lookupMap] [-p parallelCount] [-t timeout] [ServiceKey]\n"
"\n"
"Measure the response time of a pluton service\n"
"\n"
"Where:\n"
" -c   Stop after sending 'count' requests (default: infinite)\n"
" -C   Set context with this value (the normal echo service uses this\n"
"       as a sleep value)\n"
" -d   Turn on debug\n"
" -h   Print this usage message on STDOUT and exit(0)\n"
" -i   Wait 'wait' seconds between requests (default: 1)\n"
"       If set to zero, this is effectively a ping flood.\n"
" -K   Keep Affinity. Set the 'keepAffinityAttr' attribute\n"
" -k   Send echo a random die request\n"
" -L   The Lookup Map used to connect with the manager (default: '')\n"
" -N   Need Affinity. Set the 'needAffinityAttr' attribute\n"
" -n   Do *not* check the response for a valid echo\n"
" -o   Oneshot. Set the 'noRetryAttr' attribute\n"
" -p   Number of requests to send in parallel (default: 1)\n"
" -s   Size of random data to generate in the request (default: 0)\n"
" -t   Numbers of seconds to wait for a response (default: 5)\n"
" -w   NoWait. Set the 'noWaitAttr' attribute\n"
"\n"
" ServiceKey: name of service to request (default: system.echo.0.raw)\n"
"\n"
"See also: " PACKAGE_URL "\n"
"\n";

//////////////////////////////////////////////////////////////////////

static bool	terminateFlag = false;

static long	totalReadBytes = 0;
static long	totalWriteBytes = 0;
static int	totalPingCount = 0;
static timeval	minPingTime;
static timeval	maxPingTime;
static timeval totalPingTime;

static void
catchINT(int sig)
{
  terminateFlag = true;
  signal(SIGINT, SIG_DFL);	// Let a second kill, really kill me
}

static	void
trackResults(int ix, timeval& st, timeval& et, int rb, int wb, bool printFlag)
{
  timeval diff;
  util::timevalDiff(diff, et, st);

  ++totalPingCount;

  if (printFlag) {
    cout << rb << " bytes: seq=" << totalPingCount << "/" << ix
	 << " time=" << diff.tv_sec << "." << setfill('0') << setw(6) << diff.tv_usec;
    cout << endl;
  }

  if (totalPingCount == 1) {
    totalPingTime = diff;
    minPingTime = diff;
    maxPingTime = diff;
  }
  else {
    if ((diff.tv_sec < minPingTime.tv_sec)
	|| ((diff.tv_sec == minPingTime.tv_sec) && (diff.tv_usec < minPingTime.tv_usec))) {
      minPingTime = diff;
    }

    if ((diff.tv_sec > maxPingTime.tv_sec)
	|| ((diff.tv_sec == maxPingTime.tv_sec) && (diff.tv_usec > maxPingTime.tv_usec))) {
      maxPingTime = diff;
    }
    totalPingTime.tv_sec += diff.tv_sec;
    totalPingTime.tv_usec += diff.tv_usec;
    if (totalPingTime.tv_usec >= util::MICROSECOND) {
      totalPingTime.tv_usec -= util::MICROSECOND;
      totalPingTime.tv_sec += 1;
    }
  }

  totalReadBytes += rb;
  totalWriteBytes += wb;
}


//////////////////////////////////////////////////////////////////////

static void
printResults()
{
  if (terminateFlag) cout << endl;	// NL past the ^C to look pretty
  cout << "plPings: " << totalPingCount;
  if (totalPingCount == 0) {
    cout << endl;
    return;
  }

  string work;
  cout << " Read=" << util::intToEnglish(totalReadBytes, work);
  cout << " Write=" << util::intToEnglish(totalWriteBytes, work);

  cout << " Min: " << minPingTime.tv_sec << "." << setfill('0') << setw(6) << minPingTime.tv_usec;
  cout << " Max: " << maxPingTime.tv_sec << "." << setfill('0') << setw(6) << maxPingTime.tv_usec;

  if (totalPingCount > 1) {		// Don't do average unless we have enough
    long secs = totalPingTime.tv_sec;
    secs *= util::MICROSECOND;
    secs /= totalPingCount;
    long msecs = totalPingTime.tv_usec;
    msecs /= totalPingCount;
    msecs += secs % util::MICROSECOND;
    secs /= util::MICROSECOND;

    cout << " Avg: " << secs << "." << setfill('0') << setw(6) << msecs;
  }

  cout << " Elapse: " << totalPingTime.tv_sec << "." << setfill('0') << setw(3) << totalPingTime.tv_usec;


  cout << endl;
}

//////////////////////////////////////////////////////////////////////
// Fill the buffer with variable data so that corruptions are
// detected, but make it printable so that debugging is not a
// line-noise nightmare. Assume the startChar is isprint()
//////////////////////////////////////////////////////////////////////

static void
fillRequestData(char *cp, char startChar, int bytes)
{
  while (bytes-- > 0) {
    *cp++ = startChar++;
    if (!isprint(startChar)) startChar = '0';
  }
}

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////

class myRequest : public pluton::clientRequest {
public:
  int	index;
};

static void
checkOneResponse(bool checkEcho, myRequest* retR, struct timeval& startTime, const string& payload,
		 int waitSeconds)
{
  const char* resp;
  int respLen;
  struct timeval endTime;
  gettimeofday(&endTime, 0);

  if (retR->getResponseData(resp, respLen) != pluton::noFault) {
    cerr << "Fault: " << retR->getFaultCode() << " " << retR->getFaultText() << endl;
    exit(1);
  }

  if ((unsigned) respLen != payload.length()) {
    cerr << "Error: Payload length changed from "
	 << payload.length() << " to " << respLen << endl;
    exit(2);
  }

  if (checkEcho && (strncmp(payload.data(), resp, respLen) != 0)) {
    cerr << "Error: Payload corrupted" << endl;
    exit(3);
  }

  trackResults(retR->index, startTime, endTime, payload.length(), respLen, (waitSeconds > 0));
}


int
main(int argc, char **argv)
{
  char	optionChar;

  bool	keepAffinity = false;
  bool	needAffinity = false;

  int	repeatCount = -1;
  int	originalRepeatCount = -1;
  int	parallelCount = 1;
  bool	debugFlag = false;
  int	waitSeconds = 1;
  int	payloadSize = 0;
  int	timeoutSeconds = 5;
  const char* context = 0;
  const char* lookupMap = "";
  bool	oneShot = false;
  bool  sendDie = false;
  bool	noWait = false;
  bool	checkEcho = true;

  while ((optionChar = getopt(argc, argv, "C:c:dhi:KkL:Nnop:s:t:w")) != -1) {
    switch (optionChar) {

    case 'C': context = optarg; break;

    case 'c':
      repeatCount = atoi(optarg);
      if (repeatCount <= 0) {
	cerr << "Error: Repeat Count must be greater than zero" << endl;
	cerr << usage;
	exit(4);
      }
      originalRepeatCount = repeatCount;
      break;

    case 'd': debugFlag = true; break;

    case 'K': keepAffinity = true; break;

    case 'k': sendDie = true; break;

    case 'i':
      waitSeconds = atoi(optarg);
      if (waitSeconds < 0) {
	cerr << "Error: Wait seconds must not be less than zero" << endl;
	cerr << usage;
	exit(5);
      }
      break;

    case 'L': lookupMap = optarg; break;

    case 'N': needAffinity = true; break;

    case 'n': checkEcho = false; break;

    case 'o': oneShot = true; break;

    case 'p':
      parallelCount = atoi(optarg);
      if (parallelCount <= 0) {
	cerr << "Error: Parallel Count must be greater than zero" << endl;
	cerr << usage;
	exit(6);
      }
      break;

    case 's':
      payloadSize = atoi(optarg);
      if (payloadSize < 0) {
	cerr << "Error: Payload Size must not be less than zero" << endl;
	cerr << usage;
	exit(7);
      }
      break;

    case 't':
      timeoutSeconds = atoi(optarg);
      if (timeoutSeconds <= 0) {
	cerr << "Error: Timeout must be greater than zero" << endl;
	cerr << usage;
	exit(8);
      }
      break;

    case 'w': noWait = true; break;

    case 'h':
      cerr << usage;
      exit(0);
    default:
      cerr << usage;
      exit(9);
    }
  }

  argc -= optind;
  argv += optind;

  const char* serviceKey = "system.echo.0.raw";

  if (argc > 0) {
    serviceKey = argv[0];
    argc--;
    argv++;
  }
    
  if (argc > 0) {
    cerr << "Error: Too many command line parameters: " << *argv << "?" << endl;
    cerr << usage;
    exit(10);
  }

  ////////////////////////////////////////
  // Have all the options - let's do it
  ////////////////////////////////////////

  if (debugFlag) {
    cout << "plPing " << lookupMap << " Repeats=";
    if (repeatCount < 0) {
      cout << "forever";
    }
    else {
      cout << repeatCount;
    }
   cout << " Delay=" << waitSeconds << " Payload Size=" << payloadSize
	<< " Timeout=" << timeoutSeconds << endl;
  }

  pluton::client C("plPing", timeoutSeconds * 1000);
  if (debugFlag) C.setDebug(true);
  if (!C.initialize(lookupMap)) {
    cerr << C.getFault().getMessage("plPing could not initialize") << endl;
    exit(11);
  }
  struct timeval startTime;

  // Let the user break out of an otherwise infinite number of pings

  signal(SIGINT, catchINT);
  siginterrupt(SIGINT, true);

  char* pl = 0;
  if (payloadSize > 0) pl = new char [payloadSize];

  // Everything is ready - start Sending

  myRequest* R = new myRequest[parallelCount];

  //  pluton::clientRequest* R = new pluton::clientRequest[parallelCount];

  while ((repeatCount < 0) || (repeatCount > 0)) {
    if (repeatCount > 0) --repeatCount;

  // Construct the request

    string payload;
    if (payloadSize > 0) {
      char randomChar = (time(0) % 26) + 'A';
      fillRequestData(pl, randomChar, payloadSize);
      payload.append(pl, payloadSize);
    }
    const char* payloadPtr = payload.data();
    int payloadLen = payload.length();

    for (int ix=0; ix < parallelCount; ++ix) {
      if (! keepAffinity) R[ix].reset();
      R[ix].index = ix;
      R[ix].setRequestData(payloadPtr, payloadLen);
      if (oneShot) R[ix].setAttribute(pluton::noRetryAttr);
      if (noWait) R[ix].setAttribute(pluton::noWaitAttr);
      if (keepAffinity) R[ix].setAttribute(pluton::keepAffinityAttr);
      if (needAffinity && totalPingCount) R[ix].setAttribute(pluton::needAffinityAttr);

      if (context)  R[ix].setContext("echo.sleepMS", context);
      if (sendDie)  R[ix].setContext("echo.die", "");

      if (!C.addRequest(serviceKey, R[ix])) {
	cerr << C.getFault().getMessage() << endl;
	exit(12);
      }
    }

    gettimeofday(&startTime, 0);

    myRequest* retR;
    retR = &R[0];
    int res = C.executeAndWaitOne(retR);
    if (res == 0) {
      cerr << "Request timed out" << endl;
      exit(16);
    }
    if (res == 1) {
      checkOneResponse(checkEcho, retR, startTime, payload, waitSeconds);
    }
    else {
      cerr << "Request Failed: Code=" << C.getFault().getFaultCode()
	   << " Message=" << C.getFault().getMessage("executeAndWaitOne") << endl;
      exit(13);
    }

    while ((retR = dynamic_cast<myRequest*>(C.executeAndWaitAny()))) {
      checkOneResponse(checkEcho, retR, startTime, payload, waitSeconds);
    }
    if (terminateFlag) break;
    if (C.hasFault()) {
      cerr << C.getFault().getMessage("executeAndWaitAny") << endl;
      exit(14);
    }
    if ((waitSeconds > 0) && (repeatCount != 0)) sleep(waitSeconds);
  }

  printResults();

  // If a specific count was requested, indicate via exit whether all
  // requests were successful.

  if ((originalRepeatCount > 0) && (originalRepeatCount*parallelCount != totalPingCount)) exit(15);

  return(0);
}
