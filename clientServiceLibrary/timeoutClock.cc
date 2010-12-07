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
