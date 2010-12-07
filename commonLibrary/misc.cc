#include <sys/time.h>
#include <string.h>

#include <iostream>
#include <iomanip>

#include "misc.h"

//////////////////////////////////////////////////////////////////////
// Application-specific class-less routines live here.
//////////////////////////////////////////////////////////////////////

const char*
pluton::misc::packetToEnglish(pluton::packetType pt)
{
  switch (pt) {
  case (remoteLoginPT) : return "remoteLoginPT";
  case (requestPT) : return "requestPT";
  case (loginResponsePT) : return "loginResponsePT";
  case (responsePT) : return "responsePT";
  }

  return "?unknownPT?";
}


const char*
pluton::misc::NSTypeToEnglish(pluton::netStringType nst)
{
  switch (nst) {
  case (clientIDNT): return "clientIDNT";
  case (requestIDNT): return "requestIDNT";
  case (serviceKeyNT): return "serviceKeyNT";

  case (attributeNoWaitNT): return "attributeNoWaitNT";
  case (attributeNoRemoteNT): return "attributeNoRemoteNT";
  case (attributeNoRetryNT): return "attributeNoRetryNT";

  case (attributeKeepAffinityNT): return "attributeKeepAffinityNT";
  case (attributeNeedAffinityNT): return "attributeNeedAffinityNT";

  case (contextNT): return "contextNT";
  case (requestDataNT): return "requestDataNT";
  case (timeoutMSNT): return "timeoutMSNT";
  case (fileDescriptorNT): return "fileDescriptorNT";

  case (faultCodeNT): return "faultCodeNT";
  case (responseDataNT): return "responseDataNT";
  case (faultTextNT): return "faultTextNT";
  case (serviceIDNT): return "serviceIDNT";

  case (credentialsNT): return "credentialsNT";
  case (allowedNT): return "allowedNT";

  case (endPacketNT): return "endPacketNT";
  }

  return "?unknownNT?";
}


const char*
pluton::misc::serializationTypeToEnglish(pluton::serializationType st)
{
  switch (st) {
  case COBOL: return "COBOL";
  case HTML: return "HTML";
  case JSON: return "JSON";
  case JMS: return "JMS";
  case NETSTRING: return "NETSTRING";
  case PHP: return "PHP";
  case SOAP: return "SOAP";
  case XML: return "XML";
  case raw: return "raw";
  default: break;
  }

  return "??";
}


//////////////////////////////////////////////////////////////////////
// Convert to the serialization type. The supplied string is not \0
// terminated.
//////////////////////////////////////////////////////////////////////

pluton::serializationType
pluton::misc::englishToSerializationType(const char* p, int l)
{
  if ((l == 3) && (strncmp(p, "JMS", l) == 0)) return pluton::JMS;
  if ((l == 3) && (strncmp(p, "PHP", l) == 0)) return pluton::PHP;
  if ((l == 3) && (strncmp(p, "XML", l) == 0)) return pluton::XML;
  if ((l == 3) && (strncmp(p, "raw", l) == 0)) return pluton::raw;
  if ((l == 4) && (strncmp(p, "HTML", l) == 0)) return pluton::HTML;
  if ((l == 4) && (strncmp(p, "JSON", l) == 0)) return pluton::JSON;
  if ((l == 4) && (strncmp(p, "SOAP", l) == 0)) return pluton::SOAP;
  if ((l == 5) && (strncmp(p, "COBOL", l) == 0)) return pluton::COBOL;
  if ((l == 9) && (strncmp(p, "NETSTRING", l) == 0)) return pluton::NETSTRING;

  return pluton::unknownSerialization;
}


//////////////////////////////////////////////////////////////////////

std::ostream&
pluton::misc::logtime(std::ostream& s)
{
  struct timeval now;
  gettimeofday(&now, 0);

  return s << now.tv_sec << "." << std::setfill('0') << std::setw(6) << now.tv_usec << ": ";
}
