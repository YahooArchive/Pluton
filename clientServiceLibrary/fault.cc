#include <string>
#include "faultImpl.h"

//////////////////////////////////////////////////////////////////////
// All client and service returns from user-visible APIs make a
// pluton::fault structure available for a detailed explanation of the
// last error return. Various accessors let the caller get at and
// output these details.
//////////////////////////////////////////////////////////////////////


using namespace pluton;

pluton::fault::fault()
{
  _impl = new faultImpl;
}

pluton::fault::~fault()
{
  delete _impl;
}


//////////////////////////////////////////////////////////////////////

bool
pluton::fault::hasFault() const
{
  return _impl->hasFault();
}

pluton::faultCode
pluton::fault::getFaultCode() const
{
  return _impl->getFaultCode();
}


//////////////////////////////////////////////////////////////////////
// Return a nice message explaining the fault.
//////////////////////////////////////////////////////////////////////

const std::string
pluton::fault::getMessage(const std::string& prefix, bool longFormat) const
{
  return _impl->getMessage(prefix.c_str(), longFormat);
}


const std::string
pluton::fault::getMessage(const char* prefix, bool longFormat) const
{
  return _impl->getMessage(prefix, longFormat);
}
