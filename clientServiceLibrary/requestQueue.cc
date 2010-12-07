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
