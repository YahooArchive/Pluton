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
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>

#include <dirent.h>

#include "util.h"
#include "debug.h"
#include "logging.h"
#include "serviceKey.h"
#include "configParser.h"
#include "service.h"
#include "manager.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// This is a convenient place for the constructor
//////////////////////////////////////////////////////////////////////

serviceConfig::serviceConfig() :
  enableLeakRampingFlag(true),
  prestartProcessesFlag(false),

  affinityTimeout(60),
  execFailureBackoff(60),
  idleTimeout(120),
  maximumRequests(10000),
  maximumRetries(1),
  maximumProcesses(10),
  minimumProcesses(1),
  maximumThreads(1),
  occupancyPercent(70),
  recorderCycle(0),
  ulimitCPUMilliSeconds(0),
  ulimitOpenFiles(0),
  ulimitDATAMemory(0),
  unresponsiveTimeout(10)
{
}


//////////////////////////////////////////////////////////////////////////
// Scan the config directory for valid names and load them in. If
// there are any changes, then a new serviceMap is built to replace
// the old one.
//
// For any given service, one of four things can occur:
//
// 1) New: create a service and add into the map
// 2) Deleted: do not add into the new map, initiate shutdown
// 3) Changed: create new service and add into new map
// 4) Unchanged: transfer to new map
//
// Once the new map is built, the residual entries in the old map are
// shutdown and deleted.
//
// Return false if load fails catastrophically.
//////////////////////////////////////////////////////////////////////////

bool
manager::loadConfigurations(time_t loadSince)
{
  if (logging::serviceConfig()) LOGPRT << "Config Scan Start: " << _configurationDirectory << endl;

  DIR* D = opendir(_configurationDirectory);
  if (!D) {
    string em;
    util::messageWithErrno(em, "Config Error: Cannot open service directory",
			   _configurationDirectory);
    LOGPRT << em << endl;
    return false;
  }

  serviceMapType* oldMap = _serviceMap;		// Anticipate a new map
  _serviceMap = new serviceMapType;

  int newServiceCount = 0;
  int retainedServiceCount = 0;
  int replacedServiceCount = 0;

  struct dirent* dent;
  while((dent = readdir(D))) {
    string dname;

    // Unified Unix Bwaaaahahaha (DT_UNKNOWN usually means NFS)

#if defined(DT_REG) && defined(DT_LNK) && defined(DT_UNKNOWN)
    if ((dent->d_type != DT_REG) && (dent->d_type != DT_LNK) && (dent->d_type != DT_UNKNOWN)) {
      continue;
    }
#endif

    if (dent->d_type == DT_DIR) continue;

#ifdef __linux__
    dname.assign(dent->d_name);                 // They promise \0 termination
#else
    dname.assign(dent->d_name, dent->d_namlen); // but others define length
#endif

    //////////////////////////////////////////////////////////////////////
    // Only load filenames that have a valid service key syntax. Don't
    // complain too loudly if there are invalid names, it might be a
    // README or a renamed service.
    //////////////////////////////////////////////////////////////////////

    if ((dname == ".") || (dname == "..")) continue;

    pluton::serviceKey SK;
    const char* err = SK.parse(dname, false);
    if (err) {
      if (logging::serviceConfig()) LOGPRT << "Config Warning: Ignoring file '"
					   << dname << "' - " << err << endl;
      continue;
    }

    //////////////////////////////////////////////////////////////////////
    // Find out some details about this service to see whether it's
    // new, updated or unchanged.
    //////////////////////////////////////////////////////////////////////

    string path = _configurationDirectory;
    path += "/";
    path += dname;
    struct stat sb;
    int res = stat(path.c_str(), &sb);
    if (res == -1) {
      if (logging::serviceConfig()) {
	LOGPRT << "Config Warning: Cannot stat '" << path << "'" << endl;
      }
      continue;
    }

    service* oldSM = 0;
    serviceMapIter mi = oldMap->find(dname);
    if (mi != _serviceMap->end()) oldSM = mi->second;

    if (!oldSM) {
      if (logging::serviceConfig()) LOGPRT << "Config Loading new: " << dname << endl;
      service* newSM = createService(dname, path, sb);
      if (newSM) {
	(*_serviceMap)[dname] = newSM;
	++newServiceCount;
      }
      continue;
    }

    //////////////////////////////////////////////////////////////////////
    // If the file hasn't changed in any obvious way, then retain this
    // service. This is the only way an existing service survives
    // across a configuration reload.
    //////////////////////////////////////////////////////////////////////

    if ((sb.st_ino == oldSM->getCFInode()) &&
	(sb.st_size == oldSM->getCFSize()) &&
	(sb.st_mtime == oldSM->getCFMtime()) &&
	(sb.st_mtime < loadSince)) {
      oldMap->erase(dname);
      (*_serviceMap)[dname] = oldSM;
      if (debug::config()) DBGPRT << "Config Retaining: " << dname << endl;
      ++retainedServiceCount;
      continue;
    }


    //////////////////////////////////////////////////////////////////////
    // The configuration file has changed, shutdown the existing
    // service and start a new one.
    //////////////////////////////////////////////////////////////////////

    if (logging::serviceConfig()) LOGPRT << "Config Loading changed: " << dname << endl;
    service* newSM = createService(dname, path, sb);
    if (newSM) {
      oldMap->erase(dname);
      (*_serviceMap)[dname] = newSM;
      ++replacedServiceCount;
      oldSM->initiateShutdownSequence("config changed");	// Shutdown after starting new
    }
  }

  closedir(D);

  //////////////////////////////////////////////////////////////////////
  // All done with scanning. If any changes were detected, rebuild and
  // remap the share memory lookup table.
  //////////////////////////////////////////////////////////////////////

  int deletedServiceCount = oldMap->size();
  if (newServiceCount || deletedServiceCount || (loadSince == 0)) {
    const char* err = rebuildLookup();
    if (err) {
      string em;
      string msg = "Config Error: Cannot rebuild LookupMap: ";
      msg += err;
      util::messageWithErrno(em, msg.c_str(), _lookupMapFile);
      LOGPRT << em << endl;
      return false;
    }
  }

  //////////////////////////////////////////////////////////////////////
  // Finally, any services left in the old map must be shut down
  //////////////////////////////////////////////////////////////////////

  for (serviceMapIter mi=oldMap->begin(); mi != oldMap->end(); ++mi) {
    if (logging::serviceConfig()) LOGPRT << "Config Removing: " << mi->second->getName() << endl;
    mi->second->initiateShutdownSequence("config deleted");
  }

  delete oldMap;

  if (logging::serviceConfig()) LOGPRT << "Config Scan Complete:"
				       << " New=" << newServiceCount
				       << " Retained=" << retainedServiceCount
				       << " Replaced=" << replacedServiceCount
				       << endl;

  return true;
}


