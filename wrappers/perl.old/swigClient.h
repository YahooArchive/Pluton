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

#ifndef _PLUTON_SWIG_CLIENT_H
#define _PLUTON_SWIG_CLIENT_H

//////////////////////////////////////////////////////////////////////
// A simple non-derived wrapper to pluton::client. The sole purpose of
// this interface is to be easily parsed by swig so that a perl
// wrapper can be auto-generated.
//////////////////////////////////////////////////////////////////////

#include <pluton/common.h>

#include <string>

namespace pluton {
  class client;
  class clientRequest;
  namespace swig {

    class clientRequest {
    private:
      clientRequest& operator=(const clientRequest& rhs);	// Assign not ok
      clientRequest(const clientRequest& rhs); 		// Copy not ok

    public:
      clientRequest();
      ~clientRequest();

      void	setRequestData(const std::string request);

      void 	setAttribute(pluton::requestAttributes);
      bool 	getAttribute(pluton::requestAttributes);
      void 	clearAttribute(pluton::requestAttributes);

      bool 	setContext(const char* key, const std::string value);

      std::string	getResponseData() const;	// Differs from C++ version

      bool 		inProgress() const;
      bool 		hasFault() const;
      pluton::faultCode getFaultCode() const;
      std::string 	getFaultText() const;
      std::string 	getServiceName() const;

      pluton::clientRequest*	getPCR() { return _pcr; }

    private:
      std::string		_copyRequest;
      pluton::clientRequest*	_pcr;
    };

    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////

    class client {
    private:
      client& operator=(const client& rhs);       // Assign not ok
      client(const client& rhs);                  // Copy not ok

    public:
      client(const char* yourName="", unsigned int defaultTimeoutMilliSeconds=4000);
      ~client();

      static const char*  	getAPIVersion();
      bool			initialize(const char* lookupMapPath);
      bool			hasFault() const;
      const pluton::fault&	getFault();

      void 		setDebug(bool nv);

      void		setTimeoutMilliSeconds(unsigned int timeoutMilliSeconds);
      unsigned int	getTimeoutMilliSeconds() const;

      bool		addRequest(const char* serviceKey, pluton::swig::clientRequest* request);

      int                         executeAndWaitSent();
      int                         executeAndWaitAll();
      pluton::swig::clientRequest*      executeAndWaitAny();
      int                         executeAndWaitOne(pluton::swig::clientRequest*);


    private:
      pluton::client*	_pc;
    };
  }
}

#endif
