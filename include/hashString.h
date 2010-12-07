#ifndef _HASHSTRING_H
#define _HASHSTRING_H

//////////////////////////////////////////////////////////////////////
// Define a common hash function for hash_maps using std::string as
// their key.
//////////////////////////////////////////////////////////////////////

#include <string>

struct hashString
{
  size_t operator()(const std::string key) const
  {
    size_t hash = 0;
    for (std::string::const_iterator si = key.begin(); si != key.end(); ++si) {
      hash += (hash << 5);
      hash ^= (unsigned char) *si;
    }
    return hash;
  }
};

#endif
