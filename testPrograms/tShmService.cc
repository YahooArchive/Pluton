#include <iostream>

#include <sys/types.h>

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#include "shmService.h"

using namespace std;

int
main(int argc, char** argv)
{
  pluton::shmServiceHandler	shmManager;
  pluton::shmServiceHandler	shmService;

  int fd = open("tShmService.mmap", O_RDWR | O_CREAT | O_TRUNC, 0600);
  if (fd == -1) {
    perror("open");
    exit(1);
  }

  pluton::faultCode fc = shmManager.mapManager(fd, 3, 5);
  if (fc != pluton::noFault) {
    cout << "mapManager() failed: " << fc << endl;
    exit(1);
  }

  fc = shmService.mapService(fd, getpid(), 0);
  if (fc != pluton::noFault) {
    cout << "mapService failed: " << fc << endl;
    exit(1);
  }

  unlink("tShmService.mmap");

  return 0;
}
