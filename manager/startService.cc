#include <iostream>

#include <st.h>

#include "debug.h"
#include "logging.h"
#include "util.h"

#include "manager.h"
#include "service.h"
#include "process.h"

using namespace std;


//////////////////////////////////////////////////////////////////////
// The thread wrapper for a service. Create and run.
//////////////////////////////////////////////////////////////////////

static void*
serviceHandlerThread(void* voidS)
{
  service* S = static_cast<service*>(voidS);

  if (debug::service()) DBGPRT << "Service started: " << S->getLogID() << endl;

  S->preRun();
  S->run();
  S->runUntilIdle();

  if (S->error()) LOGPRT << "Service Error: "
			 << S->getLogID() << " " << S->getErrorMessage() << endl;

  if (debug::process()) DBGPRT << "Service shutting down: " << S->getLogID() << endl;

  S->completeShutdownSequence();

  string id = S->getLogID();		// Save across destroy
  S->getParent()->destroyOffspring(S);	// Tell parent to destroy my object

  if (debug::service()) DBGPRT << "Service shutdown complete: " << id << endl;

  return 0;
}


//////////////////////////////////////////////////////////////////////

bool
service::startThread(service* S)
{
  S->setThreadID(st_thread_create(serviceHandlerThread, (void*) S, 0,
				  S->getMANAGER()->getStStackSize()));
  if (debug::thread()) DBGPRT << "Service Start: " << S->getThreadID() << endl;

  return true;
}
