#include <iostream>

#include <sys/time.h>
#include <assert.h>
#include <stdlib.h>

#include "util.h"
#include <pluton/client.h>

using namespace std;

// Check to see that a service exit(0) causes timeouts and an exit(1)
// causes an immediate failure.


static void
failed(const char* err, int val=0)
{
  cout << "Failed: tServiceExit " << err << " val=" << val << endl;
}

const char* SK = "system.exit.0.raw";
const char* SKno = "nobackoff.exit.0.raw";

int
main(int argc, char** argv)
{
  assert(argc == 2);

  const char* goodPath = argv[1];

  int res;
  pluton::client C("tServiceExit", 4000);

  if (!C.initialize(goodPath)) {
    failed("bad return from good path");
    cout << C.getFault().getMessage("C.initialize()") << endl;
    exit(1);
  }

  // First time will timeout as client retries

  cout << time(0) << " Running first request - expect immediate timeout" << endl;
  pluton::clientRequest R;
  R.setContext("exit.code", "1");
  if (!C.addRequest(SK, R)) {
    failed("bad return from addRequest 1");
    cout << C.getFault().getMessage("C.addRequest() 1") << endl;
    exit(2);
  }
  res = C.executeAndWaitAll();
  if (res != 1) {
    failed("bad executeAndWaitAll 1");
    cout << C.getFault().getMessage("executeAndWaitAll() 1") << endl;
    exit(3);
  }

  if (R.hasFault()) cout << "Request faultcode=" << R.getFaultCode() << endl;

  // But second time should fail quickly

  cout << time(0) << " Running second request - expect no delay" << endl;
  R.setContext("exit.code", "1");
  if (!C.addRequest(SK, R)) {
    failed("bad return from addRequest 1");
    cout << C.getFault().getMessage("C.addRequest() 1") << endl;
    exit(4);
  }
  time_t startTime = time(0);
  res = C.executeAndWaitAll();
  if (res  <= 0) cout << res << " " << C.getFault().getMessage() << endl;
  time_t endTime = time(0);

  // Duration must be less than 4000ms

  cout << time(0) << " Checking Delay " << endTime - startTime << endl;

  if ((endTime - startTime) > 1) {
    failed("Delay is > 1", endTime - startTime);
    exit(5);
  }

  // Now run an exit(1) with a zero exec-failure-backoff. The results
  // are checked by the shell script.

  R.setContext("exit.code", "1");
  if (!C.addRequest(SKno, R)) {
    failed("bad return from addRequest 1");
    cout << C.getFault().getMessage("C.addRequest() 1") << endl;
    exit(6);
  }
  res = C.executeAndWaitAll();
  if (res  <= 0) cout << res << " " << C.getFault().getMessage() << endl;


  return(0);
}
