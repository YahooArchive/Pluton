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

#include <iostream>
#include <hash_mapWrapper.h>

#include <sys/types.h>

#include <unistd.h>

#include "pluton/fault.h"
#include "pluton/client.h"
#include "pluton/clientEvent.h"

#include "util.h"
#include "clientImpl.h"

using namespace pluton;

pluton::clientEvent::clientEvent(const char* yourName)
  : clientBase(yourName, 0)
{
}

pluton::clientEvent::~clientEvent()
{
}


bool
pluton::clientEvent::addRequest(const char* serviceKey, pluton::clientRequest& R,
				unsigned int timeoutMilliSeconds)
{
  return addRequest(serviceKey, &R, timeoutMilliSeconds);
}

bool
pluton::clientEvent::addRequest(const char* serviceKey, pluton::clientRequest* R,
				unsigned int timeoutMilliSeconds)
{
  pluton::clientRequestImpl* rImpl = R->getImpl();
  rImpl->setClientName(_pcClient->getClientName());
  rImpl->setClientRequestPtr(R);
  rImpl->setTimeoutMS(timeoutMilliSeconds);
  return _pcClient->getMyThreadImpl()->addRequest(_pcClient, serviceKey, rImpl);
}

const pluton::clientEvent::eventWanted*
pluton::clientEvent::getNextEventWanted(const struct timeval* now,
					const pluton::clientRequest* onlyThisR)
{
  return _pcClient->getMyThreadImpl()->getNextEventWanted(_pcClient, now, onlyThisR);
}

int
pluton::clientEvent::sendCanReadEvent(int fd)
{
  return _pcClient->getMyThreadImpl()->sendCanReadEvent(_pcClient, fd);
}

int
pluton::clientEvent::sendCanWriteEvent(int fd)
{
  return _pcClient->getMyThreadImpl()->sendCanWriteEvent(_pcClient, fd);
}

int
pluton::clientEvent::sendTimeoutEvent(int fd, bool abortFlag)
{
  return _pcClient->getMyThreadImpl()->sendTimeoutEvent(_pcClient, fd, abortFlag);
}

pluton::clientRequest*
pluton::clientEvent::getCompletedRequest()
{
  return _pcClient->getMyThreadImpl()->getCompletedRequest(_pcClient);
}
