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

#ifndef _DEBUG_H
#define _DEBUG_H

#include <st.h>

#include "ostreamWrapper.h"

#include "bitmask.h"

class debug {
 private:

  static bitmask	DEBUG;

 public:

  static const unsigned int	managerBit = 		1 << 0;
  static const unsigned int	serviceBit = 		1 << 1;
  static const unsigned int	processBit =		1 << 2;
  static const unsigned int	threadBit =		1 << 3;
  static const unsigned int	childBit = 		1 << 4;
  static const unsigned int	stioBit = 		1 << 5;
  static const unsigned int	configBit =		1 << 6;
  static const unsigned int	reportingChannelBit =	1 << 7;
  static const unsigned int	calibrateBit =		1 << 8;

  static const unsigned int	oneShotBit =		1 << 31;

  static bool	setFlag(const char *);
  static bool	clearFlag(const char *);

  static bool	ANY() { return DEBUG.isAny(); }

  inline static bool child() { return DEBUG.isSet(debug::childBit); }
  inline static bool manager() { return DEBUG.isSet(debug::managerBit); }
  inline static bool service() { return DEBUG.isSet(debug::serviceBit); }
  inline static bool process() { return DEBUG.isSet(debug::processBit); }
  inline static bool thread() { return DEBUG.isSet(debug::threadBit); }
  inline static bool stio() { return DEBUG.isSet(debug::stioBit); }
  inline static bool config() { return DEBUG.isSet(debug::configBit); }
  inline static bool reportingChannel() { return DEBUG.isSet(debug::reportingChannelBit); }
  inline static bool calibrate() { return DEBUG.isSet(debug::calibrateBit); }

  inline static bool oneShot() { return DEBUG.isSet(debug::oneShotBit); }

  static std::ostream& log(std::ostream&);
};

#define DBGPRT	debug::log(std::cerr) << " " << st_thread_self() << ": "

#endif
