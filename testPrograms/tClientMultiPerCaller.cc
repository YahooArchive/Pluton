#include <iostream>

#include <sys/time.h>

#include <assert.h>
#include <stdlib.h>

#include "util.h"
#include <pluton/client.h>

using namespace std;

//////////////////////////////////////////////////////////////////////
// Test out multiple perCaller instances.
//
// Nest perCallers that execute on behalf of the parent so that the
// delay appears to be zero.
//////////////////////////////////////////////////////////////////////

static void
failed(const char* f, const char* m)
{
  cout << "Failed: " << f << " " << m << endl;
  exit(1);
}


const char* SK = "system.echo.0.raw";

static void
recurse(int depth)
{
  pluton::client C;

  if (!C.initialize("")) failed(C.getFault().getMessage().c_str(), "initialize failed");

  pluton::clientRequest R;
  R.setContext("echo.sleepMS", "1000");
  C.addRequest(SK, R);

  --depth;
  if (depth > 0) recurse(depth);

  C.executeAndWaitAll();
  if (C.hasFault()) failed(C.getFault().getMessage().c_str(), "Execute 1");
}

int
main(int argc, char** argv)
{
  assert(argc >= 2);

  const char* goodPath = argv[1];

  pluton::client C;

  if (!C.initialize(goodPath)) failed(C.getFault().getMessage().c_str(), "initialize failed");

  if (argc > 2) C.setDebug(true);

  pluton::clientRequest R;
  R.setContext("echo.sleepMS", "1000");
  C.addRequest(SK, R);

  struct timeval startTime;
  gettimeofday(&startTime, 0);

  recurse(3);

  struct timeval middleTime;
  gettimeofday(&middleTime, 0);
  C.executeAndWaitAll();
  if (C.hasFault()) failed(C.getFault().getMessage().c_str(), "Execute parent");

  //////////////////////////////////////////////////////////////////////
  // Given the recursed routines, the parent request should take
  // virtually no time as it should have been processed by a recursed
  // executeAndWait* call.
  //////////////////////////////////////////////////////////////////////

  struct timeval endTime;
  gettimeofday(&endTime, 0);

  long diff = util::timevalDiffMS(endTime, middleTime);
  clog << "Middle to endMS=" << diff << endl;

  if (diff > 500) {
    clog << "Parent request was not parallelized: parent delayMS=" << diff << endl;
    exit(1);
  }

  diff = util::timevalDiffMS(endTime, startTime);
  clog << "Start to endMS=" << diff << endl;
  if (diff < 1000) {
    clog << "Parent request did not delay. Delayms=" << diff << endl;
    exit(1);
  }

  return(0);
}
