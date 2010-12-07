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


static hash_map<pid_t, process*>			_map;
typedef hash_map<pid_t, process*>::iterator		mapIter;
typedef hash_map<pid_t, process*>::const_iterator	mapConstIter;

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
