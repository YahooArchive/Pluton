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

#include <string.h>

#include "version.h"
#include "pluton/fault.h"
#include "pluton/service.h"
#include "serviceImpl.h"

//////////////////////////////////////////////////////////////////////
// All user visible classes are encapsulated in a wrapper class to
// protect against binary compatability.
//////////////////////////////////////////////////////////////////////

int
pluton::service::setInternalAttributes(int attributes)
{
  return pluton::serviceImpl::setInternalAttributes(attributes);
}

pluton::service::service(const char* name)
  {
    _impl = new serviceImpl;
    _impl->setDefaultRequest(new pluton::serviceRequestImpl);
    _impl->setDefaultOwner(new pluton::perCallerService(name));
  }

pluton::service::~service()
{
  delete _impl->getDefaultOwner();
  delete _impl->getDefaultRequest();
  delete _impl;
}

const char*
pluton::service::getAPIVersion()
{
  return version::rcsid;
}

bool
pluton::service::initialize()
{
  _impl->getDefaultOwner()->initialize();
  return _impl->initialize(_impl->getDefaultOwner());
}

bool
pluton::service::hasFault() const
{
  return _impl->getDefaultOwner()->hasFault();
}

const pluton::fault&
pluton::service::getFault() const
{
  return _impl->getDefaultOwner()->getFault();
}

bool
pluton::service::getRequest()
{
  return _impl->getRequest(_impl->getDefaultOwner(), _impl->getDefaultRequest());
}

bool
pluton::service::sendResponse(const char* responsePointer)
{
  int responseLength = strlen(responsePointer);
  requestImpl* R = _impl->getDefaultRequest();
  R->setFault(pluton::noFault);
  R->setResponseData(responsePointer, responseLength);
  return _impl->sendResponse(_impl->getDefaultOwner(), R);
}

bool
pluton::service::sendResponse(const char* responsePointer, int responseLength)
{
  requestImpl* R = _impl->getDefaultRequest();
  R->setFault(pluton::noFault);
  R->setResponseData(responsePointer, responseLength);
  return _impl->sendResponse(_impl->getDefaultOwner(), R);
}

bool
pluton::service::sendResponse(const std::string& responseData)
{
  requestImpl* R = _impl->getDefaultRequest();
  R->setFault(pluton::noFault);
  R->setResponseData(responseData);
  return _impl->sendResponse(_impl->getDefaultOwner(), R);
}

bool
pluton::service::sendFault(unsigned int faultCode, const char* faultText)
{
  requestImpl* R = _impl->getDefaultRequest();
  R->setFault(static_cast<pluton::faultCode>(faultCode), faultText);
  R->setResponseData("", 0);
  return _impl->sendResponse(_impl->getDefaultOwner(), R);
}

void
pluton::service::terminate()
{
  _impl->terminate();
}


//////////////////////////////////////////////////////////////////////
// serviceRequest proxies
//////////////////////////////////////////////////////////////////////

const std::string&
pluton::service::getServiceKey(std::string& sk) const
{
  _impl->getDefaultRequest()->getServiceKey(sk);
  return sk;
}

const std::string&
pluton::service::getServiceApplication(std::string& sa) const
{
  _impl->getDefaultRequest()->getServiceApplication(sa);
  return sa;
}

const std::string&
pluton::service::getServiceFunction(std::string& sf) const
{
  _impl->getDefaultRequest()->getServiceFunction(sf);
  return sf;
}

unsigned int
pluton::service::getServiceVersion() const
{
  return _impl->getDefaultRequest()->getServiceVersion();
}

pluton::serializationType
pluton::service::getSerializationType() const
{
  return _impl->getDefaultRequest()->getSerializationType();
}

const std::string&
pluton::service::getClientName(std::string& cn) const
{
  _impl->getDefaultRequest()->getClientName(cn);
  return cn;
}


void
pluton::service::getRequestData(const char*& cp, int& len) const
{
  _impl->getDefaultRequest()->getRequestData(cp, len);
}

const std::string&
pluton::service::getRequestData(std::string& r) const
{
  _impl->getDefaultRequest()->getRequestData(r);
  return r;
}

bool
pluton::service::getContext(const char* key, std::string& value) const
{
  return _impl->getDefaultRequest()->getContext(key, value);
}

bool
pluton::service::hasFileDescriptor() const
{
  return _impl->getDefaultRequest()->hasFileDescriptor();
}

int
pluton::service::getFileDescriptor()
{
  return _impl->getDefaultRequest()->getFileDescriptor();
}
