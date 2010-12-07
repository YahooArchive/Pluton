#include <iostream>

#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <st.h>

#include "debug.h"
#include "logging.h"
#include "util.h"
#include "service.h"
#include "manager.h"
#include "version.h"

using namespace std;


static const char* usage =
"\n"
"Usage: plutonManager [-ho]\n"
"                     [-C configurationDirectory] [-L lookupMap]\n"
"                     [-R rendezvousDirectory]\n"
"                     [-c commandInterface:commandPort]\n"
"                     [-e emergencyExitDelay]\n"
"                     [-d debugOptions] [-g defaultGID] [-l loggingOptions]\n"
"                     [-s statisticsInterval] [-u defaultUID] [-z st_stack_size]\n"
"\n"
"The plutonManager manages service processes based on configuration files.\n"
"\n"
"Where:\n"
"\n"
" -h   Print this usage message on STDOUT and exit(0)\n"
" -o   Turn off output sync between iostreams and stdio\n"
"\n"
" -C   Directory containing service configuration files (default: '.')\n"
" -L   Name of shared memory lookup file (default: './lookup.map')\n"
" -R   Directory where sockets and shared memory files are created (default: '.')\n"
"\n"
" -c   Defines the interface and port for the command line interface.\n"
" -e   Seconds after the start of an orderly shutdown prior to commencing\n"
"      an emergency (and thus disorderly) shutdown. (default: 30)\n"
" -d   Debug options: all, child, manager, service, process, thread,\n"
"       stio, config, reportingChannel and calibrate (default: none)\n"
" -g   setegid services to this gid - only valid when run as root.\n"
"      (default: -1 inherit manager euid)\n"
" -l   Logging option: all, process, service,\n"
"       processStart, processLog, processExit and processUsage,\n"
"       serviceConfig, serviceStart and serviceExit and\n"
"       calibrate (default: none)\n"
" -s   Seconds between periodic statistics log entries (default: 600)\n"
" -u   seteuid services to this uid - only valid when run as root.\n"
"      (default: -1 inherit manager egid)\n"
" -z   Size of the stack for each state-thread (default: 0)\n"
"\n"
"See also: http://localhost/docs/pluton/\n"
"\n";



////////////////////////////////////////////////////////////////////////////////
// Signals offer a small amount of control over the server. Currently
// they are undifferentiated, but they might be useful later. Signal
// handlers have to be very careful not to touch any internal state,
// either the Manager's or folks like state threads as they can be
// completely indeterminant. Consequently, all of these handlers
// simple set a safe flag and return.
////////////////////////////////////////////////////////////////////////////////

static manager* staticPlutonD = 0;

static void catchTERM(int sig) { staticPlutonD->setQuitMessage("Caught SIGTERM"); }
static void catchURG(int sig) { staticPlutonD->setQuitMessage("Caught SIGURG"); }
static void catchUSR1(int sig) { staticPlutonD->setQuitMessage("Caught SIGUSR1"); }
static void catchUSR2(int sig) { staticPlutonD->setQuitMessage("Caught SIGUSR2"); }

static void catchINT(int sig)
{
  staticPlutonD->setConfigurationReload(true, 0);	// Reload everything
}

static void catchHUP(int sig)
{
  staticPlutonD->setConfigurationReload(true, st_time());	// Reload changed
}

static void
catchCHLD(int sig)
{
  staticPlutonD->setReapChildren(true);	// Could do the self-pipe trick to notify the manager
}

static void
catchALRM(int sig)
{
  static char alarmMessage[] = "--\nEmergency Exit Delay limit reached\n";
  write(1, alarmMessage, sizeof(alarmMessage)-1);
  write(2, alarmMessage, sizeof(alarmMessage)-1);
  exit(2);
}


static void
initializeSignalHandlers(manager* M)
{

  staticPlutonD = M;

  signal(SIGTERM, catchTERM); siginterrupt(SIGTERM, true);
  signal(SIGINT, catchINT); siginterrupt(SIGINT, true);
  signal(SIGURG, catchURG); siginterrupt(SIGURG, true);

  signal(SIGUSR1, catchUSR1); siginterrupt(SIGUSR1, true);
  signal(SIGUSR2, catchUSR2); siginterrupt(SIGUSR2, true);

  signal(SIGCHLD, catchCHLD); siginterrupt(SIGCHLD, true);
  signal(SIGPIPE, SIG_IGN);

  signal(SIGHUP, catchHUP); siginterrupt(SIGHUP, true);
  signal(SIGALRM, catchALRM); siginterrupt(SIGALRM, true);
}


static void
die(string reason)
{
  LOGPRT << "FATAL: " << reason << endl;
  LOGPRT << "FATAL: " << reason << endl;
  exit(1);
}


////////////////////////////////////////////////////////////////////////////////

