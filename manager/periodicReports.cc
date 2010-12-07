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

#include <iostream>
#include <sstream>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifndef RUSAGE_SELF
#define   RUSAGE_SELF     0
#endif

#include "logging.h"
#include "util.h"
#include "processExitReason.h"
#include "process.h"
#include "service.h"
#include "manager.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Wrapper function to get the current memory usage in MB
//////////////////////////////////////////////////////////////////////

static  int
getResourceUsage(int& MBytes, int& percentCPU, int upFor)
{

  MBytes = percentCPU = 0;

  struct rusage ru;
  int res = getrusage(RUSAGE_SELF, &ru);
  if (res == -1) return -1;

  MBytes = ru.ru_maxrss / 1024;

  long cpu = ru.ru_utime.tv_sec + ru.ru_stime.tv_sec;
  if (upFor < 1) upFor = 1;     // Protect division
  percentCPU = cpu * 100 / upFor;

  return 0;
}

void
manager::periodicStatisticsReport(time_t now)
{
  string s;
  getStatistics(s, _statisticsLogInterval, now, true);
  LOGPRT << s;
}


//////////////////////////////////////////////////////////////////////
// Generate the formated lines of stats. If this is a periodic report,
// add the interval counters to the report then reset interval
// counters.
//////////////////////////////////////////////////////////////////////

void
manager::getStatistics(std::string& s, int interval, time_t now, bool periodicFlag)
{
  ostringstream os;

  if (periodicFlag) {
    time_t upTime = now - _startTime;
    util::durationToEnglish(upTime, s);
    int MBytes, percentCPU;
    getResourceUsage(MBytes, percentCPU, upTime);
    os << "Uptime: " << s << " CPU=" << percentCPU << "% Mem=" << MBytes << "MB ";
  }

  os << "Objects:"
     << " Services=" << service::getCurrentObjectCount()
     << " Process=" << process::getCurrentObjectCount()
     << " Max (" << service::getMaximumObjectCount()
     << "/" << process::getMaximumObjectCount()
     << ")"
     << endl;

  if (_zombieCount || (_childCount > _activeProcessCount)) {
    os << "Zombies: Services=" << _zombieCount
       << " Children=" << _childCount - _activeProcessCount
       << endl;
  }

  os << "Stats: Processes=" << _activeProcessCount << " Children=" << _childCount;

  if (_exitReasonCount[processExit::serviceShutdown])
    os << " serviceShutdown=" << _exitReasonCount[processExit::serviceShutdown];
  if (_exitReasonCount[processExit::unresponsive])
    os << " unresponsive=" << _exitReasonCount[processExit::unresponsive];
  if (_exitReasonCount[processExit::abnormalExit])
    os << " abnormalExit=" << _exitReasonCount[processExit::abnormalExit];
  if (_exitReasonCount[processExit::lostIO])
    os << " lostIO=" << _exitReasonCount[processExit::lostIO];
  if (_exitReasonCount[processExit::runawayChild])
    os << " runawayChild=" << _exitReasonCount[processExit::runawayChild];
  if (_exitReasonCount[processExit::maxRequests])
    os << " maxRequests=" << _exitReasonCount[processExit::maxRequests];
  if (_exitReasonCount[processExit::excessProcesses])
    os << " excessProcesses=" << _exitReasonCount[processExit::excessProcesses];
  if (_exitReasonCount[processExit::idleTimeout])
    os << " idleTimeout=" << _exitReasonCount[processExit::idleTimeout];
  if (_exitReasonCount[processExit::acceptFailed])
    os << " acceptFailed=" << _exitReasonCount[processExit::acceptFailed];

  os << endl;

  if (_processAdded || _requestsReported) {
    os << "Rates: Processes=" << _processAdded << " Requests=" << _requestsReported << endl;
  }
    
  if (periodicFlag) {
    _processAdded = 0;
    _requestsReported = 0;
    zeroPeriodicCounts();
  }

  s = os.str();
}
