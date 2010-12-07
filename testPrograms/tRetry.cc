#include <iostream>
#include <string>

#include <stdlib.h>

#include "serviceImpl.h"

using namespace std;

// Fail if context says to

int
main(int argc, char** argv)
{
  pluton::serviceImpl S;
  pluton::perCallerService owner("tRetry");
  pluton::serviceRequestImpl R;

  S.setDefaultRequest(&R);
  S.setDefaultOwner(&owner);

  owner.initialize();
  if (!S.initialize(&owner)) {
    cerr << "S.initialize() failed" << endl;
    exit(1);
  }

  bool sleepFlag = false;
  bool garbageFlag = false;

  string ct;
  while (S.getRequest(&owner, &R)) {
    if (sleepFlag) {
      clog << "Sleeping for 10" << endl;
      sleep(10);
      sleepFlag = false;
      R.setFault(pluton::noFault);
      R.setResponseData("", 0);
      S.sendResponse(&owner, &R);
      continue;
    }

    if (garbageFlag) {
      clog << "Sending garbage" << endl;
      S.sendRawResponse(&owner, R.getRequestDataLength(), "4:some,junkers", 10, "funct", 1);
      garbageFlag = false;
      continue;
    }

    sleepFlag = R.getContext("sleep", ct);
    garbageFlag = R.getContext("garbage", ct);

    clog << "SF=" << sleepFlag << ", GF=" << garbageFlag << endl;

    R.setFault(pluton::noFault);
    R.setResponseData("", 0);
    S.sendResponse(&owner, &R);
  }

  return 0;
}
