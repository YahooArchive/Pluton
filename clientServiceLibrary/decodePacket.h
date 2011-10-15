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

#ifndef P_DECODEPACKET_H
#define P_DECODEPACKET_H 1


//////////////////////////////////////////////////////////////////////
// A class that decodes incoming netStrings into a
// request/response. The caller is responsible for clearing any
// previous request values prior to passing them to this routine.
//////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>

#include "global.h"
#include "requestImpl.h"

namespace pluton {
  class decodePacket {
  protected:
    decodePacket(pluton::packetType startingType, pluton::requestImpl* R=0);

  public:
    virtual ~decodePacket() = 0;

    void	reset();
    bool	addType(char nsType, const char* nsDataPtr, int nsDataLength,
			int nsDataOffset, std::string& em);
    bool	haveCompleteRequest() const { return _state == haveFullRequest; }

    void 			setRequest(pluton::requestImpl* R) { _requestIn = R; }
    pluton::requestImpl*	getRequest() const { return _requestIn; }
    unsigned int 		getRequestID() const { return _requestID; }

  protected:
    pluton::packetType	_startingType;

  private:
    decodePacket&	operator=(const decodePacket& rhs);	// Assign not ok
    decodePacket(const decodePacket& rhs);			// Copy not ok

    pluton::requestImpl *_requestIn;
    enum { needStartingType, acceptingOthers, haveFullRequest } _state;
    unsigned int	_requestID;

    static const unsigned int	_haveMapSize = 'z' + 1;			// Track which types have
    char			_haveMap[_haveMapSize];			// already been seen
  };

  class decodeRequestPacket : public decodePacket {
  public:
    decodeRequestPacket(pluton::requestImpl* R=0);
    ~decodeRequestPacket();
  };

  class decodeResponsePacket : public decodePacket {
  public:
    decodeResponsePacket(pluton::requestImpl* R=0);
    ~decodeResponsePacket();
  };
}

#endif
