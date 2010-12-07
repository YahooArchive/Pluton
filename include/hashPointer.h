#ifndef _HASHPOINTER_H
#define _HASHPOINTER_H

//////////////////////////////////////////////////////////////////////
// Define a common hash function for hash_maps using void* pointers as
// their key.
//////////////////////////////////////////////////////////////////////

struct hashPointer
{
  size_t operator()(const void* p) const
  {
    size_t hash = (size_t) p;

    return hash;
  }
};

#endif
