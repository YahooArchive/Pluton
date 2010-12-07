#ifndef _PLUTON_CLIENTREQUEST_H
#define _PLUTON_CLIENTREQUEST_H

#include <string>

//////////////////////////////////////////////////////////////////////
// Define the clientRequest APIs for the Pluton Framework
//////////////////////////////////////////////////////////////////////

#include "pluton/common.h"
#include "pluton/fault.h"

namespace pluton {
  class clientRequestImpl;

  class clientRequest {
  public:
    clientRequest();
    virtual 	~clientRequest();	// Callers can derive this class so they can wrap
					// it in a container with other stuff
    void	reset();

    //////////////////////////////////////////////////////////////////////
    // Various signatures for adding requests
    //////////////////////////////////////////////////////////////////////

    void	setRequestData(const char* p);	// Library will strlen(p)
    void	setRequestData(const char* p, int len);
    void	setRequestData(const std::string& request);

    void	setAttribute(pluton::requestAttributes);
    bool	getAttribute(pluton::requestAttributes);
    void	clearAttribute(pluton::requestAttributes);

    bool	setContext(const char* key, const std::string& value);


    pluton::faultCode	getResponseData(const char*& responsePointer, int& responseLength) const;
    pluton::faultCode	getResponseData(std::string& responseData) const;

    //////////////////////////////////////////////////////////////////////
    // Give plenty of options to access fault information. We hardly
    // want the lack of APis to be used as an excuse for not checking
    // and reporting on faults now, do we?
    //////////////////////////////////////////////////////////////////////

    bool		inProgress() const;		// If being processed by the client API

    bool		hasFault() const;		// otherwise it'll be a fault
    pluton::faultCode	getFault(std::string& faultText) const;
    pluton::faultCode	getFaultCode() const;
    std::string		getFaultText() const;
    std::string 	getServiceName() const;

    //////////////////////////////////////////////////////////////////////
    // The caller can associate data with a given request and gain
    // acess to it as needed. This is likely to be of most value when
    // using the executeAndWaitAny() method as the caller can
    // associate the request with some other data structure of theirs.
    //////////////////////////////////////////////////////////////////////

    void		setClientHandle(void*);		// Users can store
    void*		getClientHandle() const;	// anything here


    //////////////////////////////////////////////////////////////////////
    // End of user methods.
    //////////////////////////////////////////////////////////////////////

    pluton::clientRequestImpl*		getImpl() { return _impl; }	// Ugly
    const pluton::clientRequestImpl*	getImpl() const { return _impl; }

  private:
    clientRequest&	operator=(const clientRequest& rhs);	// Assign not ok
    clientRequest(const clientRequest& rhs);			// Copy not ok

    pluton::clientRequestImpl*	_impl;
  };
}

#endif
