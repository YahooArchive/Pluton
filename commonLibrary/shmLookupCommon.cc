#include <sys/types.h>
#include <sys/mman.h>

#include "shmLookup.h"


shmLookup::shmLookup() : _mapSize(0), _baseAddress(0)
{
}


shmLookup::~shmLookup()
{
  if (_baseAddress) munmap(_baseAddress, _mapSize);
}


unsigned int
shmLookup::hashData(const char* cp, int len)
{
  unsigned int hash = 0;
  while (len-- > 0) {
    hash += (hash << 5);
    hash ^= (unsigned char) *cp++;
  }
  return hash;
}
