#ifndef _LOGGING_H
#define _LOGGING_H

#include "bitmask.h"

class logging {
 private:

  static bitmask	LOGGING;

 public:
  static const unsigned int 	processStartBit =	1 << 1;
  static const unsigned int 	processLogBit =		1 << 2;
  static const unsigned int 	processExitBit =	1 << 3;
  static const unsigned int 	processUsageBit =	1 << 4;

  static const unsigned int 	serviceConfigBit =	1 << 10;
  static const unsigned int 	serviceStartBit =	1 << 11;
  static const unsigned int 	serviceExitBit =	1 << 12;

  static const unsigned int	calibrateBit =		1 << 17;

  static bool	setFlag(const char *);
  static bool	clearFlag(const char *);

  static bool	ANY() { return LOGGING.isAny(); }

  inline static bool processStart() { return LOGGING.isSet(logging::processStartBit); }
  inline static bool processLog() { return LOGGING.isSet(logging::processLogBit); }
  inline static bool processExit() { return LOGGING.isSet(logging::processExitBit); }
  inline static bool processUsage() { return LOGGING.isSet(logging::processUsageBit); }

  inline static bool serviceConfig() { return LOGGING.isSet(logging::serviceConfigBit); }
  inline static bool serviceStart() { return LOGGING.isSet(logging::serviceStartBit); }
  inline static bool serviceExit() { return LOGGING.isSet(logging::serviceExitBit); }

  inline static bool calibrate() { return LOGGING.isSet(logging::calibrateBit); }
};

#define	LOGPRT	cout
#define LOGFLUSH cout.flush()


#endif
