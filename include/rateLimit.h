#ifndef _RATELIMIT_H
#define _RATELIMIT_H

//////////////////////////////////////////////////////////////////////
// A class that constrains how many actions can be performed per unit
// of time.
//////////////////////////////////////////////////////////////////////

#include <time.h>

namespace util {
  class rateLimit {

  public:
    rateLimit(int setRatePerTimeUnit=1, int setTimeUnitInSeconds=1);

    void	setRate(int setRatePerTimeUnit) { _rate = setRatePerTimeUnit; }
    void	setTimeUnit(int setTimeUnitInSeconds) { _timeUnit = setTimeUnitInSeconds; }

    int		getRate() const { return _rate; }
    int		getTimeUnit() const { return _timeUnit; }

    bool	allowed(time_t, bool justTestingFlag=false);	// Test and increment if allowed

  private:
    int		_rate;
    int		_timeUnit;

    int		_attempts;
    time_t	_lastTimeUnit;
};
}

#endif
