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

#ifndef P_FAULTIMPL_H
#define P_FAULTIMPL_H 1

//////////////////////////////////////////////////////////////////////
// The fault class is returned to the caller so they can get extensive
// information about the fault returned. Mainly it's a container of
// return codes, errno values, textual matter and the code location of
// the fault.
//////////////////////////////////////////////////////////////////////

#include "pluton/fault.h"

namespace pluton {

  class faultImpl {
  public:
    faultImpl();
    ~faultImpl();
    void inline clear(const char* entry)
      {
	_hasFault = false;
	if (entry && *entry) _entryName = entry;
      }

    bool	set(faultCode setF, const char* setMN, int setLN,
		    int setSC=0, int setEN=0,
		    const char* setTX1=0, const char* setTX2=0);

    bool	hasFault() const { return _hasFault; }
    faultCode 	getFaultCode() const { return _hasFault ? _code : pluton::noFault; }

    const std::string	getMessage(const char* pre=0, bool longFormat=true) const;

  private:
    bool	_hasFault;
    const char*	_entryName;
    faultCode	_code;
    int		_subCode;	// Returned from subroutine within caller
    int		_errno;		// Copy of the system value in <errno.h>

    int		_tx1Size;	// Max size of space in tx1, tx2
    int		_tx2Size;
    bool	_tx1Relevant;	// If tx1 and tx2 are relevant to current fault
    bool	_tx2Relevant;
    char*	_tx1;
    char*	_tx2;

    const char*	_moduleName;	// Defaults to __FILE__ and __LINE__
    int		_lineNumber;
  };

  class faultInternal : public pluton::fault {
  public:
    ~faultInternal() {};

    bool	set(faultCode setF, const char* setMN, int setLN,
		    int setSC=0, int setEN=0,
		    const char* setTX1=0, const char* setTX2=0)
      {
	return _impl->set(setF, setMN, setLN, setSC, setEN, setTX1, setTX2);
      }

    void inline clear(const char* entry) { _impl->clear(entry); }
  };
}

#endif
