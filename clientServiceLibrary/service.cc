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
