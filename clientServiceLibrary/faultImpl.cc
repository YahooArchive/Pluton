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

#include <string>
#include <iostream>
#include <sstream>

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "faultImpl.h"

//////////////////////////////////////////////////////////////////////
// All returns from user-visible APIs make a pluton::fault structure
// available for a detailed explanation of the last error
// return. Various accessors let the caller get at and output these
// details.
//////////////////////////////////////////////////////////////////////

#include "faultEnglish.cc.txt"

using namespace pluton;

pluton::faultImpl::faultImpl()
  : _hasFault(false),
    _entryName(0),
    _code(noFault), _subCode(0), _errno(0),
    _tx1Size(0), _tx2Size(0), _tx1(0), _tx2(0),
    _moduleName(0), _lineNumber(0)
{
}

pluton::faultImpl::~faultImpl()
{
  if (_tx1) delete _tx1;
  if (_tx2) delete _tx2;
}


//////////////////////////////////////////////////////////////////////
// Take a copy of the char*. Reuse existing space if it's big enough -
// no doubt a premature optimization given that faults are typically
// one-off per process. Return size of buffer.
//////////////////////////////////////////////////////////////////////

void
mystrcpy(char*& dest, int& destSize, const char* src)
{
  int sizeNeeded = strlen(src) + 1;
  if (dest && (sizeNeeded > destSize)) {
    delete dest;
    dest = 0;
  }

  if (!dest) {
    dest = new char[sizeNeeded];
    destSize = sizeNeeded;
  }

  strcpy(dest, src);
}


//////////////////////////////////////////////////////////////////////
// Copy all the fault data into our object.
//////////////////////////////////////////////////////////////////////

bool
pluton::faultImpl::set(faultCode setF, const char* setMN, int setLN,
		       int setSC, int setEN,
		       const char* setTX1, const char* setTX2)
{
  if (setF == noFault) {
    _hasFault = false;
    return true;	// Indicate to caller's caller that the caller succeeded.
  }

  _hasFault = true;
  _code = setF;
  _subCode = setSC;
  _errno = setEN;

  //////////////////////////////////////////////////////////////////////
  // tx1 and tx2 may come from temporary strings, particularly via
  // std::string.c_str(), so they are copied to protect against the
  // string disappearing prior to a subsequent accessor.
  //////////////////////////////////////////////////////////////////////

  if (setTX1 && *setTX1) {
    _tx1Relevant = true;
    mystrcpy(_tx1, _tx1Size, setTX1);
  }
  else {
    _tx1Relevant = false;
  }

  if (setTX2 && *setTX2) {
    _tx2Relevant = true;
    mystrcpy(_tx2, _tx2Size, setTX2);
  }
  else {
    _tx2Relevant = false;
  }

  _moduleName = setMN;
  _lineNumber = setLN;

  return false;		// Indicate failure of the API
}


//////////////////////////////////////////////////////////////////////
// Construct a nice message from the fault data. The longFormat flag
// implies conversion to text for any code that can be converted.
//////////////////////////////////////////////////////////////////////

const std::string
pluton::faultImpl::getMessage(const char* prefix, bool longFormat) const
{
  if (!_hasFault) return "";

  std::ostringstream os;

  if (prefix && *prefix) os << prefix << " ";
  os << "pluton::Fault=" << _code;
  if (_subCode != 0) os << "/" << _subCode;
  if (longFormat) os << " (" << faultCodeToEnglish(_code) << ")";

  if (_errno > 0) {
    os << " errno=" << _errno;
    if (longFormat) os << ":" << strerror(_errno);
  }

  if (_tx1Relevant) os << " '" << _tx1 << "'";
  if (_tx2Relevant) os << " " << _tx2;

  if (longFormat) {
    os << " at ";
    if (_entryName && *_entryName) os << _entryName << "->";
    os << _moduleName << ":" << _lineNumber;
  }

  return os.str();
}
