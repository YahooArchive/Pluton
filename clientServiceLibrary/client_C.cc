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

//////////////////////////////////////////////////////////////////////
// A C wrapper around the libpluton client C++ API
//////////////////////////////////////////////////////////////////////

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include <pluton/fault.h>
#include <pluton/client.h>

#include <pluton/client_C.h>

static bool GMyDebugFlag = false;

extern "C" {

// The C++ objects are stored in a standard C structure

struct __spluton_request_C_obj {
  pluton::clientRequest* _pR;
  void* _clientHandle;
  char* _requestData;
  int _requestDataSize;		// malloc size of _requestData
  mutable std::string* _faultText;
  mutable std::string* _serviceName;
};

struct __spluton_client_C_obj {
  pluton::client* _pC;
  mutable std::string* _faultMessage;
};

static void
internal_reset(pluton_request_C_obj* R)
{
  R->_clientHandle = 0;
  R->_requestData = 0;
  R->_requestDataSize = 0;
  R->_faultText = 0;
  R->_serviceName = 0;
}

pluton_request_C_obj*
pluton_request_C_new()
{
  pluton_request_C_obj* R;
  R = (pluton_request_C_obj*) malloc(sizeof(pluton_request_C_obj));
  if (R) {
    internal_reset(R);
    R->_pR = new pluton::clientRequest();
    R->_pR->setClientHandle(R);
  }

  if (GMyDebugFlag) fprintf(stderr, "C New clientRequest %p %p\n", (void*)R, (void*)R->_pR);

  return R;
}

void
pluton_request_C_reset(pluton_request_C_obj* R)
{
  if (GMyDebugFlag) fprintf(stderr, "C reset clientRequest %p %p\n", (void*)R, (void*)R->_pR);

  void* myCH = R->_pR->getClientHandle();	// Need to retain this across reset
  R->_pR->reset();
  R->_pR->setClientHandle(myCH);

  if (R->_requestData) free(R->_requestData);
  if (R->_faultText) delete R->_faultText;
  if (R->_serviceName) delete R->_serviceName;

  internal_reset(R);
}

void
pluton_request_C_delete(pluton_request_C_obj* R)
{
  if (GMyDebugFlag) fprintf(stderr, "C delete clientRequest %p %p\n", (void*)R, (void*)R->_pR);

  delete R->_pR;

  if (R->_requestData) free(R->_requestData);
  if (R->_faultText) delete R->_faultText;
  if (R->_serviceName) delete R->_serviceName;
  free(R);
}

pluton_client_C_obj*
pluton_client_C_new(const char* yourName, int defaultTimeoutMilliSeconds)
{
  pluton_client_C_obj* C;
  C = (pluton_client_C_obj*) malloc(sizeof(pluton_client_C_obj));
  if (C) {
    C->_pC = new pluton::client(yourName, defaultTimeoutMilliSeconds);
    C->_faultMessage = new std::string();
  }

  if (GMyDebugFlag) fprintf(stderr, "C new client %p %p\n", (void*)C, (void*)C->_pC);

  return C;
}

void
pluton_client_C_reset(pluton_client_C_obj* C)
{
  if (GMyDebugFlag) fprintf(stderr, "C reset client %p %p\n", (void*)C, (void*)C->_pC);
  C->_pC->reset();
}

void
pluton_client_C_delete(pluton_client_C_obj* C)
{
  if (GMyDebugFlag) fprintf(stderr, "C delete client %p %p\n", (void*)C, (void*)C->_pC);
  delete C->_faultMessage;
  delete C->_pC;

  free(C);
}


//////////////////////////////////////////////////////////////////////
// If the caller signifies that the data is volatile, take a copy so
// that we don't have to rely on the caller to keep it around.
//////////////////////////////////////////////////////////////////////

void
pluton_request_C_setRequestData(pluton_request_C_obj* R,
				const char* p, int len,
				int copyData)
{
  if (GMyDebugFlag) fprintf(stderr, "C setRequestData %p %p\n", (void*)R, (void*)R->_pR);
  if (!copyData) {
    R->_pR->setRequestData(p, len);
    return;
  }

  // Does the new requestData fit in the malloced buffer?

  if (R->_requestData) {
    if (R->_requestDataSize < len) {
      delete R->_requestData;
      R->_requestData = 0;
    }
  }

  // If there is no malloc buffer, allocate one

  if (!R->_requestData) {
    R->_requestData = (char*) malloc(len);
    R->_requestDataSize = len;
  }

  memcpy(R->_requestData, p, len);
  R->_pR->setRequestData(R->_requestData, len);
}

void
pluton_request_C_setAttribute(pluton_request_C_obj* R, int attrs)
{
  R->_pR->setAttribute(attrs);
}

int
pluton_request_C_getAttribute(const pluton_request_C_obj* R, int attrs)
{
  return R->_pR->getAttribute(attrs) ? 1 : 0;
}

void
pluton_request_C_clearAttribute(pluton_request_C_obj* R, int attrs)
{
  R->_pR->clearAttribute(attrs);
}

int
pluton_request_C_setContext(pluton_request_C_obj* R,
			    const char* key, const char* value, int valueLength)
{
  if (GMyDebugFlag) fprintf(stderr, "C setContext %p %p %s\n", (void*)R, (void*)R->_pR, key);
  std::string V;
  V.assign(value, valueLength);

  return R->_pR->setContext(key, V);
}


int
pluton_request_C_getResponseData(const pluton_request_C_obj* R,
				 const char** responsePointer, int* responseLength)
{
  if (GMyDebugFlag) fprintf(stderr, "C getResponseData %p %p\n", (void*)R, (void*)R->_pR);
  const char* rp;
  int rl;
  int res;

  res = R->_pR->getResponseData(rp, rl);
  *responsePointer = rp;
  *responseLength = rl;

  return res;
}

int
pluton_request_C_inProgress(const pluton_request_C_obj* R)
{
  return R->_pR->inProgress() ? 1 : 0;
}

int
pluton_request_C_hasFault(const pluton_request_C_obj* R)
{
  return R->_pR->hasFault() ? 1 : 0;
}

int
pluton_request_C_getFaultCode(const pluton_request_C_obj* R)
{
  return R->_pR->getFaultCode();
}


const char*
pluton_request_C_getFaultText(const pluton_request_C_obj* R)
{
  if (!R->_faultText) R->_faultText = new std::string;
  *R->_faultText = R->_pR->getFaultText();
  return R->_faultText->c_str();
}


const char*
pluton_request_C_getServiceName(const pluton_request_C_obj* R)
{
  if (!R->_serviceName) R->_serviceName = new std::string;
  *R->_serviceName = R->_pR->getServiceName();
  return R->_serviceName->c_str();
}


//////////////////////////////////////////////////////////////////////
// We manage the client handle as we use the pluton::clientRequest
// handle to identify ourselves.
//////////////////////////////////////////////////////////////////////

void
pluton_request_C_setClientHandle(pluton_request_C_obj* R, void* H)
{
if (GMyDebugFlag) fprintf(stderr, "C SCH %p/%p to %p\n", (void*)R, (void*)R->_pR, H);
  R->_clientHandle = H;
}


void*
pluton_request_C_getClientHandle(const pluton_request_C_obj* R)
{
if (GMyDebugFlag) fprintf(stderr, "C GCH %p/%p from %p\n", (void*)R, (void*)R->_pR, R->_clientHandle);
  return R->_clientHandle;
}

//////////////////////////////////////////////////////////////////////

const char*
pluton_client_C_getAPIVersion()
{
  return pluton::client::getAPIVersion();
}



int
pluton_client_C_initialize(pluton_client_C_obj* C, const char* lookupMapPath)
{
  if (GMyDebugFlag) fprintf(stderr, "C initialize %p %p\n", (void*)C, (void*)C->_pC);
  return C->_pC->initialize(lookupMapPath) ? 1 : 0;
}


int
pluton_client_C_hasFault(const pluton_client_C_obj* C)
{
  return C->_pC->hasFault() ? 1 : 0;
}

extern	int
pluton_client_C_getFaultCode(const pluton_client_C_obj* C)
{
  return C->_pC->getFault().getFaultCode();
}


extern	const char*
pluton_client_C_getFaultMessage(const pluton_client_C_obj* C, const char* prefix, int longFormat)
{
  *(C->_faultMessage) = C->_pC->getFault().getMessage(prefix, longFormat ? true : false);

  return C->_faultMessage->c_str();
}


void
pluton_client_C_setDebug(pluton_client_C_obj* C, int nv)
{
  C->_pC->setDebug(nv ? true : false);
  GMyDebugFlag = nv ? true : false;
}


void
pluton_client_C_setTimeoutMilliSeconds(pluton_client_C_obj* C, int timeoutMilliSeconds)
{
  C->_pC->setTimeoutMilliSeconds(timeoutMilliSeconds);
}


int
pluton_client_C_getTimeoutMilliSeconds(const pluton_client_C_obj* C)
{
  return C->_pC->getTimeoutMilliSeconds();
}


int
pluton_client_C_addRequest(pluton_client_C_obj* C, const char* serviceKey, pluton_request_C_obj* R)
{
  if (GMyDebugFlag) fprintf(stderr, "C addRequest %p %p : %p %p ch=%p %p\n",
			    (void*)C, (void*)C->_pC, (void*)R, (void*)R->_pR,
			    R->_clientHandle, R->_pR->getClientHandle());
  return C->_pC->addRequest(serviceKey, R->_pR) ? 1 : 0;
}


int
pluton_client_C_executeAndWaitSent(pluton_client_C_obj* C)
{
  if (GMyDebugFlag) fprintf(stderr, "C executeAndWaitSent %p %p\n",(void*)C, (void*)C->_pC);
  return C->_pC->executeAndWaitSent();
}


int
pluton_client_C_executeAndWaitAll(pluton_client_C_obj* C)
{
  if (GMyDebugFlag) fprintf(stderr, "C executeAndWaitAll %p %p\n",(void*)C, (void*)C->_pC);
  return C->_pC->executeAndWaitAll();
}


int
pluton_client_C_executeAndWaitOne(pluton_client_C_obj* C, pluton_request_C_obj* R)
{
  if (GMyDebugFlag) fprintf(stderr, "C executeAndWaitOne %p %p : %p %p\n",
			    (void*)C, (void*)C->_pC, (void*)R, (void*)R->_pR);
  return C->_pC->executeAndWaitOne(R->_pR);
}


pluton_request_C_obj*
pluton_client_C_executeAndWaitAny(pluton_client_C_obj* C)
{
  if (GMyDebugFlag) fprintf(stderr, "C executeAndWaitOne %p %p\n", (void*)C, (void*)C->_pC);
  pluton::clientRequest* R = C->_pC->executeAndWaitAny();

  if (R) return (pluton_request_C_obj*) R->getClientHandle();

  return 0;
}


// Terminate extern "C"
}
