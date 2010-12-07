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

#define	DEBUG	1


static time_t
baseTime()
{
  static time_t startTime = 0;
  if (startTime == 0) startTime = time(0);
  time_t now = time(0);

  return now - startTime;
}

static pluton::thread_t
tSelf(const char* who)
{
  st_thread_t self = st_thread_self();

#ifdef DEBUG
  clog << "tSelf called by " << who << " returning " << (void*)(self) << endl;
#endif

  return reinterpret_cast<pluton::thread_t>(self);
}


static pluton::mutex_t
mNew(const char* who)
{
  st_mutex_t myM = st_mutex_new();

#ifdef DEBUG
  clog << "mNew called by " << who << " returning " << (void*)(myM) << endl;
#endif

  return reinterpret_cast<pluton::mutex_t>(myM);
}

static void
mDelete(const char* who, pluton::mutex_t m)
{
#ifdef DEBUG
  clog << "mDelete called by " << who << " with " << (void*)(m) << endl;
#endif

  st_mutex_t myM = reinterpret_cast<st_mutex_t>(m);

  st_mutex_destroy(myM);
}


static int
mLock(const char* who, pluton::mutex_t m)
{
  st_mutex_t myM = reinterpret_cast<st_mutex_t>(m);

  int res = st_mutex_lock(myM);

#ifdef DEBUG
  clog << "mLock called by " << who << " on " << (void*)(m) << " returns " << res << endl;
#endif

  return res;
}

static int
mUnlock(const char* who, pluton::mutex_t m)
{
  st_mutex_t myM = reinterpret_cast<st_mutex_t>(m);

  int res = st_mutex_unlock(myM);

#ifdef DEBUG
  clog << "mUnlock called by " << who << " on " << (void*)(m) << " returns " << res << endl;
#endif

  return res;
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

  if (C.executeAndWaitAll() <= 0) {
    clog << C.getFault().getMessage("Fatal: executeAndWaitAll() failed in tThread1") << endl;
    exit(4);
  }

  clog << baseTime() << " " << who << " return from executeAndWaitAll" << endl;

  //////////////////////////////////////////////////////////////////////
  // A request can have a fault, even though the execute worked
  //////////////////////////////////////////////////////////////////////

  if (R1.hasFault()) {
     clog << "R1 Fault Code=" << R1.getFaultCode() << " Text=" << R1.getFaultText() << endl;
  }
  if (R2.hasFault()) {
     clog << "R2 Fault Code=" << R2.getFaultCode() << " Text=" << R2.getFaultText() << endl;
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


void*
tStart(void* a1)
{
  const threadParams* p = (threadParams*)(a1);
  clog << "thread " << p->name << " started" << endl;
  st_sleep(1);
  clog << "thread " << p->name << " post sleep" << endl;

  pluton::client C(p->name);

  C.initialize("");
  st_sleep(1);
  doBasicCalls(p->name, C, p->sleepTime1, p->sleepTime2);
  clog << "thread " << p->name << " exiting" << endl;
  return 0;
}


int
main(int argc, char** argv)
{
  st_init();

  pluton::client::setThreadHandlers(tSelf, mNew, mDelete, mLock, mUnlock);

  pluton::client cBase("tThread1");
  if (!cBase.initialize(argv[1])) {
    clog << cBase.getFault().getMessage("Fatal: initialize() failed in tThread1")  << endl;
    exit(1);
  }

  cBase.setPollProxy(st_poll);

  //////////////////////////////////////////////////////////////////////
  // Do some basic unthreaded stuff first.
  //////////////////////////////////////////////////////////////////////

  doBasicCalls("main", cBase);

  pluton::client c2;
  c2.initialize("");

  st_thread_t t1, t2, t3;
  threadParams p1("st t1", "1000", "3000");
  threadParams p2("st t2", "100", "200");
  threadParams p3("st t3", "1000", "2000");

  t1 = st_thread_create(tStart, (void*)(&p1), 1, 0);
  t2 = st_thread_create(tStart, (void*)(&p2), 1, 0);
  t3 = st_thread_create(tStart, (void*)(&p3), 1, 0);

  clog << "Waiting on t1" << endl;
  st_thread_join(t1, 0);

  clog << "Waiting on t2" << endl;
  st_thread_join(t2, 0);

  clog << "Waiting on t3" << endl;
  st_thread_join(t3, 0);

  exit(0);
}
