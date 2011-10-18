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

#include "config.h"

using namespace std;

#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "util.h"
#include "pluton/client.h"


static const char* usage =
"\n"
"Usage: plBatch [ -dh ] [-c repeatCount] [-i seconds] [ -m minrequests ] [-L lookupMap]\n"
"\n"
"Read in a batch file from STDIN and execute that repeatedly.\n"
"\n"
" -d    Turn on client library debug\n"
" -h    Display Usage and exit\n"
" -c    Number of times to run the batch (default: 1)\n"
" -i	Number of seconds between batches (default: 1)\n"
" -m    Minimum requests per batch (default: 1)\n"
" -L   The Lookup Map used to connect with the manager (default: '')\n"
"\n"
"The batch file consists of a single line per request with columns:\n"
"\n"
"                     Request Data           CPU                 Latency\n"
"command probability MinSize MaxSize   MinValue MaxValue    MinValue MaxValue\n"
"\n"
"See also: " PACKAGE_URL "\n"
"\n";

//////////////////////////////////////////////////////////////////////

class command {
public:
  string 	serviceKey;
  float		probability;
  bool		sentInBatch;
  int		requestMinSize;
  int		requestMaxSize;
  int		cpuMinValue;
  int		cpuMaxValue;
  int		latencyMinValue;
  int		latencyMaxValue;
  string	requestData;
  pluton::clientRequest R;
};

static  int	totalBatchCount = 0;
static	int	rqCount = 0;
static	int	totalRequestCount = 0;
static  float	totalPercentOfOrig = 0;
static  timeval	minBatchTime;
static  timeval	maxBatchTime;
static  timeval totalElapseTime;

static	void
trackResults(timeval& st, timeval& et, int latencyMS, int cpuMS, int maxLatency, bool printFlag)
{
  timeval elapse;
  util::timevalDiff(elapse, et, st);

  ++totalBatchCount;
  timeval latency = { 0, latencyMS * 1000 };
  util::timevalNormalize(latency);

  float latencyF = latency.tv_usec;
  latencyF /= util::MICROSECOND;
  latencyF += latency.tv_sec;

  float elapseF = elapse.tv_usec;
  elapseF /= util::MICROSECOND;
  elapseF += elapse.tv_sec;
  float savingsF = latencyF - elapseF;
  float cpuMSF = cpuMS;
  cpuMSF /= 1000;
  float maxLatencyF = maxLatency;
  maxLatencyF /= 1000;
  float percentOfOrigF = elapseF / latencyF * 100;
  totalPercentOfOrig += percentOfOrigF;

  if (printFlag) {
    cout << "seq=" << totalBatchCount << " requests="
	 << rqCount
	 << " ifSerial=" << setprecision(3) << latencyF
	 << " elapsed=" << setprecision(3) << elapseF
	 << " Saved=" << setprecision(3) << savingsF
	 << " " << setprecision(3) << percentOfOrigF << "%"
	 << " CPU=" << setprecision(3) << cpuMSF << " MaxL=" << maxLatencyF
	 << " Opt+ " << setprecision(3) << elapseF * 100 / maxLatencyF - 100 << "%"
	 << endl;
  }

  if (totalBatchCount == 1) {
    totalElapseTime = elapse;
    minBatchTime = elapse;
    maxBatchTime = elapse;
  }
  else {
    if (util::timevalCompare(elapse, minBatchTime) < 0) minBatchTime = elapse;
    if (util::timevalCompare(elapse, maxBatchTime) > 0) maxBatchTime = elapse;
    util::timevalAdd(totalElapseTime, elapse);
  }
}


//////////////////////////////////////////////////////////////////////

static void
printResults()
{
  cout << "plBatchs: " << totalBatchCount;
  if (totalBatchCount == 0) {
    cout << endl;
    return;
  }

  float minF = minBatchTime.tv_usec;
  minF /= util::MICROSECOND;
  minF += minBatchTime.tv_sec;
  float maxF = maxBatchTime.tv_usec;
  maxF /= util::MICROSECOND;
  maxF += maxBatchTime.tv_sec;
  int avgPct = int (totalPercentOfOrig / totalBatchCount);

  cout << " TotRqs: " << totalRequestCount
       << " Min: " << setprecision(3) << minF
       << " Max: " << setprecision(3) << maxF
       << " Avg of Orig: " << avgPct << "%";

  if (totalBatchCount > 1) {		// Don't do average unless we have enough
    float secs = totalElapseTime.tv_usec;
    secs /= util::MICROSECOND;
    secs += totalElapseTime.tv_sec;
    secs /= totalBatchCount;
    cout << " Avg: " << setprecision(3) << secs;
  }

  float elapse = totalElapseTime.tv_usec;
  elapse /= util::MICROSECOND;
  elapse += totalElapseTime.tv_sec;
  cout << " Elapse: " << setprecision(3) << elapse << endl;
}

//////////////////////////////////////////////////////////////////////

