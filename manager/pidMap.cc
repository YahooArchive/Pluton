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

#include "config.h"

#include "hash_mapWrapper.h"
#include <iostream>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include "debug.h"
#include "process.h"
#include "pidMap.h"

using namespace std;


static P_STLMAP<pid_t, process*>			_map;
typedef P_STLMAP<pid_t, process*>::iterator		mapIter;
typedef P_STLMAP<pid_t, process*>::const_iterator	mapConstIter;

//////////////////////////////////////////////////////////////////////
// Maintain a mapping between process id and process* so that child
// exits can be associated with the process thread managing the child.
//////////////////////////////////////////////////////////////////////

bool
pidMap::add(pid_t pid, process* procP)
{
  mapIter psi = _map.find(pid);
  if (psi != _map.end()) return false;

  _map[pid] = procP;

  return true;
}

void
pidMap::remove(pid_t pid)
{
  _map.erase(pid);
}


//////////////////////////////////////////////////////////////////////

process*
pidMap::find(pid_t pid)
{
    mapIter psi = _map.find(pid);
    if (psi == _map.end()) return 0;

    return psi->second;
}


//////////////////////////////////////////////////////////////////////
// Reap in any children that have exited and notify the daemon.
//////////////////////////////////////////////////////////////////////

void
pidMap::reapChildren(const char* who)
{
  if (debug::child()) DBGPRT << who << ":Checking children" << endl;
  int status;
  struct rusage ru;

  while (true) {
    pid_t pid = wait4(-1, &status, WNOHANG, &ru);
    if (pid <= 0) break;
    if (debug::child()) DBGPRT << "Wait4 pid=" << pid << " st=" << status << endl;
    process* procP = find(pid);
    if (!procP) {
      DBGPRT << "Warning: Child not associated with process: pid=" << pid << endl;
      continue;
    }
  
    procP->notifyChildExit(status, ru);
  }

  if (debug::child()) DBGPRT << who << ":Reap complete" << endl;
}
