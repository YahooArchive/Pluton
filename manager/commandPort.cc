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
#include <sstream>

#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <st.h>

#include "debug.h"
#include "logging.h"
#include "util.h"
#include "lineToArgv.h"
#include "commandPort.h"
#include "manager.h"
#include "service.h"
#include "process.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Accept various maintenance and status commands through a
// socket. You can telnet in and issue these commands. Usually the
// command port is bound to loopback, so the access control is simply
// if you can get onto the machine.
//////////////////////////////////////////////////////////////////////

commandPort::commandPort(st_netfd_t newSocket, manager* setM)
    : ioSocket(newSocket), M(setM)
{
}

commandPort::~commandPort()
{
  st_netfd_close(ioSocket);
}


//////////////////////////////////////////////////////////////////////
// Wrappers to start a thread with a commandPort
//////////////////////////////////////////////////////////////////////

static void*
runCommandThread(void* voidD)
{
  commandPort* CP = static_cast<commandPort*> (voidD);

  LOGPRT << "Command Port: Start" << endl;

  CP->run();

  delete CP;

  LOGPRT << "Command Port: Exit" << endl;


  return 0;                     // Destroy this thread
}


//////////////////////////////////////////////////////////////////////
// Take in command port connections and start a handler thread for
// each one.
//////////////////////////////////////////////////////////////////////

void*
commandPort::listen(void *voidManager)
{
  manager* M = static_cast<manager*>(voidManager);

  st_netfd_t stAcceptFD = st_netfd_open_socket(M->getCommandAcceptSocket());

  while (true) {
    st_netfd_t newFD = st_accept(stAcceptFD, 0, 0, 100 * util::MICROSECOND);
    if (!newFD) {
      if (errno == ETIME) continue;
      break;
    }
    if (util::setCloseOnExec(st_netfd_fileno(newFD)) == -1) {
      string em;
      util::messageWithErrno(em, "FD_CLOEXEC failed on accepted socket");
      LOGPRT << "Error: CommandPort: " << em << endl;
      st_netfd_close(newFD);
      continue;
    }
    commandPort* CP = new commandPort(newFD, M);
    if (!st_thread_create(runCommandThread, (void*) CP, 0, M->getStStackSize())) {
      string em;
      util::messageWithErrno(em, "st_thread_create(runCommandThread) failed");
      LOGPRT << "Error: CommandPort: " << em << endl;
      delete CP;
      continue;
    }
  }

  st_netfd_close(stAcceptFD);

  return 0;
}


//////////////////////////////////////////////////////////////////////
// Command Handlers
//////////////////////////////////////////////////////////////////////

static int
helpCmd(manager* M, int argc, char** argv, ostringstream& os);


static int
quitCmd(manager*, int argc, char** argv, ostringstream& os)
{
  return -1;
}

static int
statsCmd(manager* M, int argc, char** argv, ostringstream& os)
{
  string s;
  M->getStatistics(s);
  os << s;

  return 0;
}

static int
serviceCmd(manager*, int argc, char** argv, ostringstream& os)
{
  --argc; ++argv;	// Skip over command
  service::list(os, argv[0]);
  return 0;
}

static int
processCmd(manager*, int argc, char** argv, ostringstream& os)
{
  --argc; ++argv;	// Skip over command
  process::list(os, argv[0]);
  return 0;
}

static int
debugOnCmd(manager*, int argc, char** argv, ostringstream& os)
{
  --argc; ++argv;	// Skip over command
  while (argc > 0) {
    bool res = debug::setFlag(argv[0]);
    if (!res) {
      os << "Unknown debug flag: " << argv[0] << endl;
    }
    else {
      os << "Debug: " << argv[0] << " set" << endl;
    }
    --argc;
    ++argv;
  }

  return 0;
}

static int
debugOffCmd(manager*, int argc, char** argv, ostringstream& os)
{
  --argc; ++argv;	// Skip over command
  while (argc > 0) {
    bool res = debug::clearFlag(argv[0]);
    if (!res) {
      os << "Unknown debug flag: " << argv[0] << endl;
    }
    else {
      os << "Debug: " << argv[0] << " cleared" << endl;
    }
    --argc;
    ++argv;
  }

  return 0;
}

