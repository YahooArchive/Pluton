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

#ifndef _REQUESTIMPL_H
#define _REQUESTIMPL_H

#include <string>
#include "hashString.h"
#include "hash_mapWrapper.h"
#include "ostreamWrapper.h"

#include "pluton/fault.h"
#include "pluton/clientEvent.h"

#include "netString.h"
#include "serviceKey.h"


//////////////////////////////////////////////////////////////////////
// The Request Implementation class is an omnibus container used by
// both client and service to store request information. Partitioning
// out the two uses is probably the *right* thing to do, but that
// creates work for some of the tools that treat a request as both a
// service and a client - particularly plTransmitter-type tools.
//////////////////////////////////////////////////////////////////////

namespace pluton {

  class requestImpl {
  public:
    requestImpl(const std::string& name="", const std::string& function="",
		pluton::serializationType type=raw,
		unsigned int version=0);
    virtual ~requestImpl() = 0;

    ////////////////////////////////////////

    void	setDebug(bool f) { _debugFlag = f; }
    void	resetRequestValues();
    void	resetResponseValues();

    ////////////////////////////////////////

    void		setRequestID(unsigned int rid) { _requestID = rid; }
    unsigned int	getRequestID() const { return _requestID; }

    void	getServiceKey(std::string& sk) const { sk = _SK.getEnglishKey(); }
    void	getServiceApplication(std::string& sa) const { sa = _SK.getApplication(); }
    void 	getServiceFunction(std::string& sf) const { sf = _SK.getFunction(); }
    const std::string&	getServiceFunction() const { return _SK.getFunction(); }
    unsigned int	getServiceVersion() const { return _SK.getVersion(); }
    pluton::serializationType	getSerializationType() const { return _SK.getSerialization(); }

    void		setTimeoutMS(unsigned int toMS) { _timeoutMS = toMS; }
    unsigned int	getTimeoutMS() const { return _timeoutMS; }
    unsigned long	getTimeoutuS() const { return 1000 * (unsigned long) _timeoutMS; }

    ////////////////////////////////////////

    const char*	setServiceKey(const char* p, int l) { return _SK.parse(p, l); }
    const char*	setServiceKey(const std::string& key) { return _SK.parse(key); }

    void	getSearchKey(std::string& searchK) const { return _SK.getSearchKey(searchK); }
    std::string	getServiceKey() const { return _SK.getEnglishKey(); }

    void	setClientName(const std::string& cid) { _clientNameStr = cid; }
    void	setClientName(const char* p, int l) { _clientNameStr.assign(p, l); }
    void	getClientName(std::string& cn) const { cn = _clientNameStr; }

    void	setRequestData(const std::string&);
    void	setRequestData(const char* p, int len);
    void	setRequestOffset(int offset, int len);

    void	getRequestData(const char*& p, int& length) const;
    void	getRequestData(std::string&) const;
    int		getRequestDataLength() const { return _requestDataLen; }

    void	setResponseData(const std::string&);
    void	setResponseData(const char* p, int len);
    void	setResponseOffset(int offset, int len);

    pluton::faultCode	getResponseData(const char*& p, int& length) const;
    pluton::faultCode	getResponseData(std::string&) const;
    int		getResponseDataLength() const { return _responseDataLen; }

    void	setPacketOffset(int of) { _inboundPacketOffset = of; }	// Tools only
    void	adjustOffsets(const char*, int endingOffset);		// Tools only
    void	getInboundPacketData(const char*& p, int& len) const;	// Tools only

    void	setContextData(const char* p, int len);
    void	getContextData(const char*& p, int& len) const;

    bool	setContext(const char* key, const std::string& value);
    bool	getContext(const char* key, std::string& value);

    void	setFault(pluton::faultCode, const std::string& faultText);
    void	setFault(pluton::faultCode, const char* p="");
    void	setFaultCode(pluton::faultCode fc) { _faultCode = fc; }
    void	setFaultText(const char* p, int len);

    std::string		getServiceName() const { return _serviceNameStr; }
    void		setServiceName(const std::string& sid) { _serviceNameStr = sid; }
    void		setServiceName(const char* p, int l) { _serviceNameStr.assign(p, l); }

    bool		hasFault() const { return _faultCode != pluton::noFault; }
    pluton::faultCode	getFault(std::string&) const;
    pluton::faultCode	getFaultCode() const { return _faultCode; }
    std::string		getFaultText() const { return _faultText; }

    void	setAttribute(pluton::requestAttributes);
    bool	getAttribute(pluton::requestAttributes) const;
    void	clearAttribute(pluton::requestAttributes);

    void	setByPassIDCheck(bool tf) { _byPassIDCheck = tf; }
    bool	byPassIDCheck() const { return _byPassIDCheck; }

    void	setFileDescriptor(int fd);
    void	setHasFileDescriptor(bool tf) { _hasFileDescriptor = tf; }
    bool	hasFileDescriptor() const { return _hasFileDescriptor; }
    int		getFileDescriptor();

    ////////////////////////////////////////

    void	setEventTypeWanted(pluton::clientEvent::eventType ew) { _eventTypeWanted = ew; }
    void	clearEventTypeWanted() { _eventTypeWanted = pluton::clientEvent::wantNothing; }
    bool	getEventTypeWanted(pluton::clientEvent::eventType ew) const
      { return _eventTypeWanted == ew; }

    ////////////////////////////////////////

    void	assembleRequestPacket(netStringGenerate& pre, netStringGenerate& post,
				      bool doBegin=true) const;

    void	assembleResponsePacket(const std::string& serviceNameStr,
				       netStringGenerate& pre, netStringGenerate& post,
				       bool doBegin=true) const;

  ////////////////////////////////////////

    std::ostream&		put(std::ostream& s) const;

  protected:
    bool		_debugFlag;

  private:
    requestImpl&	operator=(const requestImpl& rhs);	// Assign not ok
    requestImpl(const requestImpl& rhs);			// Copy not ok

    bool	parseContext();

    //////////////////////////////////////////////////////////////////////
    // Values exchanged in request and/or response
    //////////////////////////////////////////////////////////////////////

    unsigned int	_requestID;		// Req/Resp
    std::string		_clientNameStr;		// Req/Resp
    serviceKey		_SK;			// Req
    unsigned long	_attributeBits;		// Req

    netStringGenerate	_contextNS;		// Req

    bool		_hasFileDescriptor;	// Req

    const char*	_requestDataPtr;		// Req
    int		_requestDataOffset;		// Req
    int		_requestDataLen;		// Req

    const char*	_responseDataPtr;		// Resp
    int		_responseDataOffset;		// Resp
    int		_responseDataLen;		// Resp

    const char*	_inboundPacketPtr;		// Req/Resp
    int		_inboundPacketOffset;		// Req/Resp
    int		_inboundPacketLen;		// Req/Resp

    std::string		_serviceNameStr;	// Resp
    pluton::faultCode	_faultCode;		// Resp
    std::string		_faultText;		// Resp

    bool	_byPassIDCheck;			// Req

    ////////////////////////////////////////

    int			_passedFileDescriptor;
    unsigned int 	_timeoutMS;

    ////////////////////////////////////////

    std::string		_contextStr;		// Service decode
    bool		_contextParsed;

    ////////////////////////////////////////

    pluton::clientEvent::eventType	_eventTypeWanted;

    ////////////////////////////////////////

    hash_map<std::string, std::string, hashString>	contextMap;
    typedef hash_map<std::string, std::string, hashString>::const_iterator contextMapConstIter;
  };
}

std::ostream&	operator<<(std::ostream&, const pluton::requestImpl&);

#endif
