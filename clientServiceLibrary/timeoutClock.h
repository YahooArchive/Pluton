#ifndef _TIMEOUTCLOCK_H
#define _TIMEOUTCLOCK_H

#include <sys/time.h>

namespace pluton {

  class timeoutClock {
  public:
    timeoutClock();

    bool	isRunning() const { return _isRunningFlag; }
    void	start(const struct timeval* now, int setTimeoutMS);
    int 	getMSremaining(const struct timeval* now, struct timeval* remainingPtr=0);
    void	stop() { _isRunningFlag = false; }

  private:
    bool		_isRunningFlag;
    struct timeval	_startTime;
    int			_timeoutMS;
  };
}

#endif