static int
logOnCmd(manager*, int argc, char** argv, ostringstream& os)
{
  --argc; ++argv;	// Skip over command
  while (argc > 0) {
    bool res = logging::setFlag(argv[0]);
    if (!res) {
      os << "Unknown logging flag: " << argv[0] << endl;
    }
    else {
      os << "logging: " << argv[0] << " set" << endl;
    }
    --argc; ++argv;
  }

  return 0;
}

static int
logOffCmd(manager*, int argc, char** argv, ostringstream& os)
{
  --argc; ++argv;	// Skip over command
  while (argc > 0) {
    bool res = logging::clearFlag(argv[0]);
    if (!res) {
      os << "Unknown logging flag: " << argv[0] << endl;
    }
    else {
      os << "logging: " << argv[0] << " cleared" << endl;
    }
    --argc;
    ++argv;
  }

  return 0;
}

static int
test1Cmd(manager*, int argc, char** argv, ostringstream& os)
{
  DBGPRT << "test1 sent to DBGPRT" << endl;
  LOGPRT << "test1 sent to LOGPRT" << endl;

  cout << "test1 sent to cout" << endl;
  cerr << "test1 sent to cerr" << endl;

  write(1, "test1 sent to fd1\n", 19);
  write(2, "test1 sent to fd2\n", 19);

  os << "test1: Sent to DBG, LOG, cout, cerr, f1 and f2" << endl;

  if (cout.bad()) os << "test1: cout has .bad" << endl;
  if (clog.bad()) os << "test1: clog has .bad" << endl;
  if (cerr.bad()) os << "test1: cerr has .bad" << endl;

  return 0;
}

static int
test2Cmd(manager*, int argc, char** argv, ostringstream& os)
{
  os << "test2: Resetting if .bad" << endl;

  if (cout.bad())  {
    cout.clear();
    os << "test2: cout cleared" << endl;
  }

  if (clog.bad())  {
    clog.clear();
    os << "test2: clog cleared" << endl;
  }

  if (cerr.bad())  {
    cerr.clear();
    os << "test2: cerr cleared" << endl;
  }

  if (cout.bad()) os << "test2: Uh oh, cout still has .bad" << endl;
  if (clog.bad()) os << "test2: Uh oh, clog still has .bad" << endl;
  if (cerr.bad()) os << "test2: Uh oh, cerr still has .bad" << endl;

  return 0;
}

//////////////////////////////////////////////////////////////////////

struct commandListStruct {
  const char*	command;
  const char* syntax;
  int	minArgs;
  int	maxArgs;
  int	(*handler)(manager*, int argc, char**argv, ostringstream& os);
};


static commandListStruct commandList[] = {
  { "help",	0,					-1, -1, helpCmd },
  { "?", 	0,					-1, -1, helpCmd },
  { "q", 	0,					-1, -1, quitCmd },
  { "quit", 	0,					-1, -1, quitCmd },
  { "exit", 	0,					-1, -1, quitCmd },

  // Ambiguous short commands must preceed longer commands

  { "s",	"Show services",			-1, -1, serviceCmd},

  { "p",	"Show processes for all service",	-1, -1, processCmd},

  { "stats", 	"Daemon-wide statistics", 		-1, -1, statsCmd },

  { "debugon",	"Turn on a debug flag",			1, -1, debugOnCmd },
  { "debugoff",	"Turn off a debug flag",		1, -1, debugOffCmd },
  { "logon",	"Turn on a logging flag",		1, -1, logOnCmd },
  { "logoff",	"Turn off a logging flag",		1, -1, logOffCmd },

  { "test1",	"Do some internal tests",		-1, -1, test1Cmd },
  { "test2",	"Do some more internal tests",		-1, -1, test2Cmd },

  { 0 }
};


static int
helpCmd(manager* M, int argc, char** argv, ostringstream& os)
{
  os << "Command List" << endl;
  for (commandListStruct* clp = commandList; clp->command; ++clp) {
    os << clp->command;
    if (clp->syntax) os << "\t" << clp->syntax;
    os << endl;
  }

  os << endl;
  return 0;
}


