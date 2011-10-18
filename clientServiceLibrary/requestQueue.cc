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

#include "config.h"

#include <iostream>

#include "requestQueue.h"


pluton::requestQueue::requestQueue(const char* name) : _name(name), _headOfQueue(0), _count(0)
{
}


pluton::requestQueue::~requestQueue()
{
}


void
pluton::requestQueue::addToHead(pluton::clientRequestImpl* R)
{
  R->setNext(_headOfQueue);
  _headOfQueue = R;
  ++_count;
}


//////////////////////////////////////////////////////////////////////
// Return true if deleted. Queue is stable in that a call to
// R->getNext() prior to deletion still points to the first of the
// rest of the list after this call regardless of deletion.
//
// Unlike the sys/queue.h routines, this method is safe regardless of
// whether the element is currently on the list.
//////////////////////////////////////////////////////////////////////

bool
pluton::requestQueue::deleteRequest(pluton::clientRequestImpl* deleteR)
{
  if (!_headOfQueue) return false;

  if (deleteR == _headOfQueue) {
    _headOfQueue = deleteR->getNext();
    deleteR->setNext(0);
    --_count;
    return true;
  }
    
  pluton::clientRequestImpl* nextR = _headOfQueue;
  while (nextR->getNext()) {
    if (nextR->getNext() == deleteR) {
      nextR->setNext(deleteR->getNext());
      deleteR->setNext(0);
      --_count;
      return true;
    }
    nextR = nextR->getNext();
  }

  return false;
}

void
pluton::requestQueue::dump(const char* who) const
{
  std::clog << "Queue dump: '" << _name << "' by '" << who << "' count=" << _count << std::endl;

  const pluton::clientRequestImpl* R = getFirst();
  while (R) {
    std::clog << "R @ " << (void*) R << "="
	      << (void*) R->getClientRequestPtr() << "/" << R->getRequestID()
	      << " next=" << (void*) getNext(R) << std::endl;
    R=getNext(R);
    std::clog << "Next R set to " << (void*) R << std::endl;
  }
}
