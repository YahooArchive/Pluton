#include <iostream>

using namespace std;

#include <stdlib.h>

#include <st.h>

#include "pluton/client.h"

// Test thread/mutex support using state threads //

static pluton::thread_t
myThreadSelf(const char* who)
{
  return (pluton::thread_t) st_thread_self();
}

static pluton::mutex_t
myMutexNew(const char* who)
{
  clog << "mutex_new: " << who << " " << (void*) myThreadSelf(who) << endl;

  return (pluton::mutex_t) st_mutex_new();
}

static void
myMutexDelete(const char* who, pluton::mutex_t m)
{
  clog << "mutex_destroy: " << who << " " << (void*) myThreadSelf(who) << endl;
  st_mutex_destroy((st_mutex_t) m);
}

static int
myMutexLock(const char* who, pluton::mutex_t m)
{
  //  clog << "mutex_lock: " << who << " " << (void*) myThreadSelf(who) << endl;
  if (st_mutex_trylock((st_mutex_t) m) == 0) return 0;

  clog << "mutex_lock and waiting: " << (void*) myThreadSelf(who) << endl;
  return st_mutex_lock((st_mutex_t) m);
}

static int
myMutexUnlock(const char* who, pluton::mutex_t m)
{
  //  clog << "mutex_unlock: " << who << " " << (void*) myThreadSelf(who) << endl;
  return st_mutex_unlock((st_mutex_t) m);
}


static void*
issueRequests(void *arg)
{
  clog << "Thread start: " << (void*) myThreadSelf("issueRequests") << endl;

  pluton::client C;
  pluton::clientRequest R1;
  pluton::clientRequest R2;

  for (int ix=0; ix < 200; ++ix) {
    C.addRequest("system.echo.0.raw", R1);
    R2.setContext("echo.sleepMS", "100");
    C.addRequest("system.echo.0.raw", R2);
    int res = C.executeAndWaitAll();
    if (res != 2) clog << "Thread:  " << (void*) myThreadSelf("issueRequests")
		       << " E&W=" << res << endl;
  }
  
  clog << "Thread exit: " << (void*) myThreadSelf("issueRequests") << endl;
  return 0;
}

int
main(int argc, char** argv)
{
  const char* goodPath = 0;
  if (argc > 1) goodPath = argv[1];

  st_init();

  clog << "state thread test started self=" << (void*) st_thread_self() << endl;

  pluton::client::setThreadHandlers(myThreadSelf, myMutexNew, myMutexDelete,
				    myMutexLock, myMutexUnlock);

  clog << "Globals set" << endl;

  pluton::client C("tClientStateThreads");
  if (!C.initialize(goodPath)) exit(2);

  for (int ix=0; ix < 20; ++ix) {
    st_thread_create(issueRequests, 0, 0, 0);
  }

  issueRequests(0);
  clog << "state thread test done" << endl;

  return(0);
}
