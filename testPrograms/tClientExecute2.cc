#include <iostream>

#include <sys/time.h>
#include <assert.h>
#include <stdlib.h>

#include "util.h"
#include <pluton/client.h>

using namespace std;

// Exercise the executeAndWait() interfaces.

static void
failed(const char* err, int val=0)
{
  cout << "Failed: tClientExecute2 " << err << " val=" << val << endl;

  exit(1);
}

const char* SK = "system.echo.0.raw";

int
main(int argc, char** argv)
{
  assert(argc == 2);

  const char* goodPath = argv[1];

  int res;
  pluton::client C;

  if (!C.initialize(goodPath)) {
    cout << C.getFault().getMessage("C.initialize()") << endl;
    failed("bad return from good path");
  }

  C.setDebug(true);

  // This test assumes that the config has been set up to have two
  // prestarted instances of the echo service. The idea here is to
  // queue three requests knowning that the third will have to wait
  // for one of the earlier ones to complete.

  pluton::clientRequest R1;
  pluton::clientRequest R2;
  pluton::clientRequest R3;

  R1.setClientHandle((void*) "R1");
  R2.setClientHandle((void*) "R2");
  R3.setClientHandle((void*) "R3");
  R1.setContext("echo.sleepMS", "1000");
  R2.setContext("echo.sleepMS", "1000");
  R3.setContext("echo.sleepMS", "1000");

  struct timeval start;
  gettimeofday(&start, 0);

  cerr << "Test1" << endl;
  if (!C.addRequest(SK, R1)) failed("bad return from addRequest(R1)");
  if (!C.addRequest(SK, R2)) failed("bad return from addRequest(R2)");
  if (!C.addRequest(SK, R3)) failed("bad return from addRequest(R3)");

  // Get one request back

  pluton::clientRequest* rFirst = C.executeAndWaitAny();
  if (!rFirst) failed("Nothing came back from executeAndWaitAny()");
  cout << "WaitAny returned " << (char*) rFirst->getClientHandle() << endl;
  if (rFirst->hasFault()) {
    cout << "Wait Any Fault text: " << rFirst->getFaultText() << endl;
    failed("Unexpected fault", rFirst->getFaultCode());
  }
  struct timeval middle;
  gettimeofday(&middle, 0);
  long diff = util::timevalDiffMS(middle, start);
  cout << "MS Diff for waitAny=" << diff << endl;
  if (diff < 800) failed("WaitAny() took less than 1.0 seconds - expected 1.0", diff);
  if (diff > 1500) failed("WaitAny() took longer than 1.5 seconds - expected 1.0", diff);

  // and get the other two

  cerr << "Test2" << endl;
  res = C.executeAndWaitAll();
  if (res <= 0) failed("Expected +ve back from executeAndWaitAll", res);

  if (R1.hasFault()) failed("T2 R1 has fault", R1.getFaultCode());
  if (R2.hasFault()) failed("T2 R2 has fault", R2.getFaultCode());
  if (R3.hasFault()) failed("T2 R3 has fault", R3.getFaultCode());

  struct timeval end;
  gettimeofday(&end, 0);
  diff = util::timevalDiffMS(end, start);
  cout << "MS Diff for waitAll=" << diff << endl;
  if (diff < 1900) failed("WaitAny() took less than 1.9 seconds - expected 2.0", diff);
  if (diff > 2500) failed("WaitAny() took longer than 2.5 seconds - expected 2.0", diff);

  // Do all three at once to ensure one request queues

  cerr << "Test3" << endl;
  if (!C.addRequest(SK, R1)) {
    cout << C.getFault().getMessage("add R1") << endl;
    failed("bad return from addRequest(R1)");
  }
  if (!C.addRequest(SK, R2)) {
    cout << C.getFault().getMessage("add R3") << endl;
    failed("bad return from addRequest(R2)");
  }
  if (!C.addRequest(SK, R3)) {
    cout << C.getFault().getMessage("add R3") << endl;
    failed("bad return from addRequest(R3)");
  }

  gettimeofday(&start, 0);
  res = C.executeAndWaitAll();
  if (res <= 0) failed("Expected +ve back from executeAndWaitAll", res);

  if (R1.hasFault()) failed("T3 R1 has fault", R1.getFaultCode());
  if (R2.hasFault()) failed("T3 R2 has fault", R2.getFaultCode());
  if (R3.hasFault()) failed("T3 R3 has fault", R3.getFaultCode());

  gettimeofday(&end, 0);
  diff = util::timevalDiffMS(end, start);
  cout << "MS Diff for waitAll2=" << diff << endl;
  if (diff < 2000) failed("WaitAny() took less than 2.0 seconds - expected 2.0", diff);
  if (diff > 2500) failed("WaitAny() took longer than 2.5 seconds - expected 2.0", diff);

  return(0);
}
