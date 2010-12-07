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

#ifndef _PLUTON_CLIENTREQUEST_H
#define _PLUTON_CLIENTREQUEST_H

#include <string>

//////////////////////////////////////////////////////////////////////
// Define the clientRequest APIs for the Pluton Framework
//////////////////////////////////////////////////////////////////////

#include "pluton/common.h"
#include "pluton/fault.h"

namespace pluton {
  class clientRequestImpl;

  class clientRequest {
  public:
    clientRequest();
    virtual 	~clientRequest();	// Callers can derive this class so they can wrap
					// it in a container with other stuff
    void	reset();

    //////////////////////////////////////////////////////////////////////
    // Various signatures for adding requests
    //////////////////////////////////////////////////////////////////////

    void	setRequestData(const char* p);	// Library will strlen(p)
    void	setRequestData(const char* p, int len);
    void	setRequestData(const std::string& request);

    void	setAttribute(pluton::requestAttributes);
    bool	getAttribute(pluton::requestAttributes);
    void	clearAttribute(pluton::requestAttributes);

    bool	setContext(const char* key, const std::string& value);


    pluton::faultCode	getResponseData(const char*& responsePointer, int& responseLength) const;
    pluton::faultCode	getResponseData(std::string& responseData) const;

    //////////////////////////////////////////////////////////////////////
    // Give plenty of options to access fault information. We hardly
    // want the lack of APis to be used as an excuse for not checking
    // and reporting on faults now, do we?
    //////////////////////////////////////////////////////////////////////

    bool		inProgress() const;		// If being processed by the client API

    bool		hasFault() const;		// otherwise it'll be a fault
    pluton::faultCode	getFault(std::string& faultText) const;
    pluton::faultCode	getFaultCode() const;
    std::string		getFaultText() const;
    std::string 	getServiceName() const;

    //////////////////////////////////////////////////////////////////////
    // The caller can associate data with a given request and gain
    // acess to it as needed. This is likely to be of most value when
    // using the executeAndWaitAny() method as the caller can
    // associate the request with some other data structure of theirs.
    //////////////////////////////////////////////////////////////////////

    void		setClientHandle(void*);		// Users can store
    void*		getClientHandle() const;	// anything here


    //////////////////////////////////////////////////////////////////////
    // End of user methods.
    //////////////////////////////////////////////////////////////////////

    pluton::clientRequestImpl*		getImpl() { return _impl; }	// Ugly
    const pluton::clientRequestImpl*	getImpl() const { return _impl; }

  private:
    clientRequest&	operator=(const clientRequest& rhs);	// Assign not ok
    clientRequest(const clientRequest& rhs);			// Copy not ok

    pluton::clientRequestImpl*	_impl;
  };
}

#endif
