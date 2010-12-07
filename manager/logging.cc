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

#include <iostream>

#include "logging.h"

bitmask	logging::LOGGING;

static bitmask::nameBitMap	nb[] = {

  { "processStart", logging::processStartBit },
  { "processLog", logging::processLogBit },
  { "processExit", logging::processExitBit },
  { "processUsage", logging::processUsageBit },

  { "serviceConfig", logging::serviceConfigBit },
  { "serviceStart", logging::serviceStartBit },
  { "serviceExit", logging::serviceExitBit },

  { "calibrate", logging::calibrateBit },

  // Combination bits

  { "process", logging::processStartBit | logging::processLogBit | logging::processExitBit
    | logging::processUsageBit },
  { "service", logging::serviceConfigBit | logging::serviceStartBit | logging::serviceExitBit },
  { "all", 0xFFFFFFFF },

  { 0 }
};



bool
logging::setFlag(const char* name)
{
  return LOGGING.setByName(name, nb);
}

bool
logging::clearFlag(const char* name)
{
  return LOGGING.clearByName(name, nb);
}
