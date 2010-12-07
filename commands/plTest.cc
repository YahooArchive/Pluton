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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "netString.h"
#include "decodePacket.h"
#include "clientRequestImpl.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Run a single service executable with one request.
//////////////////////////////////////////////////////////////////////

static const char* usage =
"Usage: plTest [-hen]  [-C name=context .. ] [-K ServiceKey]\n"
"              [-i filein] [-o fileout]\n"
"              -- [serviceExecutionPath [Arguments]]\n"
"\n"
"Generate a request and run the service using STDIN as the source of\n"
"request data. Response Data is written to STDOUT.\n"
"\n"
"Where:\n"
" -C   Set the context for this request - multiple occurences ok\n"
" -h   Print this usage message on STDOUT and exit(0)\n"
" -e   Print Child Exit details\n"
" -i   Use this file as the request data instead of STDIN\n"
"       Useful when running the service under gdb\n"
" -K   Define the serviceKey passed to the service in the\n"
"       request (default: plTest.service.0.raw\n"
" -n   Append a Newline to the output\n"
" -o   Use this file to write the response data instead of STDOUT\n"
"       Useful when running the service under gdb\n"
"\n"
" ServiceKey: Place in request (default: plTest.service.0.raw)\n"
"\n"
"ServiceExecutionpath: Location of the service executable. If not\n"
"supplied, the request packet is written to STDOUT and -o is ignored.\n"
"\n"
"Arguments: passed to the service executable via execvp()\n"
"\n"
"See also: http://localhost/docs/pluton/\n"
"\n";


static void startChild(int childIn, int childOut, int argc, char** argv);
static void writeRequest(istream&, int fd, pluton::clientRequestImpl&);
static void readResponse(int fd, ostream&, pluton::clientRequestImpl&, bool appendNewLine);

static bool	sigChildKillsParent = true;
static bool	childExitDetails = false;

void
catchSIGCHILD(int sig)
{
  int status;
  wait(&status);

  if (childExitDetails) cerr << "Child exit status = " << WEXITSTATUS(status) << endl;

  if (WIFEXITED(status) && (WEXITSTATUS(status) != 0)) {
    cerr << "Warning: plTest Service exit(" << WEXITSTATUS(status) << ")" << endl;
  }

  if (WIFSIGNALED(status)) {
    cerr << "Warning: plTest Service killed(" << WTERMSIG(status) << ")" << endl;
  }

  if (sigChildKillsParent) {
    cerr << "Child has exited prematurely" << endl;
    exit(1);
  }
}

//////////////////////////////////////////////////////////////////////
// Split out the key=value tokens from the context string and and them
// into the request.
//////////////////////////////////////////////////////////////////////

static bool
addContext(pluton::clientRequestImpl& R, const char* context)
{
  char* dupe = strdup(context);
  char** dupeP = &dupe;
  const char* key = strsep(dupeP, "=");
  if (key == dupe) {
    cerr << "Usage Error: Context string needs an '=' character" << endl;
    return false;
  }

  if (!R.setContext(key, string(*dupeP))) {
    cerr << "Error: plSend Fault: " << R.getFaultCode() << ":" << R.getFaultText() << endl;
    return false;
  }

  return true;
}


//////////////////////////////////////////////////////////////////////

int
main(int argc, char **argv)
{
  const char* serviceKey = "plTest.service.0.raw";
  bool appendNewline = false;
  bool useNewSTDIN = false;
  bool useNewSTDOUT = false;
  ifstream newSTDIN;
  ofstream newSTDOUT;

  pluton::clientRequestImpl R;

  char	optionChar;
  while ((optionChar = getopt(argc, argv, "C:ehi:K:no:")) != EOF) {
    switch (optionChar) {
    case 'C':
      if (!addContext(R, optarg)) {
	cerr << endl << usage;
	exit(1);
      }
      break;

    case 'h':
      cout << endl << usage;
      exit(0);

    case 'e':
      childExitDetails = true;
      break;

    case 'i':
      newSTDIN.open(optarg, ifstream::in);
      if (!newSTDIN.good()) {
	cerr << "Error: Could not open '" << optarg << "' for input" << endl;
	exit(1);
      }
      useNewSTDIN = true;
      break;

    case 'K':
      serviceKey = optarg;
      break;

    case 'n':
      appendNewline = true;
      break;

    case 'o':
      newSTDOUT.open(optarg, ifstream::out);
      if (!newSTDOUT.good()) {
	cerr << "Error: Could not open '" << optarg << "' for output" << endl;
	exit(1);
      }
      useNewSTDOUT = true;
      break;

    default:
      cerr << endl << usage;
      exit(1);
    }
  }

  argc -= optind;
  argv += optind;

  // No more args means that we simply copy the request data from in to out
  // if (argc == 0) cout ???

  const char* err = R.setServiceKey(serviceKey);
  if (err) {
    cerr << "plTest ServiceKey Error: " << err << endl;
    exit(1);
  }

  // Need some pipes to exchange the request

  int meToThem[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, meToThem) == -1) {
    perror("plTest: socketpair() meToThem failed");
    exit(1);
  }

  int themToMe[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, themToMe) == -1) {
    perror("plTest: socketpair() themToMe failed");
    exit(1);
  }

  signal(SIGCHLD, catchSIGCHILD);
  pid_t pid = fork();
  if (pid == -1) {
    perror("plTest: fork() failed");
    exit(1);
  }

  if (pid == 0) {
    close(meToThem[1]);
    close(themToMe[0]);
    startChild(meToThem[0], themToMe[1], argc, argv);
    _exit(10);
  }

  close(meToThem[0]);
  close(themToMe[1]);

  if (useNewSTDIN) {
    writeRequest(newSTDIN, meToThem[1], R);
  }
  else {
    writeRequest(cin, meToThem[1], R);
  }

  sigChildKillsParent = false;
  close(meToThem[1]);		// Do this early so child exits

  if (useNewSTDOUT) {
    readResponse(themToMe[0], newSTDOUT, R, appendNewline);
  }
  else {
    readResponse(themToMe[0], cout, R, appendNewline);
  }


  close(themToMe[0]);

  // Wait for the child to exit

  signal(SIGCHLD, SIG_DFL);
  catchSIGCHILD(0);

  return(0);
}


