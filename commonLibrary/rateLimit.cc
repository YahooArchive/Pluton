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
