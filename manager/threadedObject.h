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

#ifndef P_THREADEDOBJECT_H
#define P_THREADEDOBJECT_H 1

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
