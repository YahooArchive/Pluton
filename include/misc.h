#ifndef _PLUTON_MISC_H
#define _PLUTON_MISC_H

//////////////////////////////////////////////////////////////////////
// Static functions unique to pluton.
//////////////////////////////////////////////////////////////////////

#include <ostreamWrapper.h>

#define DBGPRT      if (_debugFlag) pluton::misc::logtime(std::clog)
#define DBGPRTMORE  if (_debugFlag) std::clog

#include "pluton/common.h"

#include "global.h"

namespace pluton {
  namespace misc {

    const char*	packetToEnglish(pluton::packetType);
    const char*	NSTypeToEnglish(pluton::netStringType);

    const char*	serializationTypeToEnglish(pluton::serializationType);
    pluton::serializationType englishToSerializationType(const char*, int);
    bool		validateKey(const char* key);

    std::ostream&	logtime(std::ostream&);
  }
}

#endif
