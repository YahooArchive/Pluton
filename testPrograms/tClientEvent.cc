// A hack of plPing that uses libevents

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

#include <event.h>

#include "util.h"

#include "pluton/clientEvent.h"


using namespace std;


#define	MICROSECS	1000000	// per second

static char* usage =
"Usage: plPing_ev [-dhknow] [-c count] [-C context] [-s packetsize] [-p parallelCount]\n"
"                           [-S unixSocket] [-t timeout] [ ServiceKey ]\n"
"\n"
"Measure the response time of an pluton server using the libevent interface\n"
"\n"
"Where:\n"
" -c      Stop after sending 'count' requests (default: infinite)\n"
" -C      Set context with this value (the normal echo service uses this\n"
"         as a sleep value)\n"
" -d      Turn on debug\n"
" -h      Display Usage and exit\n"
" -i      Wait 'wait' seconds between requests (default: 1)\n"
"         If set to zero, this is effectively a ping flood.\n"
" -k      Send echo a random die request\n"
" -o      Oneshot. Set the 'noRetryAttr' attribute\n"
" -n      Do Not check the response for a valid echo\n"
" -p      Number of requests to send in parallel (default: 1)\n"
" -s      Size of added data to supply in the ping packet (default: 0)\n"
" -S      The Unix socket used to connect to the server (default: '')\n"
" -t      Numbers of seconds to wait for a response (default: 2)\n"
" -w      NoWait. Set the 'noWaitAttr' attribute\n"
"\n"
" ServiceKey is the service sent the request (default: system.echo.0.raw)\n"
"\n";

//////////////////////////////////////////////////////////////////////

static bool	terminateFlag = false;
static bool	debugFlag = false;

static long	totalReadBytes = 0;
static long	totalWriteBytes = 0;
static int	totalPingCount = 0;
static timeval	minPingTime;
static timeval	maxPingTime;
static timeval totalPingTime;


class myRequest : public pluton::clientRequest {
public:
  int		_myIndex;
  int		_repeatCount;
  string 	_payload;
  struct timeval _startTime;
  pluton::clientEvent* client;
};

static void catchINT(int sig) { terminateFlag = true; }

static void	plutonEventProxy(int fd, short event, void* voidMyR);


//////////////////////////////////////////////////////////////////////
// Convert pending pluton events into pending libevents
//////////////////////////////////////////////////////////////////////

static void
transferEvents(pluton::clientEvent *C)
{
  if (terminateFlag) return;

  struct timeval now;
  gettimeofday(&now, 0);

  pluton::clientEvent::eventWanted* ewp;
  while ((ewp = C->getNextEventWanted(&now))) {
    event_once(ewp->fd,
	       (ewp->type == pluton::clientEvent::wantRead) ? EV_READ : EV_WRITE,
	       plutonEventProxy, (void*) ewp->clientRequestPtr, &ewp->timeout);
  }
}


class config {
public:
  config() :
    repeatCount(-1),
    parallelCount(1),
    waitSeconds(1),
    payloadSize(0),
    timeoutSeconds(2),
    context(0),
    unixSocket(""),
    serviceKey("system.echo.0.raw"),
    oneShot(false),
    sendDie(false),
    noWait(false),
    checkEcho(true) {}

  int	repeatCount;
  int	parallelCount;
  int	waitSeconds;
  int	payloadSize;
  int	timeoutSeconds;
  const char* context;
  const char* unixSocket;
  const char* serviceKey;
  bool	oneShot;
  bool  sendDie;
  bool	noWait;
  bool	checkEcho;
  pluton::client*	pc;
};

static config	CFG;


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

static void
checkOneResponse(int myIndex, bool checkEcho,
		 pluton::clientRequest* retR, struct timeval& startTime, const string& payload,
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
    exit(1);
  }

  if (checkEcho && (strncmp(payload.data(), resp, respLen) != 0)) {
    cerr << "Error: Payload corrupted" << endl;
    exit(1);
  }

  trackResults(myIndex, startTime, endTime, payload.length(), respLen, (waitSeconds > 0));
}


void
setRequest(config& CFG, myRequest* R)
{
  static char* pl = 0;
  if (!pl) pl = new char [CFG.payloadSize+1];
  if (CFG.payloadSize > 0) {
    char randomChar = ((R->_myIndex + time(0)) % 26) + 'A';
    memset(pl, randomChar, CFG.payloadSize);
    R->_payload.assign(pl, CFG.payloadSize);
  }
  const char* payloadPtr = R->_payload.data();
  int payloadLen = R->_payload.length();
  if (debugFlag) cout << "Payload: " << R->_payload << endl;

  R->reset();
  R->setRequestData(payloadPtr, payloadLen);
  if (CFG.oneShot) R->setAttribute(pluton::noRetryAttr);
  if (CFG.noWait)  R->setAttribute(pluton::noWaitAttr);
  if (CFG.context) {
    R->setContext("echo.sleepMS", CFG.context);
    R->setContext("echo.log", "Howdy Doody");
  }
  if (CFG.sendDie) R->setContext("echo.die", "");
  struct timeval now;
  gettimeofday(&now, 0);
  R->_startTime = now;
}


