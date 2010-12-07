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

#ifndef _CONFIGPARSER_H
#define _CONFIGPARSER_H

#include <string>
#include <sstream>
#include <map>

// Yes, there must be 10 million of these classes and 9.9 million of them
// are better than this one...

class configParser {

 public:
  bool	loadFile(const std::string& filename, std::string& error);
  bool	loadString(const std::string& filename, const std::string& s, std::string& error);

  bool	getBool(const std::string& name, bool defaultValue, bool& newValue, std::string& error);
  bool 	getNumber(const std::string& name, long defaultValue, long& newValue, std::string& error);
  bool	getString(const std::string& name, const std::string& defaultValue,
		  std::string& newValue, std::string& error);
  bool	found(const std::string& name) const;

  std::string	getFirstUnfetched();		// Return a config value not fetched

  void		dump();

 private:
  bool 	loadStream(const std::string& filename, std::istream& is, std::string& error);

  std::string	configPath;

  struct parameter {
    bool	fetched;
    int		lineNumber;
    std::string	value;
  };

  std::map<std::string, parameter>				paramMap;
  typedef std::map<std::string, parameter>::iterator		paramIter;
  typedef std::map<std::string, parameter>::const_iterator	paramConstIter;
};

#endif
