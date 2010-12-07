#include <stdio.h>
#include <stdlib.h>

#include <pluton/client_C.h>

/* Exercise the client request C wrapper */

void
failed(int ec, const char* msg, int ev)
{
  printf("tCclient1 failed: ec=%d %s. Ev=%d\n", ec, msg, ev);
  exit(ec);
}


int
main(int argc, char** argv)
{
  pluton_request_C_obj* R0;
  int res;
  const char* rp;
  int rl;
  const char* map;

  if (argc != 2) exit(1);
  map = argv[1];

  R0 = pluton_request_C_new();
  if (!R0) failed(4, "request constructor returned", 0);

  pluton_request_C_setRequestData(R0, "Echo Request 0", 14, 0);

  res = pluton_request_C_getAttribute(R0, pluton_request_C_noWaitAttr);
  if (res) failed(5, "unexpected attributes A", pluton_request_C_noWaitAttr);

  res = pluton_request_C_getAttribute(R0, pluton_request_C_noRemoteAttr);
  if (res) failed(6, "unexpected attributes A", pluton_request_C_noRemoteAttr);

  res = pluton_request_C_getAttribute(R0, pluton_request_C_noRetryAttr);
  if (res) failed(7, "unexpected attributes A", pluton_request_C_noRetryAttr);

  res = pluton_request_C_getAttribute(R0, pluton_request_C_keepAffinityAttr);
  if (res) failed(8, "unexpected attributes A", pluton_request_C_keepAffinityAttr);

  res = pluton_request_C_getAttribute(R0, pluton_request_C_needAffinityAttr);
  if (res) failed(9, "unexpected attributes A", pluton_request_C_needAffinityAttr);

  pluton_request_C_setAttribute(R0, pluton_request_C_noRemoteAttr);
  res = pluton_request_C_getAttribute(R0, pluton_request_C_noRemoteAttr);
  if (!res) failed(10, "pluton_request_C_noRemoteAttr not set", res);

  pluton_request_C_clearAttribute(R0, pluton_request_C_noRemoteAttr);
  res = pluton_request_C_getAttribute(R0, pluton_request_C_noRemoteAttr);
  if (res) failed(11, "pluton_request_C_noRemoteAttr still set", res);

  pluton_request_C_setContext(R0, "echo.sleepMS", "1600", 4);

  rp = 0;
  rl = 0;
  res = pluton_request_C_getResponseData(R0, &rp, &rl);

  if (rp && *rp) failed(30, "Did not expect return data", 0);
  if (rl) failed(31, "Did not expect non-zero length", rl);

  pluton_request_C_reset(R0);

  /* Nothing left after reset */

  res = pluton_request_C_getAttribute(R0, pluton_request_C_noRemoteAttr);
  if (res) failed(32, "unexpected attributes A", pluton_request_C_noRemoteAttr);

  pluton_request_C_delete(R0);

  exit(0);
}
