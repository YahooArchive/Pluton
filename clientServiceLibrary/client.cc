#include <iostream>
#include <hash_mapWrapper.h>

#include <sys/types.h>

#include <assert.h>
#include <signal.h>
#include <unistd.h>

#include "version.h"

#include "pluton/fault.h"
#include "pluton/client.h"

#include "util.h"
#include "clientImpl.h"

using namespace pluton;

//////////////////////////////////////////////////////////////////////
// All user visible classes are encapsulated in a wrapper class to
// protect against binary compatability.
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// The three main data-structures used by this library are:
//
// 1) the requestImpl itself of course, which contains the request
// information
//
// 2) the perCallerClient which presents the veneer of a queue of
// requests for a given caller (the definition of a caller is a
// pluton::client instance).
//
// 3) the clientImpl which manages the exchange of requests for all
// the perCallerClient objects associated with a single thread.
//
// There are two reasons for the distinction between the
// perCallerClient and the clientImpl objects.
//
// The first is that the prescribed (and desired) functionality
// provided by the pluton client library is that *all*
// pluton::clientRequests added to all pluton::client objects will be
// executed when control is passed to any of the
// pluton::client::executeAndWait*() methods for a given thread.
//
// The reason for this functionality is that a parent routine can
// populate its own pluton::client with requests, call a subroutine
// that in turn creates its own set of requests on another instance of
// a pluton::client. While this library is executing on the subroutine
// requests, due to the shared clientImpl, this library also *sees*
// and progresses the parent requests thus maximizing the parallel
// nature of the library.
//
// The second reason for this distinction is thread support. A unique
// clientImpl is created for each thread so that the only internal
// thread-safe locking required is at construction of the
// perCallerClient. To achive this minimalist locking, The only
// imposition on a threaded application is that it *must not* share
// pluton::client and pluton::clientRequest objects across threads.
//
// Assigning a clientImpl to each thread means that there are actually
// separate queues of requests for each thread and that they are not
// progressed until the owning thread calls one of the
// executeAndWait*() methods. In other words one thread cannot rely on
// another thread to progress its own requests, unlike the case with
// separate pluton::client objects within a single thread.
//
// In summary then, each thread has one clientImpl which contains the
// queue of all outstanding pluton::clientRequests for that
// thread. All pluton::client objects created by that thread share
// this one clientImpl and pluton::clientRequests in all those
// pluton::client objects will progress with any call to
// executeAndWait*().
//
// An unthreaded application is simply a degenerate case where there
// is just one thread and thus one instance of a clientImpl which is
// in effect a singleton for the whole program.
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// Thread-related classes and statics. The idea is that these static
// methods such as thread_self() can always be called regardless of
// what the application has done.
//////////////////////////////////////////////////////////////////////

class threadHandlers {
public:
  pluton::thread_self_t 	_threadSelf;
  pluton::mutex_new_t		_mutexNew;
  pluton::mutex_delete_t 	_mutexDelete;
  pluton::mutex_lock_t	      	_mutexLock;
  pluton::mutex_unlock_t      	_mutexUnlock;
};


//////////////////////////////////////////////////////////////////////
// These next static values ensure that the caller follows the rules
// regarding threads and pluton. Namely that the threadHandlers must
// be set prior to any constructor calls and that they can only be set
// the once.
//
// The assert() macro is used if they fail to follow these rules as
// it's a failure of insidious consequences that warrants a more
// significant response than hoping a caller notices a error return
// code.
//////////////////////////////////////////////////////////////////////

static bool tooLateToSetThreadHandlers = false;
static threadHandlers* tHandlers = 0;
static pluton::mutex_t threadToClientImplMapMutex = 0;

static pluton::thread_t
thread_self(const char* who)
{
  return tHandlers && tHandlers->_threadSelf ? tHandlers->_threadSelf(who) : 0;
}

static pluton::mutex_t
mutex_new(const char* who)
{
  return tHandlers && tHandlers->_mutexNew ? tHandlers->_mutexNew(who) : 0;
}

#ifdef NO

This method is not needed because the solitory mutex protects the tid -> impl
map which never gets destroyed.

static void
mutex_delete(const char* who, pluton::mutex_t m)
{
  if (tHandlers && tHandlers->_mutexDelete) tHandlers->_mutexDelete(who, m);
}

#endif

static int
mutex_lock(const char* who, pluton::mutex_t m)
{
  return tHandlers && tHandlers->_mutexLock ? tHandlers->_mutexLock(who, m) : 0;
}

