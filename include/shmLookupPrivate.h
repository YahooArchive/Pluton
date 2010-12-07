/*
Copyright (c) 2010, Yahoo! Inc. All rights reserved.

Redistribution and use of this software in source and binary forms, with or
without modification, are permitted provided that the following conditions are
met: 

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation and/or 
other materials provided with the distribution.

* Neither the name of Yahoo! Inc. nor the names of its contributors may be used 
to endorse or promote products derived from this software without specific prior 
written permission of Yahoo! Inc.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

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
