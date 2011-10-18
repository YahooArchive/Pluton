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

#include "config.h"

#include "hashString.h"

#include <string>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "pluton/fault.h"

#include "shmLookup.h"
#include "shmLookupPrivate.h"


//////////////////////////////////////////////////////////////////////
// A shared memory lookup with a "specific-to-general" suffix
// truncation search. If any entry for A.BB.CC.DD exists and an entry
// for mail.getMessage exists, then a lookup of mail.getMessage.1 will
// find mail.getMessage. The lookup simply removes suffixes until it
// finds a match.
//
// There are no arbitrary constraints on how many components there
// are, but note that each truncation search requires a re-hash and
// re-lookup.
//
// Most of this code is dealing with fitting variable length
// structures into a linear allocation model and making sure that all
// pointers are managed as offsets.
//
// Return: -1 if error (faultCode set), >0 for map size
//////////////////////////////////////////////////////////////////////


int
shmLookup::mapReader(const char* path, pluton::faultCode& fc)
{

  // Invalidate the old mapping, if any

  if (_baseAddress) {
    munmap(_baseAddress, _mapSize);
    _baseAddress = 0;
  }
  else {
    _mapFileName = path;
  }

  int fd = open(_mapFileName.c_str(), O_RDONLY, 0);
  if (fd == -1) {
    fc = pluton::openForMmapFailed;
    return -1;
  }

  // Need to know the size to map

  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    close(fd);
    fc = pluton::shmFstatFailed;
    return -1;
  }

  _mapSize = sb.st_size;
  if (_mapSize < (signed) sizeof(relativeHashMap)) {
    close(fd);
    fc = pluton::shmImpossiblySmall;
    return -1;
  }

  void* shmp = mmap((void*) 0, _mapSize, PROT_READ, MAP_SHARED, fd, 0);
  if (shmp == MAP_FAILED) {
    close(fd);
    fc = pluton::shmMmapFailed;
    return -1;
  }
  close(fd);

  _baseAddress = static_cast<relativeHashMap*>(shmp);
  if (_baseAddress->_version != relativeHashMap::SHMVERSION) {
    fc = pluton::shmVersionMisMatch;
    return -1;
  }

  return _mapSize;
}


//////////////////////////////////////////////////////////////////////
// Given a key to lookup, start with the full key then backup each
// component until we get a match. For a key of a.b.c.d, first look up
// a.b.c.d, then a.b.c then a.b and finally a until a match is found.
//
// Return: < 0 error (faultCode set), 0 = not found > 0, length of key
//////////////////////////////////////////////////////////////////////

int
shmLookup::findService(const char* questionPtr, int questionLen,
		       std::string& answer, pluton::faultCode& fc)
{
  if (!_baseAddress) {
    fc = pluton::lookupButNoMap;
    return -1;
  }

  //////////////////////////////////////////////////////////////////////
  // This is not thread-safe, but there is one of these per thread, so
  // we're ok.
  //////////////////////////////////////////////////////////////////////

  if (_baseAddress->_remapFlag != 'n') {
    int res = mapReader(0, fc);
    if (res < 0) return res;
    if (_baseAddress->_remapFlag != 'n') {	// That was a quick remap!
      res = mapReader(0, fc);			// Try one more time
      if (res < 0) return res;
      if (_baseAddress->_remapFlag != 'n') {	// This is getting ridiculous so..
	fc = pluton::persistentRemapFlag;	// ..give up.
	return -1;
      }
    }
  }

  rhmPointers H(_baseAddress);
  const char* cp = questionPtr;
  int compareLength = questionLen;

  // While we have at least one question component

  while (compareLength > 0) {

    // Find the hash bucket

    int ix = hashData(cp, compareLength) % _baseAddress->_hashTableSize;
    int hi = H.getEntryIndex(ix);
    hashMapEntry* hme = H.getHmePtr(hi);

    // Walk the link-list of matching entries

    while (hme) {
      char* key = H.getStringPtr(hme->_keyOffset);
      int keyLength = hme->_keyLength;

      // If this entry matches the question, return the answer.

      if ((keyLength == compareLength) && (strncmp(cp, key, keyLength) == 0)) {
	answer.assign(H.getStringPtr(hme->_valueOffset), hme->_valueLength);
	return answer.length();
      }

      hme = H.getHmePtr(hme->_nextHMEIndex);	      // Otherwise, get next in link list
    }

    // Not found with this many components, backup one component and try again.

    for (compareLength--; compareLength > 0; compareLength--) {
      if (cp[compareLength] == '.') break;
    }
  }

  answer.erase();
  fc = pluton::serviceNotFound;

  return 0;
}