//////////////////////////////////////////////////////////////////////

void
startChild(int childIn, int childOut, int argc, char** argv)
{
  static const int        FAUX_STDIN = 3;
  static const int        FAUX_STDOUT = 4;

  if (childIn != FAUX_STDIN) {
    if (dup2(childIn, FAUX_STDIN) == -1) {
      perror("plTest: dup2(FAUX_STDIN) failed");
      _exit(1);
    }
    close(childIn);
  }


  if (childOut != FAUX_STDOUT) {
    if (dup2(childOut, FAUX_STDOUT) == -1) {
      perror("plTest: dup2(FAUX_STDOUT) failed");
      _exit(1);
    }
    close(childOut);
  }

  // If they gave a command, run it

  if (argc > 0) {
    execvp(argv[0], argv);
    perror("plTest: execvp() failed");
    _exit(1);
  }

  // Otherwise, just copy in to out

  char buffer[2000];
  int bytes;
  while ((bytes = read(FAUX_STDIN, buffer, sizeof(buffer))) > 0) {
    write(1, buffer, bytes);
  }
  close(FAUX_STDIN);
  close(FAUX_STDOUT);
  exit(0);
}


//////////////////////////////////////////////////////////////////////

void
writeRequest(istream& fin, int fd, pluton::clientRequestImpl& R)
{
  stringstream os;
  os << fin.rdbuf();			// Slurp in all of STDIN
  string requestStr = os.str();
  R.setRequestData(requestStr.data(), requestStr.length());

  netStringGenerate pre;
  netStringGenerate post;

  R.assembleRequestPacket(pre, post, true);

  if (write(fd, pre.data(), pre.length()) != (signed) pre.length()) {
    perror("plTest: write() pre error");
    exit(1);
  }

  const char* p;
  int l;
  R.getRequestData(p, l);
  if (write(fd, p, l) != l) {
    perror("plTest: write() data error");
    exit(1);
  }

  if (write(fd, post.data(), post.length()) != (signed) post.length()) {
    perror("plTest: write() post error");
    exit(1);
  }
}


//////////////////////////////////////////////////////////////////////

void
readResponse(int fd, ostream& fout, pluton::clientRequestImpl& R, bool appendNewline)
{
  R._socket = fd;
  R.prepare(false);
  R.setByPassIDCheck(true);		// As the request has no owner
  R.setFault(pluton::noFault);
  bool firstRead = true;

  while (!R._decoder.haveCompleteRequest()) {
    int res = R.issueRead(0);
    if (res == 0) {
      if (!firstRead) {			// Part way thru a response is bad
	perror("plTest: End of File on response socket");
	exit(1);
      }
      exit(0);				// But no response is ok
    }

    if (res < 0) {
      perror("plTest: read() error on response socket");
      exit(1);
    }

    firstRead = false;
    const char* parseError = 0;
    while (R._packetIn.haveNetString(parseError)) {
      string em;
      int res = R.decodeResponse(em);
      if (res == 1) break;

      if (res < 0) {
	cerr << "plTest: Packet decode failed: " << em << endl;
	exit(1);
      }
    }
    if (parseError) {
      cerr << "plTest: Packet parse error: " << parseError << endl;
      exit(1);
    }
  }

  if (R.hasFault()) {
    cerr << "plTest: Fault " << R.getFaultCode() << ":" << R.getFaultText() << endl;
    exit(1);
  }

  // All good. Write the response data to the ostream

  const char* p;
  int l;
  R.getResponseData(p, l);
  fout.write(p, l);

  if (appendNewline) fout.write("\n", 1);
}
