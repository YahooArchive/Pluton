#include <iostream>

#include <sys/time.h>

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"
#include <pluton/client.h>
#include <pluton/clientEvent.h>

using namespace std;

// Exercise the reset method

static void
failed(const char* err, int val=0)
{
  cout << "Failed: tClientReset " << err << " val=" << val << endl;

  exit(1);
}

const char* SK = "system.echo.0.raw";

static void blocking(const char*);
static void nonBlocking(const char*);

int
main(int argc, char** argv)
{
  assert(argc == 2);

  const char* goodPath = argv[1];


  cout << endl << endl << "NonBlocking reset" << endl;
  nonBlocking(goodPath);

  cout << "Blocking reset" << endl;
  blocking(goodPath);

  cout << "Ok" << endl;
  exit(0);
}


void
blocking(const char* goodPath)
{
  int res;
  pluton::client C;

  if (!C.initialize(goodPath)) {
    cout << C.getFault().getMessage("C.initialize()") << endl;
    failed("bad return from blocking good path");
  }

  C.setDebug(true);

  pluton::clientRequest R1;
  pluton::clientRequest R2;
  R1.setContext("echo.sleepMS", "100");
  R1.setContext("echo.sleepMS", "200");

  if (!C.addRequest(SK, R1)) failed("bad return from blocking addRequest(R1)");
  if (!C.addRequest(SK, R2)) failed("bad return from blocking addRequest(R2)");

  // Get one request back. Doesn't really matter which

  pluton::clientRequest* rFirst = C.executeAndWaitAny();
  if (!rFirst) failed("Nothing came back from executeAndWaitAny()");
  if (rFirst->hasFault()) {
    cout << "Wait Any Fault text: " << rFirst->getFaultText() << endl;
    failed("Unexpected fault", rFirst->getFaultCode());
  }

  R1.reset();
  R2.reset();

  // Should be able to add them back in now

  R1.setContext("echo.sleepMS", "100");
  R2.setContext("echo.sleepMS", "200");

  if (!C.addRequest(SK, R1)) failed("bad return from second blocking addRequest(R1)");
  if (!C.addRequest(SK, R2)) failed("bad return from second blocking addRequest(R2)");

  res = C.executeAndWaitAll();
  if (res <= 0) failed("Expected +ve back from executeAndWaitAll", res);

  if (R1.hasFault()) failed("T2 R1 has blocking fault", R1.getFaultCode());
  if (R2.hasFault()) failed("T2 R2 has blocking fault", R2.getFaultCode());
}


void
nonBlocking(const char* goodPath)
{
  int res;
  pluton::clientEvent C;

  if (!C.initialize(goodPath)) {
    cout << C.getFault().getMessage("C.initialize()") << endl;
    failed("bad return from nonBlocking good path");
  }

  C.setDebug(true);

  pluton::clientRequest R1;
  pluton::clientRequest R2;
  R1.setContext("echo.sleepMS", "100");
  R1.setContext("echo.sleepMS", "200");

  if (!C.addRequest(SK, R1)) failed("bad return from nonBlocking addRequest(R1)");
  if (!C.addRequest(SK, R2)) failed("bad return from nonBlocking addRequest(R2)");

  struct timeval now;
  const pluton::clientEvent::eventWanted* ewp = C.getNextEventWanted(&now, &R1);

  sleep(1);
  res = C.sendCanReadEvent(ewp->fd);
  if (res < 0) failed("bad sendCanReadEvent", res);

  // Requests have been started

  R1.reset();
  R2.reset();

  // Should be able to add them back in now

  R1.setContext("echo.sleepMS", "100");
  R2.setContext("echo.sleepMS", "200");

  if (!C.addRequest(SK, R1)) failed("bad return from second nonBlocking addRequest(R1)");
  if (!C.addRequest(SK, R2)) failed("bad return from second nonBlocking addRequest(R2)");
}
