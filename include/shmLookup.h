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

#ifndef P_SHMLOOKUP_H
#define P_SHMLOOKUP_H

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
