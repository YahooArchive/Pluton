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