static void
plutonEventProxy(int fd, short event, void* voidMyR)
{
  if (debugFlag) clog << "plutonEventProxy: fd=" << fd << " t=" << event << endl;

  myRequest* myR = (myRequest*) voidMyR;
  pluton::clientEvent* C = myR->client;

  int completedCount = 0;
  switch (event) {
  case EV_READ:
    if (debugFlag) clog << "sendCanReadEvent " << fd << endl;
    completedCount = C->sendCanReadEvent(fd);
    break;

  case EV_WRITE:
    if (debugFlag) clog << "sendCanWriteEvent " << fd << endl;
    completedCount = C->sendCanWriteEvent(fd);
    break;

    // We use timeout to re-schedule a request after waitTime

  case EV_TIMEOUT:
    if (debugFlag) clog << "sendTimeoutEvent " << fd << endl;
    setRequest(CFG, myR);
    C->addRequest(CFG.serviceKey, myR, CFG.timeoutSeconds * 1000);
    break;
  }

  if (debugFlag) clog << "completedCount = " << completedCount << endl;

  // Take back control of any completed requests

  pluton::clientRequest* R;
  if (completedCount > 0) {
    while ((R = C->getCompletedRequest())) {
      myRequest* myR = dynamic_cast<myRequest*>(R);
      checkOneResponse(myR->_myIndex, CFG.checkEcho, R, myR->_startTime, myR->_payload,
		       CFG.waitSeconds);
      if (debugFlag) clog << "EventProxy Repeat = " << myR->_repeatCount << endl;
      if ((myR->_repeatCount < 0) || (myR->_repeatCount-- > 0)) {
	if (CFG.waitSeconds > 0) {
	  myR->_startTime.tv_sec = CFG.waitSeconds;
	  myR->_startTime.tv_usec = 0;
	  event_once(-1, EV_TIMEOUT, plutonEventProxy, (void*) myR, &myR->_startTime);
	}
	else {
	  setRequest(CFG, myR);
	  C->addRequest(CFG.serviceKey, R, CFG.timeoutSeconds * 1000);
	}
      }
    }
  }

  transferEvents(C);	// Find any new events and give them to libevent
}


int
main(int argc, char **argv)
{
  char	optionChar;

  while ((optionChar = getopt(argc, argv, "c:C:dhki:nop:s:S:t:w")) != -1) {
    switch (optionChar) {
    case 'c':
      CFG.repeatCount = atoi(optarg);
      if (CFG.repeatCount <= 0) {
	cerr << "Error: Repeat Count must be greater than zero" << endl;
	cerr << usage;
	exit(1);
      }
      break;

    case 'C': CFG.context = optarg; break;

    case 'd': debugFlag = true; break;

    case 'k': CFG.sendDie = true; break;

    case 'i':
      CFG.waitSeconds = atoi(optarg);
      if (CFG.waitSeconds < 0) {
	cerr << "Error: Wait seconds must not be less than zero" << endl;
	cerr << usage;
	exit(1);
      }
      break;

    case 'n': CFG.checkEcho = false; break;

    case 'o': CFG.oneShot = true; break;

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

    case 'S': CFG.unixSocket = optarg; break;

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

  if (debugFlag) {
    cout << "plPing_ev " << CFG.unixSocket << " Repeats=";
    if (CFG.repeatCount < 0) {
      cout << "forever";
    }
    else {
      cout << CFG.repeatCount;
    }
   cout << " Delay=" << CFG.waitSeconds << " Payload Size=" << CFG.payloadSize
	<< " Timeout=" << CFG.timeoutSeconds << endl;
  }

  pluton::clientEvent C("plPing_ev");

  if (debugFlag) C.setDebug(true);
  if (C.initialize(CFG.unixSocket) < 0) {
    cerr << "Error: Could not initialize pluton." << endl;
    exit(1);
  }

  // Let the user break out of an otherwise infinite number of pings

  event_init();

  signal(SIGINT, catchINT);
  siginterrupt(SIGINT, true);

  // Construct the initial requests to prime the event queue

  myRequest* RL = new myRequest[CFG.parallelCount];

  for (int ix=0; ix < CFG.parallelCount; ++ix) {
    RL[ix]._myIndex = ix;
    RL[ix]._repeatCount = CFG.repeatCount;
    RL[ix].client = &C;
    if (debugFlag) clog << "Repeat C set to " << CFG.repeatCount << endl;
    setRequest(CFG, RL+ix);
    C.addRequest(CFG.serviceKey, RL+ix, CFG.timeoutSeconds * 1000);
  }

  transferEvents(&C);

  event_dispatch();

  printResults();
}
