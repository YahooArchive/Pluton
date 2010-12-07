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
