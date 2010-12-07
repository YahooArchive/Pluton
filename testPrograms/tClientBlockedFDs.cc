#include <iostream>

#if defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#else
#include <sys/select.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <poll.h>

#include <pluton/client.h>

using namespace std;

// Use the executeAndWaitBlocked() interface in conjunction with the
// setPollProxy method to show how a client can have blocked fds
// return so that they can do the wait - potentially with other fds
// they may be interested in.

static void
failed(const char* err, int val=0)
{
  cout << "Failed: tClientBlockedFDs " << err << " val=" << val << endl;
}

const char* SK = "system.echo.0.raw";

static int    s_maxFD = 0;
static fd_set s_readFDs;
static fd_set s_writeFDs;
static int    s_timeoutMS;

static int
pollProxy(struct pollfd *fds, int nfds, unsigned long long timeoutMicroSeconds)
{
  int toMS = timeoutMicroSeconds / 1000;
  int res = poll(fds, nfds, toMS);
  cout << "pollProxy called: nfds=" << nfds << " to=" << toMS << "mSecs res=" << res << endl;
  s_maxFD = 0;
  if (res != 0) return res;

  // On timeout and nothing done, transfer outstanding poll fds to my
  // select fds

  FD_ZERO(&s_readFDs);
  FD_ZERO(&s_writeFDs);
  s_timeoutMS = toMS;
  s_maxFD = 0;

  for (;nfds > 0; nfds--, fds++) {
    int missingEvents = (fds->events & ~fds->revents);
    cout << "Checking fd=" << fds->fd << " events=" << fds->events << "/" << fds->revents
	 << " missing=" << missingEvents << endl;
    if (!missingEvents) continue;
    if (missingEvents & (POLLIN|POLLRDNORM|POLLRDBAND|POLLPRI)) {
      FD_SET(fds->fd, &s_readFDs);
      if (fds->fd >= s_maxFD) s_maxFD = fds->fd + 1;
      cout << "Transfering " << fds->fd << " to readFD" << endl;
    }

    if (missingEvents & (POLLOUT|POLLWRNORM|POLLWRBAND)) {
      FD_SET(fds->fd, &s_writeFDs);
      if (fds->fd >= s_maxFD) s_maxFD = fds->fd + 1;
      cout << "Transfering " << fds->fd << " to writeFD" << endl;
    }
  }

  return res;
}

static int
doSelect()
{
  cout << "doSelect maxFD = " << s_maxFD << endl;
  if (s_maxFD > 0) {
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    int res = select(s_maxFD, &s_readFDs, &s_writeFDs, 0, &tv);
    cout << "Select returned " << res << " errno=" << errno << endl;
  }
  return s_maxFD;
}


int
main(int argc, char** argv)
{
  const char* goodPath = 0;
  if (argc > 1) goodPath = argv[1];

  int res;
  pluton::client C;

  if (!C.initialize(goodPath)) {
    failed("bad return from good path");
    cout << C.getFault().getMessage("C.initialize()") << endl;
    exit(1);
  }

  C.setPollProxy(pollProxy);

  pluton::clientRequest R1;
  R1.setContext("echo.sleepMS", "100");
  if (!C.addRequest(SK, R1)) {
    failed("bad return from addRequest 1");
    cout << C.getFault().getMessage("C.addRequest() 1") << endl;
    exit(3);
  }

  pluton::clientRequest R2;
  R2.setContext("echo.sleepMS", "4000");
  char buffer[300000];	// Ensure we get some blocking writes
  R2.setRequestData(buffer, sizeof(buffer));
  if (!C.addRequest(SK, R2)) {
    failed("bad return from addRequest 1");
    cout << C.getFault().getMessage("C.addRequest() 1") << endl;
    exit(3);
  }

  res = C.executeAndWaitBlocked(5);

  if (res != 0) {
    failed("bad return from first executeAndWaitBlocked(0)", res);
    cout << C.getFault().getMessage() << endl;
    exit(4);
  }

  // R1 should come back within 100ms, but let's give it 3 seconds
  for (int ix=0; ix < 3; ix++) {
    res = C.executeAndWaitBlocked(1000);
    if (res != 0) break;
    cout << time(0) << ": doSelect() = " << doSelect() << endl;
  };

  if (res != 1) {
    failed("bad return from second executeAndWaitBlocked(100)", res);
    cout << C.getFault().getMessage() << endl;
    exit(5);
  }

  pluton::clientRequest* retR = C.executeAndWaitAny();
  if (retR != &R1) {
    failed("bad return from e&W Any", retR == 0);
    exit(6);
  }

  return(0);
}
