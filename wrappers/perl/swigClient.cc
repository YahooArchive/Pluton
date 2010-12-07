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
