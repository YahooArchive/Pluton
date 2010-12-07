#include <iostream>
#include <string>

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#else
#include <sys/select.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "pluton/clientEvent.h"

// Exercise the optional parameter to getNextEventWanted to filter
// returned events and exercise the optional abort flag to
// clientEvent::sendTimeoutEvent to force a timeout of the request.

using namespace std;

static struct timeval
minimumTimeval(const struct timeval* A, const struct timeval *B)
{
  cout << "minimumTimeout: " << A->tv_sec << "." << A->tv_usec
       << " vs " << B->tv_sec << "." << B->tv_usec << endl;

  if ((A->tv_sec == 0) && (A->tv_usec == 0)) return *B;
  if ((B->tv_sec == 0) && (B->tv_usec == 0)) return *A;
  if (A->tv_sec < B->tv_sec)  return *A;
  if (B->tv_sec < A->tv_sec) return *B;
  if (A->tv_usec < B->tv_usec) return *A;
  return *B;
}


static void
waitOn(pluton::clientEvent& C, pluton::clientRequest* R, int waitSeconds)
{
  time_t endTime = time(0) + waitSeconds;	// Make sure we don't get trapped

  fd_set plutonReadFDs, plutonWriteFDs;
  FD_ZERO(&plutonReadFDs);
  FD_ZERO(&plutonWriteFDs);
  int maxFD = 0;

  while (R->inProgress() && (endTime > time(0))) {

    cout << "Top of loop waitOn" << endl;

    struct timeval now;		// Collect any new events from clientEvent
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    gettimeofday(&now, 0);
    const pluton::clientEvent::eventWanted* ep;
    while ((ep = C.getNextEventWanted(&now, R))) {
      cout << "getNextEventWanted fd=" << ep->fd << " "
	   << ((ep->type == pluton::clientEvent::wantRead) ? "read" : "write")
	   << " to=" << ep->timeout.tv_sec << "." << ep->timeout.tv_usec << endl;
      if (ep->fd > maxFD) maxFD = ep->fd;
      switch (ep->type) {
      case pluton::clientEvent::wantRead:
	FD_SET(ep->fd, &plutonReadFDs);
	break;

      case pluton::clientEvent::wantWrite:
	FD_SET(ep->fd, &plutonWriteFDs);
	break;

      default: exit(4);
      }

      // Keep the smallest non-zero timeout

      timeout = minimumTimeval(&ep->timeout, &timeout);
      if (timeout.tv_sec > waitSeconds) timeout.tv_sec = waitSeconds;
    }

    // Need a copy of fd sets as they are zeroed by select()

    fd_set readFDs;
    fd_set writeFDs;

    readFDs = plutonReadFDs;
    writeFDs = plutonWriteFDs;
    gettimeofday(&now, 0);
    cout << now.tv_sec << "." << now.tv_usec << ":select(" << maxFD+1 << ", ...,"
	 << timeout.tv_sec << "." << timeout.tv_usec << ")" << endl;

    int res = select(maxFD+1, &readFDs, &writeFDs, 0, &timeout);
    cout << "select() returned " << res << " maxFD=" << maxFD << endl;
    if (res == -1) {
      cerr << "Unexpected error from select() errno=" << errno << endl;
      exit(1);
    }

    if (res > 0) {
      for (int ix=1; ix <= maxFD; ++ix) {
	if (FD_ISSET(ix, &readFDs)) {
	  cout << "Returning read event for " << ix << endl;
	  int sr = C.sendCanReadEvent(ix);
	  if (sr < 0) exit(-sr + 10);
	  FD_CLR(ix, &plutonReadFDs);	// We no longer own the request
	  continue;
	}
	if (FD_ISSET(ix, &writeFDs)) {
	  cout << "Returning write event for " << ix << endl;
	  int sr = C.sendCanWriteEvent(ix);
	  if (sr < 0) exit(-sr + 20);
	  FD_CLR(ix, &plutonWriteFDs);
	  continue;
	}
      }
    }
    else {
      for (int ix=1; ix <= maxFD; ++ix) {
	if (FD_ISSET(ix, &plutonReadFDs) || FD_ISSET(ix, &plutonWriteFDs)) {
	  int sr = C.sendTimeoutEvent(ix);
	  if (sr < 0) exit(-sr + 30);
	  FD_CLR(ix, &plutonReadFDs);
	  FD_CLR(ix, &plutonWriteFDs);
	}
      }
    }

    pluton::clientRequest* doneR; 		// Collected completed requests
    while ((doneR = C.getCompletedRequest())) {
      cout << time(0) << ":Completed:" << (void*) doneR << " "
	   << (char*) doneR->getClientHandle() << endl;
      if (doneR->hasFault()) cout << "Fault: " << (char*) doneR->getClientHandle() << endl;
      if (doneR != R) exit(10);
    }
  }
}


int
main(int argc, char **argv)
{

  pluton::clientEvent C("tClientEvent2");

  if (!C.initialize(argv[1])) {
    cerr << "Error: Could not initialize pluton." << endl;
    exit(1);
  }

  pluton::clientRequest R1;
  pluton::clientRequest R2;

  R1.setClientHandle((void*) "R1");
  R1.setContext("echo.sleepMS", "4000");
  C.addRequest("system.echo.0.raw", &R1, 4000);

  R2.setClientHandle((void*) "R2");
  R2.setContext("echo.sleepMS", "500");
  C.addRequest("system.echo.0.raw", &R2, 1000);

  waitOn(C, &R2, 2);

  if (!R1.inProgress()) exit(5);
  if (R2.inProgress()) exit(6);

  waitOn(C, &R1, 1);

  if (!R1.inProgress()) exit(7);

  // Send an aborted timeout to the event handler for the second request

  struct timeval now;
  gettimeofday(&now, 0);

  const pluton::clientEvent::eventWanted* ep = C.getNextEventWanted(&now);
  if (!ep) exit(8); // should have one
  C.sendTimeoutEvent(ep->fd, true);

  pluton::clientRequest* doneR = C.getCompletedRequest();
  if (doneR != &R1) exit(9);


  pluton::faultCode fc = R1.getFaultCode();
  cout << "r1 fault = " << fc << " " << R1.getFaultText() << endl;

  if (fc != pluton::serviceTimeout) exit(11);

  return(0);
}
