#include <iostream>

#include <assert.h>
#include <stdlib.h>

#include <pluton/client.h>

using namespace std;

// Check that the fault structure is unique per pluton::Client object
// (Changed at 0.48 of the library).


static int failedFlag = false;
static void
failed(const char* err, const char* tx1=0)
{
  cout << "Failed: " << err;
  if (tx1) cout << " " << tx1;
  cout << endl;
  failedFlag = true;
}

int
main(int argc, char** argv)
{
  assert(argc == 3);

  const char* goodPath = argv[2];

  // Initialization failures

  pluton::client C1;
  pluton::client C2;
  C1.setDebug(true);
  C2.setDebug(true);

  if (!C1.initialize(goodPath)) {
    failed("C1: bad return from good path", goodPath);
    cout << C1.getFault().getMessage("initialize goodpath unexpectedly failed") << endl;
    exit(1);
  }

  if (!C2.initialize(goodPath)) {
    failed("C2: bad return from good path", goodPath);
    cout << C2.getFault().getMessage("initialize goodpath unexpectedly failed") << endl;
    exit(1);
  }

  if (C1.hasFault()) {
    failed("C1: true return gave fault 1", goodPath);
    cout << C1.getFault().getMessage() << endl;
 }

  if (C2.hasFault()) {
    failed("C2: true return gave fault 1", goodPath);
    cout << C2.getFault().getMessage() << endl;
 }

  pluton::clientRequest R1;
  pluton::clientRequest R2;

  const char* badKey = "toofewtokens.b.c";
  const char* goodKey = "system.echo.0.raw";

  if (C1.addRequest(badKey, R1)) failed("C1: addRequest allowed with bad serviceKey", badKey);

  // Pre 0.48, a singleton fault stucture was used internally. 0.48
  // introduced a fault structure per pluton::client which makes more
  // sense as faults associated with one client instance have nothing
  // to do with faults in another client instance.

  // addRequest clears the fault structure. If that structure is still
  // a global, then C1.hasFault() will be cleared as well since it is
  // an alias of C1.hasFault.

  if (!C2.addRequest(goodKey, R2)) failed("C2: addRequest failed with good serviceKey", goodKey);

  if (!C1.hasFault()) failed("C1: does not have fault when expected", badKey);
  if (C2.hasFault()) {
    failed("C2: has fault when non expected", goodKey);
    cout << C2.getFault().getMessage() << endl;
  }

  return(failedFlag ? 1 : 0);
}
