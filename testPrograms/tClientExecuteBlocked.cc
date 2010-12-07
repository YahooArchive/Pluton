#include <iostream>

#include <sys/time.h>
#include <assert.h>
#include <stdlib.h>

#include "util.h"
#include <pluton/client.h>

using namespace std;

// Exercise the executeAndWaitBlocked() interface.

static void
failed(const char* err, int val=0)
{
  cout << "Failed: tClientExecuteBlocked " << err << " val=" << val << endl;
}

const char* SK = "system.echo.0.raw";

int
main(int argc, char** argv)
{
  const char* goodPath = 0;
  if (argc > 1) goodPath = argv[1];

  int res;
  pluton::client C;
  pluton::clientRequest R1;
  pluton::clientRequest R2;

  if (!C.initialize(goodPath)) {
    failed("bad return from good path");
    cout << C.getFault().getMessage("C.initialize()") << endl;
    exit(1);
  }

  R2.setContext("echo.sleepMS", "1000");
  if (!C.addRequest(SK, R2)) {
    failed("bad return from addRequest 2");
    cout << C.getFault().getMessage("C.addRequest() 1") << endl;
    exit(2);
  }

  if (!C.addRequest(SK, R1)) {
    failed("bad return from addRequest 1");
    cout << C.getFault().getMessage("C.addRequest() 1") << endl;
    exit(3);
  }

  // R1 should come back pretty immediately so Blocked should return 1
  // This relies on the fact that the library pushes new requests and
  // pops them when processing, so it's a LIFO queue. If that changes,
  // this test will likely fail.

  res = C.executeAndWaitBlocked(1);
  if (res != 1) {
    failed("bad return from first executeAndWaitBlocked(1)", res);
    cout << C.getFault().getMessage() << endl;
    exit(4);
  }

  pluton::clientRequest* retR = C.executeAndWaitAny();
  if (retR != &R1) {
    failed("bad return from e&W Any", retR == 0);
    exit(5);
  }

  // Nothing should come back within the next 1000ms or so

  res = C.executeAndWaitBlocked(500);
  if (res  != 0) {
    failed("bad return from second executeAndWaitBlocked(500)", res);
    cout << C.getFault().getMessage() << endl;
    exit(6);
  }

  res = C.executeAndWaitAll();
  if (res  != 1) {
    failed("bad return from executeAndWaitAll()", res);
    cout << C.getFault().getMessage() << endl;
    exit(7);
  }

  return(0);
}
