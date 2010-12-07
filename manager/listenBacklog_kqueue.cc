//////////////////////////////////////////////////////////////////////
// This version of detecting the listen backlog used kqueue
// EVFILT_READ which is defined to return the listen queue length for
// an socket that is listening.
//////////////////////////////////////////////////////////////////////

#include <string>

#include <errno.h>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include <st.h>

#include "listenBacklog.h"
#include "util.h"


listenBacklog::listenBacklog()
  : controlFD(-1)
{
}

listenBacklog::~listenBacklog()
{
  if (controlFD != -1) close(controlFD);
}


bool
listenBacklog::initialize(std::string em)
{
  controlFD = kqueue();
  if (controlFD == -1) {
    util::messageWithErrno(em, "Error return from kqueue()");
    return false;
  }

  return true;
}


//////////////////////////////////////////////////////////////////////
// Return the number of connections pending on the listen queue
//////////////////////////////////////////////////////////////////////

int
listenBacklog::queueLength(st_netfd_t sFD)
{
  int fd = st_netfd_fileno(sFD);

  struct kevent	kev;
  EV_SET(&kev, fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, -1, 0);

  struct timespec timeout;
  timeout.tv_sec = 0;
  timeout.tv_nsec = 0;

  int res = kevent(controlFD, &kev, 1, &kev, 1, &timeout);

  if ((res == 1) && (kev.data > 0)) return kev.data;

  return 0;
}
