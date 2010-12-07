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


using namespace std;

static void
dumpFDs(const char* who, int maxFD, fd_set& fds)
{
  clog << "dumpFDs " << who << ":";
  for (int ix=0; ix <= maxFD; ++ix) {
    if (FD_ISSET(ix, &fds)) clog << " " << ix;
  }
  clog << endl;
}


int
main(int argc, char **argv)
{
  clog << "Starting tClientEvent2" << endl;

  pluton::clientEvent C("tClientEvent2");

  if (!C.initialize(argv[1])) {
    cerr << "Error: Could not initialize pluton." << endl;
    exit(1);
  }

  pluton::clientRequest R1;
  pluton::clientRequest R2;

  // R1 will purposely time out

  R1.setClientHandle((void*) "R1");
  R1.setContext("echo.sleepMS", "3000");
  C.addRequest("system.echo.0.raw", &R1, 1500);

  // R2 will complete

  R2.setClientHandle((void*) "R2");
  R2.setContext("echo.sleepMS", "500");
  C.addRequest("system.echo.0.raw", &R2, 1000);

  time_t endTime = time(0) + 4;	// Make sure we don't get trapped
  int completedCount = 0;

  //////////////////////////////////////////////////////////////////////
  // As the documentation says, once an fd/event pair is given to us
  // we have to remember it until returned. Fortunately it's easy to
  // use an fd_set as a container recording which fds we have.
  //////////////////////////////////////////////////////////////////////

  fd_set plutonReadFDs, plutonWriteFDs;
  FD_ZERO(&plutonReadFDs);
  FD_ZERO(&plutonWriteFDs);
  int maxFD = 0;

  while ((completedCount < 2) && (endTime > time(0))) {

    clog << "Top of completion loop: completedCount = " << completedCount << endl;

    struct timeval now;		// Collect any new events from clientEvent
    gettimeofday(&now, 0);
    const pluton::clientEvent::eventWanted* ep;
    while ((ep = C.getNextEventWanted(&now))) {
      clog << "getNextEventWanted fd=" << ep->fd << " "
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
    }

    // Copy fd sets as they are zeroed by select()

    fd_set readFDs, writeFDs;

    readFDs = plutonReadFDs;
    writeFDs = plutonWriteFDs;

    FD_SET(0, &readFDs);	// Add the fds that I'm interesed in (stdin)

    struct timeval timeout;	// Issue select
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;	// 1/10th of a second

    gettimeofday(&now, 0);
    clog << now.tv_sec << "." << now.tv_usec << ":select(" << maxFD+1 << ", ...,"
	 << timeout.tv_sec << "." << timeout.tv_usec << ")" << endl;

    dumpFDs("readFDS", maxFD, readFDs);
    dumpFDs("writeFDS", maxFD, writeFDs);
    
    int res = select(maxFD+1, &readFDs, &writeFDs, 0, &timeout);
    clog << "select() returned " << res << " maxFD=" << maxFD << endl;
    if (res == -1) {
      clog << "Unexpected error from select() errno=" << errno << endl;
      exit(5);
    }

    // Check my fds first

    if (FD_ISSET(0, &readFDs)) {
      clog << "STDIN has traffic" << endl;
      char rbuf[100];
      read(0, rbuf, sizeof(rbuf));
    }

    // Now check pluton fds

    if (res > 0) {
      for (int ix=1; ix <= maxFD; ++ix) {
	if (FD_ISSET(ix, &readFDs)) {
	  clog << "Returning read event for " << ix << endl;
	  int sr = C.sendCanReadEvent(ix);
	  if (sr < 0) exit(-sr + 10);
	  FD_CLR(ix, &plutonReadFDs);	// We're no longer responsible
	  continue;
	}
	if (FD_ISSET(ix, &writeFDs)) {
	  clog << "Returning write event for " << ix << endl;
	  int sr = C.sendCanWriteEvent(ix);
	  if (sr < 0) exit(-sr + 20);
	  FD_CLR(ix, &plutonWriteFDs);
	  continue;
	}
      }
    }
    else {	// Send timeout to all of them
      for (int ix=1; ix <= maxFD; ++ix) {
	if (FD_ISSET(ix, &plutonReadFDs) || FD_ISSET(ix, &plutonWriteFDs)) {
	  clog << "Returning timeout event for " << ix << endl;
	  int sr = C.sendTimeoutEvent(ix);
	  if (sr < 0) exit(-sr + 30);
	  FD_CLR(ix, &plutonReadFDs);
	  FD_CLR(ix, &plutonWriteFDs);
	}
      }
    }

    pluton::clientRequest* R; 		// Collected completed requests
    while ((R = C.getCompletedRequest())) {
      ++completedCount;
      clog << time(0) << ":Completed:" << (void*) R << " " << (char*) R->getClientHandle() << endl;
      if (R->hasFault()) clog << "Fault: " << (char*) R->getClientHandle() << endl;
    }
  }

  clog << "Done: Completed=" << completedCount << endl;

  if (completedCount != 2) exit(10);

  if (!R1.hasFault()) exit(12);	// Should have timeout
  if (R2.hasFault()) exit(6);	// Should be good-to-go

  pluton::faultCode fc = R1.getFaultCode();
  clog << "r1 fault = " << fc << " " << R1.getFaultText() << endl;

  if (fc != pluton::serviceTimeout) exit(7);

  return(0);
}