//////////////////////////////////////////////////////////////////////
// Parse the command line and dispatch to the appropriate handler.
//////////////////////////////////////////////////////////////////////

static int
dispatch(manager* M, const char* cp, ostringstream& os)
{
  char*	argv[10];
  int cpLen = strlen(cp);
  char* buffer = new char[cpLen+1];
  const char* err = 0;

  int argc = util::lineToArgv(cp, cpLen, buffer, argv, 10, err);
  if (argc < 0) {
    os << "Error: Parse of command line failed: " << err << endl;
    delete [] buffer;
    return 0;
  }

  if (argc == 0) {	// Nothing to do
    delete [] buffer;
    return 0;
  }
  unsigned int argv0Length = strlen(argv[0]);

  commandListStruct* foundP = 0;
  for (commandListStruct* searchP = commandList; searchP->command; ++searchP) {
    if (strncasecmp(argv[0], searchP->command, argv0Length)) continue;

    if (foundP) {		// Ambiguous
      os << "Error: " << argv[0] << " is ambiguous. Did you mean "
	 << searchP->command << " or " << foundP->command << endl; 
      delete [] buffer;
      return 0;
    }
    foundP = searchP;
    if (argv0Length == strlen(foundP->command)) break;	// No need to disambiguate
  }

  if (!foundP) {
    os << "Error: Unrecognized command: '" << argv[0] << "' - try the help command" << endl;
    delete [] buffer;
    return 0;
  }

  // Found. Check that the argument count is in range

  if ((foundP->minArgs != -1) && (argc-1) < foundP->minArgs) {
    os << "Error:  " << argv[0] << " requires at least " << foundP->minArgs << " arguments"
       << endl;
    delete [] buffer;
    return 0;

  }
  if ((foundP->maxArgs != -1) && (argc-1) > foundP->maxArgs) {
    os << "Error:  " << argv[0] << " requires at most " << foundP->maxArgs << " arguments"
       << endl;
    delete [] buffer;
    return 0;
  }


  int res = (foundP->handler)(M, argc, argv, os);

  delete [] buffer;

  return res;
}


//////////////////////////////////////////////////////////////////////
// Read in a line upto the \n then dispatch to a handler. One
// character per read is fine for a command port where people type in
// commands. As an aside, if the connection is via telnet, it's
// typically in linemode so this routine does not get to manage edit
// characters, such as backspace.
//
// An empty command line re-does the last command.
//////////////////////////////////////////////////////////////////////

void
commandPort::run()
{
  char* banner = "plutonManager command line connection. Try the 'help' command\n\n$ ";

  st_write(ioSocket, banner, strlen(banner), 0);
  static const int maxBufferSize = 200;
  char buf[maxBufferSize+1];
  char lastCommand[maxBufferSize+1] = "";
  bool quitFlag = false;

  while (!quitFlag) {
    int ix = 0;
    while (ix < maxBufferSize) {
      int res = st_read(ioSocket, buf+ix, 1, 5 * util::MICROSECOND);
      if (res == 0) return;
      if (res == -1) {
	if ((errno == EINTR) || (errno == ETIME)) continue;
	return;
      }

      if (ix == 0) {
	if (buf[0] == '\004') return;	// ^D means EOF
	if (buf[0] == '\n') break;
	if (isspace(buf[0])) continue;	// Ignore leading whitespace
      }
      if (buf[ix] == '\n') break;
      if (isprint(buf[ix])) ++ix;	// Ignore allbut printable characters
    }

    buf[ix] = '\0';

    ostringstream os;		// All output is assembled here

    // If the current command is just whitespace, use the previous
    // command.

    if (ix == 0) {
      strcpy(buf, lastCommand);
      os << "redo: " << buf;
    }
    else {
      strcpy(lastCommand, buf);
    }
    if (dispatch(M, buf, os) == -1) {
      quitFlag = true;
    }

    string s = os.str();  	// Send command output back to user
    st_write(ioSocket, s.data(), s.length(), 0);

    st_write(ioSocket, "$ ", 2, 0);		// Prompt for command
  }
}
