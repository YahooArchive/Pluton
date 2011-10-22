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

#ifndef _PLUTON_SWIG_SERVICE_H
#define _PLUTON_SWIG_SERVICE_H

//////////////////////////////////////////////////////////////////////
// A simple non-derived wrapper to pluton::service. The sole purpose
// of this interface is to be easily parsed by swig so that a perl
// wrapper can be auto-generated.
//////////////////////////////////////////////////////////////////////

#include <string>

namespace pluton {
  class service;
  namespace swig {

    class service {
    private:
      service& operator=(const service& rhs);       // Assign not ok
      service(const service& rhs);                  // Copy not ok

    public:
      service(const char* yourName="");
      ~service();

      static const char*  	getAPIVersion();
      bool			initialize();
      bool			hasFault() const;
      const pluton::fault&	getFault();

      bool	getRequest();
      bool	sendResponse(const std::string& responseData);

      bool	sendFault(unsigned int faultCode, const char* faultText);

      void	terminate();

      //////////////////////////////////////////////////////////////////////
      // serviceRequest proxy methods
      //////////////////////////////////////////////////////////////////////

      const std::string			getServiceKey() const;
      const std::string			getServiceApplication() const;
      const std::string			getServiceFunction() const;
      unsigned int			getServiceVersion() const;
      const std::string		 	getSerializationType() const;

      const std::string			getClientName() const;

      const std::string			getRequestData() const;

      const std::string			getContext(const char* key) const;

    private:
      pluton::service*			_pc;
    };
  }
}

#endif
