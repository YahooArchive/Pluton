#include <string>
#include <sstream>

#include <strings.h>

#include "util.h"
#include "misc.h"
#include "decodePacket.h"

//////////////////////////////////////////////////////////////////////
// A generic packet decoder for clients and services. Accumulate the
// types that have been supplied and determine when a full packet has
// arrived.
//////////////////////////////////////////////////////////////////////


pluton::decodePacket::decodePacket(pluton::packetType setStartingType, pluton::requestImpl* R)
  : _startingType(setStartingType), _requestIn(R), _state(needStartingType)
{
  bzero((void*) _haveMap, sizeof(_haveMap));
}

pluton::decodePacket::~decodePacket()
{
}


pluton::decodeRequestPacket::decodeRequestPacket(pluton::requestImpl* R)
  : decodePacket(pluton::requestPT, R)
{
}

pluton::decodeRequestPacket::~decodeRequestPacket()
{
}

pluton::decodeResponsePacket::decodeResponsePacket(pluton::requestImpl* R)
  : decodePacket(pluton::responsePT, R)
{
}
  
pluton::decodeResponsePacket::~decodeResponsePacket()
{
}


//////////////////////////////////////////////////////////////////////
// Reset all internal state
//////////////////////////////////////////////////////////////////////

void
pluton::decodePacket::reset()
{
  _state = needStartingType;
  bzero((void*) _haveMap, sizeof(_haveMap));

  _requestID = 0;
  _requestIn = 0;
}


//////////////////////////////////////////////////////////////////////
// Switch on state then type to continue decoding the packet. Return
// false if there is an unrecoverable error in errorMessage. This
// routine should be pickier about which data types to accept
// depending on whether it is a request or a response packet.
//////////////////////////////////////////////////////////////////////

bool
pluton::decodePacket::addType(char nsType, const char* nsDataPtr, int nsDataLength,
			      int nsDataOffset, std::string& errorMessage)
{
  if (_state == needStartingType) {
    if (nsType != _startingType) {
      std::ostringstream os;

      os << "decodePacket: Unexpected netString type. Wanted: "
	 << pluton::misc::packetToEnglish(_startingType)
	 << ", but got: '" << nsType << "' ("
	 <<  pluton::misc::packetToEnglish((pluton::packetType) nsType)
	 << ")";
      errorMessage = os.str();
      return false;
    }
    _state = acceptingOthers;
    return true;
  }

  if (_state == haveFullRequest) {
    errorMessage = "decodePacket: Have a complete request ready, already";
    return false;
  }

  //////////////////////////////////////////////////////////////////////
  // _state is accepting other netString types - check for impossibles
  // and duplicates
  //////////////////////////////////////////////////////////////////////

  if ((unsigned) nsType >= _haveMapSize) {
    errorMessage = "decodePacket: Out-of-range netString type: (";
    errorMessage += util::ltos(nsType);
    errorMessage += ")";
    return false;
  }

  if (_haveMap[(unsigned) nsType]) {
    errorMessage = "decodePacket: Duplicate netString type of ";
    errorMessage += pluton::misc::NSTypeToEnglish(static_cast<pluton::netStringType> (nsType));
    return false;
  }
  _haveMap[(unsigned int) nsType] = 1;

  const char* err;

  switch (nsType) {

  case pluton::serviceKeyNT:
    if (_requestIn) {
      err = _requestIn->setServiceKey(nsDataPtr, nsDataLength);
      if (err) {
	errorMessage = "decodePacket: Invalid Service Key: ";
	errorMessage += err;
	return false;
      }
    }
    break;

  case pluton::faultCodeNT:
    if (_requestIn) {
      _requestIn->setFaultCode(static_cast<pluton::faultCode>(strtol(nsDataPtr, 0, 10)));
    }
    break;

  case pluton::clientIDNT:
    if (_requestIn) _requestIn->setClientName(nsDataPtr, nsDataLength);
    break;

  case pluton::serviceIDNT:
    if (_requestIn) _requestIn->setServiceName(nsDataPtr, nsDataLength);
    break;

  case pluton::requestIDNT:
    _requestID = strtol(nsDataPtr, 0, 10);	// The NS terminator stops strtol()
    break;

  case pluton::contextNT:
    if (_requestIn) _requestIn->setContextData(nsDataPtr, nsDataLength);
    break;

  case pluton::requestDataNT:
    if (_requestIn) _requestIn->setRequestOffset(nsDataOffset, nsDataLength);
    break;

  case pluton::timeoutMSNT:
    if (_requestIn) _requestIn->setTimeoutMS(strtol(nsDataPtr, 0, 10));
    break;

  case pluton::fileDescriptorNT:
    if (_requestIn) _requestIn->setHasFileDescriptor(true);
    break;

  case pluton::responseDataNT:
    if (_requestIn) _requestIn->setResponseOffset(nsDataOffset, nsDataLength);
    break;

  case pluton::faultTextNT:
    if (_requestIn) _requestIn->setFaultText(nsDataPtr, nsDataLength);
    break;

  case pluton::attributeNoWaitNT:
    if (_requestIn) _requestIn->setAttribute(pluton::noWaitAttr);
    break;

  case pluton::attributeNoRemoteNT:
    if (_requestIn) _requestIn->setAttribute(pluton::noRemoteAttr);
    break;

  case pluton::attributeNoRetryNT:
    if (_requestIn) _requestIn->setAttribute(pluton::noRetryAttr);
    break;

  case pluton::attributeKeepAffinityNT:
    if (_requestIn) _requestIn->setAttribute(pluton::keepAffinityAttr);
    break;

  case pluton::attributeNeedAffinityNT:
    if (_requestIn) _requestIn->setAttribute(pluton::needAffinityAttr);
    break;

  case pluton::endPacketNT:
    _state = haveFullRequest;
    break;

    ////////////////////////////////////////////////////////////
    // Allow unknown types as it might be a new server talking to an
    // older version of the library.
    ////////////////////////////////////////////////////////////

  default:
    break;
  }

  return true;
}
