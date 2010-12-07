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
