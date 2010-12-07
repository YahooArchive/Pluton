#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include <string>
#include <iostream>

using namespace std;

#include "pluton/client.h"

//////////////////////////////////////////////////////////////////////
// Test the threaded interface to the pluton client library.
//////////////////////////////////////////////////////////////////////

#define	DEBUG	1

static time_t
baseTime()
{
  static time_t startTime = 0;
  if (startTime == 0) startTime = time(0);
  time_t now = time(0);

  return now - startTime;
}


class pluton::mutex {
public:
  mutex() { pthread_mutex_init(&pm, 0); }
  ~mutex() { pthread_mutex_destroy(&pm); }
  pthread_mutex_t pm;
};

static pluton::mutex_t mNew(const char* who) { return new pluton::mutex; }
static void mDelete(const char* who, pluton::mutex_t m) { delete m; }
static int mLock(const char* who, pluton::mutex_t m) { return pthread_mutex_lock(&m->pm); }
static int mUnlock(const char* who, pluton::mutex_t m) { return pthread_mutex_unlock(&m->pm); }

static pluton::thread_t
tSelf(const char* who)
{
  return reinterpret_cast<pluton::thread_t>(pthread_self());
}


static void
doBasicCalls(const char* who, pluton::client& C,
	     const char* sleepTime1="1000", const char* sleepTime2="2000")
{
  pluton::clientRequest R1;
  pluton::clientRequest R2;

  R1.setRequestData("This is some request data");

  R1.setContext("echo.log", "y");
  R1.setContext("echo.sleepMS", sleepTime1);

  string r2req = "More of the same";
  R2.setRequestData(r2req);

  R2.setContext("echo.log", "y");
  R2.setContext("echo.sleepMS", sleepTime2);

  //////////////////////////////////////////////////////////////////////
  // Add them to the execution list. This time alternate the use
  // of hasFault() to test for errors rather than the return value.
  //////////////////////////////////////////////////////////////////////

  clog << baseTime() << " " << who << " Adding R1" << endl;
  C.addRequest("system.echo.0.raw", &R1);
  if (C.hasFault()) {
    clog << C.getFault().getMessage("Fatal: addRequest(R1) failed in tThread1") << endl;
    exit(2);
  }

  clog << baseTime() << " " << who << " Adding R2" << endl;
  if (!C.addRequest("system.echo.0.raw", &R2)) {
    clog << C.getFault().getMessage("Fatal: addRequest(R2) failed in tThread1") << endl;
    exit(3);
  }

  //////////////////////////////////////////////////////////////////////
  // Now execute the requests and wait for them all to complete
  //////////////////////////////////////////////////////////////////////

  clog << baseTime() << " " << who << " entering executeAndWaitAll" << endl;

  int res = C.executeAndWaitAll();
  if (res <= 0) {
    clog << C.getFault().getMessage("Fatal: executeAndWaitAll() failed in tThread1")
	 << " res=" << res << endl;
    exit(4);
  }

  clog << baseTime() << " " << who << " return from executeAndWaitAll" << endl;

  //////////////////////////////////////////////////////////////////////
  // A request can have a fault, even though the execute worked
  //////////////////////////////////////////////////////////////////////

  if (R1.hasFault()) {
    clog << baseTime()
	 << "R1 Fault Code=" << R1.getFaultCode() << " Text=" << R1.getFaultText() << endl;
  }
  if (R2.hasFault()) {
    clog << baseTime()
	 << "R2 Fault Code=" << R2.getFaultCode() << " Text=" << R2.getFaultText() << endl;
  }

  string resp1;
  string resp2;
  R1.getResponseData(resp1);
  R2.getResponseData(resp2);

  clog << baseTime() << " " << who << " R1 Response: " << resp1 << endl;
  clog << baseTime() << " " << who << " R2 Response: " << resp2 << endl;

  const char* p;
  int l;
  R1.getResponseData(p, l);

  clog << baseTime() << " " << who << " R1 another way: " << string(p,l) << endl;
}



class threadParams {
public:
  threadParams(const char* n, const char* s1, const char* s2)
    : name(n), sleepTime1(s1), sleepTime2(s2) {}

  const char* name;
  const char* sleepTime1;
  const char* sleepTime2;
};


static const char* lookupMap = 0;

void*
tStart(void* a1)
{
  const threadParams* p = (threadParams*)(a1);
  clog << baseTime() << "thread " << p->name << " started" << endl;
  sleep(1);
  clog << baseTime() << "thread " << p->name << " post sleep" << endl;

  pluton::client C(p->name);

  C.initialize(lookupMap);
  sleep(1);
  doBasicCalls(p->name, C, p->sleepTime1, p->sleepTime2);
  clog << baseTime() << "thread " << p->name << " exiting" << endl;
  return 0;
}


int
main(int argc, char** argv)
{
  lookupMap = argv[1];
  pluton::client::setThreadHandlers(tSelf, mNew, mDelete, mLock, mUnlock);

  pluton::client cBase("tThread1");
  if (!cBase.initialize(lookupMap)) {
    clog << baseTime()
	 << cBase.getFault().getMessage("Fatal: initialize() failed in tThread1")  << endl;
    exit(1);
  }

  //////////////////////////////////////////////////////////////////////
  // Do some basic unthreaded stuff first.
  //////////////////////////////////////////////////////////////////////

  doBasicCalls("main", cBase);

  pthread_t t1, t2, t3;
  threadParams p1("pthread t1", "1000", "3000");
  threadParams p2("pthread t2", "100", "200");
  threadParams p3("pthread t3", "1000", "2000");

  pthread_create(&t1, 0, tStart, (void*)(&p1));
  pthread_create(&t2, 0, tStart, (void*)(&p2));
  pthread_create(&t3, 0, tStart, (void*)(&p3));

  clog << baseTime()  << "Waiting on t1" << endl;
  pthread_join(t1, 0);

  clog << baseTime() << "Waiting on t2" << endl;
  pthread_join(t2, 0);

  clog << baseTime() << "Waiting on t3" << endl;
  pthread_join(t3, 0);

  exit(0);
}
