#include <iostream>
#include <string>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "pluton/fault.h"

#include "util.h"
#include "global.h"
#include "serviceKey.h"
#include "shmLookup.h"

using namespace std;


static const char* usage =
"Usage: plLookup [-dhq] [-t timeout] [-L lookupMap] [ServiceKeys...]\n"
"\n"
"Lookup each Service Key and return the path of the Rendezvous Socket\n"
"\n"
"Where:\n"
" -d   Turn on debug output\n"
" -h   Print this usage message on STDOUT and exit(0)\n"
" -q   Quiet Mode. Do not output the path.\n"
" -L   The Lookup Map used to connect with the manager (default: '')\n"
" -t   Numbers of seconds to keep looking if lookup fails (default: 0)\n"
"       The intent of this option is to have plLookup wait until a\n"
"       service configuration has been loaded by the Manager.\n"
"\n"
"See also: http://localhost/docs/pluton/\n"
"\n";

//////////////////////////////////////////////////////////////////////

int
main(int argc, char **argv)
{
  char	optionChar;

  bool	debugFlag = false;
  int	timeoutSeconds = 0;
  bool	quietMode = false;
  const char*	lookupPath = "";

  while ((optionChar = getopt(argc, argv, "dhqL:t:")) != -1) {
    switch (optionChar) {

    case 'd': debugFlag = true; break;

    case 'q': quietMode = true; break;

    case 'L': lookupPath = optarg; break;

    case 't':
      timeoutSeconds = atoi(optarg);
      if (timeoutSeconds <= 0) {
	cerr << "Error: Timeout must be greater than zero" << endl;
	cerr << usage;
	exit(1);
      }
      break;

    case 'h':
      cout << usage;
      exit(0);

    default:
      cerr << usage;
      exit(1);
    }
  }

  // Use the same logic as clientImpl to determine lookup path

  if (!*lookupPath) {
    const char* envPath = getenv("plutonLookupMap");
    if (envPath) {
      lookupPath = envPath;
    }
    else {
      lookupPath = pluton::lookupMapPath;
    }
  }

  if (!quietMode) cout << "Lookup Path: " << lookupPath << endl;

  argc -= optind;
  argv += optind;

  if (argc == 0) exit(0);	// That was easy...

  shmLookup	lookupMap;


  int checkingCount = argc;
  int retryCount = timeoutSeconds * 10;

  while (true) {

    if (debugFlag) cout << "Top of Loop checking=" << checkingCount << endl;

    pluton::faultCode fc;
    int res = lookupMap.mapReader(lookupPath, fc);
    if (res <= 0) {
      cout << "Error: map of " << lookupPath << " failed: " << fc << endl;
      exit(1);
    }

    for (int ix=0; ix < argc; ++ix) {
      if (!argv[ix]) continue;
      pluton::serviceKey SK;
      if (debugFlag) cout << "Checking: " << ix << " " << argv[ix] << endl;

      const char* err = SK.parse(argv[ix], strlen(argv[ix]));
      if (err) {
	cout << "Bad Service Key: " << argv[ix] << ": " << err << endl;
	argv[ix] = 0;	// Don't check again
	--checkingCount;
	continue;
      }

      string rendezvousPath;
      string sk;
      SK.getSearchKey(sk);
      res = lookupMap.findService(sk.data(), sk.length(), rendezvousPath, fc);
      if (res > 0) {
	if (!quietMode) cout << argv[ix] << "=" << sk << "=" << rendezvousPath << endl;
	argv[ix] = 0;	// Don't check again
	--checkingCount;
      }
    }

    if (retryCount-- == 0) break;
    if (checkingCount == 0) break;

    usleep(util::MICROSECOND / 10);		// 1/10th of a second
  }

  if (checkingCount == 0) return(0);

  // Dump out unresolved keys

  if (quietMode) return(1);

  for (int ix=0; ix < argc; ++ix) {
    if (argv[ix]) cout << "Not Found: " << argv[ix] << endl;
  }

  return(1);
}
