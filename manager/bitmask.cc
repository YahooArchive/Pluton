#include <strings.h>

#include "bitmask.h"

//////////////////////////////////////////////////////////////////////
// The debug and logging options are maintained via a bitmask.
//////////////////////////////////////////////////////////////////////

bool
bitmask::setByName(const char* setName, nameBitMap* nb)
{
  for (bitmask::nameBitMap* mp = nb; mp->name; ++mp) {
    if (strcasecmp(setName, mp->name) == 0) {
      set(mp->bitNumber);
      return true;
    }
  }

  return false;
}

bool
bitmask::clearByName(const char* setName, nameBitMap* nb)
{
  for (bitmask::nameBitMap* mp = nb; mp->name; ++mp) {
    if (strcasecmp(setName, mp->name) == 0) {
      clear(mp->bitNumber);
      return true;
    }
  }

  return false;
}
