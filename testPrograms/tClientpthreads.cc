#include <iostream>

using namespace std;

#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#include "pluton/client.h"

// Test thread/mutex support using pthreads //

static pluton::thread_t
myThreadSelf(const char* who)
{
  return (pluton::thread_t) pthread_self();
}

static pluton::mutex_t
myMutexNew(const char* who)
{
  clog << "mutex_new: " << (void*) myThreadSelf(who) << endl;

  pthread_mutex_t *m = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  if (pthread_mutex_init(m, 0) != 0) return 0;

  return (pluton::mutex_t) m;
}

static void
myMutexDelete(const char* who, pluton::mutex_t m)
{
  clog << "mutex_destroy: " << (void*) myThreadSelf(who) << endl;
  pthread_mutex_destroy((pthread_mutex_t*) m);
  free((void*) m);
}

static int
myMutexLock(const char* who, pluton::mutex_t m)
{
  clog << "mutex_lock: " << (void*) myThreadSelf(who) << endl;
  if (pthread_mutex_trylock((pthread_mutex_t*) m) == 0) return 0;

  clog << "mutex_lock and waiting: " << (void*) myThreadSelf(who) << endl;
  return pthread_mutex_lock((pthread_mutex_t*) m);
}

static int
myMutexUnlock(const char* who, pluton::mutex_t m)
{
  clog << "mutex_unlock: " << (void*) myThreadSelf(who) << endl;
  return pthread_mutex_unlock((pthread_mutex_t*) m);
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

  clog << "pthread test started self=" << (void*) pthread_self() << endl;

  pluton::client::setThreadHandlers(myThreadSelf, myMutexNew, myMutexDelete,
				    myMutexLock, myMutexUnlock);

  pluton::client C("tClientpthreads");
  if (!C.initialize(goodPath)) exit(2);

  for (int ix=0; ix < 20; ++ix) {
    pthread_t thr;
    pthread_create(&thr, 0, issueRequests, 0);
  }

  issueRequests(0);
  clog << "pthread thread test done" << endl;

  return(0);
}
