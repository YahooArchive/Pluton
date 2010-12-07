#ifndef _DECODEPACKET_H
#define _DECODEPACKET_H


//////////////////////////////////////////////////////////////////////
// A class that decodes incoming netStrings into a
// request/response. The caller is responsible for clearing any
// previous request values prior to passing them to this routine.
//////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>

#include "global.h"
#include "requestImpl.h"

namespace pluton {
  class decodePacket {
  protected:
    decodePacket(pluton::packetType startingType, pluton::requestImpl* R=0);

  public:
    virtual ~decodePacket() = 0;

    void	reset();
    bool	addType(char nsType, const char* nsDataPtr, int nsDataLength,
			int nsDataOffset, std::string& em);
    bool	haveCompleteRequest() const { return _state == haveFullRequest; }

    void 			setRequest(pluton::requestImpl* R) { _requestIn = R; }
    pluton::requestImpl*	getRequest() const { return _requestIn; }
    unsigned int 		getRequestID() const { return _requestID; }

  protected:
    pluton::packetType	_startingType;

  private:
    decodePacket&	operator=(const decodePacket& rhs);	// Assign not ok
    decodePacket(const decodePacket& rhs);			// Copy not ok

    pluton::requestImpl *_requestIn;
    enum { needStartingType, acceptingOthers, haveFullRequest } _state;
    unsigned int	_requestID;

    static const unsigned int	_haveMapSize = 'z' + 1;			// Track which types have
    char			_haveMap[_haveMapSize];			// already been seen
  };

  class decodeRequestPacket : public decodePacket {
  public:
    decodeRequestPacket(pluton::requestImpl* R=0);
    ~decodeRequestPacket();
  };

  class decodeResponsePacket : public decodePacket {
  public:
    decodeResponsePacket(pluton::requestImpl* R=0);
    ~decodeResponsePacket();
  };
}

#endif
