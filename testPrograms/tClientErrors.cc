#include <iostream>

#include <assert.h>
#include <stdlib.h>

#include <pluton/client.h>

using namespace std;

// Get all the error returns available via the client api without
// invoking a service.

static int failedFlag = false;
static void
failed(const char* err, const char* tx1=0)
{
  cout << "Failed: " << err;
  if (tx1) cout << " " << tx1;
  cout << endl;
  failedFlag = true;
}

static const char* badKeys[] = {
  "badserialization.b.c.d",
  "toofewtokens.b.c",
  "",
  "nonumericversion.b.xxx.raw",
  "octalversion.b.034.raw",
  "nonnumericversion.b.9999x.raw",
  "hugeversion.b.9999999999999.raw",
  "missingfunction..23.raw",
  ".missingapplication.24.raw",
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.longapplication.1.raw",
  "longfunction.bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb.1.raw",
  "system.echo.0.raw.",
  "system.echo.0.raw.C",
  0 };

int
main(int argc, char** argv)
{
  assert(argc == 3);

  const char* badPath = argv[1];
  const char* goodPath = argv[2];
  const char* SK = "system.echo.0.raw";

  // Initialization failures

  pluton::client C;
  C.setDebug(true);

  if (C.initialize(badPath)) failed("Good return from badpath");
  if (!C.hasFault()) failed("false return but no fault 1");
  if (C.hasFault()) cout << "f1: " << C.getFault().getMessage() << endl;

  if (C.initialize("/dev/zero")) failed("good return from zero path");
  if (!C.hasFault()) failed("false return but no fault 2");
  if (C.hasFault()) cout << "f2: " << C.getFault().getMessage() << endl;

  // Use before init

  pluton::clientRequest R;
  if (C.addRequest("a.b.c.d", R)) failed("Expected not initialized fault");
  if (!C.hasFault()) failed("false return but no fault 3");
  if (C.hasFault()) cout << "f3: " << C.getFault().getMessage() << endl;

  // Reserved context

  if (R.setContext("pluton.reserved", "data")) failed("pluton. contexted allowed");
  if (!R.hasFault()) failed("false return but no fault 4");


  // Subsequent tests need an initialized client

  if (!C.initialize(goodPath)) {
    failed("bad return from good path", goodPath);
    cout << C.getFault().getMessage("initialize goodpath unexpectedly failed") << endl;
    exit(1);
  }
  if (C.hasFault()) {
    failed("true return gave fault 1", goodPath);
    cout << C.getFault().getMessage() << endl;
 }

  // Dupe init is ok as we allow perCallerClients

  if (!C.initialize(goodPath)) failed("Subsequent initialize() allowed");
  if (C.hasFault()) failed(C.getFault().getMessage("initialize").c_str(),"");
  if (!C.hasFault()) cout << "f4: dupe inits ok" << endl;

  // Various bad service keys

  const char** kp = badKeys;
  while (*kp) {
    if (C.addRequest(*kp, R)) failed("addRequest allowed with bad serviceKey", *kp);
    if (!C.hasFault()) failed("false return but not fault 5a", *kp);
    if (C.hasFault()) cout << "f5a: " << *kp << " " << C.getFault().getMessage() << endl;
    ++kp;
  }

  // Add a request twice should fail

  C.reset();
  if (!C.addRequest(SK, R)) failed("addRequest failed for dupe test");
  if (C.hasFault()) cout << "f10a: " << C.getFault().getMessage("add dupe1") << endl;

  if (C.addRequest(SK, R)) failed("addRequest did NOT failed for dupe test");
  if (C.hasFault()) cout << "f10b: " << C.getFault().getMessage("add dupe2") << endl;

  C.reset();

  // Attribute testing

  R.clearAttribute(pluton::allAttrs);
  if (R.getAttribute(pluton::allAttrs)) failed("Got attrs when all clear");

  R.setAttribute(pluton::noWaitAttr);
  if (!R.getAttribute(pluton::noWaitAttr)) failed("set attr lost");

  R.clearAttribute(pluton::allAttrs);

  // Make sure affinity checks are done by API

  // Should fail as noRetry is not set

  R.setAttribute(pluton::keepAffinityAttr);
  if (C.addRequest("a.b.c.d", R)) failed("addRequest allowed with keepAffinity");
  if (!C.hasFault()) failed("false return but not fault 6");
  if (C.hasFault()) cout << "f6: " << C.getFault().getMessage() << endl;

  // Should fail as no previous affinity

  R.clearAttribute(pluton::allAttrs);
  R.setAttribute(pluton::needAffinityAttr);
  if (C.addRequest("a.b.c.d", R)) failed("addRequest allowed with bad needAffinity");
  if (C.hasFault()) cout << "f7: " << C.getFault().getMessage() << endl;


  // Should fail as affinity cannot have noWait

  R.clearAttribute(pluton::allAttrs);
  R.setAttribute(pluton::keepAffinityAttr);
  R.setAttribute(pluton::noWaitAttr);
  if (C.addRequest(SK, R)) failed("addRequest allowed with bad needAffinity+noWait");
  if (C.hasFault()) cout << "f8: " << C.getFault().getMessage() << endl;

  // Should succeed

  R.clearAttribute(pluton::allAttrs);
  R.setAttribute(pluton::keepAffinityAttr);
  R.setAttribute(pluton::noRetryAttr);
  if (!C.addRequest(SK, R)) failed("addRequest failed for affinity");
  if (C.hasFault()) {
    failed("true return gave fault 2");
    cout << "f9: fault: " << C.getFault().getMessage() << endl;
  }

  // Empty request list should fail

  C.reset();
  if (C.executeAndWaitOne(R)) failed("waitOne didn't fail");
  if (!C.hasFault()) failed("expected fault waitOne");
  if (C.hasFault()) cout << C.getFault().getMessage("waitOneFailure") << endl;

  return(failedFlag ? 1 : 0);
}
