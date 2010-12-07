#include <iostream>
#include <string>

#include <sys/time.h>

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"
#include <pluton/client.h>

#define	REPEATCOUNT	50

using namespace std;


static void
failed(int ec, const pluton::fault& F, const char* err)
{
  cout << "Failed: " << F.getMessage(err, true) << endl;

  exit(ec);
}

const char* SK = "system.echo.0.raw";

int
main(int argc, char** argv)
{
  assert(argc >= 2);

  const char* goodPath = argv[1];

  pluton::client C;

  if (!C.initialize(goodPath)) failed(10, C.getFault(), "bad return from initialize");

  if (argc > 2) C.setDebug(true);

  pluton::clientRequest R1;
  R1.setAttribute(pluton::keepAffinityAttr | pluton::noRetryAttr);
  C.addRequest(SK, R1);
  C.executeAndWaitAll();
  if (C.hasFault()) failed(11, C.getFault(), "Execute 1");

  string service = R1.getServiceName();
  cout << "Affinity with " << service << endl;
  R1.setAttribute(pluton::needAffinityAttr);
  for (int ix=0; ix < 100; ++ix) {
    C.addRequest(SK, R1);
    C.executeAndWaitAll();
    if (C.hasFault()) failed(12, C.getFault(), "Execute 2");
    if (service != R1.getServiceName()) {
      cout << "Failed: service differs " << service << " != " <<  R1.getServiceName() << endl;
      exit(1);
    }
  }

  // Hold onto the connection for longer than the affinity timeout

  sleep(20);
  C.addRequest(SK, R1);
  C.executeAndWaitAll();
  if (C.hasFault()) failed(13, C.getFault(), "Execute 3");

  if (!R1.hasFault()) {
    cout << "Failed: Expected fault" << endl;
    exit(2);
  }

  // Now re-run the request as normal, it should work as the service
  // closed the connection.

  R1.clearAttribute(pluton::allAttrs);
  C.addRequest(SK, R1);
  C.executeAndWaitAll();
  if (C.hasFault()) failed(14, C.getFault(), "Execute 4");

  if (R1.hasFault()) {
    cout << "Failed: Fault on post timeout: " << R1.getFaultText() << endl;
    exit(3);
  }

  return(0);
}
