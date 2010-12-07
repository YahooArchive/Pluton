//////////////////////////////////////////////////////////////////////
// This program tries to simulate the cost of a service. The context
// contains the cpu and latency to burn.
//////////////////////////////////////////////////////////////////////


#include <string>
#include <iostream>
#include <sstream>

using namespace std;

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifndef RUSAGE_SELF
#define   RUSAGE_SELF     0
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"
#include "pluton/service.h"

extern char* optarg;
extern int optreset;
extern int optind;

void
burnAthousand()
{
  int r = rand() % 1000;
  for (int ix=0; ix < 10000; ++ix) {
    if ((r+ix) < 0) break;	// Hopeully throw the compiler out of optimization
  }
}

//////////////////////////////////////////////////////////////////////
// Work out how many times we have to spin to burn a millisec of CPU
//////////////////////////////////////////////////////////////////////
int
calibrateCPU()
{
  struct rusage ruStart;
  int res = getrusage(RUSAGE_SELF, &ruStart);
  if (res == -1) {
    perror("gerusage 1");
    exit(1);
  }

  int ix;
  for (ix=0; ix < 1000000; ++ix) {
    burnAthousand();
    struct rusage ruEnd;
    int res = getrusage(RUSAGE_SELF, &ruEnd);
    if (res == -1) {
      perror("getrusage 2");
      exit(2);
    }
    long ms = util::timevalDiffMS(ruEnd.ru_utime, ruStart.ru_utime);
    if (ms > 100) return ix / 100;
  }

  clog << "Could not calibrate CPU" << endl;
  exit(3);
}


static int duplicateCount = 1;

int
main(int argc, char** argv)
{
  char optionChar;
  while ((optionChar = getopt(argc, argv, "m:")) != EOF) {
    switch (optionChar) {
    case 'm': duplicateCount = atoi(optarg); break;
    default:
      clog << "Invalid option sent" << endl;
      exit(4);
    }
  }

  int burnCount = calibrateCPU();		// Work out how to burn a ms of CPU
  if (burnCount <= 0) {
    clog << "Burn Rate of zero is impossible to use" << endl;
    exit(5);
  }

  clog << "1MS burn takes " << burnCount << " cycles" << endl;

  pluton::service S("loadSimulator");
  if (!S.initialize()) {
    clog << S.getFault().getMessage("loadSimulator") << endl;
    exit(1);
  }

  while (S.getRequest()) {
    string sr;
    S.getRequestData(sr);

    int cpuMS=0;
    int elapseMS=0;
    string sc;
    if (S.getContext("cpu", sc)) {
      istringstream is(sc);
      is >> cpuMS;
    }

    if (S.getContext("latency", sc)) {
      istringstream is(sc);
      is >> elapseMS;
    }

    int bc = burnCount * cpuMS;
    while (bc-- > 0) burnAthousand();
    if ((elapseMS-cpuMS) > 0) usleep((elapseMS-cpuMS) * 1000);

    string r;
    int dc = duplicateCount;
    while (dc-- > 0) r.append(sr);
    if (!S.sendResponse(r)) break;
  }

  if (S.hasFault()) {
    clog << "Error: " << S.getFault().getMessage() << endl;
    exit(1);
  }

  S.terminate();

  exit(0);
}
