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
#include <unistd.h>

#include "pluton/common.h"
#include "pluton/client.h"

#include "global.h"

#include "util.h"
#include "requestImpl.h"


pluton::requestImpl::requestImpl(const std::string& name, const std::string& function,
				 pluton::serializationType type,
				 unsigned int version)
  : _debugFlag(false),
    _requestID(0), _SK(name, function, type, version), _attributeBits(0),
    _hasFileDescriptor(false),
    _requestDataPtr(""), _requestDataOffset(0), _requestDataLen(0),
    _responseDataPtr(""), _responseDataOffset(0), _responseDataLen(0),
    _inboundPacketPtr(""), _inboundPacketOffset(0), _inboundPacketLen(0),
    _faultCode(pluton::requestNotAdded),
    _byPassIDCheck(false),
    _passedFileDescriptor(-1), _timeoutMS(0),
    _contextParsed(false), _eventTypeWanted(pluton::clientEvent::wantNothing)
{
}

pluton::requestImpl::~requestImpl()
{
  if (_passedFileDescriptor != -1) close(_passedFileDescriptor);
}


void
pluton::requestImpl::resetRequestValues()
{
  _requestID = 0;
  _clientNameStr.erase();
  _SK.erase();
  _attributeBits = 0;
  _contextNS.erase();
  if (_passedFileDescriptor != -1) close(_passedFileDescriptor);
  _passedFileDescriptor = -1;
  _hasFileDescriptor = false;
  _requestDataPtr = "";
  _requestDataOffset = 0;
  _requestDataLen = 0;

  _inboundPacketPtr = "";
  _inboundPacketOffset = 0;
  _inboundPacketLen = 0;

  _byPassIDCheck = false;
  _timeoutMS = 0;
}

void
pluton::requestImpl::resetResponseValues()
{
  _serviceNameStr.erase();
  _faultCode = pluton::requestNotAdded;
  _faultText.erase();
  _contextStr.erase();
  _contextParsed = false;
  _responseDataPtr = "";
  _responseDataOffset = 0;
  _responseDataLen = 0;

  _inboundPacketPtr = "";
  _inboundPacketOffset = 0;
  _inboundPacketLen = 0;
}


//////////////////////////////////////////////////////////////////////

void
pluton::requestImpl::setRequestData(const char *cp, int len)
{
  _requestDataPtr = cp;
  _requestDataOffset = 0;
  _requestDataLen = len;
}

void
pluton::requestImpl::setRequestData(const std::string& ns)
{
  _requestDataPtr = ns.data();
  _requestDataOffset = 0;
  _requestDataLen = ns.length();
}

void
pluton::requestImpl::setRequestOffset(int offset, int len)
{
  _requestDataPtr = 0;
  _requestDataOffset = offset;
  _requestDataLen = len;
}

void
pluton::requestImpl::setResponseData(const char *cp, int len)
{
  _responseDataPtr = cp;
  _responseDataOffset = 0;
  _responseDataLen = len;
}

void
pluton::requestImpl::setResponseData(const std::string& ns)
{
  _responseDataPtr = ns.data();
  _responseDataOffset = 0;
  _responseDataLen = ns.length();
}

void
pluton::requestImpl::setResponseOffset(int offset, int len)
{
  _responseDataPtr = 0;
  _responseDataOffset = offset;
  _responseDataLen = len;
}

void
pluton::requestImpl::adjustOffsets(const char* cp, int endingOffset)
{
  _requestDataPtr = cp + _requestDataOffset;
  _responseDataPtr = cp + _responseDataOffset;
  _inboundPacketPtr = cp + _inboundPacketOffset;
  _inboundPacketLen = endingOffset - _inboundPacketOffset;
}

void
pluton::requestImpl::getRequestData(const char*& cp, int& len) const
{
  cp = _requestDataPtr;
  len = _requestDataLen;
}

void
pluton::requestImpl::getRequestData(std::string& req) const
{
  req.assign(_requestDataPtr, _requestDataLen);
}

pluton::faultCode
pluton::requestImpl::getResponseData(const char*& cp, int& len) const
{
  cp = _responseDataPtr;
  len = _responseDataLen;

  return _faultCode;
}


//////////////////////////////////////////////////////////////////////
// Inbound is in the eye of the beholder. For a service it's the
// request, for a client, it's the response.
//////////////////////////////////////////////////////////////////////

void
pluton::requestImpl::getInboundPacketData(const char*& cp, int& len) const
{
  cp = _inboundPacketPtr;
  len = _inboundPacketLen;
}

pluton::faultCode
pluton::requestImpl::getResponseData(std::string& resp) const
{
  resp.assign(_responseDataPtr, _responseDataLen);

  return _faultCode;
}