//////////////////////////////////////////////////////////////////////
// Get a config number and check that it's in range. Return true on
// error with errormessage.
//////////////////////////////////////////////////////////////////////

static	bool
getNumber(configParser& conf, const char* param,
	  long defaultVal, long& val, long lowerLimit, long upperLimit, string& em)
{
  if (conf.getNumber(param, defaultVal, val, em)) return true;

  if ((lowerLimit != -1) && (val < lowerLimit)) {
    ostringstream os;
    os << "Config Error: " << param << "=" << val
       << " but it must not be less than lower limit " << lowerLimit;
    em = os.str();
    return true;
  }

  if ((upperLimit != -1) && (val > upperLimit)) {
    ostringstream os;
    os << "Config Error: " << param << "=" << val
       << " but it must not be greater than upper limit " << upperLimit;
    em = os.str();
    return true;
  }

  return false;
}


//////////////////////////////////////////////////////////////////////
// Given a config file, create and start a new service.
//////////////////////////////////////////////////////////////////////

service*
manager::createService(const std::string& name, const std::string& path, struct stat& sb)
{
  service* S = new service(name, this);

  S->setConfigurationPath(path);
  S->setRendezvousDirectory(_rendezvousDirectory);
  S->setCFInode(sb.st_ino);
  S->setCFSize(sb.st_size);
  S->setCFMtime(sb.st_mtime);

  if (!S->initialize()) {		// Creates the Rendezvous Socket and thread
    LOGPRT << "Service Error: " << S->getLogID() << " Initialize failed: "
	   << S->getErrorMessage() << endl;
    delete S;
    return 0;
  }

  ++_serviceCount;

  return S;
}


//////////////////////////////////////////////////////////////////////
// Given a string of config data, parse and set up the config
// parameters for this service.
//////////////////////////////////////////////////////////////////////

