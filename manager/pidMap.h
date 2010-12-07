#ifndef _PIDMAP_H
#define _PIDMAP_H

//////////////////////////////////////////////////////////////////////
// Track pid to process mappings so that the child reaper can find the
// associated process.
//////////////////////////////////////////////////////////////////////

#include <sys/types.h>

class process;


class pidMap {
 public:
  static void		reapChildren(const char* who);

  static bool 		add(pid_t pid, process* sp);
  static void 		remove(pid_t pid);
  static process*	find(pid_t);
};

#endif
