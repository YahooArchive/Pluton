#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <pluton/client_C.h>

/* Exercise the client C wrapper */

static const char* R0Data = "Request Zero Data";
static char* R1Data = "Request One Data";
static char* R2Data = "Request Two Data";

void
failed(int ec, const char* msg, int ev)
{
  printf("tCclient2 failed: ec=%d %s. Ev=%d\n", ec, msg, ev);
  exit(ec);
}


int
main(int argc, char** argv)
{
  pluton_client_C_obj* C;
  pluton_request_C_obj* R0;
  pluton_request_C_obj* R1;
  pluton_request_C_obj* R2;
  pluton_request_C_obj* Rret;
  int res;
  const char* rp;
  int rl;
  const char* map;
  time_t started;
  time_t now;

  if (argc != 2) exit(1);
  map = argv[1];

  /* Create the Client container using the default lookup map */

  printf("one\n");
  C = pluton_client_C_new("tCclient1", 4000);
  if (!C) failed(1, "client constructor returned", 0);

  printf("Client API Version: %s\n", pluton_client_C_getAPIVersion());

  R0 = pluton_request_C_new();
  if (!R0) failed(2, "request constructor R0", 0);

  res = pluton_client_C_addRequest(C, "system.echo.0.raw", R0);
  if (res) failed(3, "addRequest R0 should have failed before init", res);

  if (!pluton_client_C_hasFault(C)) failed(4, "Expected client fault", 0);

  printf("one\n");
  res = pluton_client_C_getFaultCode(C);
  if (res == 0) failed(53, "Expected non-zero fault code", 0);
  printf("Fault code=%d\n", res);
  rp = pluton_client_C_getFaultMessage(C, "prefix text", 1);	/* Long format */
  printf("two\n");
  if (!rp) failed(54, "pluton_client_C_getFaultMessage returned NULL", 0);
  printf("Long Fault Message=%s\n", rp);

  printf("three\n");
  rp = pluton_client_C_getFaultMessage(C, "prefix text", 0);	/* Short format */
  if (!rp) failed(55, "pluton_client_C_getFaultMessage returned NULL", 0);
  printf("four\n");
  printf("Short Fault Message=%s\n", rp);

  /* Init the Client container using the default lookup map */

  res = pluton_client_C_initialize(C, map);
  if (res == 0) failed(5, "initialize returned", 0);

  res = pluton_client_C_getTimeoutMilliSeconds(C);
  if (res != 4000) failed(6, "init t/o != 4000", res);

  pluton_client_C_setTimeoutMilliSeconds(C, 5000);
  res = pluton_client_C_getTimeoutMilliSeconds(C);
  if (res != 5000) failed(7, "set t/o != 5000", res);

  /* Use a bad SK to force a request Fault */

  res = pluton_client_C_addRequest(C, "system.echo.z.XXX", R0);
  if (res) failed(8, "addRequest R0 should have failed with bad SK", res);
  if (!pluton_request_C_hasFault(R0)) failed(9, "R0 should have fault", 0);

  res = pluton_request_C_getFaultCode(R0);
  if (!res) failed(10, "R0 should have non-zero fault code", res);

  rp = pluton_request_C_getFaultText(R0);
  if (!rp) failed(11, "NULL returned from pluton_request_C_getFaultText", 0);

  printf("FaultCode=%d Text=%s\n", res, rp);

  R1 = pluton_request_C_new();
  if (!R1) failed(12, "request constructor R1", 0);

  R2 = pluton_request_C_new();
  if (!R2) failed(13, "request constructor R2", 0);

  pluton_request_C_setClientHandle(R0, (void*) 10001);
  pluton_request_C_setContext(R0, "echo.sleepMS", "1100", 4);
  pluton_request_C_setContext(R1, "echo.sleepMS", "2000", 4);
  pluton_request_C_setContext(R2, "echo.sleepMS", "3000", 4);

  pluton_request_C_setRequestData(R0, R0Data, strlen(R0Data), 0);
  pluton_request_C_setRequestData(R1, R1Data, strlen(R1Data), 1); /* Take a copy */
  pluton_request_C_setRequestData(R2, R2Data, strlen(R2Data), 1); /* Take a copy */

  /* Add request into container and wait for completion */

  res = pluton_client_C_addRequest(C, "system.echo.0.raw", R0);
  if (!res) failed(14, "addRequest R0", res);

  res = pluton_client_C_addRequest(C, "system.echo.0.raw", R1);
  if (!res) failed(15, "addRequest R1", res);

  res = pluton_client_C_addRequest(C, "system.echo.0.raw", R2);
  if (!res) failed(16, "addRequest R2", res);

  started = time(0);
  res = pluton_client_C_executeAndWaitSent(C);
  printf("E&W = %d\n", res);
  if (res <= 0) failed(17, "pluton_client_C_executeAndWaitSent", res);

  now = time(0);
  if (now > (started+1)) failed(18, "E&W took too long", now - started);
  if (!pluton_request_C_inProgress(R0)) failed(19, "R0 Should be inProgress, but isn't", 0);
  if (!pluton_request_C_inProgress(R1)) failed(20, "R1 Should be inProgress, but isn't", 0);
  if (!pluton_request_C_inProgress(R2)) failed(21, "R2 Should be inProgress, but isn't", 0);

  Rret = pluton_client_C_executeAndWaitAny(C);	/* R0 should come back first */
  if (!Rret) failed(22, "pluton_client_C_executeAndWaitAny returned NULL", 0);
  if (Rret != R0) failed(23, "pluton_client_C_executeAndWaitAny returned wrong Ptr", 0);
  now = time(0);
  if (now == started) failed(25, "Service didn't delay for a least 1 sec", 0);

  if (pluton_request_C_inProgress(R0)) failed(26, "R0 Should not be inProgress, but is", 0);
  if (!pluton_request_C_inProgress(R1)) failed(27, "R1 Should be inProgress, but isn't", 0);
  if (!pluton_request_C_inProgress(R2)) failed(28, "R2 Should be inProgress, but isn't", 0);

  res = pluton_client_C_executeAndWaitOne(C, R1);
  if (res <= 0) failed(30, "pluton_client_C_executeAndWaitOne", res);

  if (!pluton_request_C_inProgress(R2)) failed(31, "R2 Should be inProgress, but isn't", 0);

  res = pluton_client_C_executeAndWaitAll(C);	/* Should wait on R2 */
  if (res <= 0) failed(32, "pluton_client_C_executeAndWaitAny", res);

  if (pluton_request_C_inProgress(R2)) failed(33, "R2 Should not be inProgress, but is", 0);

  rp = 0;
  rl = 0;
  res = pluton_request_C_getResponseData(R0, &rp, &rl);
  if (!rp) failed(41, "response data ptr for R0 is NULL", 0);
  if (!rl) failed(42, "response length for R0 is 0", 0);
  if (strncmp(rp, R0Data, rl) != 0) failed(43, "R0 Response data is bad", rl-strlen(R0Data));

  rp = 0;
  rl = 0;
  res = pluton_request_C_getResponseData(R1, &rp, &rl);
  if (!rp) failed(44, "response data ptr for R1 is NULL", 0);
  if (!rl) failed(45, "response length for R1 is 0", 0);
  if (strncmp(rp, R1Data, rl) != 0) failed(46, "R1 Response data is bad", rl-strlen(R1Data));

  rp = 0;
  rl = 0;
  res = pluton_request_C_getResponseData(R2, &rp, &rl);
  if (!rp) failed(47, "response data ptr for R2 is NULL", 0);
  if (!rl) failed(48, "response length for R2 is 0", 0);
  if (strncmp(rp, R2Data, rl) != 0) failed(49, "R2 Response data is bad", rl-strlen(R2Data));

  res = (int) pluton_request_C_getClientHandle(R0);
  if (res != 10001) failed(50, "ClientHandle not the same", res);

  rp = pluton_request_C_getServiceName(R0);
  if (!rp) failed(54, "No service Name", 0);
  printf("Service Name=%s\n", rp);

  pluton_request_C_reset(R0);
  res = (int) pluton_request_C_getClientHandle(R0);
  if (res) failed(53, "ClientHandle not reset", res);

  pluton_request_C_reset(R2);
  rp = 0;
  rl = 0;
  res = pluton_request_C_getResponseData(R2, &rp, &rl);
  if (rp && *rp) failed(51, "response data preset after reset", rl);
  if (rl) failed(52, "response length present after reset", rl);

  pluton_client_C_setDebug(C, 1);
  pluton_client_C_reset(C);


  pluton_request_C_delete(R0);
  pluton_request_C_delete(R1);
  pluton_request_C_delete(R2);

  pluton_client_C_delete(C);

  exit(0);
}
