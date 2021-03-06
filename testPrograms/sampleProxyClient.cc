#include <time.h>
#include <stdlib.h>

#include <iostream>

#include <st.h>
#include <pluton/client.h>

using namespace std;

//////////////////////////////////////////////////////////////////////
// This is a sample pluton client that demonstrates the use of the
// setPollProxy() in a State Threads environment.
//////////////////////////////////////////////////////////////////////

static void*
threadStart(void* v)
{
  const char* me = (char*) v;
  cout << me << " started" << endl;

  pluton::client C(me);		// *Must* create a per-thread instance of this

  pluton::clientRequest r1;	// and this
  r1.setContext("echo.sleepMS", "2000");
  if (!C.addRequest("system.echo.0.raw", r1)) {
    const pluton::fault& f = C.getFault();
    cout << me << " addRequest Failed c=" << f.getFaultCode() << " M=" << f.getMessage() << endl;
    return (void*)1;
  }

  cout << me << " waiting" << endl;
  if (C.executeAndWaitAll() <= 0) {
    clog << me << " "
         << C.getFault().getMessage("Fatal: executeAndWaitAll() failed in sampleClient") << endl;
    return (void*)2;
  }

  if (r1.hasFault()) {
    clog << me << " "
         << "r1 Fault Code=" << r1.getFaultCode() << " Text=" << r1.getFaultText() << endl;
    return (void*)3;
  }

  cout << me << " done" << endl;

  return (void*)0;
}


int
main(int argc, char** argv)
{
  st_init();			// Init state threads

  pluton::client C;		// For historical reasons, setPollProxy is a per-instance call

  C.setPollProxy(st_poll);	// setPollProxy was designed around st_poll
  if (!C.initialize("")) {
    clog << C.getFault().getMessage("Fatal: initialize() failed in sampleClient")  << endl;
    exit(1);
  }

  //////////////////////////////////////////////////////////////////////
  // Start three threads that call the same start routine. Note that
  // each thread has its own stack thus the construction of the
  // pluton::client on the stack makes them unique per thread - which
  // is what they have to be.
  //////////////////////////////////////////////////////////////////////

  st_thread_t t1 = st_thread_create(threadStart, (void*)"one", 1, 0);
  st_sleep(1);
  st_thread_t t2 = st_thread_create(threadStart, (void*)"two", 1, 0);
  st_thread_t t3 = st_thread_create(threadStart, (void*)"three", 1, 0);

  cout << "Waiting for t1" << endl;
  st_thread_join(t1, 0);

  cout << "Waiting for t2" << endl;
  st_thread_join(t2, 0);

  cout << "Waiting for t3" << endl;
  st_thread_join(t3, 0);

  return 0;
}
