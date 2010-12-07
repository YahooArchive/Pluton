#include <sys/types.h>

#include <assert.h>
#include <unistd.h>
#include <signal.h>

#include "util.h"

#include "clientRequestImpl.h"
#include "perCallerClient.h"

using namespace pluton;

//////////////////////////////////////////////////////////////////////
// The perCallerClient class holds unique per-caller data but most of
// the functionality is in the singleton clientImpl.
//////////////////////////////////////////////////////////////////////

pluton::perCallerClient::perCallerClient(const char* yourName, unsigned int timeoutMilliSeconds)
  : _oneAtATimePerCaller(false), _threadedApplication(false),
    _timeoutMilliSeconds(timeoutMilliSeconds), _todoCount(0), _readingCount(0),
    _originalHandler(SIG_DFL), _myThreadImpl(0),
    _fdsSize(20), _fds(new struct pollfd[_fdsSize]),
    _fdsToRequests(new pluton::clientRequestImpl*[_fdsSize]),
    _completedQueue("completed")
{
  util::IA ia;
  _clientName = yourName;
  _clientName.append(":");
  _clientName.append(util::ltoa(ia, static_cast<int>(getpid())));
}

void
pluton::perCallerClient::blockSIGPIPE()
{
  _originalHandler = signal(SIGPIPE, SIG_IGN);
}

void
pluton::perCallerClient::restoreSIGPIPE()
{
  signal(SIGPIPE, _originalHandler);
}


void
pluton::perCallerClient::setThreadedApplication(bool tf, pluton::thread_t newID)
{
  _threadedApplication = tf;
  _owningThreadID = newID;
}


void
pluton::perCallerClient::addTodoCount()
{
  if (_todoCount++ == 0) {	// Timeout clock resets on transition from zero to one requests
    _clock.stop();
  }
}

//////////////////////////////////////////////////////////////////////
// Serialization protection methods. Neither the perCallerClient nor
// the clientImpl can be shared between threads. While this is well
// documented (and should be a default assumption of any object by all
// programmers), we "trust but verify".
//////////////////////////////////////////////////////////////////////

void
pluton::perCallerClient::assertMutexFreeThenLock()
{
  assert(_oneAtATimePerCaller == false);

  _oneAtATimePerCaller = true;
}

//////////////////////////////////////////////////////////////////////
// Manage the mapping between fd and clientRequestImpl so that poll
// events can be easily associated with the original request.
//////////////////////////////////////////////////////////////////////

void
pluton::perCallerClient::setRequestIndex(unsigned int ix, pluton::clientRequestImpl* R)
{
  assert(ix < _fdsSize);		// Internal bug
  _fdsToRequests[ix] = R;
}

pluton::clientRequestImpl*
pluton::perCallerClient::getRequestByIndex(unsigned int ix)
{
  if (ix >= _fdsSize) return 0;		// callback giving us bogus fds?
  return _fdsToRequests[ix];
}


//////////////////////////////////////////////////////////////////////
// Make sure the poll() structure and fd-to-Request map are big enough
// for all the possible fds. Reallocation does not retain original
// data so this call should only be made prior to populating the
// pollfd struct and clientRequestImpl pointer array.
//////////////////////////////////////////////////////////////////////

struct pollfd*
pluton::perCallerClient::sizePollArray(unsigned int entriesNeeded)
{
  if (entriesNeeded > _fdsSize) {
    delete [] _fds;
    delete [] _fdsToRequests;

    _fdsSize = entriesNeeded + entriesNeeded/2 + 1;
    _fds = new struct pollfd[_fdsSize];
    _fdsToRequests = new pluton::clientRequestImpl*[_fdsSize];
  }

  return _fds;
}


//////////////////////////////////////////////////////////////////////
// Queue management methods that are longer than one-liners.
//////////////////////////////////////////////////////////////////////

void
pluton::perCallerClient::eraseCompletedQueue(pluton::clientRequestImpl::state newState)
{
  while (!_completedQueue.empty()) {
    pluton::clientRequestImpl* R = _completedQueue.getFirst();
    R->setState("eraseQueue", newState);
    _completedQueue.deleteRequest(R);
  }
}


pluton::clientRequestImpl*
pluton::perCallerClient::popCompletedRequest()
{
  pluton::clientRequestImpl* R = _completedQueue.getFirst();
  if (R) _completedQueue.deleteRequest(R);

  return R;
}