void
pluton::requestImpl::setContextData(const char *cp, int len)
{
  _contextStr.assign(cp, len);
}

void
pluton::requestImpl::getContextData(const char*& cp, int& len) const
{
  cp = _contextStr.data();
  len = _contextStr.length();
}


//////////////////////////////////////////////////////////////////////
// The "pluton." namespace is reserved in context
//////////////////////////////////////////////////////////////////////

bool
pluton::requestImpl::setContext(const char* key, const std::string& value)
{
  static const char* offLimits = "pluton.";
  static unsigned int offLimitsLength = 7;

  if (strncmp(offLimits, key, offLimitsLength) == 0) {
    _faultCode = pluton::contextReservedNamespace;
    _faultText = "Context Key uses reserved prefix of 'pluton.'";
    return false;
  }

  _contextNS.append('k', key);
  _contextNS.append('v', value);

  return true;
}


bool
pluton::requestImpl::getContext(const char* key, std::string& value)
{
  if (!_contextParsed) {			// Only decode the context NS on demand
    _contextParsed = true;
    contextMap.clear();
    if (!_contextStr.empty()) if (!parseContext()) return false;
  }

  ////////////////////////////////////////////////////////////
  // Once the map is built, fetch the context by name
  ////////////////////////////////////////////////////////////

  contextMapConstIter ci = contextMap.find(key);
  if (ci == contextMap.end()) {
    value.erase();
    return false;
  }

  value = ci->second;

  return true;
}

//////////////////////////////////////////////////////////////////////
// The Context comes in as pairs of netstrings v,k. Parsing of these
// netStrings is avoided until the caller makes a getContext call. The
// hope is they won't normally fetch them :-}
//
// The downside of deferring this parsing is that any parse errors may
// go un-noticed as fault is set after a caller would normally expect
// to check it. Oh well. The chance of a parse error should be close
// to zero anyhoo.
//////////////////////////////////////////////////////////////////////

bool
pluton::requestImpl::parseContext()
{
  netStringParse parse(_contextStr);

  char 	nsType;
  const	char* nsDataPtr;
  int 	nsLength;
  while (!parse.eof()) {
    const char* err = parse.getNext(nsType, nsDataPtr, nsLength);
    if (err) {
      _faultCode = pluton::contextFormatError;
      _faultText = "Service Error: Malformed context: ";
      _faultText += err;
      return false;
    }
    if (nsType != 'k') {
      _faultCode = pluton::contextFormatError;
      _faultText = "Service Error: Unexpected Context type. Wanted: 'k', but got: '";
      _faultText += nsType;
      _faultText += "'";
      return false;
    }

    std::string k(nsDataPtr, nsLength);

    if (parse.eof()) {
      _faultCode = pluton::contextFormatError;
      _faultText = "Service Error: Incomplete Context. Expected value with key";
      return false;
    }
    err = parse.getNext(nsType, nsDataPtr, nsLength);
    if (err) {
      _faultCode = pluton::contextFormatError;
      _faultText = "Service Error: Malformed context: ";
      _faultText += err;
      return false;
    }
    if (nsType != 'v') {
      _faultCode = pluton::contextFormatError;
      _faultText = "Service Error: Unexpected Context type. Wanted: 'v', but got: '";
      _faultText += nsType;
      _faultText += "'";
      return false;
    }

    std::string v(nsDataPtr, nsLength);

    contextMap[k] = v;
  }

  return true;
}

//////////////////////////////////////////////////////////////////////
// Various convenience methods to set and get fault info
//////////////////////////////////////////////////////////////////////

void
pluton::requestImpl::setFault(pluton::faultCode fc, const std::string& ns)
{
  _faultCode = fc;
  _faultText = ns;
}

void
pluton::requestImpl::setFault(pluton::faultCode fc, const char* p)
{
  _faultCode = fc;
  _faultText.assign(p);
}

void
pluton::requestImpl::setFaultText(const char* p, int l)
{
  _faultText.assign(p, l);
}


pluton::faultCode
pluton::requestImpl::getFault(std::string& fault) const
{
  fault = _faultText;

  return _faultCode;
}


void
pluton::requestImpl::setAttribute(pluton::requestAttributes A)
{
  _attributeBits |= A;
}

void
pluton::requestImpl::clearAttribute(pluton::requestAttributes A)
{
  _attributeBits &= ~A;
}

bool
pluton::requestImpl::getAttribute(pluton::requestAttributes mask) const
{
  return (mask & _attributeBits) != 0;
}


void
pluton::requestImpl::setFileDescriptor(int fd)
{
  _passedFileDescriptor = fd;

  _hasFileDescriptor = (fd >= 0);
}

