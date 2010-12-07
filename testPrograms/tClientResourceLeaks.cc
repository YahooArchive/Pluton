#include <iostream>

#include <sys/time.h>

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"
#include <pluton/client.h>

#define	REPEATCOUNT	500

using namespace std;

// Exercise the executeAndWait() interfaces to test for lost
// resources, namely file descriptors and memory.

static void
failed(const char* err, int val1=0, int val2=0)
{
  cout << "Failed: " << err << " val1=" << val1 << " val2=" << val2 << endl;

  exit(1);
}

const char* SK = "system.echo.0.raw";

int
main(int argc, char** argv)
{
  assert(argc >= 2);

  const char* goodPath = argv[1];

  pluton::client C;

  char* sbrkStart = (char*) sbrk(0);
  char* sbrkMiddle = sbrkStart;

  if (!C.initialize(goodPath)) {
    cout << C.getFault().getMessage("C.initialize()") << endl;
    failed("bad return from good path");
  }

  if (argc > 2) C.setDebug(true);

  // Run a lot of requests to test for resource leaks

  static const int RQS = 9;

  pluton::clientRequest R[RQS];

  char buffer[10000];

  for (int ix=0; ix < RQS; ix+=3) {
    R[ix].setContext("echo.sleepMS", "10");
  }

  for (int rc=0; rc < REPEATCOUNT; ++rc) {

    if (rc == REPEATCOUNT/2) sbrkMiddle = (char*) sbrk(0);

    int requestSize = sizeof(buffer) / ((rc & 0xff) + 1);
    bool haveAffinity = false;
    for (int ix=0; ix < RQS; ++ix) {
      R[ix].setRequestData(buffer, requestSize);
      if (!haveAffinity && (ix & (rc&0xff) & 0x4)) {
	R[ix].setAttribute(pluton::keepAffinityAttr|pluton::noRetryAttr);
	haveAffinity = true;	// Only one per iteration
      }
      if (!C.addRequest(SK, R[ix])) {
	cout << C.getFault().getMessage("addRequest") << endl;
	failed("addRequest", ix);
      }
    }

    {
      pluton::clientRequest bye1;
      bye1.setRequestData(buffer, requestSize);
      bye1.setContext("echo.sleepMS", "50");
      bye1.setContext("echo.log", "yes");
      if (!C.addRequest(SK, bye1)) {
	cout << C.getFault().getMessage("addRequest") << endl;
	failed("addRequest bye1");
      }

      // Get two request back before lettings bye1 get destroyed

      pluton::clientRequest* rFirst = C.executeAndWaitAny();
      if (!rFirst) failed("Nothing came back from first executeAndWaitAny()");

      rFirst = C.executeAndWaitAny();
      if (!rFirst) failed("Nothing came back from second executeAndWaitAny()");
      // Destroys bye1!
    }

    int res = C.executeAndWaitAll();
    if (res <= 0) failed("Expected +ve back from executeAndWaitAll", rc, res);

    // Check for faults

    const char* p;
    int l;
    for (int ix=0; ix < RQS; ++ix) {
      if (R[ix].hasFault()) failed("Request has fault", rc, R[ix].getFaultCode());
      R[ix].getResponseData(p, l);
      if (l != requestSize) failed("Response size differs", l, requestSize);
    }
  }

  char* sbrkEnd = (char*) sbrk(0);

  cout << "Sbreak growth=" << sbrkEnd - sbrkMiddle << " and " << sbrkEnd - sbrkStart
       << " sme=" << (void*) sbrkStart << "," << (void*) sbrkMiddle << "," << (void*) sbrkEnd
       << endl;

  if (sbrkMiddle != sbrkEnd) {
    failed("Growth beyond mid-point", (sbrkEnd - sbrkMiddle), (sbrkEnd - sbrkStart));
  }

  return(0);
}
