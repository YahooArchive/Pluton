#ifndef _SHMLOOKUPPRIVATE_H
#define _SHMLOOKUPPRIVATE_H

#include "stdintWrapper.h"

//////////////////////////////////////////////////////////////////////
//
// Private structures that define the shared memory layout for the
// service lookup.
//
// The shared memory layout looks like this:
//
//	+-------------------------+
//	| relativeHashMap         |	mapPtr, mapSize
//	+-------------------------+
//	| hashMapEntry * keyCount | 	entryBasePtr, entrySize
//	/			  /
//	\			  /
//	+-------------------------+
//	| Key1Value1Key2Value2Keyn|	stringBasePtr
//	+-------------------------+
//
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// The relativeHashMap is the base of the shmem lookup structure. It
// is primarily characteristics of the lookup map and the array of
// hash buckets. It's important to define the types to be of a
// specific size as, even though this structure is only ever visible
// to executables on a single CPU, there is the question of 32bit and
// 64bit applications sharing the same data structure.
//////////////////////////////////////////////////////////////////////

class relativeHashMap {
 public:
  static const uint32_t VERSION = 1001;

  uint32_t	_version;
  char 		_remapFlag;
  uint32_t	_hashTableSize;
  uint32_t	_maximumEntryIndex;
  uint32_t	_mapSize;
  uint32_t	_entryBaseOffset;
  uint32_t	_stringBaseOffset;
  typedef uint32_t BUCKETTYPE;
  BUCKETTYPE	_hashTable[1];
};


//////////////////////////////////////////////////////////////////////
// Each valid hash table entry is represented by an hashMapEntry which
// contains details about the hash key and value.
//////////////////////////////////////////////////////////////////////

class hashMapEntry {
 public:
  int32_t	_keyOffset;		// Where does the key start
  int32_t	_keyLength;		// How long is the key
  int32_t	_valueOffset;		// Ditto for the value
  int32_t	_valueLength;
  int32_t	_nextHMEIndex;
};


//////////////////////////////////////////////////////////////////////
// rhmPointers provides various relative-to-absolute conversion
// functions.
//////////////////////////////////////////////////////////////////////

class rhmPointers {
 public:
  rhmPointers(relativeHashMap* setBase) : _mapBase(setBase)
    {
      char* cpBase = (char *) _mapBase;
      _hmeBase = (hashMapEntry*) (cpBase + _mapBase->_entryBaseOffset);
      _stringBase = cpBase + _mapBase->_stringBaseOffset;
    }

  relativeHashMap*	getHashMapPtr() const { return _mapBase; }
  unsigned int		getEntryIndex(unsigned int ix) const { return _mapBase->_hashTable[ix]; }

  hashMapEntry*	getHmePtr(unsigned int ix) const
    {
      if (ix == 0) return 0;
      if (ix > _mapBase->_maximumEntryIndex) return 0;
      return _hmeBase + ix;
    }
  char*		getStringPtr(unsigned int offset) const
    {
      if (offset >= _mapBase->_mapSize) return 0;
      return _stringBase + offset;
    }

 private:
  relativeHashMap* 	_mapBase;
  hashMapEntry*		_hmeBase;
  char*			_stringBase;
};

#endif
