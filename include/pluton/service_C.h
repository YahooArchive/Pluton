#ifndef PLUTON_SERVICE_C_H
#define PLUTON_SERVICE_C_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  unknownSerialization='?',
  COBOL='c',
  HTML='h',
  JMS='m',
  JSON='j',
  NETSTRING='n',
  PHP='p',
  SOAP='s',
  XML='x',
  raw='r'
} pluton_C_serializationType;

  extern int 		pluton_service_C_initialize();
  extern const char* 	pluton_service_C_getAPIVersion();
  extern void		pluton_service_C_terminate();
  extern int		pluton_service_C_getRequest();
  extern int		pluton_service_C_sendResponse(const char* ptr, int len);
  extern int		pluton_service_C_sendFault(unsigned int code, const char* txt);
  extern int		pluton_service_C_hasFault();
  extern int		pluton_service_C_getFaultCode();
  extern const char*	pluton_service_C_getFaultMessage(const char* preMessage, int longFormat);

  extern const char*	pluton_service_C_getServiceKey();
  extern const char*	pluton_service_C_getServiceApplication();
  extern const char*	pluton_service_C_getServiceFunction();
  extern unsigned int	pluton_service_C_getServiceVersion();
  extern pluton_C_serializationType	pluton_service_C_getSerializationType();
  extern const char*	pluton_service_C_getClientName();

  extern void		pluton_service_C_getRequestData(const char** ptr, int* len);

  extern const char*	pluton_service_C_getContext(const char* key);


#ifdef __cplusplus
}
#endif

#endif
