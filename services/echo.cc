/*
Copyright (c) 2010, Yahoo! Inc. All rights reserved.

Redistribution and use of this software in source and binary forms, with or
without modification, are permitted provided that the following conditions are
met: 

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation and/or 
other materials provided with the distribution.

* Neither the name of Yahoo! Inc. nor the names of its contributors may be used 
to endorse or promote products derived from this software without specific prior 
written permission of Yahoo! Inc.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

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