bool
service::loadConfiguration(const std::string& path)
{
  configParser	C;

  if (C.loadFile(path, _errorMessage)) return false;

  // Transfer to internal variables, checking as we go.

  if (getNumber(C, "affinity-timeout", _config.affinityTimeout,
		_config.affinityTimeout, 10, 600, _errorMessage)) return false;

  if (C.getString("cd", _config.cd,
		  _config.cd, _errorMessage)) return false;

  if (C.getBool("enable-leak-ramping", _config.enableLeakRampingFlag,
		_config.enableLeakRampingFlag, _errorMessage)) return false;

  if (C.getString("exec", _config.exec,
		  _config.exec, _errorMessage)) return false;

  if (getNumber(C, "exec-failure-backoff", _config.execFailureBackoff,
		_config.execFailureBackoff, 0, -1, _errorMessage)) return false;

  if (getNumber(C, "idle-timeout", _config.idleTimeout,
		_config.idleTimeout, 10, -1, _errorMessage)) return false;

  if (getNumber(C, "maximum-processes", _config.maximumProcesses,
		_config.maximumProcesses, 1, 10000, _errorMessage)) return false;

  if (getNumber(C, "maximum-requests", _config.maximumRequests,
		_config.maximumRequests, 1, -1, _errorMessage)) return false;

  if (getNumber(C, "maximum-retries", _config.maximumRetries,
		_config.maximumRetries, 0, 10, _errorMessage)) return false;

  if (getNumber(C, "maximum-threads", _config.maximumThreads,
		_config.maximumThreads, 1, 10000, _errorMessage)) return false;

  if (getNumber(C, "minimum-processes", _config.minimumProcesses,
		_config.minimumProcesses, 0, -1, _errorMessage)) return false;

  if (getNumber(C, "occupancy-percent", _config.occupancyPercent,
		_config.occupancyPercent, 50, 100, _errorMessage)) return false;

  if (C.getBool("prestart-processes", _config.prestartProcessesFlag,
		_config.prestartProcessesFlag, _errorMessage)) return false;

  if (getNumber(C, "recorder-cycle", _config.recorderCycle,
		_config.recorderCycle, 0, -1, _errorMessage)) return false;

  if (C.getString("recorder-prefix", _config.recorderPrefix,
		_config.recorderPrefix, _errorMessage)) return false;

  if (getNumber(C, "ulimit-cpu-milliseconds", _config.ulimitCPUMilliSeconds,
		_config.ulimitCPUMilliSeconds, 0, -1, _errorMessage)) return false;

  if (getNumber(C, "ulimit-open-files", _config.ulimitOpenFiles,
		_config.ulimitOpenFiles, 0, 1000, _errorMessage)) return false;

  if (getNumber(C, "ulimit-data-memory", _config.ulimitDATAMemory,
		_config.ulimitDATAMemory, 0, -1, _errorMessage)) return false;

  if (getNumber(C, "unresponsive-timeout", _config.unresponsiveTimeout,
		_config.unresponsiveTimeout, 5, 600, _errorMessage)) return false;


  //////////////////////////////////////////////////////////////////////
  // By allowing a noop exit config parameter, the config file can
  // look like a shell script, smell like a shell script and walk like
  // a shell script. Ergo, it's a duck!
  //
  // #! /bin/sh
  // cd whereever
  // exec program args
  // exit 1
  // pararms
  //////////////////////////////////////////////////////////////////////

  long ignoreExit = 0;
  string ignoreError;
  getNumber(C, "exit", ignoreExit, ignoreExit, 0, -1, ignoreError);

  // Make sure there are no mis-spellings in the config

  string unknown = C.getFirstUnfetched();
  if (!unknown.empty()) {
    _errorMessage = "Config Error: Unknown configuration parameter: ";
    _errorMessage += unknown;
    return false;
  }

  // Check relationship between config values and do final fixups

  if ((_config.minimumProcesses > _config.maximumProcesses) && C.found("minimum-processes")) {
    _errorMessage = "Config Error: minimum-processes must not be greater than maximum-processes";
    return false;
  }

  if (_config.exec.empty()) {
    _errorMessage = "Config Error: Must include an 'exec' parameter";
    return false;
  }

  if (_config.execFailureBackoff == 0) {
    if (logging::serviceConfig()) {
	LOGPRT << "Config Warning: Zero exec-failure-backoff can be deleterious to your system"
	       << endl;
    }
  }

  return true;
}
