//////////////////////////////////////////////////////////////////////
// This version of detecting the listen backlog used st_netfd_poll to
// see if the accept socket is readable. All this tells us is that the
// backlog is at least one deep. This routine is a last-resort in the
// event that your OS does not have a function to determine queue
// length.
//////////////////////////////////////////////////////////////////////

#include <string>

#include <st.h>

#include "listenBacklog.h"
#include "util.h"


listenBacklog::listenBacklog()
{
}

listenBacklog::~listenBacklog()
{
}


bool
listenBacklog::initialize(std::string em)
{
  return true;
}

int
listenBacklog::queueLength(st_netfd_t sFD)
{
  if (st_netfd_poll(sFD, POLLIN, 0) == 0) return 1;

  return 0;
}