static int
mutex_unlock(const char* who, pluton::mutex_t m)
{
  return tHandlers && tHandlers->_mutexUnlock ? tHandlers->_mutexUnlock(who, m) : 0;
}


//////////////////////////////////////////////////////////////////////
// Static routines to Create/Assign a clientImpl per thread. A
// hash_map is used to map the thread id to a unique clientImpl. In
// effect these are per-thread singletons.
//////////////////////////////////////////////////////////////////////

struct hash_thread_t
{
  size_t operator()(const pluton::thread_t key) const
  {
    return (size_t) key;
  }
};


typedef hash_map<pluton::thread_t, pluton::clientImpl*, hash_thread_t> threadToClientImplMap;
typedef threadToClientImplMap::iterator threadToClientImplMapIter;

static threadToClientImplMap* _threadToClientMap = 0;


//////////////////////////////////////////////////////////////////////
// Assign a clientImpl for this particular thread or return the
// pre-allocated one if it already exists. Using the typical method of
// getting tls would involve a static reference to a pthread-specific
// implementation, ie: PTHREAD_ONCE_INIT - thus a custom tls method is
// implemented here for pluton.
//////////////////////////////////////////////////////////////////////

pluton::clientImpl*
getPerThreadClientImpl(const char* who)
{
  pluton::thread_t self = thread_self(who);	// Get this thread's identity to
  mutex_lock(who, threadToClientImplMapMutex);	// protect possible map changes

  // Create the map if it does not yet exist

  if (!_threadToClientMap) _threadToClientMap = new threadToClientImplMap;

  threadToClientImplMapIter iter = _threadToClientMap->find(self);	// has self got a clientImpl?

  pluton::clientImpl* cip = 0;
  if (iter == _threadToClientMap->end()) {	// Has this thread already got a clientImpl?
    cip = new pluton::clientImpl;		// Nope, create one
    (*_threadToClientMap)[self] = cip;		// and add it into the map for this thread
  }
  else {
    cip = iter->second;				// Otherwise, retrieve the existing impl
  }

  mutex_unlock(who, threadToClientImplMapMutex);	// Done with map

  cip->incrementUseCount();			// Record how many perCallers are using it

  return cip;
}


//////////////////////////////////////////////////////////////////////
// A pluton::client is being destroyed. If it is the last one that
// uses the underlying clientImpl for this thread, then remove the
// clientImpl altogether.
//////////////////////////////////////////////////////////////////////

static void
releasePerThreadClientImpl(const char* who, pluton::clientImpl* cip)
{
  assert(_threadToClientMap != 0);		// If we don't have this by now, panic

  //////////////////////////////////////////////////////////////////////
  // If this thread now has zero pluton::client instances, then
  // destroy the underlying objects and existence thereof.
  //////////////////////////////////////////////////////////////////////

  int useCountNow = cip->decrementUseCount();
  if (useCountNow == 0) {
    delete cip;
    mutex_lock(who, threadToClientImplMapMutex);	// Protect the map
    _threadToClientMap->erase(thread_self(who));
    mutex_unlock(who, threadToClientImplMapMutex);	// Done with map
  }
}


//////////////////////////////////////////////////////////////////////
// On some older OSes, namely FreeBSD4, there is no way to stop a
// SIGPIPE apart from blocking the signal. New OSes let you disable
// them with setsockopt() or a send() option. Original the following
// two routines were part of determining whether either of those
// options were available, if not, turn off the signal around any
// methods that issue I/O to the socket. Gack.
//
// As it turns out, this was a pain in the butsky - in part because of
// the desire to preserve existing signal handlers. So, for the
// longest time, these routines have done nothing which has turned out
// just fine.
//////////////////////////////////////////////////////////////////////

#define	blockSIGPIPE()
#define restoreSIGPIPE()


//////////////////////////////////////////////////////////////////////
// Bridge to implementation, using a wrapper for per-caller unique
// data which in turn points to a per-thread object for the request
// execution queue.
//////////////////////////////////////////////////////////////////////

pluton::clientBase::clientBase(const char* yourName, unsigned int timeoutMilliSeconds)
{
  tooLateToSetThreadHandlers = true;	// Must call setThreadHandlers() prior to this constructor

  _pcClient = new pluton::perCallerClient(yourName, timeoutMilliSeconds);
  _pcClient->setThreadedApplication(tHandlers != 0, thread_self("pluton::clientBase::clientBase"));
  _pcClient->setMyThreadImpl(getPerThreadClientImpl("pluton::clientBase::clientBase"));
}

