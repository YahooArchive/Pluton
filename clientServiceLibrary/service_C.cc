//////////////////////////////////////////////////////////////////////
// A C wrapper around the libpluton service C++ API
//////////////////////////////////////////////////////////////////////

#include <string>

#include <pluton/fault.h>
#include <pluton/service.h>
#include <pluton/service_C.h>

#include "serviceAttributes.h"


static pluton::service S;

class accessService : public pluton::service {
public:
  static void inJVM()
  {
    accessService::setInternalAttributes(pluton::serviceAttributes::insideLinuxJVM);
  }
};

#ifdef __cplusplus
extern "C" {
#endif

  /* This is an invisible method to regular callers */

  void
  pluton_service_C_insideLinuxJVM()
  {
    accessService::inJVM();
  }

  int
  pluton_service_C_initialize()
  {
    return S.initialize();
  }

  const char*
  pluton_service_C_getAPIVersion()
  {
    return S.getAPIVersion();
  }

  void
  pluton_service_C_terminate()
  {
    S.terminate();
  }

  int
  pluton_service_C_getRequest()
  {
    return S.getRequest();
  }

  int
  pluton_service_C_sendResponse(const char* p, int l)
  {
    return S.sendResponse(p, l);
  }

  int
  pluton_service_C_sendFault(unsigned int code, const char* txt)
  {
    return S.sendFault(code, txt);
  }

  int
  pluton_service_C_hasFault()
  {
    return S.hasFault();
  }

  int
  pluton_service_C_getFaultCode()
  {
    static const pluton::fault& F = S.getFault();
    return F.getFaultCode();
  }

  const char*
  pluton_service_C_getFaultMessage(const char* preMessage, int longFormat)
  {
    static std::string faultMessage;
    static const pluton::fault& F = S.getFault();
    faultMessage = F.getMessage(preMessage, longFormat != 0);
    return faultMessage.c_str();
  }


  const char*
  pluton_service_C_getServiceKey()
  {
    static std::string serviceKey;
    S.getServiceKey(serviceKey);

    return serviceKey.c_str();
  }

  const char*
  pluton_service_C_getServiceApplication()
  {
    static std::string serviceApplication;
    S.getServiceApplication(serviceApplication);

    return serviceApplication.c_str();
  }

  const char*
  pluton_service_C_getServiceFunction()
  {
    static std::string serviceFunction;
    S.getServiceFunction(serviceFunction);

    return serviceFunction.c_str();
  }

  unsigned int
  pluton_service_C_getServiceVersion()
  {
    return S.getServiceVersion();
  }

  pluton_C_serializationType
  pluton_service_C_getSerializationType()
  {
    return (pluton_C_serializationType) S.getSerializationType();	/* Nawty conversion */
  }

  const char*
  pluton_service_C_getClientName()
  {
    static std::string clientName;
    clientName = S.getClientName(clientName);
    return clientName.c_str();
  }

  void
  pluton_service_C_getRequestData(const char** rp, int* len)
  {
    const char* localP;
    int localLen;
    S.getRequestData(localP, localLen);

    *rp = localP;
    *len = localLen;
  }

  const char*
  pluton_service_C_getContext(const char* key)
  {
    static std::string context;
    if (S.getContext(key, context)) return context.c_str();

    return 0;
  }



#ifdef __cplusplus
}
#endif
