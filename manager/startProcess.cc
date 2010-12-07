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
#include <string>

#include <sys/stat.h>
#include <sys/types.h>

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <st.h>

#include "debug.h"
#include "logging.h"
#include "util.h"
#include "lineToArgv.h"
#include "global.h"
#include "pidMap.h"
#include "processExitReason.h"
#include "process.h"
#include "service.h"
#include "manager.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// All of the child interactions occurs here. In particular the
// forking, arrangement and monitoring of fds, as well as st_thread
// management. A process is the managing object for a child and
// follows the life-cycle of a child.
//////////////////////////////////////////////////////////////////////

static void*
processHandlerThread(void* voidP)
{
  process* P = static_cast<process*> (voidP);

  if (debug::process()) DBGPRT << "Process Started: " << P->getLogID() << endl;

  P->run();
  P->runUntilIdle();

  if (P->error()) LOGPRT << "Process Error: "
			 << P->getLogID() << " " << P->getErrorMessage() << endl;

  if (debug::process()) DBGPRT << "processHandlerThread: shutting down: " << P->getLogID() << endl;
  P->completeShutdownSequence();

  const char* backoff = P->getBackoffReason();
  string id = P->getLogID();			// P gets destroyed, so save for debug
  const service* S = P->getSERVICE();		// P gets destroyed, ...
  P->getParent()->destroyOffspring(P, backoff);	// Tell parent to destroy my object

  if (debug::process()) DBGPRT << "processHandlerThread: shutdown complete: " << id
			       << " remaining=" << S->getActiveProcessCount() << endl;

  return 0;
}


//////////////////////////////////////////////////////////////////////
// If we're running as root, setuid the service down to either the
// default uid or the uid of the executable if it is setuid and is not
// setuid root.
//////////////////////////////////////////////////////////////////////

int
process::setuidMaybe(const char* execPath) const
{
  if (geteuid() != 0) return 0;		      // Can't change anything bud!

  uid_t defaultUID = getSERVICE()->getMANAGER()->getDefaultUID();
  if (defaultUID < 1) return 0;		      // Won't or can't
  gid_t defaultGID = getSERVICE()->getMANAGER()->getDefaultGID();
  if (defaultGID < 1) return 0;		      // Won't or can't

  struct stat sb;
  if (stat(execPath, &sb) == -1) {
    perror("Warning: stat() of exec path failed");
    return 0;
  }

  //////////////////////////////////////////////////////////////////////
  // setuid to our default uid unless the executable is setuid to some
  // other non-root value. We do the setuid so that service
  // programmers don't have to wrap scripting programs (that are not
  // allowed to be setuid by the kernel).
  //////////////////////////////////////////////////////////////////////

  if ((sb.st_mode & S_ISUID) && (sb.st_uid > 0)) defaultUID = sb.st_uid;
  if ((sb.st_mode & S_ISGID) && (sb.st_gid > 0)) defaultGID = sb.st_gid;

  if (debug::child()) DBGPRT << "Child seteuid(" << defaultUID << ") "
			     << "setegid(" << defaultGID << ")" << endl;

  if (setegid(defaultUID) == -1) return -23;
  if (seteuid(defaultGID) == -1) return -24;

  return defaultUID;
}


//////////////////////////////////////////////////////////////////////
// Start the process instance and associated thread.
//////////////////////////////////////////////////////////////////////

bool
process::startThread(process* P)
{
  P->setThreadID(st_thread_create(processHandlerThread, (void*) P, 0,
				  P->_S->getMANAGER()->getStStackSize()));
  if (debug::thread()) DBGPRT << "Process Thread Start: " << P->getThreadID() << endl;

  return (P->getThreadID() != 0);
}


//////////////////////////////////////////////////////////////////////
// Fork/exec the child and create the necessary plumbing around
// it. Return false if process failed to start. errorMessage will be
// set.
//
// One a separate process has started, it has no direct access to the
// parent structures for the purposes of reporting errors. To
// communicate start-up errors, the child process uses the following
// exit codes:
//////////////////////////////////////////////////////////////////////