int
main(int argc, char **argv)
{
  manager	M;

  M.parseCommandLineOptions(argc, argv);
  initializeSignalHandlers(&M);

  if (st_init() == -1) die("State Threads: st_init() failed");
  st_timecache_set(1);

  M.initialize();
  if (M.error()) die(M.getErrorMessage());
  std::string em;
  if (!M.initializeCommandPort(em)) {
    LOGPRT << "Warning: Cannot open Command Port: " << em << endl;
  }
  if (!M.loadConfigurations(0)) die("Cannot continue without a configuration");

  M.preRun();

  LOGPRT << "Manager ready: " << version::rcsid << endl;
  while (M.run()) ;

  M.initiateShutdownSequence(M.getQuitMessage());

  LOGPRT << "Manager orderly shutdown limit: " << M.getEmergencyExitDelay() << endl;
  alarm(M.getEmergencyExitDelay());	// Make sure manager emergency exits within limits

  M.runUntilIdle();			// Don't exit until all services have exited

  M.completeShutdownSequence();

  LOGPRT << "Manager exiting: " << version::rcsid << endl;

  return 0;	// return rather than exit so that ~M is called
}


//////////////////////////////////////////////////////////////////////
// Do the getopt() jig and then do any post-parsing checking.
//////////////////////////////////////////////////////////////////////

void
manager::parseCommandLineOptions(int argc, char **argv)
{
  char		optionChar;

  while ((optionChar = getopt(argc, argv, "hoC:L:R:c:d:e:l:s:u:z:")) != -1) {

    switch (optionChar) {

    case 'h':
      cout << usage << version::rcsid << endl;
      exit(0);

    case 'o':
      std::ios::sync_with_stdio(false);
      LOGPRT << "Option: sync_with_stdio(false) (iff this message is first)" << endl;
      break;

    case 'C':
      _configurationDirectory = optarg;
      LOGPRT << "Option: Configuration Directory=" << _configurationDirectory << endl;
      break;

    case 'L':
      _lookupMapFile = optarg;
      LOGPRT << "Option: LookupMap Path=" << _lookupMapFile << endl;
      break;

    case 'R':
      _rendezvousDirectory = optarg;
      LOGPRT << "Option: Rendezvous Directory=" << _rendezvousDirectory << endl;
      break;

    case 'c':
      _commandPortInterface = optarg;
      LOGPRT << "Option: Command Port Interface: " << _commandPortInterface << endl;
      break;

    case 'd':
      LOGPRT << "Option: debug=" << optarg << endl;
      if (!debug::setFlag(optarg)) {
	cerr << "Error: Unrecognized debug option: " << optarg << endl;
        cerr << usage << version::rcsid << endl;
	die("Invoked with bad parameter");
      }
      break;

    case 'e':
      _emergencyExitDelay = atoi(optarg);
      if (_emergencyExitDelay < 1) {
	cerr << "Error: Emergency Exit Delay is too small: " << optarg << endl;
	cerr << usage << version::rcsid << endl;
        die("Invoked with bad parameter");
      }
      LOGPRT << "Option: Emergency Shutdown Delay=" << optarg << endl;
      break;

    case 'g':
      if (geteuid() != 0) {
	cerr << "Error: default gid can only be set when running as root" << endl;
	cerr << usage << version::rcsid << endl;
	die("Invoked root-only option as non-root");
      }
      _defaultGID = atoi(optarg);
      if (_defaultGID < 1) {
	cerr << "Error: default gid is too small: " << optarg << endl;
	cerr << usage << version::rcsid << endl;
	die("Invoked with bad parameter");
      }
      LOGPRT << "Option: default gid=" << _defaultGID << "s" << endl;
      break;

    case 'l':
      LOGPRT << "Option: logging=" << optarg << endl;
      if (!logging::setFlag(optarg)) {
	cerr << "Error: Unrecognized logging option: " << optarg << endl;
	cerr << usage << version::rcsid << endl;
	die("Invoked with bad parameter");
      }
      break;

    case 's':
      _statisticsLogInterval = atoi(optarg);
      if (_statisticsLogInterval < 1) {
	cerr << "Error: statistics log interval is too small: " << optarg << endl;
        cerr << usage << version::rcsid << endl;
	die("Invoked with bad parameter");
      }
      LOGPRT << "Option: statistics log interval=" << _statisticsLogInterval << "s" << endl;
      break;

    case 'u':
      if (geteuid() != 0) {
	cerr << "Error: default uid can only be set when running as root" << endl;
	cerr << usage << version::rcsid << endl;
	die("Invoked root-only option as non-root");
      }
      _defaultUID = atoi(optarg);
      if (_defaultUID < 1) {
	cerr << "Error: default uid is too small: " << optarg << endl;
	cerr << usage << version::rcsid << endl;
	die("Invoked with bad parameter");
      }
      LOGPRT << "Option: default uid=" << _defaultUID << "s" << endl;
      break;

    case 'z':
      _stStackSize = atoi(optarg);
      LOGPRT << "Option: state-thread stack size=" << _stStackSize << endl;
      break;

    default:
      cerr << usage << version::rcsid << endl;
      die("Invoked with invalid options");
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 0) die("Extraneous parameters on command line");

  if (geteuid() == 0) {
    if (_defaultUID < 1) {
      cerr << "Error: Must provide a default uid with -u when running seteuid() root" << endl;
      cerr << usage << version::rcsid << endl;
      die("Invoked without mandatory parameter");
    }
 
    if (_defaultGID < 1) {
      cerr << "Error: Must provide a default gid with -g when running seteuid() root" << endl;
      cerr << usage << version::rcsid << endl;
      die("Invoked without mandatory parameter");
    }
  }
}
