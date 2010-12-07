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

#ifndef _PLUTON_CLIENT_H
#define _PLUTON_CLIENT_H

//////////////////////////////////////////////////////////////////////
// Define the client APIs for the Pluton Framework. This is the only
// include a client needs to access the pluton client library.
//
// A client can either be a simple procedural program that blocks on
// calls to executeAndWait*() methods, or it can be a non-blocking
// client that manages events for the pluton library. The blocking
// client uses this class and the non-blocking client use the
// clientEvent class.
//////////////////////////////////////////////////////////////////////

#include <string>

#include <sys/time.h>

#include "pluton/fault.h"
#include "pluton/common.h"
#include "pluton/clientRequest.h"

#ifndef NO_INCLUDE_POLL
#include <poll.h>
#endif

namespace pluton {

  //////////////////////////////////////////////////////////////////////
  // Define the typedefs and prototypes for setThreadHandlers().
  //////////////////////////////////////////////////////////////////////

  typedef struct mutex* mutex_t;
  typedef struct thread* thread_t;

  typedef pluton::thread_t (*thread_self_t)(const char* who);		// Get thread ID
  typedef pluton::mutex_t (*mutex_new_t)(const char* who);		// Allocate and init a mutex
  typedef void (*mutex_delete_t)(const char* who, pluton::mutex_t);	// Destroy and free a mutex
  typedef int (*mutex_lock_t)(const char* who, pluton::mutex_t);	// Block until locked
  typedef int (*mutex_unlock_t)(const char* who, pluton::mutex_t);

  typedef int (*poll_handler_t)(struct pollfd *fds, int nfds, unsigned long long timeout);

  class perCallerClient;

  // pluton::client and pluton::clientEvent are derived from clientBase

  class clientBase {
  protected:
    clientBase(const char* yourName, unsigned int defaultTimeoutMilliSeconds);

  public:
    virtual ~clientBase() = 0;

    static const char*	getAPIVersion();

    bool		initialize(const char* lookupMapPath=0);	// BCI ok
    bool		hasFault() const;
    const pluton::fault& getFault() const;

    void		setDebug(bool nv);

  private:
    clientBase&	 operator=(const clientBase& rhs);	// Assign not ok
    clientBase(const clientBase& rhs);			// Copy not ok

  protected:
    perCallerClient*	_pcClient;
  };


  //////////////////////////////////////////////////////////////////////
  // This is the procedure-oriented class that most standard programs
  // will use to interface with the pluton services. The model is that
  // the client blocks on one of the executeAndWait*() calls until
  // all outstanding requests have completed or timed out.
  //
  // This class is also the one to use for psuedo-threads that are
  // also procedurally oriented, in particular State Threads that will
  // want to supply a proxy poll() routine via the setPollProxy()
  // method.
  //
  // Finally, this class now supports true threaded applications such
  // as those that use pthreads. There are rules and caveats around
  // how to do this that are covered in the docs.
  //////////////////////////////////////////////////////////////////////

  class client : public clientBase {
  public:
    client(const char* yourName="", unsigned int defaultTimeoutMilliSeconds=4000);
    ~client();

    void		setTimeoutMilliSeconds(unsigned int timeoutMilliSeconds);
    unsigned int	getTimeoutMilliSeconds() const;


    bool		addRequest(const char* serviceKey, pluton::clientRequest*);
    bool		addRequest(const char* serviceKey, pluton::clientRequest&);
    bool		addRawRequest(const char* serviceKey, pluton::clientRequest*,
				      const char* rawPtr, int rawLength);	// Tools only

    int				executeAndWaitSent();
    int				executeAndWaitAll();
    pluton::clientRequest*	executeAndWaitAny();
    int				executeAndWaitOne(pluton::clientRequest&);
    int				executeAndWaitOne(pluton::clientRequest*);

    void			reset();

    //////////////////////////////////////////////////////////////////////
    // Identify the user provided method that emulates the poll(2)
    // system call. While this is a per-instance method, that was a
    // mistake as it should have been a static method as the setting
    // applies to all future pluton calls. It only remains a
    // per-instance method to preserve binary compatability.
    //////////////////////////////////////////////////////////////////////

    pluton::poll_handler_t setPollProxy(pluton::poll_handler_t);

    //////////////////////////////////////////////////////////////////////
    // Identify the methods that will be used internally by the pluton
    // client when it needs to do thread-oriented actions.
    //////////////////////////////////////////////////////////////////////

    static void	setThreadHandlers(pluton::thread_self_t,
				  pluton::mutex_new_t mNew=0,
				  pluton::mutex_delete_t mDelete=0,
				  pluton::mutex_lock_t mLock=0,
				  pluton::mutex_unlock_t mUnlock=0);

  private:
    client&  operator=(const client& rhs);	// Assign not ok
    client(const client& rhs);			// Copy not ok
  };
}

#endif
