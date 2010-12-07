//////////////////////////////////////////////////////////////////////
// A simple timeout manager
//////////////////////////////////////////////////////////////////////

#include "util.h"
#include "timeoutClock.h"

pluton::timeoutClock::timeoutClock()
  : _isRunningFlag(false)
{
}

void
pluton::timeoutClock::start(const struct timeval* now, int setTimeoutMS)
{
  _startTime = *now;
  _timeoutMS = setTimeoutMS;
  _isRunningFlag = true;
}


//////////////////////////////////////////////////////////////////////
// Given the start time of the request and the current time, determine
// how much time is left.
//
// Return: MS remaining in the return value (-ve means
// expired). Populate remainingPtr, if provided.
//////////////////////////////////////////////////////////////////////

int
pluton::timeoutClock::getMSremaining(const struct timeval* now, struct timeval* remainingPtr)
{
  if (!_isRunningFlag) return 0;

  int remainingMS = _timeoutMS - util::timevalDiffMS(*now, _startTime);
  if (remainingMS <= 0) return remainingMS;

  //////////////////////////////////////////////////////////////////////
  // Catch the bizarre case of the system clock being set backward
  // such that timeouts could last for a *long* time. It takes just
  // two lines of code and at worst a clock change will result in a
  // maximum timeout of 2 * requested, so why not?
  //////////////////////////////////////////////////////////////////////

  if (remainingMS > _timeoutMS) {
    remainingMS = _timeoutMS;
    _startTime = *now;
  }

  if (remainingPtr) {
    remainingPtr->tv_sec = remainingMS / 1000;
    remainingPtr->tv_usec = (remainingMS % 1000) * 1000;
  }

  return remainingMS;
}
