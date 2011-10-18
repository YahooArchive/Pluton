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

#include <sstream>

#include <string>

#include "misc.h"
#include "serviceKey.h"

//////////////////////////////////////////////////////////////////////
// Application.Function.Version.Serialization
//////////////////////////////////////////////////////////////////////

pluton::serviceKey::serviceKey(const std::string& sName, const std::string& sFunction,
			       pluton::serializationType sType, unsigned int sVersion)
  : _applicationStr(sName), _functionStr(sFunction), _serialization(sType), _version(sVersion),
    _regenEnglish(true)
{
}


void
pluton::serviceKey::erase()
{
  _applicationStr.erase();
  _functionStr.erase();
  _serialization = pluton::raw;
  _version = 0;
  _regenEnglish = true;
}


//////////////////////////////////////////////////////////////////////
// Return a nicely formatted key string. Optimize by only creating the
// returned string as needed. Probably a bogus optimization.
//////////////////////////////////////////////////////////////////////

std::string
pluton::serviceKey::getEnglishKey() const
{
  if (!_regenEnglish) return _englishKey;

  std::ostringstream os;
  os << _applicationStr
     << "."
     << _functionStr
     << "."
     << _version
     << "."
     << pluton::misc::serializationTypeToEnglish(_serialization)
    ;

  _englishKey = os.str();
  _regenEnglish = false;

  return _englishKey;
}


//////////////////////////////////////////////////////////////////////
// Return keys formatted for a wild-card prefix-first search
//////////////////////////////////////////////////////////////////////

void
pluton::serviceKey::getSearchKey(std::string& sk) const
{
  std::ostringstream os;
  os << _applicationStr
     << "."
     << pluton::misc::serializationTypeToEnglish(_serialization)
     << "."
     << _version;

  if (!_functionStr.empty()) os << "." << _functionStr;
  sk = os.str();
}


//////////////////////////////////////////////////////////////////////
// Split a string into the component parts of a Service Key. Return a
// pointer to an error message or NULL if ok. If it's a client
// calling, all parameters must be supplied. If it's a
// service/configuration caller, allow an empty function name.
//////////////////////////////////////////////////////////////////////

const char*
pluton::serviceKey::parse(const char *cp, int bytesRemaining, bool clientFlag)
{
  _englishKey.assign(cp, bytesRemaining);
  _regenEnglish = false;

  const int maxTokens = 4;

  const int applicationIX = 0;		// Define the order of the components
  const int functionIX = 1;
  const int versionIX = 2;
  const int serializationIX = 3;

  const char* tokens[maxTokens];
  int tokenLength[maxTokens];

  int ix = 0;
  while (ix < maxTokens) {
    tokens[ix] = cp;
    int bytes = 0;
    while ((bytesRemaining > 0) && (*cp != '.')) {
      --bytesRemaining;
      ++bytes;
      ++cp;
    }
    tokenLength[ix] = bytes;
    ++ix;
    if (bytesRemaining > 0) {
      if (ix == maxTokens) return "invalid syntax - more than four dot-separated tokens";
      ++cp;	// Skip over dot
      --bytesRemaining;
    }
    else if (ix < maxTokens) return "invalid syntax - need four dot-separated tokens";
  }


  //////////////////////////////////////////////////////////////////////
  // Application Name - must be present and length between 1-32
  //////////////////////////////////////////////////////////////////////

  if ((tokenLength[applicationIX] < 1) || (tokenLength[applicationIX] > 32)) {
    return "Application Name length is not in the range 1-32 characters";
  }
  _applicationStr.assign(tokens[applicationIX], tokenLength[applicationIX]);


  //////////////////////////////////////////////////////////////////////
  // Service Function - must be present for client and less than or
  // equal to 32 for everyone.
  //////////////////////////////////////////////////////////////////////

  if (clientFlag && (tokenLength[functionIX] == 0)) return "Service Function is zero length";
  if (tokenLength[functionIX] > 32) return "Service Function length is greater than 32 characters";
  _functionStr.assign(tokens[functionIX], tokenLength[functionIX]);


  //////////////////////////////////////////////////////////////////////
  // Version - must be numeric and of length 1-6
  //////////////////////////////////////////////////////////////////////

  if ((tokenLength[versionIX] < 1) || (tokenLength[versionIX] > 6)) {
    return "Service Version length is not in the range 1-6 characters";
  }
  if ((tokenLength[versionIX] > 1) && (tokens[versionIX][0] == '0')) {
    return "version has ambiguous leading zero";
  }

  _version = 0;
  cp = tokens[versionIX];
  int len = tokenLength[versionIX];
  while (len > 0) {
    if (!isdigit(*cp)) return "version is not a positive integer";

    _version *= 10;
    _version += *cp - '0';
    ++cp;
    --len;
  }

  //////////////////////////////////////////////////////////////////////
  // Serialization Type - must be a known type
  //////////////////////////////////////////////////////////////////////

  if (tokenLength[serializationIX] == 0) return "serialization is zero length";
  _serialization = pluton::raw;
  _serialization = pluton::misc::englishToSerializationType(tokens[serializationIX],
							    tokenLength[serializationIX]);
  if (_serialization == pluton::unknownSerialization) return "serialization type is unrecognized";


  return 0;
}
