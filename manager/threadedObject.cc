#include <iostream>

#include <assert.h>

#include <st.h>

#include "debug.h"
#include "threadedObject.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// The hierarchy of manager, service and process all run the same
// way. A state thread is created and associated with a single
// instance of their corresponding object. They also have the same
// life-cycle of calls from intialization, run, termination.
//////////////////////////////////////////////////////////////////////


threadedObject::threadedObject(threadedObject* setParent)
  : _threadID(0), _interruptsEnabled(false),
    _parent(setParent), _shutdownInProgressFlag(false), _shutdownCompleteFlag(false)
{
}

threadedObject::~threadedObject()
{
}


void
threadedObject::baseInitiateShutdownSequence()
{
  if (debug::thread()) DBGPRT << getType() << " baseInitiateShutdownSequence" << endl;
  _shutdownInProgressFlag = true;
}

void
threadedObject::enableInterrupts()
{
  _interruptsEnabled = true;
}

void
threadedObject::disableInterrupts()
{
  _interruptsEnabled = false;
}


void
threadedObject::notifyOwner(const char* who, const char* why)
{
  if (debug::thread()) {
    DBGPRT << getType() << ":notifyOwner(" << _threadID << ", "
	   << who << ", " << why << ")" << endl;
  }

  assert(_threadID);
  if (_interruptsEnabled) st_thread_interrupt(_threadID);
}
