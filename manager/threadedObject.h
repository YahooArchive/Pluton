#ifndef _THREADEDOBJECT_H
#define _THREADEDOBJECT_H

//////////////////////////////////////////////////////////////////////
// All of the significant objects in the plutonManager have the same
// framework:
//
// - a controlling thread
// - a common set of methods to start, stop, initialize, etc.
// - manage objects of the same form
// - communicate with a parent thread to indicate state changes.
//
// Even though state threads makes sharing objects amongst threads a
// lot easier, all thread approaches require a careful distribution of
// work so that anyone operation doesn't block a thread from doing the
// work it should do. Simply put, that means that long-running
// operations are only performed by the owning thread.
//////////////////////////////////////////////////////////////////////

#include <string>

#include <st.h>

class threadedObject {
 public:
  threadedObject(threadedObject* setParent=0);
  virtual ~threadedObject();

  //////////////////////////////////////////////////////////////////////
  // Interfaces supplied by derived class - LR = Long Running, owner
  // thread only and NOok means that it is ok for the non-owning
  // thread to call this method.
  //////////////////////////////////////////////////////////////////////


  virtual bool	initialize() = 0;		// NOok: Called once - creates owner thread
  virtual void	preRun() = 0;			// Called once by owning thread
  virtual bool	run() = 0;			// LR: Called repeatedly
  virtual void	runUntilIdle() = 0;		// LR: Called after shutdown initiated
  virtual void	initiateShutdownSequence(const char* reason) = 0;	// NOok: Called once
  virtual void	completeShutdownSequence() = 0;				// LR: Called once
  virtual void	destroyOffspring(threadedObject* to=0, const char* reason="") = 0;

  virtual const char*	getType() const = 0;

  // Predefined standard interfaces

  void	notifyOwner(const char* who, const char* why);

  void		setThreadID(st_thread_t nid) { _threadID = nid; }
  st_thread_t	getThreadID() const { return _threadID; }

  const	char* 	getLogID() const { return _logID.c_str(); }
  bool		error() const { return !_errorMessage.empty(); }

  threadedObject*	getParent() { return _parent; }

  const std::string&	getErrorMessage() const { return _errorMessage; } 
  void          	clearError() { _errorMessage.erase(); }

  bool	shutdownInProgress() const { return _shutdownInProgressFlag; }
  bool	shutdownComplete() const { return _shutdownCompleteFlag; }

 protected:
  void	baseInitiateShutdownSequence();

  void	enableInterrupts();
  void	disableInterrupts();


  std::string	_errorMessage;

  st_thread_t	_threadID;
  bool		_interruptsEnabled;

  std::string	_logID;

 private:
  threadedObject*	_parent;
  bool			_shutdownInProgressFlag;
  bool			_shutdownCompleteFlag;
};

#endif
