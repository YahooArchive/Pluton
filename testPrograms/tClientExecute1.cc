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
  cout << "Failed: tClientExecute1 " << err << " val=" << val << endl;
}

const char* SK = "system.echo.0.raw";

int
main(int argc, char** argv)
{
  const char* goodPath = 0;
  if (argc > 1) goodPath = argv[1];

  int res;
  pluton::client C;
  pluton::clientRequest R;

  if (!C.initialize(goodPath)) {
    failed("bad return from good path");
    cout << C.getFault().getMessage("C.initialize()") << endl;
    exit(1);
  }

  // Basic test first to make sure the service is running

  if (!C.addRequest(SK, R)) {
    failed("bad return from addRequest 1");
    cout << C.getFault().getMessage("C.addRequest() 1") << endl;
    exit(1);
  }

  res = C.executeAndWaitAll();
  if (res  != 1) {
    failed("bad return from executeAndWaitAll()", res);
    cout << C.getFault().getMessage() << endl;
    exit(1);
  }

  // WaitSent. The idea here is to send a request that will take 2
  // seconds to complete but the wait send should return virtually
  // instantaneously and the waitAll should take the full 2 seconds or
  // close to.

  struct timeval start;
  gettimeofday(&start, 0);
  R.setContext("echo.sleepMS", "2000");
  R.setContext("echo.log", "yes");
  R.setRequestData("Hello", 5);
  if (!C.addRequest(SK, R)) {
    failed("bad return from addRequest 2");
    cout << C.getFault().getMessage("C.addRequest() 2") << endl;
    exit(1);
  }

  C.setDebug(true);
  res = C.executeAndWaitSent();
  if (res != 1) {
    failed("bad return from executeAndWaitSent() 1", res);
    cout << C.getFault().getMessage("executeAndWaitSent") << endl;
    exit(1);
  }

  if (!R.inProgress()) {
    cout << "Fault Text: " << R.getFaultText() << endl;
    failed("request has completed unexpectedly", R.getFaultCode());
  }

  struct timeval sent;
  gettimeofday(&sent, 0);
  long diff = util::timevalDiffMS(sent, start);
  cout << "MS Diff for waitSent=" << diff << endl;

  if (diff > 500) failed("WaitSent() took more than 0.5 of a second", diff);

  res = C.executeAndWaitAll();
  if (res <= 0) {
    failed("bad return from executeAndWaitAll() 2", res);
    cout << C.getFault().getMessage() << endl;
    exit(1);
  }

  if (R.hasFault()) failed(R.getFaultText().c_str(), R.getFaultCode());

  string resp;
  R.getResponseData(resp);
  cout << "Response from " << R.getServiceName() << " =" << resp << endl;

  if (resp != "Hello") failed("Response mismatch", resp.length());

  struct timeval end;
  gettimeofday(&end, 0);

  diff = util::timevalDiffMS(end, start);
  cout << "MS Diff for waitAll=" << diff << endl;
  if (diff < 1500) {
    failed("WaitAll() took less than 1.5 seconds", diff);
  }

  return(0);
}
