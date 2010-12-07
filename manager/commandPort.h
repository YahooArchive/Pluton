#ifndef _COMMANDPORT_H
#define _COMMANDPORT_H

//////////////////////////////////////////////////////////////////////
// The command port accepts commands to display various internal
// aspects of the daemon.
//////////////////////////////////////////////////////////////////////

#include <st.h>

class manager;


class commandPort {
 public:
  commandPort(st_netfd_t newSocket, manager* setM);
  ~commandPort();

  void		run();
  manager*	getMANAGER() const { return M; }

  static void*	listen(void*);

 private:
  commandPort&	operator=(const commandPort& rhs);	// Assign not ok
  commandPort(const commandPort& rhs);			// Copy not ok

  st_netfd_t		ioSocket;
  manager*		M;
};

#endif
