#include <iostream>

#include <sys/time.h>

#include <assert.h>
#include <stdlib.h>

#include <pluton/client.h>

using namespace std;

// Force a service to be left in a partially read request state, then
// send a new request to ensure that it resets properly.


const char* SK = "system.echo.0.raw";

int
main(int argc, char** argv)
{
  assert(argc == 2);

  const char* goodPath = argv[1];

  int res;
  pluton::client C;

  if (!C.initialize(goodPath)) {
    cout << "Failed: " << C.getFault().getMessage("Failed: C.initialize()") << endl;
    exit(1);
  }

  C.setDebug(true);
  C.setTimeoutMilliSeconds(1);
  pluton::clientRequest R;

  static const int bufSize = 1000 * 1000 * 60;		// 60MB ought to timeout after 1MS
  char* buf = new char[bufSize];
  R.setRequestData(buf, bufSize);

  if (!C.addRequest(SK, R)) {
    cout << "Failed Addr 1: " << C.getFault().getMessage("C.addRequest() 1") << endl;
    exit(2);
  }
  res = C.executeAndWaitAll();
  if (res  != 0) {
    cout << "Failed: Expected T/O from executeAndWaitAll: " << res << endl;
    exit(3);
  }

  // Now do a second request to make sure the service isn't broken

  C.setTimeoutMilliSeconds(4000);	// Make the t/o really small
  R.setRequestData(buf, 0);

  if (!C.addRequest(SK, R)) {
    cout << "Failed: AddR 2: " << C.getFault().getMessage("C.addRequest() 2") << endl;
    exit(4);
  }
  res = C.executeAndWaitAll();
  if (res  <= 0) {
    cout << "Failed: E&W2 " << res << " " << C.getFault().getMessage("executeAndWaitAll") << endl;
    exit(5);
  }

  if (R.hasFault()) {
    cout << "Failed: request Faulted: " << R.getFaultText() << endl;
    exit(6);
  }

  return(0);
}
