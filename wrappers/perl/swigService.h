#ifndef _PLUTON_SWIG_SERVICE_H
#define _PLUTON_SWIG_SERVICE_H

//////////////////////////////////////////////////////////////////////
// A simple non-derived wrapper to pluton::service. The sole purpose
// of this interface is to be easily parsed by swig so that a perl
// wrapper can be auto-generated.
//////////////////////////////////////////////////////////////////////

#include <string>

namespace pluton {
  class service;
  namespace swig {

    class service {
    private:
      service& operator=(const service& rhs);       // Assign not ok
      service(const service& rhs);                  // Copy not ok

    public:
      service(const char* yourName="");
      ~service();

      static const char*  	getAPIVersion();
      bool			initialize();
      bool			hasFault() const;
      const pluton::fault&	getFault();

      bool	getRequest();
      bool	sendResponse(const std::string& responseData);

      bool	sendFault(unsigned int faultCode, const char* faultText);

      void	terminate();

      //////////////////////////////////////////////////////////////////////
      // serviceRequest proxy methods
      //////////////////////////////////////////////////////////////////////

      const std::string			getServiceKey() const;
      const std::string			getServiceApplication() const;
      const std::string			getServiceFunction() const;
      unsigned int			getServiceVersion() const;
      const std::string		 	getSerializationType() const;

      const std::string			getClientName() const;

      const std::string			getRequestData() const;

      const std::string			getContext(const char* key) const;

    private:
      pluton::service*			_pc;
    };
  }
}

#endif
