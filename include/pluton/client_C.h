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

#ifndef PLUTON_CLIENT_C_H
#define PLUTON_CLIENT_C_H

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************
 * Declarations, constructors and destructors
 **********************************************************************/

typedef struct  __spluton_request_C_obj	pluton_request_C_obj;
typedef struct  __spluton_client_C_obj 	pluton_client_C_obj;

extern pluton_request_C_obj*
		pluton_request_C_new();
extern void	pluton_request_C_reset(pluton_request_C_obj*);
extern void     pluton_request_C_delete(pluton_request_C_obj*);

extern pluton_client_C_obj*
		pluton_client_C_new(const char* yourName, int defaultTimeoutMilliSeconds);
extern void	pluton_client_C_reset(pluton_client_C_obj*);
extern void     pluton_client_C_delete(pluton_client_C_obj*);

/**********************************************************************
 * pluton::clientRequest
 **********************************************************************/

extern void     pluton_request_C_setRequestData(pluton_request_C_obj*,
					       const char* p, int len, int copyData);

#define pluton_request_C_noWaitAttr 		0x0001
#define pluton_request_C_noRemoteAttr 		0x0002
#define pluton_request_C_noRetryAttr          	0x0004
#define pluton_request_C_keepAffinityAttr     	0x0008
#define pluton_request_C_needAffinityAttr     	0x0010

extern void	pluton_request_C_setAttribute(pluton_request_C_obj*, int attrs);
extern int	pluton_request_C_getAttribute(const pluton_request_C_obj*, int attrs);
extern void	pluton_request_C_clearAttribute(pluton_request_C_obj*, int attrs);

extern int	pluton_request_C_setContext(pluton_request_C_obj*,
					    const char* key,
					    const char* value, int valueLength);

extern	int	pluton_request_C_getResponseData(const pluton_request_C_obj*,
						 const char** responsePointer, int* responseLength);

extern	int	pluton_request_C_inProgress(const pluton_request_C_obj*);

extern	int	pluton_request_C_hasFault(const pluton_request_C_obj*);
extern	int	pluton_request_C_getFaultCode(const pluton_request_C_obj*);

extern const char*	pluton_request_C_getFaultText(const pluton_request_C_obj*);
extern const char*	pluton_request_C_getServiceName(const pluton_request_C_obj*);

extern void	pluton_request_C_setClientHandle(pluton_request_C_obj*, void*);
extern void*	pluton_request_C_getClientHandle(const pluton_request_C_obj*);


/**********************************************************************
 * pluton::client
 **********************************************************************/

extern const char*	pluton_client_C_getAPIVersion();


extern  int 	pluton_client_C_initialize(pluton_client_C_obj*, const char* lookupMapPath);
extern	int 	pluton_client_C_hasFault(const pluton_client_C_obj*);

extern	int	pluton_client_C_getFaultCode(const pluton_client_C_obj*);

extern	const char*	pluton_client_C_getFaultMessage(const pluton_client_C_obj*,
							const char* pre, int longFormat);

extern  void	pluton_client_C_setDebug(pluton_client_C_obj*, int nv);

extern  void	pluton_client_C_setTimeoutMilliSeconds(pluton_client_C_obj*,
						       int timeoutMilliSeconds);
extern  int 	pluton_client_C_getTimeoutMilliSeconds(const pluton_client_C_obj*);


extern  int	pluton_client_C_addRequest(pluton_client_C_obj*,
					   const char* serviceKey, pluton_request_C_obj*);

extern  int	pluton_client_C_executeAndWaitSent(pluton_client_C_obj*);
extern  int	pluton_client_C_executeAndWaitAll(pluton_client_C_obj*);
extern  int	pluton_client_C_executeAndWaitOne(pluton_client_C_obj*, pluton_request_C_obj*);

extern  pluton_request_C_obj* pluton_client_C_executeAndWaitAny(pluton_client_C_obj*);


#ifdef __cplusplus
}
#endif

#endif
