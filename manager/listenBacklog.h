#ifndef _LISTENBACKLOG_H
#define _LISTENBACKLOG_H

#include <string>

#include <st.h>

//////////////////////////////////////////////////////////////////////
// Provide an OS-independent method of determining the depth of the
// listen queue on a socket. Some OSes, namely FBSD, provide a way of
// getting the exact number, others, such as Linux only let you know
// whether the queue is empty or not.
//////////////////////////////////////////////////////////////////////

class listenBacklog {
 public:
  listenBacklog();
  ~listenBacklog();

  bool 	initialize(std::string em);
  int	queueLength(st_netfd_t sFD);

 private:
  int	controlFD;
};

#endif
