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

#include "pluton/service.h"

#include "swigService.h"


pluton::swig::service::service(const char* yourName)
{
  _pc = new pluton::service(yourName);
}

pluton::swig::service::~service()
{
  delete _pc;
}

const char*
pluton::swig::service::getAPIVersion()
{
  return pluton::service::getAPIVersion();
}

bool
pluton::swig::service::initialize()
{
  return _pc->initialize();
}

bool
pluton::swig::service::hasFault() const
{
  return _pc->hasFault();
}

const pluton::fault&
pluton::swig::service::getFault()
{
  return _pc->getFault();
}

bool
pluton::swig::service::getRequest()
{
  return _pc->getRequest();
}

bool
pluton::swig::service::sendResponse(const std::string& responseData)
{
  return _pc->sendResponse(responseData);
}

bool
pluton::swig::service::sendFault(unsigned int faultCode, const char* faultText)
{
  return _pc->sendFault(faultCode, faultText);
}

void
pluton::swig::service::terminate()
{
  return _pc->terminate();
}

const std::string
pluton::swig::service::getServiceKey() const
{
  std::string s;
  _pc->getServiceKey(s);

  return s;
}

const std::string
pluton::swig::service::getServiceApplication() const
{
  std::string s;
  _pc->getServiceApplication(s);

  return s;
}

const std::string
pluton::swig::service::getServiceFunction() const
{
  std::string s;
  _pc->getServiceApplication(s);

  return s;
}

unsigned int
pluton::swig::service::getServiceVersion() const
{
  return _pc->getServiceVersion();
}

const std::string
pluton::swig::service::getSerializationType() const
{
  std::string s;
  switch (_pc->getSerializationType()) {
  case COBOL: s = "COBOL"; break;
  case HTML: s = "HTML"; break;
  case JSON: s = "JSON"; break;
  case JMS: s = "JMS"; break;
  case NETSTRING: s = "NETSTRING"; break;
  case PHP: s = "PHP"; break;
  case SOAP: s = "SOAP"; break;
  case XML: s = "XML"; break;
  case raw: s = "raw"; break;
  default:
    s = "??"; break;
  }

  return s;
}


const std::string
pluton::swig::service::getClientName() const
{
  std::string s;
  _pc->getClientName(s);

  return s;
}


const std::string
pluton::swig::service::getRequestData() const
{
  std::string s;
  _pc->getRequestData(s);

  return s;
}


const std::string
pluton::swig::service::getContext(const char* key) const
{
  std::string s;
  _pc->getContext(key, s);

  return s;
}
