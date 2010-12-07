#include <stdlib.h>

#include <iostream>

using namespace std;

#include "shmService.h"

void
checkSizeOf(const char* name, int sz)
{
  if ((sz % 8) != 0) {
    std::cerr << "sizeof " << name << " is not 64bit aligned at " << sz << std::endl;
    exit(1);
  }
}

void
checkOffsetOf(const char* name, int sz)
{
  if ((sz % 8) != 0) {
    std::cerr << "offsetof " << name << " is not 64bit aligned at " << sz << std::endl;
    exit(1);
  }
}

int
main(int argc, char** argv)
{
  checkSizeOf("shmConfig", sizeof(shmConfig));
  checkSizeOf("shmTimeVal", sizeof(shmTimeVal));
  checkSizeOf("shmThread", sizeof(shmThread));
  checkSizeOf("shmProcess", sizeof(shmProcess));
  checkSizeOf("shmService", sizeof(shmService));

  checkOffsetOf("shmConfig,_maximumRequests", offsetof(shmConfig,_maximumRequests));
  checkOffsetOf("shmConfig,_recorderPrefix", offsetof(shmConfig,_recorderPrefix));

  checkOffsetOf("shmThread,_firstActive", offsetof(shmThread,_firstActive));
  checkOffsetOf("shmThread,_lastActive", offsetof(shmThread,_lastActive));
  checkOffsetOf("shmThread,_clientRequestStartTime", offsetof(shmThread,_clientRequestStartTime));
  checkOffsetOf("shmThread,_clientRequestName", offsetof(shmThread,_clientRequestName));

  checkOffsetOf("shmProcess,_faultCount", offsetof(shmProcess,_faultCount));
  checkOffsetOf("shmProcess,_firstActive", offsetof(shmProcess,_firstActive));
  checkOffsetOf("shmProcess,_thread", offsetof(shmProcess,_thread));

  checkOffsetOf("shmService,_managerHeartbeat", offsetof(shmService,_managerHeartbeat));
  checkOffsetOf("shmService,_config", offsetof(shmService,_config));
  checkOffsetOf("shmService,_process", offsetof(shmService,_process));

  exit(0);
}

