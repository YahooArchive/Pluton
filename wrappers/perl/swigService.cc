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
