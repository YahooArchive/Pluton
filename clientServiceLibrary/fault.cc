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
#include "faultImpl.h"

//////////////////////////////////////////////////////////////////////
// All client and service returns from user-visible APIs make a
// pluton::fault structure available for a detailed explanation of the
// last error return. Various accessors let the caller get at and
// output these details.
//////////////////////////////////////////////////////////////////////


using namespace pluton;

pluton::fault::fault()
{
  _impl = new faultImpl;
}

pluton::fault::~fault()
{
  delete _impl;
}


//////////////////////////////////////////////////////////////////////

bool
pluton::fault::hasFault() const
{
  return _impl->hasFault();
}

pluton::faultCode
pluton::fault::getFaultCode() const
{
  return _impl->getFaultCode();
}


//////////////////////////////////////////////////////////////////////
// Return a nice message explaining the fault.
//////////////////////////////////////////////////////////////////////

const std::string
pluton::fault::getMessage(const std::string& prefix, bool longFormat) const
{
  return _impl->getMessage(prefix.c_str(), longFormat);
}


const std::string
pluton::fault::getMessage(const char* prefix, bool longFormat) const
{
  return _impl->getMessage(prefix, longFormat);
}