const char*
process::exitCodeToEnglish(int ec)
{
  switch (ec) {
  case 11: return "calloc() of argv[] failed";
  case 12: return "Parse of exec command line failed";
  case 13: return "execv() failed";
  case 14: return "chdir failed";
  case 15: return "dup2 of STDIN to STDOUT failed";
  case 16: return "dup2 of STDERR failed";
  case 17: return "dup2 of acceptSocket failed";
  case 18: return "fcntl(acceptSocket, NON_BLOCK) failed";
  case 19: return "dup2 of shmFD failed";
  case 20: return "dup2 of reportingSocket failed";
  case 21: return "Could not open /dev/null to STDIN";
  case 22: return "Could not open /dev/null to STDOUT";
  case 23: return "Could not seteuid to match exec file";
  case 24: return "Could not setegid to match exec file";
  case 25: return "Spare";
  case 26: return "Spare";
  case 27: return "Spare";
  case 28: return "Spare";
  case 29: return "Spare";
  case 30: return "Spare";
  default: break;
  }

  return "Abnormal Exit Code";
}


//////////////////////////////////////////////////////////////////////
// Fork/exec the service process. In addition:
//
// o Arrange for the inherited fds to be duped to specific fds
//
// o Release all resources that the child would otherwise inherit from
// the parent.
//////////////////////////////////////////////////////////////////////