int
main(int argc, char **argv)
{
  char	optionChar;
  const char* lookupMap = "";
  int	repeatCount = 1;
  int	delaySeconds = 1;
  int	minimumRequestsPerBatch = 1;
  bool	debugFlag = false;

  while ((optionChar = getopt(argc, argv, "dhc:i:L:m:")) != -1) {

    switch (optionChar) {

    case 'd':
      debugFlag = true;
      break;

    case 'c':
      repeatCount = atoi(optarg);
      break;

    case 'i':
      delaySeconds = atoi(optarg);
      break;

    case 'm':
      minimumRequestsPerBatch = atoi(optarg);
      break;

    case 'h':
      cout << usage;
      exit(0);

    case 'L':
      lookupMap = optarg;
      break;

    default:
      cerr << usage;
      exit(1);
    }
  }

  argc -= optind;
  argv += optind;

  if (argc > 0) {
    cerr << "Error: Too many command line parameters: " << *argv << " unexpected" << endl;
    cerr << usage;
    exit(1);
  }


  ////////////////////////////////////////
  // Slurp in batch of commands
  ////////////////////////////////////////

  int payloadSize = 1;

  vector<command*>	commandList;
  typedef vector<command*>::iterator	cLIter;

  while (!cin.eof()) {
    command* c = new command;
    cin >> c->serviceKey >> c->probability
	>> c->requestMinSize >> c->requestMaxSize
	>> c->cpuMinValue >> c->cpuMaxValue
	>> c->latencyMinValue >> c->latencyMaxValue;
    if (c->serviceKey.empty()) break;
    if (c->requestMaxSize > payloadSize) payloadSize = c->requestMaxSize;
    commandList.push_back(c);
  }

  cout << "Command List=" << commandList.size() << endl;

  for (cLIter cli=commandList.begin(); cli != commandList.end(); ++cli) {
    command* c = *cli;
    cout << "Cmd: "<< c->serviceKey << ": " << c->probability
	 << " Rsz=" << c->requestMinSize << "/" << c->requestMaxSize
	 << " CPU=" << c->cpuMinValue << "/" << c->cpuMaxValue
	 << " Latency=" << c->latencyMinValue << "/" << c->latencyMaxValue
	 << endl;
  }


  ////////////////////////////////////////
  // Construct the maximum request payload
  ////////////////////////////////////////

  char* payload = new char [payloadSize];
  char randomChar = (time(0) % 26) + 'A';
  memset(payload, randomChar, payloadSize);


  ////////////////////////////////////////
  // Connect to the service
  ////////////////////////////////////////

  pluton::client	C("plBatch");
  C.setDebug(debugFlag);

  if (!C.initialize(lookupMap)) {
    cerr << C.getFault().getMessage("plBatch could not initialize") << endl;
    exit(1);
  }

  srandom(getpid());

  ////////////////////////////////////////
  // The main loop to exchange requests
  ////////////////////////////////////////

  struct timeval startTime;
  struct timeval endTime;
  bool executeFailed = false;

  while (!executeFailed && (repeatCount-- > 0)) {

    rqCount = 0;
    int totalCPU = 0;
    int totalLatency = 0;
    int maxLatency = 0;

      for (cLIter cli=commandList.begin(); cli != commandList.end(); ++cli) {
	command* c = *cli;
	c->sentInBatch = false;
      }

    do { 
      for (cLIter cli=commandList.begin(); cli != commandList.end(); ++cli) {
	command* c = *cli;
	long r = random() % 100;

	if (r > (c->probability*100)) continue;
	if (c->sentInBatch) continue;
	c->sentInBatch = true;
	++rqCount;
	++totalRequestCount;
	c->R.reset();
	r %= 10;		// Get bottom digit
	++r;		// Now in range between 1-10
	c->requestData.assign(payload,
			      c->requestMaxSize - r * (c->requestMaxSize - c->requestMinSize) / 10);
	c->R.setRequestData(c->requestData);
	int cpu = c->cpuMaxValue - r * (c->cpuMaxValue - c->cpuMinValue) / 10;
	int latency = c->latencyMaxValue - r * (c->latencyMaxValue - c->latencyMinValue) / 10;
	if (latency < cpu) latency = cpu;
	ostringstream os;
	os << cpu;
	c->R.setContext("cpu", os.str());
	ostringstream os1;
	os1 << latency;
	c->R.setContext("latency", os1.str());
	if ((maxLatency == 0) || (latency > maxLatency)) maxLatency = latency;
	C.addRequest(c->serviceKey.c_str(), c->R);
	totalCPU += cpu;
	totalLatency += latency;
      }
    } while (rqCount < minimumRequestsPerBatch);

    gettimeofday(&startTime, 0);
    if (!C.executeAndWaitAll()) {
      cerr << "Error: execute failed: " << errno << endl;
      executeFailed = true;
    }
    gettimeofday(&endTime, 0);
    trackResults(startTime, endTime, totalLatency, totalCPU, maxLatency, true);

    ////////////////////////////////////////
    // Check the results of each request
    ////////////////////////////////////////

    int rqIndex=0;
    for (cLIter cli=commandList.begin(); cli != commandList.end(); ++cli) {
      command* c = *cli;
      if (!c->sentInBatch) continue;
      if (c->R.hasFault() || executeFailed) {
	std::string faultText;
	int faultCode = c->R.getFault(faultText);
	cerr << "Fault: index " << rqIndex << " Code: " << faultCode << ": " << faultText << endl;
	executeFailed = true;
      }
      if (!c->R.hasFault()) {
	string resp;
	c->R.getResponseData(resp);
	if (resp != c->requestData) {
	  cerr << "Mismatch" << endl;
	  cout << "Rq=" << c->requestData << " Res=" << resp << endl;
	  executeFailed = true;
	}
      }
      ++rqIndex;
    }
    long diff = util::timevalDiffMS(endTime, startTime);
    long sleepTime = delaySeconds * 1000 - diff;
    sleepTime += random() % 100;		// Drift by upto 1/10 of a second
    sleepTime *= 1000;	// MS to micros

    if (sleepTime > 0) usleep(sleepTime);
  }
  printResults();
}
