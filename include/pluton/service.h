#ifndef _PLUTON_SERVICE_H
#define _PLUTON_SERVICE_H

#include <string>

#include "pluton/fault.h"
#include "pluton/common.h"


//////////////////////////////////////////////////////////////////////
// This class defines the API from a pluton service. The assumption is
// that each service only has access to a single request at a time, so
// the request access methods are embedded within the service rather
// than defined separately as is done with the client API.
//
// This approach is consistent with keeping the service interface as
// simple as possible and to discourage complicated service-side
// interactions.
//////////////////////////////////////////////////////////////////////

namespace pluton {

  class serviceImpl;

  class service {
  public:
    service(const char* name="");
    ~service();

    //////////////////////////////////////////////////////////////////////
    // Service specific methods
    //////////////////////////////////////////////////////////////////////

    static const char*	getAPIVersion();

    bool		initialize();

    bool		hasFault() const;
    const pluton::fault& getFault() const;

    bool	getRequest();
    bool	sendResponse(const char* p);	// Will strlen(p)
    bool	sendResponse(const char* p, int len);
    bool	sendResponse(const std::string& responseData);

    bool	sendFault(unsigned int faultCode, const char* faultText);

    void	terminate();

    //////////////////////////////////////////////////////////////////////
    // serviceRequest proxy methods
    //////////////////////////////////////////////////////////////////////

    const std::string&		getServiceKey(std::string&) const;
    const std::string&		getServiceApplication(std::string&) const;
    const std::string&		getServiceFunction(std::string&) const;
    unsigned int		getServiceVersion() const;
    pluton::serializationType 	getSerializationType() const;

    const std::string&		getClientName(std::string&) const;

    void		getRequestData(const char*& requestPointer, int& requestLength) const;
    const std::string&	getRequestData(std::string& requestData) const;

    bool		getContext(const char* key, std::string& value) const;

    bool		hasFileDescriptor() const;	// If an fd was passed with request
    int			getFileDescriptor();		// Transfers ownership of fd too

  private:
    service&	operator=(const service& rhs);		// Assign not ok
    service(const service& rhs);			// Copy not ok

    serviceImpl*       _impl;

  protected:
    static int		setInternalAttributes(int);
  };
}

#endif
