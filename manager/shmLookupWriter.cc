#include <iostream>
#include "hashString.h"
#include "hash_mapWrapper.h"
#include <string>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "shmLookup.h"
#include "shmLookupPrivate.h"

using namespace std;

#ifndef MAP_NOSYNC
#define MAP_NOSYNC 0
#endif


//////////////////////////////////////////////////////////////////////
// Given an image, create an mmap file and map it in. The first long
// is reserved as a flag to indicate that the map file is current. If
// there is a previous mapping, then invalidate and release that after
// the new image is ready for use.
//////////////////////////////////////////////////////////////////////

const char*
shmLookup::mapWriter(const char* path, const void* image, int imageSize)
{
  _mapFileName = path;

  string tempName = _mapFileName;
  tempName += ".tmp";

  //////////////////////////////////////////////////////////////////////
  // Make sure any residual file is removed as it may have been
  // created and left in 444 mode which will make the write fail.
  // Only complain if the file exists and the unlink fails.
  //////////////////////////////////////////////////////////////////////

  if (unlink(tempName.c_str()) == -1) {
    if (errno != ENOENT) return "unlink of previous .tmp file failed";
  }

  if (unlink(path) == -1) {
    if (errno != ENOENT) return "unlink of previous map file failed";
  }

  int fd = open(tempName.c_str(), O_RDWR|O_CREAT|O_TRUNC, 0444);	// Read Only for everyone
  if (fd == -1) return "open O_CREAT failed";

  int bytesWritten = write(fd, (char*) image, imageSize);
  if (bytesWritten != imageSize) {
    close(fd);
    return "write of image truncated";
  }

  void* shmp = mmap((void*) 0, imageSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NOSYNC, fd, 0);
  if (shmp == MAP_FAILED) {
    close(fd);
    return "mmap returned MAP_FAILED";
  }
  close(fd);

  // Validate new and then invalidate old and release, if present

  relativeHashMap* oldBaseAddress = _baseAddress;
  int oldMapSize = _mapSize;

  _baseAddress = static_cast<relativeHashMap*>(shmp);
  _mapSize = imageSize;
  _baseAddress->_version = relativeHashMap::VERSION;
  _baseAddress->_remapFlag = 'n';

  // New file is now ready for use by clients - make it visible in the file system

  if (rename(tempName.c_str(), _mapFileName.c_str()) == -1) return "rename() failed";

  // If we're replacing an existing map, invalidate it

  if (oldBaseAddress) {
    oldBaseAddress->_remapFlag = 'Y';
    munmap(oldBaseAddress, oldMapSize);
  }

  return 0;
}


//////////////////////////////////////////////////////////////////////
// Give a set of key-value pairs, create a new shared lookup map
//////////////////////////////////////////////////////////////////////

const char*
shmLookup::buildMap(const char* path, const shmLookup::mapType& keysIn)
{
  int stringByteCount = 0;		// Calculate sizes of the various structures
  shmLookup::mapType::const_iterator keysInIter;

  for (keysInIter=keysIn.begin(); keysInIter != keysIn.end(); ++keysInIter) {
    stringByteCount += keysInIter->first.length() + keysInIter->second.length();
  }

  int keyCount = keysIn.size();

  int tableSize = keyCount*2 + 7;	// Allow 100% slop plus make it odd
  int mapSize = sizeof(relativeHashMap) + tableSize * sizeof(relativeHashMap::BUCKETTYPE);

  int entriesSize = sizeof(hashMapEntry) * keyCount;
  entriesSize += sizeof(hashMapEntry);	// The zero entry is used as a fake NULL

  // Initialize the hash

  int imageSize = mapSize+entriesSize+stringByteCount;
  relativeHashMap* mapPtr = (relativeHashMap*) malloc(imageSize);
  mapPtr->_mapSize = imageSize;
  mapPtr->_hashTableSize = tableSize;
  mapPtr->_entryBaseOffset = mapSize;
  mapPtr->_stringBaseOffset = mapSize + entriesSize;

  unsigned int hmeIndex;
  for (hmeIndex=0; hmeIndex < mapPtr->_hashTableSize; hmeIndex++) mapPtr->_hashTable[hmeIndex] = 0;

  // Populate the hash

  rhmPointers H(mapPtr);
  hmeIndex = 1;				// Zero is reserved to NULL
  mapPtr->_maximumEntryIndex = keysIn.size() + 1;
  int nextStringOffset = 0;
  int lastStringOffset = 0;
  for (keysInIter=keysIn.begin(); keysInIter != keysIn.end(); ++keysInIter, ++hmeIndex) {

    if (debug::oneShot()) DBGPRT << "Map Add: K=" << keysInIter->first << " V=" << keysInIter->second << endl;
    hashMapEntry* hmePtr = H.getHmePtr(hmeIndex);		// Next free Entry to use
    hmePtr->_keyOffset = nextStringOffset;
    hmePtr->_keyLength = keysInIter->first.length();
    hmePtr->_nextHMEIndex = 0;

    char* keyPtr = H.getStringPtr(nextStringOffset);
    strncpy(keyPtr, keysInIter->first.c_str(), hmePtr->_keyLength);
    lastStringOffset = nextStringOffset;
    nextStringOffset += hmePtr->_keyLength;

    hmePtr->_valueOffset = nextStringOffset;
    hmePtr->_valueLength = keysInIter->second.length();
    char* valuePtr = H.getStringPtr(nextStringOffset);
    strncpy(valuePtr, keysInIter->second.c_str(), hmePtr->_valueLength);
    lastStringOffset = nextStringOffset;
    nextStringOffset += hmePtr->_valueLength;


    // Add the entry into the hash

    int h = hashData(keyPtr, hmePtr->_keyLength) % mapPtr->_hashTableSize;
    hmePtr->_nextHMEIndex = mapPtr->_hashTable[h];
    mapPtr->_hashTable[h] = hmeIndex;
  }

  const char* res = mapWriter(path, (void*) mapPtr, imageSize);
  free(mapPtr);

  return res;
}


void
shmLookup::dumpMap()
{
  rhmPointers H(_baseAddress);
  DBGPRT << "Hash Tab buckets: " << _baseAddress->_hashTableSize
       << " Version=" << _baseAddress->_version
       << " MaxEntries=" << _baseAddress->_maximumEntryIndex << endl;
  for (unsigned int ix=0; ix < _baseAddress->_hashTableSize; ix++) {
    hashMapEntry* hme = H.getHmePtr(H.getEntryIndex(ix));
    DBGPRT << "HTab: " << ix << ":";
    while (hme) {
      char* key = H.getStringPtr(hme->_keyOffset);
      int len = hme->_keyLength;
      DBGPRT << " K=" << string(key, len)
	     << " V=" << string(H.getStringPtr(hme->_valueOffset), hme->_valueLength);

      hme = H.getHmePtr(hme->_nextHMEIndex);
    }
    DBGPRT << endl;
  }
}
