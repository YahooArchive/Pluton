#include <iostream>

#include <time.h>
#include <assert.h>
#include <stdlib.h>

#include <pluton/client.h>

using namespace std;

int
main(int argc, char** argv)
{
  assert(argc >= 2);

  const char* goodPath = argv[1];

  pluton::client C;

  if (!C.initialize(goodPath)) {
    cout << C.getFault().getMessage("initialize unexpectedly failed") << endl;
    exit(1);
  }

  pluton::clientRequest R1;
  if (!C.addRequest("system.echo.0.raw", R1)) {
    cout << C.getFault().getMessage("addRequest(R1) unexpectedly failed") << endl;
    exit(2);
  }

  pluton::clientRequest R2;
  R2.setContext("echo.sleepMS", "1100");
  if (!C.addRequest("system.echo.0.raw", R2)) {
    cout << C.getFault().getMessage("addRequest(R2) unexpectedly failed") << endl;
    exit(2);
  }

  cout << time(0) << " Starting E&W" << endl;
  int res = C.executeAndWaitOne(R2);	// Wait for R2, but R1 should finish well first
  if (res <= 0) {
    cout << C.getFault().getMessage("executeAndWaitOne(R2) unexpectedly failed") << endl;
    exit(3);
  }
  cout << time(0) << " Completed E&W" << endl;

  if (R1.hasFault()) cout << "R1 fault: " << R1.getFaultText() << endl;
  if (R2.hasFault()) cout << "R2 fault: " << R2.getFaultText() << endl;

  // inProgress should be false with 0.52 of the library when the
  // request is completed.

  if (R1.inProgress()) {
    cout << "Error: R1.inProgress() still true!" << endl;
    exit(4);
  }

  if (R2.inProgress()) {
    cout << "Error: R2.inProgress() still true!" << endl;
    exit(5);
  }
}
