//////////////////////////////////////////////////////////////////////
// Purposely fail the protective mutex in clientImpl
//////////////////////////////////////////////////////////////////////

#include <iostream>

#include <assert.h>
#include <stdlib.h>

#include <st.h>

#include "util.h"
#include "pluton/client.h"


using namespace std;

//////////////////////////////////////////////////////////////////////
// Purposely re-use the same pluton::client against the rules to
// ensure we get an assert() alert.
//////////////////////////////////////////////////////////////////////

const char* SK = "system.echo.0.raw";

static void*
pingThread(void* voidC)
{
  pluton::client* C = (pluton::client*) voidC;
  pluton::clientRequest R;
  R.setContext("echo.sleepMS", "1000");
  if (!C->addRequest(SK, R)) {
    clog << "Fault: " << C->getFault().getMessage("addRequest", true) << endl;
    exit(1);
  }

  if (C->executeAndWaitOne(R) <= 0) {
    clog << "Fault: " << C->getFault().getMessage("executeAndWaitOne", true) << endl;
    exit(1);
  }

  return 0;
}

int
main(int argc, char **argv)
{
  assert(argc >= 2);

  const char* goodPath = argv[1];

  st_init();
  pluton::client C("tClientPollProxyBad");
  C.setPollProxy(st_poll);

  if (!C.initialize(goodPath)) {
    clog << C.getFault().getMessage("initialize") << endl;
    exit(1);
  }

  if (argc > 2) C.setDebug(true);

  for (int ix=0; ix < 10; ++ix) {
    st_thread_create(pingThread, (void*) &C, 0, 0);
  }

  pingThread((void*) &C);

  exit(0);
}