pluton::clientBase::~clientBase()
{
  releasePerThreadClientImpl("pluton::clientBase::~clientBase", _pcClient->getMyThreadImpl());
  delete _pcClient;
}

pluton::client::client(const char* yourName, unsigned int timeoutMilliSeconds)
  : clientBase(yourName, timeoutMilliSeconds)
{
}

pluton::client::~client()
{
}

const char*
pluton::clientBase::getAPIVersion()
{
  return version::rcsid;
}


//////////////////////////////////////////////////////////////////////
// We're kinda cheap by re-using the same mutex rather than getting
// our own.
//////////////////////////////////////////////////////////////////////

bool
pluton::clientBase::initialize(const char* lookupMapPath)
{
  blockSIGPIPE();
  mutex_lock("pluton::clientBase::initialize", threadToClientImplMapMutex);
  bool res = _pcClient->getMyThreadImpl()->initialize(_pcClient, lookupMapPath);
  mutex_unlock("pluton::clientBase::initialize", threadToClientImplMapMutex);
  restoreSIGPIPE();

  return res;
}


bool
pluton::clientBase::hasFault() const
{
  return _pcClient->hasFault();
}

const pluton::fault&
pluton::clientBase::getFault() const
{
  return _pcClient->getFault();
}


//////////////////////////////////////////////////////////////////////
// Reset only applies to the per-caller client
//////////////////////////////////////////////////////////////////////

void
pluton::client::reset()
{
  _pcClient->getMyThreadImpl()->reset(_pcClient);
}

void
pluton::clientBase::setDebug(bool nv)
{
  _pcClient->getMyThreadImpl()->setDebug(nv);
}


//////////////////////////////////////////////////////////////////////
// The timeout value is a per-client setting rather than the singleton
// setting.
//////////////////////////////////////////////////////////////////////

void
pluton::perCallerClient::setTimeoutMilliSeconds(unsigned int setMS)
{
  _timeoutMilliSeconds = setMS;
}


void
pluton::client::setTimeoutMilliSeconds(unsigned int timeoutMilliSeconds)
{
  _pcClient->setTimeoutMilliSeconds(timeoutMilliSeconds);
}

unsigned int
pluton::client::getTimeoutMilliSeconds() const
{
  return _pcClient->getTimeoutMilliSeconds();
}

pluton::poll_handler_t
pluton::client::setPollProxy(pluton::poll_handler_t newHandler)
{

  //////////////////////////////////////////////////////////////////////
  // Having a pollProxy routine in threaded mode can make sense with
  // pthreads (but not state threads)... Nonetheless, one hopes the
  // caller really knows what they are doing here. Originally we
  // asserted against the possibility but have allowed it - hoping
  // that only smart programmers go here.

  //  assert((newHandler == 0) || (_pcClient->getThreadedApplication() == false));
  //////////////////////////////////////////////////////////////////////

  return _pcClient->getMyThreadImpl()->setPollProxy(newHandler);
}


//////////////////////////////////////////////////////////////////////
// Thread initialization must occur just once and prior to any calls
// to pluton::client or pluton::clientEvent constructors. The reason
// for this constraint is that the constructor needs to allocate a
// clientImpl based on the thread ID ergo the ability to determine a
// thread ID must be possible prior to the constructor.
//////////////////////////////////////////////////////////////////////

void
pluton::client::setThreadHandlers(pluton::thread_self_t tSelf,
				  pluton::mutex_new_t mNew, pluton::mutex_delete_t mDelete,
				  pluton::mutex_lock_t mLock, pluton::mutex_unlock_t mUnlock)
{
  assert(tooLateToSetThreadHandlers == false);

  bool setThreadHandlersNeverCalled = (tHandlers == 0);	// Make the assert human friendly
  assert(setThreadHandlersNeverCalled == true);		// Can only call once per process

  tHandlers = new threadHandlers;

  tHandlers->_threadSelf = tSelf;
  tHandlers->_mutexNew = mNew;
  tHandlers->_mutexDelete = mDelete;
  tHandlers->_mutexLock = mLock;
  tHandlers->_mutexUnlock = mUnlock;

  //////////////////////////////////////////////////////////////////////
  // Create a mutex for the clientWrapper constructor that creates
  //  per-thread clientImpls. This mutex is never deleted..
  //////////////////////////////////////////////////////////////////////

  threadToClientImplMapMutex = mutex_new("pluton::client::setThreadHandlers");
}


