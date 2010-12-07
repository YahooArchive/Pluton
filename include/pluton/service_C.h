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