bool
process::forkExecChild(int acceptSocket, int shmFD, int reportingSocket)
{
  if (pipe(_fildesSTDERR) == -1) {
    util::messageWithErrno(_errorMessage, "pipe() failed for STDERR");
    return false;
  }
  if (util::setCloseOnExec(_fildesSTDERR[0]) == -1) perror("Warning: FD_CLOEXEC(e0) failed");

  _pid = fork();
  if (_pid == -1) {
    util::messageWithErrno(_errorMessage, "fork() failed");
    return false;
  }

  ////////////////////////////////////////////////////////////
  // Parent
  ////////////////////////////////////////////////////////////

  if (_pid != 0) {
    _shmService->setProcessPID(_id, _pid);	// Tell child where to update and
    bool added = pidMap::add(_pid, this);	// reaper about pid->process map

    //////////////////////////////////////////////////////////////////////
    // It's extremely unlikely, but possible that the OS may re-use a
    // pid prior to the manager completely cleaning up and removing
    // the pidMap entry. Check for this attempt a rear-guard action.
    //////////////////////////////////////////////////////////////////////

    if (!added) {
      _errorMessage = "pid reuse too rapid for reaping. kill -9'd the new process";
      kill(_pid, 9);
      return false;
    }

    getSERVICE()->addChild();
    getSERVICE()->getMANAGER()->addChild();
    if (debug::child()) DBGPRT << "Child forked: " << _logID << endl;
    close(_fildesSTDERR[1]); _fildesSTDERR[1] = -1;

    _stErrorNetFD = st_netfd_open(_fildesSTDERR[0]);
    if (!_stErrorNetFD) {
      util::messageWithErrno(_errorMessage, "st_netfd_open() failed for STDERR");
      return false;
    }
    return true;
  }

  ////////////////////////////////////////////////////////////
  // Child process - arrange fds and exec
  ////////////////////////////////////////////////////////////

  _shmService->setProcessPID(_id, getpid());	// Make sure this gets set

  // Switch away from root as soon as possible

  int setResults = setuidMaybe(_S->getEXEC());
  if (setResults < 0) {
    perror("Error: sete[ug]id to exec path failed");
    _exit(setResults);
  }

  close(0);
  if (open("/dev/null", O_RDWR) != 0) {
    perror("Error: Could not open /dev/null to STDIN");
    _exit(21);
  }

  if (dup2(0, 1) == -1) {
    perror("Error: Could not dup2(STDIN, STDOUT)");
    _exit(15);
  }

  if (dup2(_fildesSTDERR[1], 2) == -1) {
    perror("Error: Could not dup2(, STDERR)");		// May or may not work...
    _exit(16);
  }
  if (_fildesSTDERR[1] != 2) close(_fildesSTDERR[1]);

  if (dup2(acceptSocket, plutonGlobal::inheritedAcceptFD) == -1) {
    perror("Error: dup2(acceptSocket) failed");
    _exit(17);
  }
  if (acceptSocket != plutonGlobal::inheritedAcceptFD) close(acceptSocket);

  ////////////////////////////////////////////////////////////
  // The accept socket is set non-block by way of using it in
  // state_threads. Undoing that non-blocked for the child.
  ////////////////////////////////////////////////////////////

  if (fcntl(plutonGlobal::inheritedAcceptFD, F_SETFL, 0) == -1) {
    perror("Error: fcntl(acceptSocket) failed");
    _exit(18);
  }

  if (dup2(shmFD, plutonGlobal::inheritedShmServiceFD) == -1) {
    perror("Error: dup2(shmFD) failed");
    _exit(19);
  }
  if (shmFD != plutonGlobal::inheritedShmServiceFD) close(shmFD);


  if (dup2(reportingSocket, plutonGlobal::inheritedReportingFD) == -1) {
    perror("Error: dup2(reportingSocket) failed");
    _exit(20);
  }
  if (reportingSocket != plutonGlobal::inheritedReportingFD) close(reportingSocket);

  //////////////////////////////////////////////////////////////////////
  // This is paranoia as the parent sets close-on-exec, but better
  // safe than sorry... especially since the forked image can easily
  // be that of a daemon that has been running for months.
  //////////////////////////////////////////////////////////////////////

#ifdef __APPLE__
  service::closeAllfdsExcept(this->getSERVICE(), plutonGlobal::inheritedHighestFD);
#endif

  ////////////////////////////////////////////////////////////
  // Apply per-process limits if configured
  ////////////////////////////////////////////////////////////

  if ((_S->getUlimitCPUMilliSeconds() > 0) && (_S->getMaximumRequests() > 0)) {
    int limit = _S->getUlimitCPUMilliSeconds() * _S->getMaximumRequests() / util::MILLISECOND;
    if (limit == 0) limit = 1;
    struct rlimit rlp;
    rlp.rlim_cur = rlp.rlim_max = limit;
    ++rlp.rlim_max;		// Give some grace
    if (setrlimit(RLIMIT_CPU, &rlp) == -1) perror("Warning: setrlimit(RLIMIT_CPU) failed");
  }

  if (_S->getUlimitDATAMemory() > 0) {
    struct rlimit rlp;
    rlp.rlim_cur = rlp.rlim_max = _S->getUlimitDATAMemory() * 1024 * 1024;
    ++rlp.rlim_max;		// Give some grace
    if (setrlimit(RLIMIT_DATA, &rlp) == -1) perror("Warning: setrlimit(RLIMIT_DATA) failed");
  }

  if (_S->getUlimitOpenFiles() > 0) {
    struct rlimit rlp;
    rlp.rlim_cur = rlp.rlim_max = _S->getUlimitOpenFiles();
    ++rlp.rlim_max;		// Give some grace
    if (setrlimit(RLIMIT_NOFILE, &rlp) == -1) perror("Warning: setrlimit(RLIMIT_NOFILE) failed");
  }

  ////////////////////////////////////////////////////////////
  // And finally, exec the program.
  ////////////////////////////////////////////////////////////

  const char* cd = _S->getCD();
  const char* exec = _S->getEXEC();
  if (debug::child()) {
    write(2, "cd " , 3);
    write(2, cd, strlen(cd));
    write(2, "\n", 1);
  }

  if (*cd && (chdir(cd) == -1)) {
    string s;
    util::messageWithErrno(s, "chdir() failed", cd);
    write(2, s.data(), s.length());
    write(2, "\n", 1);
    sleep(1);
    _exit(14);
  }

  if (debug::child()) {
    write(2, "exec " , 5);
    write(2, exec, strlen(exec));
    write(2, "\n", 1);
  }

  ////////////////////////////////////////////////////////////
  // Tokenize the exec string into an argv array. All mallocs, callocs
  // and new are totally lossy as exec() replaces them or _exit()
  // kills them.
  ////////////////////////////////////////////////////////////

  char* buffer = new char[strlen(exec)+1];

  static const int MAXARGS = 100;	// More than plenty - hoy vay

  char **argv = (char**) calloc(MAXARGS+1, sizeof(char*));
  if (!argv) {
    write(2, "calloc failed\n", 14);
    sleep(1);
    _exit(11);
  }

  const char* err = 0;
  int argc = util::lineToArgv(exec, strlen(exec), buffer, argv, MAXARGS, err);
  if (argc < 0) {
    string s = "Parsing error on 'exec' command: ";
    s += exec;
    s += ": ";
    s += err;
    write(2, s.data(), s.length());
    write(2, "\n", 1);
    _exit(12);
  }

  execv(argv[0], argv);
  string s;
  util::messageWithErrno(s, "execv() failed", argv[0]);
  write(2, s.data(), s.length());
  write(2, "\n", 1);
  sleep(1);
  _exit(13);
}