//////////////////////////////////////////////////////////////////////
// addRequest signatures for adding a request to the list. We
// purposely want serviceKey to be a \0 terminated string, so
// constrain via the signatures.
//////////////////////////////////////////////////////////////////////

bool
pluton::client::addRequest(const char* serviceKey, pluton::clientRequest& R)
{
  return addRequest(serviceKey, &R);
}

bool
pluton::client::addRequest(const char* serviceKey, pluton::clientRequest* R)
{
  pluton::clientRequestImpl* rImpl = R->getImpl();
  rImpl->setClientName(_pcClient->getClientName());
  rImpl->setTimeoutMS(_pcClient->getTimeoutMilliSeconds());
  rImpl->setClientRequestPtr(R);

  return _pcClient->getMyThreadImpl()->addRequest(_pcClient, serviceKey, rImpl);
}


//////////////////////////////////////////////////////////////////////
// This minimalist interface is just for tools that are compiled with
// the library/manager. Simply pass the completed request through to
// the singleton.
//////////////////////////////////////////////////////////////////////

bool
pluton::client::addRawRequest(const char* serviceKey, pluton::clientRequest* R,
			      const char* rawPtr, int rawLength)
{
  pluton::clientRequestImpl* rImpl = R->getImpl();
  rImpl->setClientRequestPtr(R);

  return _pcClient->getMyThreadImpl()->addRequest(_pcClient, serviceKey, rImpl, rawPtr, rawLength);
}


//////////////////////////////////////////////////////////////////////
// Amongst other things, these executeAndWait* entry points are good
// times to check that a threaded application is honoring the
// constraints documented when using this library. Not all entry
// points bother with this check because it's not worth the exta
// typing when the executeAndWait* ones are the ones most at risk.
//////////////////////////////////////////////////////////////////////

int
pluton::client::executeAndWaitAll()
{
  assert(_pcClient->threadOwnerUnchanged(thread_self("executeAndWaitAll")));
  blockSIGPIPE();
  int res = _pcClient->getMyThreadImpl()->executeAndWaitAll(_pcClient);
  restoreSIGPIPE();

  return res;
}

int
pluton::client::executeAndWaitSent()
{
  assert(_pcClient->threadOwnerUnchanged(thread_self("executeAndWaitSent")));
  blockSIGPIPE();
  int res = _pcClient->getMyThreadImpl()->executeAndWaitSent(_pcClient);
  restoreSIGPIPE();

  return res;
}

#ifdef NO
int
pluton::client::executeAndWaitBlocked(unsigned int timeoutMilliSeconds)
{
  assert(_pcClient->threadOwnerUnchanged(thread_self("executeAndWaitBlocked")));
  blockSIGPIPE();
  int res = _pcClient->getMyThreadImpl()->executeAndWaitBlocked(_pcClient, timeoutMilliSeconds);
  restoreSIGPIPE();

  return res;
}
#endif

pluton::clientRequest*
pluton::client::executeAndWaitAny()
{
  assert(_pcClient->threadOwnerUnchanged(thread_self("executeAndWaitAny")));
  blockSIGPIPE();
  pluton::clientRequest* R = _pcClient->getMyThreadImpl()->executeAndWaitAny(_pcClient);
  restoreSIGPIPE();

  return R;
}

int
pluton::client::executeAndWaitOne(pluton::clientRequest* R)
{
  assert(_pcClient->threadOwnerUnchanged(thread_self("executeAndWaitOne Ptr")));
  blockSIGPIPE();
  int res = _pcClient->getMyThreadImpl()->executeAndWaitOne(_pcClient, R->getImpl());
  restoreSIGPIPE();

  return res;
}

int
pluton::client::executeAndWaitOne(pluton::clientRequest &R)
{
  assert(_pcClient->threadOwnerUnchanged(thread_self("executeAndWaitOne Ref")));
  blockSIGPIPE();
  int res = _pcClient->getMyThreadImpl()->executeAndWaitOne(_pcClient, R.getImpl());
  restoreSIGPIPE();

  return res;
}


//////////////////////////////////////////////////////////////////////
// The perCallerClient destructor is in this source module so that it
// has access to the client singleton.
//
// If a per-caller instance is destroyed, remove the matching requests
// from the singleton.
//////////////////////////////////////////////////////////////////////

pluton::perCallerClient::~perCallerClient()
{
  if (_todoCount > 0) _myThreadImpl->deleteOwner(this);
  delete [] _fds;
  delete [] _fdsToRequests;
}
