#include <string>
#include <iostream>
#include <sstream>

using namespace std;

#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include <st.h>

#include "requestImpl.h"
#include "serviceImpl.h"


int
main(int argc, char** argv)
{
  pluton::perCallerService owner(0);
  pluton::serviceImpl S("echo_st");
  time_t startTime = time(0);
  ostringstream os;

  os << "R:echo_st " << startTime;
  std::string myRK = os.str();

  int loopCount = 0;
  if (argc > 1) loopCount = atoi(argv[1]);

  if (!S.initialize(&owner, "echo_st")) {
    std::clog << owner.getFault().getMessage("echo_st") << endl;
    exit(1);
  }

  st_init();

  S.setPollProxy(st_poll);

  int	requestCount = 0;
  long long byteCount = 0;

  pluton::serviceRequestImpl R;

  while (S.getRequest(&owner, &R)) {
    const char* cp;
    int len;
    R.getRequestData(cp, len);
    std::string cid;
    R.getClientName(cid);

    std::string sleepStr;
    if (R.getContext("echo.log", sleepStr)) {
      clog << sleepStr << endl;
    }

    if (R.getContext("echo.die", sleepStr)) {
	if (time(0) & 1) {
	   cerr << "Die" << endl;
	   S.terminate();
	   exit(0);
	}
    }

    R.getContext("echo.sleepMS", sleepStr);

    int sleepTime = atoi(sleepStr.c_str());
    if (sleepTime < 0) {
      std::clog << "Sleep time: " << sleepStr << std::endl;
      R.setFault(static_cast<pluton::faultCode>(111), "sleep time negative");
      R.setResponseData("", 0);
      if (!S.sendResponse(&owner, &R)) break;
      continue;
    }

    if (!sleepStr.empty()) usleep(1000 * atoi(sleepStr.c_str()));
    for (int zz=0; zz<loopCount; zz++) ;	// Burn some CPU
    
    R.setFault(pluton::noFault);
    R.setResponseData(cp, len);
    if (!S.sendResponse(&owner, &R)) break;
    ++requestCount;
    byteCount += len;
    if ((requestCount % 10000) == 0) {
      std::clog << time(0) <<  " echo stats: Rq="
		<< requestCount << " MB=" << byteCount / 1000000 << std::endl;
    }
  }

  std::clog << "echo stats: Rq=" << requestCount << " MB=" << byteCount / 1000000 << std::endl;

  if (owner.hasFault()) {
    std::clog << "Error: " << owner.getFault().getMessage() << std::endl;
  }

  S.terminate();

  return(0);
}
