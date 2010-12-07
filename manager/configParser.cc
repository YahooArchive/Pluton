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

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include "istreamWrapper.h"
#include <string>

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "debug.h"
#include "configParser.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Parse the service config file and populate the config values. A
// hashmap is used to detect duplicate and missing parameters.
//////////////////////////////////////////////////////////////////////


bool
configParser::found(const std::string& name) const
{
  paramConstIter pi = paramMap.find(name);
  if (pi == paramMap.end()) return false;

  if (pi->second.lineNumber == 0) return false;		// Default replaced it

  return true;
}


bool
configParser::getString(const std::string& name, const std::string& defaultValue,
			string& newValue, string& error)
{
  newValue = defaultValue;
  paramIter pi = paramMap.find(name);
  if (pi == paramMap.end()) {
    paramMap[name].fetched = true;
    paramMap[name].lineNumber = 0;
    paramMap[name].value = defaultValue;
    return false;
  }
  pi->second.fetched = true;

  newValue = pi->second.value;

  return false;
}


//////////////////////////////////////////////////////////////////////

static const char*
convertBool(const char* cp, bool& newValue)
{
  if (strcasecmp(cp, "true") == 0) {
    newValue = true;
    return 0;
  }

  if (strcasecmp(cp, "false") == 0) {
    newValue = false;
    return 0;
  }

  return "Invalid boolean. Expect 'true' or 'false'";
}

bool
configParser::getBool(const std::string& name, bool defaultValue, bool& newValue, string& error)
{
  newValue = defaultValue;
  paramIter pi = paramMap.find(name);
  if (pi == paramMap.end()) {
    paramMap[name].fetched = true;
    paramMap[name].lineNumber = 0;
    paramMap[name].value = defaultValue ? "true" : "false";
    return false;
  }
  pi->second.fetched = true;

  const char* err = convertBool(pi->second.value.c_str(), newValue);
  if (!err) return false;

  ostringstream os;
  os << "Config Error: Line " << pi->second.lineNumber << ": " << name << ": " << err;
  error = os.str();

  return true;
}


//////////////////////////////////////////////////////////////////////

static const char*
convertNumber(const char* start, long& newValue)
{
  long	lnum;
  char* endptr;
  lnum = strtol(start, &endptr, 10);
  if (endptr == start) {		// No valid digits
    return "Invalid number. Expect a digit";
  }

  newValue = lnum;

  // lnum has the converted value - apply multipliers

  while (*endptr) {
    long multiplier = 1;
    switch (*endptr) {		// Look for multipliers
    case 'k': case 'K':	multiplier = 1000; break;
    case 'm': case 'M':	multiplier = 1000000; break;
    case 'g': case 'G':	multiplier = 1000000000; break;
      //  case 't': case 'T':	multiplier = 1000000000000; break;
      //  case 'p': case 'P':	multiplier = 1000000000000000; break;

    default:
      //    return "Invalid multiplier. Expect K, M, G, T or P";
      return "Invalid multiplier. Expect K, M, or G";
    }

    newValue = lnum * multiplier;
    if ((newValue / multiplier) != lnum) return "Multiplier causes overflow";
    lnum = newValue;

    ++endptr;
  }

  return 0;
}


//////////////////////////////////////////////////////////////////////
// Return: true on error
//////////////////////////////////////////////////////////////////////

bool
configParser::getNumber(const std::string& name, long defaultValue, long& newValue, string& error)
{
  newValue = defaultValue;

  paramIter pi = paramMap.find(name);
  if (pi == paramMap.end()) {		// Not found, use default
    ostringstream os;
    os << defaultValue;
    paramMap[name].fetched = true;
    paramMap[name].lineNumber = 0;
    paramMap[name].value = os.str();
    return false;
  }

  pi->second.fetched = true;

  const char* err = convertNumber(pi->second.value.c_str(), newValue);

  if (!err) return false;

  ostringstream os;
  os << "Config Error: Line " << pi->second.lineNumber << ": " << name << ": " << err;
  error = os.str();

  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Crude config file parser.
// Ignores blank lines, ignores lines that start with #
// Accepts: key anyvalue
////////////////////////////////////////////////////////////////////////////////

bool
configParser::loadFile(const std::string& filename, std::string& error)
{
  std::ifstream ifile(filename.c_str());
  if (! ifile.good()) {
    error = "Config Error: Could not open config file: ";
    error += filename;
    return true;
  }

  if (debug::config()) DBGPRT << "Config Loading " << filename << endl;
  return loadStream(filename, ifile, error);
}


bool
configParser::loadString(const std::string& filename, const std::string& s, std::string& error)
{
  istringstream ist(s);
  return loadStream(filename, ist, error);
}


bool
configParser::loadStream(const string& filename, std::istream& is, std::string& error)
{
  configPath = filename;
  string lb;
  ostringstream os;
  int lineNo = 0;

  while (getline(is, lb).good()) {

    ++lineNo;
    string::size_type ix;

    if ((ix = lb.find('#')) != string::npos) {	  // Remove everything from # to end
      lb.erase(ix, lb.size());
    }
    if ((ix = lb.find_first_not_of(" \t")) != string::npos) {	// Trim leading white spaces
      lb.erase(0, ix);
    }

    if (lb.empty()) continue;			// Ignore empty lines

    if (debug::config()) DBGPRT << "Config " << lineNo << " " << lb << endl;
    string paramone;
    string paramtwo;

    // Skip over leading whitespace

    if ((ix = lb.find_first_of(" \t")) != string::npos) {
      paramone = lb.substr(0, ix);
      paramtwo = lb.substr(ix, lb.size());
    }
    else {
      paramone = lb;
    }

    if (debug::config()) DBGPRT << "p1=" << paramone << " p2=" << paramtwo << endl;

    // Already present?

    if (paramMap.find(paramone) != paramMap.end()) {
      os << "Config Error: Line " << lineNo << ": Duplicate parameter '" << paramone
	 << "' at line " << paramMap[paramone].lineNumber;
      error = os.str();
      return true;
    }

    // Trim leading and trailing spaces

    if ((ix = paramtwo.find_first_not_of(" \t")) != string::npos) paramtwo.erase(0, ix);
    if ((ix = paramtwo.find_last_not_of(" \t")) != string::npos) paramtwo.erase(ix+1);

    // Add into the map

    paramMap[paramone].fetched = false;
    paramMap[paramone].lineNumber = lineNo;
    paramMap[paramone].value = paramtwo;
    if (debug::config()) DBGPRT << "Config set " << paramone << "=" << paramtwo << "<<" << endl;
  }

  return false;
}


//////////////////////////////////////////////////////////////////////
// The first unfetched usually means an unrecognized config name
//////////////////////////////////////////////////////////////////////

string
configParser::getFirstUnfetched()
{
  paramIter ix;
  for (ix=paramMap.begin(); ix != paramMap.end(); ++ix) {
    if (ix->second.fetched == false) return ix->first;
  }

  return "";
}


//////////////////////////////////////////////////////////////////////
// Debug routine. Dump the config to stdout
//////////////////////////////////////////////////////////////////////

void
configParser::dump()
{
  paramIter ix;

  DBGPRT << "Debug: Configuration dump from " << configPath << endl;

  for (ix=paramMap.begin(); ix != paramMap.end(); ++ix) {
    DBGPRT << "Debug: Config " << ix->first << "=" << ix->second.value;
    if (ix->second.lineNumber > 0) DBGPRT << " (from line " << ix->second.lineNumber << ")";
    DBGPRT << endl;
  }
}
