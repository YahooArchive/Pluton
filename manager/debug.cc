#include <sys/time.h>

#include <iostream>
#include <iomanip>

#include "debug.h"

bitmask	debug::DEBUG;

static bitmask::nameBitMap	nb[] = {

  { "child", debug::childBit },
  { "manager", debug::managerBit },
  { "service", debug::serviceBit },
  { "process", debug::processBit },
  { "thread", debug::threadBit },
  { "stio", debug::stioBit },
  { "config", debug::configBit },
  { "reportingChannel", debug::reportingChannelBit },
  { "calibrate", debug::calibrateBit },

  { "oneShot", debug::oneShotBit },

  { "all", 0xFFFFFFFF },

  { 0 }
};


bool
debug::setFlag(const char* name)
{
  return DEBUG.setByName(name, nb);
}

bool
debug::clearFlag(const char* name)
{
  return DEBUG.clearByName(name, nb);
}

std::ostream&
debug::log(std::ostream& s)
{
  timeval now;
  gettimeofday(&now, 0);

  return s << now.tv_sec << "." << std::setfill('0') << std::setw(6) << now.tv_usec << ": ";
}
