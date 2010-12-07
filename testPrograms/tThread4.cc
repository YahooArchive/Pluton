#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <string>
#include <iostream>

#include <st.h>

using namespace std;

#include "pluton/client.h"

//////////////////////////////////////////////////////////////////////
// Test the threaded interface to the pluton client library.
//////////////////////////////////////////////////////////////////////


class myMutex {
public:
  st_mutex_t mutex;
};

// Need to fake out that we're all the same thread to avoid another assert test

static pluton::thread_t
tSelf(const char* who)
{
  //  st_thread_t self = st_thread_self();
  st_thread_t self = 0;
  clog << "tSelf called by " << who << " returning " << (void*)(self) << endl;

  return reinterpret_cast<pluton::thread_t>(self);
}

void*
tStart(void* a1)
{
  pluton::client* C = (pluton::client*)(a1);

  pluton::clientRequest R;
  R.setContext("echo.log", "y");
  R.setContext("echo.sleepMS", "3000");
  R.setRequestData("ok");
  if (!C->addRequest("system.echo.0.raw", &R)) {
    clog << C->getFault().getMessage("Fatal: addRequest(R) failed in tThread4") << endl;
    exit(3);
  }

  if (C->executeAndWaitAll() <= 0) {
    clog << C->getFault().getMessage("Fatal: executeAndWaitAll() failed in tThread4") << endl;
    exit(4);
  }

  return 0;
}


int
main(int argc, char** argv)
{
  st_init();

  pluton::client::setThreadHandlers(tSelf);

  pluton::client C("tThread4");
  if (!C.initialize(argv[1])) {
    clog << C.getFault().getMessage("Fatal: initialize() failed in tThread1")  << endl;
    exit(1);
  }

  C.setPollProxy(st_poll);
  st_thread_t t1, t2;
  t1 = st_thread_create(tStart, (void*)(&C), 1, 0);
  st_sleep(1);
  t2 = st_thread_create(tStart, (void*)(&C), 1, 0);	// Use the same client - a nono

  clog << "Waiting on t1" << endl;
  st_thread_join(t1, 0);

  clog << "Waiting on t2" << endl;
  st_thread_join(t2, 0);

  exit(0);
}
