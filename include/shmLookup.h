#ifndef _SHMLOOKUP_H
#define _SHMLOOKUP_H

#include <string>
#include "hash_mapWrapper.h"

#include "pluton/fault.h"

#include "hashString.h"

//////////////////////////////////////////////////////////////////////
// This class manages access to the shared memory segment that
// contains the lookup table for mapping services names to connect()
// paths.
//////////////////////////////////////////////////////////////////////


class relativeHashMap;

class shmLookup {
 public:
  shmLookup();
  ~shmLookup();

  typedef hash_map<std::string, std::string, hashString> mapType;

  const char*	buildMap(const char* path, const mapType&);

  int		mapReader(const char* path, pluton::faultCode&);
  int		findService(const char* questionPtr, int questionLen,
			    std::string& servicePath, pluton::faultCode&);

  void		dumpMap();

 private:
  const char*	mapWriter(const char* path, const void* image, int imageSize);

  static unsigned int	hashData(const char*, int len);

  std::string		_mapFileName;
  int			_mapSize;
  relativeHashMap* 	_baseAddress;
};

#endif
