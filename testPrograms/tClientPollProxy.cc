//////////////////////////////////////////////////////////////////////
// This is a badly chopped-up version of plPing to test the pollProxy
// code.
//////////////////////////////////////////////////////////////////////

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

#include <sys/time.h>

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <st.h>

#if defined(__MACH__) && defined(__APPLE__)
#define NO_INCLUDE_POLL		// Coz st.h conflicts with poll.h
#endif

#include "util.h"
#include "pluton/client.h"


using namespace std;


#define	MICROSECS	1000000	// per second

static const char* usage =
"Usage: tClientPollProxy [-dhw] [-c count] [-C context] [-s packetsize] [-L lookupMap]\n"
"                        [-p parallelCount] [-t timeout] [ServiceKey]\n"
"\n"
"Measure the response time of an pluton server using the State Threads interface\n"
"A badly chopped up version of plPing. Do not use this code. It's crud!\n"
"\n"
"Where:\n"
" -c      Stop after sending 'count' requests (default: infinite)\n"
"         as a sleep value)\n"
" -C      Set echo.sleepMS context to this value\n"
" -d      Turn on debug\n"
" -h      Display Usage and exit\n"
" -i      Wait 'wait' milliseconds between requests (default: 1000)\n"
"         If set to zero, this is effectively a ping flood.\n"
" -L      The Lookup Map used to connect with the manager (default: '')\n"
" -p      Number of requests to send in parallel (default: 1)\n"
" -s      Size of added data to supply in the ping packet (default: 0)\n"
" -t      Numbers of seconds to wait for a response (default: 2)\n"
" -w      NoWait. Set the 'noWaitAttr' attribute\n"
"\n"
" ServiceKey is the service sent the request (default: system.echo.0.raw)\n"
"\n";

//////////////////////////////////////////////////////////////////////

static bool	terminateFlag = false;

static long	totalReadBytes = 0;
static long	totalWriteBytes = 0;
static int	totalPingCount = 0;
static timeval	minPingTime;
static timeval	maxPingTime;
static timeval totalPingTime;

static void catchINT(int sig) { terminateFlag = true; }

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
    if (totalPingTime.tv_usec >= MICROSECS) {
      totalPingTime.tv_usec -= MICROSECS;
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
    secs *= MICROSECS;
    secs /= totalPingCount;
    long msecs = totalPingTime.tv_usec;
    msecs /= totalPingCount;
    msecs += secs % MICROSECS;
    secs /= MICROSECS;

    cout << " Avg: " << secs << "." << setfill('0') << setw(6) << msecs;
  }

  cout << " Elapse: " << totalPingTime.tv_sec << "." << setfill('0') << setw(3) << totalPingTime.tv_usec;


  cout << endl;
}

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////

static void
checkOneResponse(int myIndex,
		 pluton::clientRequest* retR, struct timeval& startTime, const string& payload,
		 int waitMSecs)
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
    exit(1);
  }

  if (strncmp(payload.data(), resp, respLen) != 0) {
    cerr << "Error: Payload corrupted" << endl;
    exit(1);
  }

  trackResults(myIndex, startTime, endTime, payload.length(), respLen, (waitMSecs > 0));
}


class config {
public:
  config() :
    repeatCount(-1),
    parallelCount(1),
    debugFlag(false),
    waitMSecs(1000),
    payloadSize(0),
    timeoutSeconds(5),
    lookupMap(""),
    serviceKey("system.echo.0.raw"),
    context(0),
    noWait(false)
  {}


  int	repeatCount;
  int	parallelCount;
  bool	debugFlag;
  int	waitMSecs;
  int	payloadSize;
  int	timeoutSeconds;
  const char* lookupMap;
  const char* serviceKey;
  const char* context;
  bool	noWait;
  pluton::client*	pc;
};


static int globalIndex = 0;
static int globalThreadCount = 0;

