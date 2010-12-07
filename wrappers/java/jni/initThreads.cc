#include <iostream>

#include <jni.h>
#include <pthread.h>
#include <assert.h>

#include <pluton/client.h>

#include "com_yahoo_pluton_Client.h"

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

static pluton::thread_t tSelf(const char* who)
{
  return reinterpret_cast<pluton::thread_t>(pthread_self());
}

static volatile bool neverCalled = true;
static volatile pluton::thread_t me;

extern "C" JNIEXPORT void JNICALL
Java_com_yahoo_pluton_Client_JNI_1initThreads(JNIEnv *env, jclass jthis)
{
  //////////////////////////////////////////////////////////////////////
  // Make sure we're only called once to catch JNI use errors. Only
  // the pretty facade should be calling the JNI layer and that should
  // not be making these sort of mistakes! In particular, the
  // following sequence catches two pthreads trying to call me at the
  // same time. This is somewhat like Peterson's Algorithm.
  //////////////////////////////////////////////////////////////////////

  me = tSelf("");			// 01
  assert(neverCalled == true);		// 02
  neverCalled = false;			// 03

  // Threads can no longer get past step 02

  // But multiple threads might be past step 02 at the same instant in
  // which case they have executed step 01 which means at least one
  // thread must fail the next test.

  assert(me == tSelf(""));		// 04

  pluton::client::setThreadHandlers(tSelf, mNew, mDelete, mLock, mUnlock);
}
