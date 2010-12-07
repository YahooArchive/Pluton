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
