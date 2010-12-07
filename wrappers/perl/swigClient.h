#ifndef _PLUTON_SWIG_CLIENT_H
#define _PLUTON_SWIG_CLIENT_H

//////////////////////////////////////////////////////////////////////
// A simple non-derived wrapper to pluton::client. The sole purpose of
// this interface is to be easily parsed by swig so that a perl
// wrapper can be auto-generated.
//////////////////////////////////////////////////////////////////////

#include <pluton/common.h>

#include <string>

namespace pluton {
  class client;
  class clientRequest;
  namespace swig {

    class clientRequest {
    private:
      clientRequest& operator=(const clientRequest& rhs);	// Assign not ok
      clientRequest(const clientRequest& rhs); 		// Copy not ok

    public:
      clientRequest();
      ~clientRequest();

      void	setRequestData(const std::string request);

      void 	setAttribute(pluton::requestAttributes);
      bool 	getAttribute(pluton::requestAttributes);
      void 	clearAttribute(pluton::requestAttributes);

      bool 	setContext(const char* key, const std::string value);

      std::string	getResponseData() const;	// Differs from C++ version

      bool 		inProgress() const;
      bool 		hasFault() const;
      pluton::faultCode getFaultCode() const;
      std::string 	getFaultText() const;
      std::string 	getServiceName() const;

      pluton::clientRequest*	getPCR() { return _pcr; }

    private:
      std::string		_copyRequest;
      pluton::clientRequest*	_pcr;
    };

    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////

    class client {
    private:
      client& operator=(const client& rhs);       // Assign not ok
      client(const client& rhs);                  // Copy not ok

    public:
      client(const char* yourName="", unsigned int defaultTimeoutMilliSeconds=4000);
      ~client();

      static const char*  	getAPIVersion();
      bool			initialize(const char* lookupMapPath);
      bool			hasFault() const;
      const pluton::fault&	getFault();

      void 		setDebug(bool nv);

      void		setTimeoutMilliSeconds(unsigned int timeoutMilliSeconds);
      unsigned int	getTimeoutMilliSeconds() const;

      bool		addRequest(const char* serviceKey, pluton::swig::clientRequest* request);

      int                         executeAndWaitSent();
      int                         executeAndWaitAll();
      pluton::swig::clientRequest*      executeAndWaitAny();
      int                         executeAndWaitOne(pluton::swig::clientRequest*);


    private:
      pluton::client*	_pc;
    };
  }
}

#endif
