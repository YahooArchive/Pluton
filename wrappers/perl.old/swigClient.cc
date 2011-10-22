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

#include "pluton/client.h"

#include "swigClient.h"


pluton::swig::clientRequest::clientRequest()
{
  _pcr = new pluton::clientRequest();
}

pluton::swig::clientRequest::~clientRequest()
{
  delete _pcr;
}

void
pluton::swig::clientRequest::setRequestData(const std::string request)
{
  _copyRequest = request;
  _pcr->setRequestData(_copyRequest);
}

void
pluton::swig::clientRequest::setAttribute(pluton::requestAttributes at)
{
  _pcr->setAttribute(at);
}

bool
pluton::swig::clientRequest::getAttribute(pluton::requestAttributes at)
{
  return _pcr->getAttribute(at);
}

void
pluton::swig::clientRequest::clearAttribute(pluton::requestAttributes at)
{
  _pcr->clearAttribute(at);
}

bool
pluton::swig::clientRequest::setContext(const char* key, const std::string value)
{
  return _pcr->setContext(key, value);
}

std::string
pluton::swig::clientRequest::getResponseData() const
{
  std::string s;
  _pcr->getResponseData(s);

  return s;
}

bool
pluton::swig::clientRequest::inProgress() const
{
  return _pcr->inProgress();
}

bool
pluton::swig::clientRequest::hasFault() const
{
  return _pcr->hasFault();
}

pluton::faultCode
pluton::swig::clientRequest::getFaultCode() const
{
  return _pcr->getFaultCode();
}

std::string
pluton::swig::clientRequest::getFaultText() const
{
  return _pcr->getFaultText();
}

std::string
pluton::swig::clientRequest::getServiceName() const
{
  return _pcr->getServiceName();
}

//////////////////////////////////////////////////////////////////////
// Client
//////////////////////////////////////////////////////////////////////

pluton::swig::client::client(const char* yourName,
			     unsigned int defaultTimeoutMilliSeconds)
{
  _pc = new pluton::client(yourName, defaultTimeoutMilliSeconds);
}

pluton::swig::client::~client()
{
  delete _pc;
}

void
pluton::swig::client::setDebug(bool nv)
{
  _pc->setDebug(nv);
}

void
pluton::swig::client::setTimeoutMilliSeconds(unsigned int timeoutMilliSeconds)
{
  _pc->setTimeoutMilliSeconds(timeoutMilliSeconds);
}

unsigned int
pluton::swig::client::getTimeoutMilliSeconds() const
{
  return _pc->getTimeoutMilliSeconds();
}

const char*
pluton::swig::client::getAPIVersion()
{
  return pluton::client::getAPIVersion();
}

bool
pluton::swig::client::initialize(const char* lookupMapPath)
{
  return _pc->initialize(lookupMapPath);
}

bool
pluton::swig::client::hasFault() const
{
  return _pc->hasFault();
}

const pluton::fault&
pluton::swig::client::getFault()
{
  return _pc->getFault();
}

bool
pluton::swig::client::addRequest(const char* serviceKey, pluton::swig::clientRequest* request)
{
  request->getPCR()->setClientHandle((void*) request);		// Save our selves
  return _pc->addRequest(serviceKey, request->getPCR());
}

int
pluton::swig::client::executeAndWaitSent()
{
  return _pc->executeAndWaitSent();
}

int
pluton::swig::client::executeAndWaitAll()
{
  return _pc->executeAndWaitAll();
}

pluton::swig::clientRequest*
pluton::swig::client::executeAndWaitAny()
{
  pluton::clientRequest* r = _pc->executeAndWaitAny();
  if (!r) return 0;

  return (pluton::swig::clientRequest*) r->getClientHandle();
}

int
pluton::swig::client::executeAndWaitOne(pluton::swig::clientRequest* request)
{
  return _pc->executeAndWaitOne(request->getPCR());
}
