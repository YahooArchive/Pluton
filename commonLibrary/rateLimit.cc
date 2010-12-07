#include "rateLimit.h"


util::rateLimit::rateLimit(int setRate, int setTimeUnit)
  : _rate(setRate), _timeUnit(setTimeUnit), _attempts(0), _lastTimeUnit(0)
{
}

//////////////////////////////////////////////////////////////////////
// Test to see if the caller is running at or below the defined
// rate. The assumption is that the caller will consume the resource
// that is rate limited unless the justTestingFlag is set, which makes
// the call a simple test as to whether the caller is within the rate
// limit.
//////////////////////////////////////////////////////////////////////

bool
util::rateLimit::allowed(time_t now, bool justTestingFlag)
{
  int nowTimeUnit = now;
  if (_timeUnit > 1) nowTimeUnit /= _timeUnit;	// Scale clock to rate quantum

  if (_lastTimeUnit != nowTimeUnit) {		// If into a different quantum
    _lastTimeUnit = nowTimeUnit;		// then all limts are reset
    _attempts = 0;
  }

  if (!justTestingFlag) ++_attempts;

  return _attempts <= _rate;
}
