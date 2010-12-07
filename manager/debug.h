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
