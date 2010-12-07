#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include "shmLookupPrivate.h"

using namespace std;

int
main(int argc, char** argv)
{
  int fd = open("lookup.map", O_RDWR, 0644);
  if (fd == -1) {
    perror("open");
    exit(1);
  }

  relativeHashMap rhm;
  int bytesRead = read(fd, (char*) &rhm, sizeof(rhm));
  if (bytesRead != sizeof(rhm)) {
    perror("read:truncated");
    exit(1);
  }

  rhm._remapFlag = 'Y';
  if (lseek(fd, 0, 0) != 0) {
    perror("lseek");
    exit(1);
  }

  int bytesWritten = write(fd, (char*) &rhm, sizeof(rhm));
  if (bytesWritten != sizeof(rhm)) {
    perror("write:truncated");
    exit(1);
  }

  close(fd);

  exit(0);
}
