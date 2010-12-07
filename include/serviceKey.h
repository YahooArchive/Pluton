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

#ifndef _SERVICEKEY_H
#define _SERVICEKEY_H

#include <string>

#include "pluton/common.h"

//////////////////////////////////////////////////////////////////////
// Encapsulate the serviceKey definitions as they are vulnerable to
// change as the design evolves.
//////////////////////////////////////////////////////////////////////

namespace pluton {
  class serviceKey {
  public:
    serviceKey(const std::string& name="", const std::string& function="",
	       pluton::serializationType sType=pluton::raw,
	       unsigned int sVersion=0);

    void	setApplication(const std::string& newApplication)
      {
	_applicationStr = newApplication;
	_regenEnglish = true;
      }
    void	setApplication(const char* p, int l)
      {
	_applicationStr.assign(p, l);
	_regenEnglish = true;
      }

    void	setFunction(const std::string& newFunction)
      {
	_functionStr = newFunction;
	_regenEnglish = true;
      }

    void	setFunction(const char* p, int l)
      {
	_functionStr.assign(p, l);
	_regenEnglish = true;
      }

    void	setVersion(unsigned int newVersion)
      {
	_version = newVersion;
	_regenEnglish = true;
      }
    void 	setSerialization(pluton::serializationType newS=pluton::raw)
      {
	_serialization = newS;
	_regenEnglish = true;
      }

    void	erase();

  // Getters

    const std::string&		getApplication() const { return _applicationStr; }
    const std::string&		getFunction() const { return _functionStr; }
    pluton::serializationType	getSerialization() const { return _serialization; }
    unsigned int		getVersion() const { return _version; }

    std::string			getEnglishKey() const;
    void			getSearchKey(std::string& skLong) const;
    const char*			parse(const char*, int, bool clientFlag=true);
    const char*			parse(const std::string& key, bool clientFlag=true)
      {
	return parse(key.data(), key.length(), clientFlag);
      }

  private:
    std::string			_applicationStr;
    std::string			_functionStr;
    pluton::serializationType	_serialization;
    unsigned int		_version;

    mutable bool		_regenEnglish;
    mutable std::string		_englishKey;
  };
}

#endif
