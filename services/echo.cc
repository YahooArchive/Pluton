#include <string>
#include <iostream>
#include <sstream>

using namespace std;

#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"
#include "pluton/service.h"


int
main(int argc, char** argv)
{
  pluton::service S("echo");

  int loopCount = 0;
  if (argc > 1) loopCount = atoi(argv[1]);

  if (!S.initialize()) {
    clog << "echo: Cannot initialize as a service: " << S.getFault().getMessage() << std::endl;
    exit(42);
  }

  int	requestCount = 0;
  long long byteCount = 0;

  while (S.getRequest()) {
    const char* cp;
    int len;
    S.getRequestData(cp, len);
    std::string cid;
    S.getClientName(cid);

    std::string sleepStr;
    bool logFlag = S.getContext("echo.log", sleepStr);
    S.getContext("echo.sleepMS", sleepStr);

    if (logFlag) std::clog << "Context: echo.sleepMS=" << sleepStr << std::endl;
    int sleepTime = atoi(sleepStr.c_str());

    if (sleepTime < 0) {
      clog << "Sleep time: " << sleepStr << std::endl;
      if (!S.sendFault(111, "sleep time negative")) break;
      continue;
    }

    std::string sleepAfterStr;
    S.getContext("echo.sleepAfter", sleepAfterStr);
    if (logFlag) std::clog << "Context: echo.sleepAfter=" << sleepAfterStr << std::endl;
    int sleepAfter = atoi(sleepAfterStr.c_str());
    struct timeval startTime;
    gettimeofday(&startTime, 0);

    if (sleepTime > 0) {
      usleep(1000 * sleepTime);
      if (logFlag) std::clog << "Slept for " << 1000 * sleepTime << "uSecs" << std::endl;
    }

    for (int zz=0; zz<loopCount; zz++) ;	// Burn some CPU

    struct timeval endTime;
    gettimeofday(&endTime, 0);
    
    if (logFlag) {
      clog << "sleep wanted=" << sleepStr
	   << " got=" << util::timevalDiffMS(endTime, startTime) << endl;
    }

    if (!S.sendResponse(cp, len)) break;

    if (logFlag) clog << "Response sent" << endl;

    ++requestCount;
    byteCount += len;
    if ((requestCount % 10000) == 0) {
      std::clog << time(0) <<  " echo stats: Rq="
		<< requestCount << " MB=" << byteCount / 1000000 << std::endl;
    }

    if (sleepAfter > 0) {
      sleep(sleepAfter);
      if (logFlag) clog << "sleepAfter=" << sleepAfter << endl;
    }
  }

  std::clog << "echo stats: Rq=" << requestCount << " MB=" << byteCount / 1000000 << std::endl;
  clog.flush();

  if (S.hasFault()) {
    std::clog << "Warning: " << S.getFault().getMessage() << std::endl;
  }

  S.terminate();

  return(S.hasFault() ? 41 : 0);
}
