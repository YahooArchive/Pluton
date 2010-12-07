/*
Copyright (c) 2010, Yahoo! Inc. All rights reserved.

Redistribution and use of this software in source and binary forms, with or
without modification, are permitted provided that the following conditions are
met: 

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation and/or 
other materials provided with the distribution.

* Neither the name of Yahoo! Inc. nor the names of its contributors may be used 
to endorse or promote products derived from this software without specific prior 
written permission of Yahoo! Inc.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

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