static void*
pingThread(void* C)
{
  const config* CFG = (config *) C;
  int myIndex = globalIndex++;

  ostringstream os;
  os << "thread-" << myIndex;

  pluton::clientRequest R;
  int repeatCount = CFG->repeatCount;
  pluton::client pc(os.str().c_str());
  int randMS = ((long) st_thread_self() & 0x1FF);

  // Construct the request

  string payload;
  char* pl = new char [CFG->payloadSize+1];

  while ((repeatCount < 0) || (repeatCount > 0)) {
    if (repeatCount > 0) --repeatCount;

    if (CFG->payloadSize > 0) {
      char randomChar = (time(0) % 26) + 'A';
      memset(pl, randomChar, CFG->payloadSize);
      payload.assign(pl, CFG->payloadSize);
    }
    const char* payloadPtr = payload.data();
    int payloadLen = payload.length();
    if (CFG->debugFlag) cout << "Payload: " << payload << endl;

    R.reset();
    R.setRequestData(payloadPtr, payloadLen);
    if (CFG->noWait)  R.setAttribute(pluton::noWaitAttr);
    if (CFG->context)  R.setContext("echo.sleepMS", CFG->context);

    struct timeval startTime;
    gettimeofday(&startTime, 0);

    if (!pc.addRequest(CFG->serviceKey, R)) {
      clog << "Fault: " << myIndex << " " << pc.getFault().getMessage("addRequest", true) << endl;
      break;
    }

    if (pc.executeAndWaitOne(R) <= 0) {
      clog << "Fault: " << myIndex << " "
	   << pc.getFault().getMessage("executeAndWaitOne", true) << endl;
      break;
    }

    checkOneResponse(myIndex, &R, startTime, payload, CFG->waitMSecs);
    if (terminateFlag) break;
    if ((CFG->waitMSecs > 0) && (CFG->repeatCount != 0)) {
      st_usleep(util::MICROMILLI * (CFG->waitMSecs + randMS));
    }
  }

  globalThreadCount--;

  return 0;
}

int
main(int argc, char **argv)
{
  char	optionChar;

  config	CFG;

  while ((optionChar = getopt(argc, argv, "C:c:dhi:p:s:L:t:w")) != -1) {
    switch (optionChar) {
    case 'C':
      CFG.context = optarg; break;

    case 'c':
      CFG.repeatCount = atoi(optarg);
      if (CFG.repeatCount <= 0) {
	cerr << "Error: Repeat Count must be greater than zero" << endl;
	cerr << usage;
	exit(1);
      }
      break;

    case 'd': CFG.debugFlag = true; break;

    case 'i':
      CFG.waitMSecs = atoi(optarg);
      if (CFG.waitMSecs < 0) {
	cerr << "Error: Wait milliseconds must not be less than zero" << endl;
	cerr << usage;
	exit(1);
      }
      break;

    case 'p':
      CFG.parallelCount = atoi(optarg);
      if (CFG.parallelCount <= 0) {
	cerr << "Error: Parallel Count must be greater than zero" << endl;
	cerr << usage;
	exit(1);
      }
      break;

    case 's':
      CFG.payloadSize = atoi(optarg);
      if (CFG.payloadSize < 0) {
	cerr << "Error: Payload Size must not be less than zero" << endl;
	cerr << usage;
	exit(1);
      }
      break;

    case 'L': CFG.lookupMap = optarg; break;

    case 't':
      CFG.timeoutSeconds = atoi(optarg);
      if (CFG.timeoutSeconds <= 0) {
	cerr << "Error: Timeout must be greater than zero" << endl;
	cerr << usage;
	exit(1);
      }
      break;

    case 'w': CFG.noWait = true; break;

    case 'h':
      cerr << usage;
      exit(0);
    default:
      cerr << usage;
      exit(1);
    }
  }

  argc -= optind;
  argv += optind;

  if (argc > 0) {
    CFG.serviceKey = argv[0];
    argc--;
    argv++;
  }
    
  if (argc > 0) {
    cerr << "Error: Too many command line parameters: " << *argv << "?" << endl;
    cerr << usage;
    exit(1);
  }

  ////////////////////////////////////////
  // Have all the options - let's do it
  ////////////////////////////////////////

  if (CFG.debugFlag) {
    cout << "tClientPollProxy " << CFG.lookupMap << " Repeats=";
    if (CFG.repeatCount < 0) {
      cout << "forever";
    }
    else {
      cout << CFG.repeatCount;
    }
   cout << " Delay=" << CFG.waitMSecs << "MS Payload Size=" << CFG.payloadSize
	<< " Timeout=" << CFG.timeoutSeconds << endl;
  }

  st_init();

  pluton::client C("tClientPollProxy");
  C.setPollProxy(st_poll);

  if (CFG.debugFlag) C.setDebug(true);
  if (!C.initialize(CFG.lookupMap)) {
    cerr << "Error: Could not initialize pluton." << endl;
    exit(1);
  }

  // Let the user break out of an otherwise infinite number of pings

  signal(SIGINT, catchINT);
  siginterrupt(SIGINT, true);

  // Everything is ready - create the threads to issues the requests

  CFG.pc = &C;
  globalThreadCount = CFG.parallelCount;

  for (int ix=0; ix < CFG.parallelCount; ++ix) {
    st_thread_create(pingThread, (void*) &CFG, 0, 0);
  }
  while (globalThreadCount > 0) st_sleep(1);
  printResults();
}
