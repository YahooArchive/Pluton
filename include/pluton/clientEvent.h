#ifndef _PLUTON_CLIENTEVENT_H
#define _PLUTON_CLIENTEVENT_H


//////////////////////////////////////////////////////////////////////
// clientEvent is the event-oriented class that non-blocking server
// programs will want to use. This class relies on the caller to send
// in I/O available and timeout events to progress the requests
// through to completion. This class never issues any blocking calls
// of any kind.
//////////////////////////////////////////////////////////////////////

#include <sys/time.h>

#include "pluton/common.h"
#include "pluton/client.h"
#include "pluton/clientRequest.h"


namespace pluton {

  class clientEvent : public clientBase {
  public:
    clientEvent(const char* yourName="");
    ~clientEvent();

    bool	addRequest(const char* serviceKey, pluton::clientRequest*,
			   unsigned int timeoutMilliSeconds=4000);
    bool	addRequest(const char* serviceKey, pluton::clientRequest&,
			   unsigned int timeoutMilliSeconds=4000);


    //////////////////////////////////////////////////////////////////////
    // An eventWanted structure is returned for the caller to
    // process. Outstanding events *must* all come back via one of the
    // send*Event() methods. To cancel a request, send the timeout
    // event.
    //////////////////////////////////////////////////////////////////////

    enum eventType { wantNothing=0, wantRead=1, wantWrite=2 };

    typedef struct {
      eventType				type;
      int				fd;
      struct timeval	 		timeout;
      const pluton::clientRequest* 	clientRequestPtr;
    } eventWanted;

    const eventWanted*	getNextEventWanted(const struct timeval* now,
					   const pluton::clientRequest* R=0);
    int			sendCanReadEvent(int fd);
    int			sendCanWriteEvent(int fd);
    int			sendTimeoutEvent(int fd, bool abortFlag=false);

    pluton::clientRequest*	getCompletedRequest();

  private:
    clientEvent&  operator=(const clientEvent& rhs);	// Assign not ok
    clientEvent(const clientEvent& rhs);		// Copy not ok
  };
}

#endif