//////////////////////////////////////////////////////////////////////
// By getting the passed FD, the caller is taking responsibility for
// this resource and should close it when they are done. Note: fd
// passing is not yet supported.
//////////////////////////////////////////////////////////////////////

int
pluton::requestImpl::getFileDescriptor()
{
  int fd = _passedFileDescriptor;

  _passedFileDescriptor = -1;			// No longer responsible for fd

  return fd;
}

//////////////////////////////////////////////////////////////////////
// assembleRequestPacket() avoids copying data by assemble pre and
// post-request data packets so that the caller can writev() the three
// parts in-situ.
//////////////////////////////////////////////////////////////////////

void
pluton::requestImpl::assembleRequestPacket(netStringGenerate& pre, netStringGenerate& post,
					   bool doBegin) const
{
  pre.reserve(_contextNS.length()+100);
  post.reserve(20);
  if (doBegin) pre.append(pluton::requestPT);
  pre.append(pluton::requestIDNT, _requestID);	// Must be first
  if (!_clientNameStr.empty()) pre.append(pluton::clientIDNT, _clientNameStr);

  pre.append(pluton::serviceKeyNT, _SK.getEnglishKey());
  pre.append(pluton::timeoutMSNT, _timeoutMS);

  if (_attributeBits & pluton::noWaitAttr) pre.append(pluton::attributeNoWaitNT);
  if (_attributeBits & pluton::noRemoteAttr) pre.append(pluton::attributeNoRemoteNT);
  if (_attributeBits & pluton::noRetryAttr) pre.append(pluton::attributeNoRetryNT);

  if (_attributeBits & pluton::keepAffinityAttr) pre.append(pluton::attributeKeepAffinityNT);
  if (_attributeBits & pluton::needAffinityAttr) pre.append(pluton::attributeNeedAffinityNT);

  if (!_contextNS.empty()) pre.append(pluton::contextNT, _contextNS);

  if (_hasFileDescriptor) pre.append(pluton::fileDescriptorNT);

  if (_requestDataLen > 0) {
    pre.appendRawPrefix(pluton::requestDataNT, _requestDataLen);
    post.appendRawTerminator();
  }
  post.appendNL(pluton::endPacketNT);
}


//////////////////////////////////////////////////////////////////////
// The writev assembly avoids copying the response data by carefully
// constructing "pre" and "post" which, when written around the
// request data results in the same byte stream as generated by
// assembleResponsePacket().
//
// Having a fault and a response is ambiguous, but the caller is
// responsible for making that decision. This routine simply assembles
// what it is given.
//////////////////////////////////////////////////////////////////////

void
pluton::requestImpl::assembleResponsePacket(const std::string& serviceName,
					    netStringGenerate& pre, netStringGenerate& post,
					    bool doBegin) const
{
  pre.reserve(_faultText.length() + _clientNameStr.length() + serviceName.length() + 16);
  post.reserve(16);

  if (doBegin) pre.append(pluton::responsePT);
  pre.append(pluton::requestIDNT, _requestID);
  if (!_clientNameStr.empty()) pre.append(pluton::clientIDNT, _clientNameStr);
  if (!serviceName.empty()) pre.append(pluton::serviceIDNT, serviceName);

  if (_faultCode != pluton::noFault) {
    pre.append(pluton::faultCodeNT, _faultCode);
    if (!_faultText.empty()) pre.append(pluton::faultTextNT, _faultText);
  }

  if (_responseDataLen > 0) {
    pre.appendRawPrefix(pluton::responseDataNT, _responseDataLen);
    post.appendRawTerminator();
  }

  post.appendNL(pluton::endPacketNT);
}


std::ostream&
pluton::requestImpl::put(std::ostream& s) const
{
  s << "Name=" << _SK.getEnglishKey();
  if (!_clientNameStr.empty()) s << " CiD=" << _clientNameStr;

  if (_requestDataLen > 0) {
    std::string req(_requestDataPtr, std::min(10, (int) _requestDataLen));
    s << " ReqL=" << _requestDataLen << " Req=" << req << "...";
  }

  if (_responseDataLen > 0) {
    std::string resp(_responseDataPtr, std::min(10, (int) _responseDataLen));
    s << " ResL=" << _responseDataLen << " Res=" << resp << "...";
  }

  if (!_contextNS.empty()) s << " context=" << _contextNS;
  if (!_faultText.empty()) s << " fault=" << _faultText;

  if (_attributeBits & pluton::noWaitAttr) s << " attributeNoWait";
  if (_attributeBits & pluton::noRemoteAttr) s << " attributeNoRemote";
  if (_attributeBits & pluton::noRetryAttr) s << " attributeNoRetry";

  return s;
}


std::ostream&
operator<<(std::ostream& s, const pluton::requestImpl& R)
{
  return R.put(s);
}
