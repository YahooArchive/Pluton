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
