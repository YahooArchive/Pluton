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

#ifndef _SERVICECONFIG_H
#define _SERVICECONFIG_H

#include <string>

class serviceConfig {
 public:
  serviceConfig();

  bool enableLeakRampingFlag;
  bool prestartProcessesFlag;	// Start up the minimum services immediately

  long affinityTimeout;		// An idle affinity connection is closed after this many seconds
  long execFailureBackoff;	// Seconds to delay restarted a failed service when exec fails
  long idleTimeout;		// > MinimumProcesses are removed after this many seconds
  long maximumRequests;		// Shutdown after this many requests
  long maximumRetries;		// Retry a failed request this many times
  long maximumProcesses;	// How many instances to run simultaneously
  long minimumProcesses;	// Keep this many service instances running once active
  long maximumThreads;

  long occupancyPercent;	// Desired percentage of busy processes

  long recorderCycle;		// Number of recorder files to cycle through

  long ulimitCPUMilliSeconds;	// Amortized per request
  long ulimitOpenFiles;
  long ulimitDATAMemory;

  long unresponsiveTimeout;	// Presume permanently stalled after this time

  std::string cd;
  std::string exec;
  std::string recorderPrefix;

 private:
  serviceConfig&	operator=(const serviceConfig& rhs);	// Assign not ok
  serviceConfig(const serviceConfig& rhs);			// Copy not ok
};

#endif
