//////////////////////////////////////////////////////////////////////
// A general purpose program to exchange a single request with a
// service. Good for testing/debugging a service.
//////////////////////////////////////////////////////////////////////

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

#include <sys/time.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"
#include "pluton/client.h"

using namespace std;


static const char* usage =
"Usage: plSend [-edhno] [-L lookupMap] [-C name=context .. ]\n"
"              [-t timeoutSeconds] ServiceKey [requestString]\n"
"\n"
"Exchange a single request with a pluton service. Print the resulting data\n"
"to STDOUT and exit(0) on a successful request, otherwise print the fault\n"
"information to STDERR and exit(1).\n"
"\n"
" -C   Set the context for this request - multiple occurrences ok\n"
" -d   Turn on pluton::client debugging\n"
" -e   Print elapse time of the request to STDERR\n"
" -h   Print this usage message on STDOUT and exit(0)\n"
" -L   The Lookup Map used to connect with the manager (default: '')\n"
" -n   Append a Newline to the output\n"
" -o   Oneshot. Set the 'noRetryAttr' attribute\n"
" -t   Change the request timeout to 'timeoutSeconds'\n"
"\n"
" ServiceKey: name of service to request (default: system.echo.0.raw)\n"
"\n"
" requestString: if present, place this in the requestData. If not present\n"
" read STDIN for the requestData.\n"
"\n"
"Examples:\n"
"\tplSend -C echo.sleepMS=500 system.echo.0.raw ''\t\t# Sleep for 500ms\n"
"\tcat listfolderrequest | plSend Mail.Folder..XML >results.data\n"
"\n"
"See also: http://localhost/docs/pluton/\n"
"\n";


//////////////////////////////////////////////////////////////////////
// Split out the key=value tokens from the context string and and them
// into the request.
//////////////////////////////////////////////////////////////////////

static bool
addContext(pluton::clientRequest& R, const char* context)
{
  char* dupe = strdup(context);
  char** dupeP = &dupe;
  const char* key = strsep(dupeP, "=");
  if (key == dupe) {
    cerr << "Error: Context string needs an '=' character" << endl;
    return false;
  }

  if (!R.setContext(key, string(*dupeP))) {
    cerr << "Error: plSend Fault: " << R.getFaultCode() << ":" << R.getFaultText() << endl;
    return false;
  }

  return true;
}

//////////////////////////////////////////////////////////////////////

int
main(int argc, char **argv)
{
  char optionChar;
  bool debugFlag = false;
  bool newlineFlag = false;
  bool elapseTimeFlag = false;
  bool oneShotFlag = false;
  const char* lookupMap = "";
  int timeoutSeconds = -1;

  pluton::clientRequest R;

  while ((optionChar = getopt(argc, argv, "hC:deL:not:")) != -1) {

    switch (optionChar) {

    case 'C':
      if (!addContext(R, optarg)) {
	cerr << endl << usage;
	exit(1);
      }
      break;

    case 'd': debugFlag = true; break;

    case 'e': elapseTimeFlag = true; break;

    case 'h':
      cout << usage;
      exit(0);

    case 'L': lookupMap = optarg; break;

    case 'n': newlineFlag = true; break;

    case 'o': oneShotFlag = true; break;

    case 't':
      timeoutSeconds = atoi(optarg);
      if (timeoutSeconds <= 0) {
	cerr << "Error: timeout seconds must be a positive integer" << endl;
	cerr << endl << usage;
	exit(1);
      }
      break;

    default:
      cerr << endl << usage;
      exit(1);
    }
  }

  argc -= optind;
  argv += optind;

  if (argc < 1) {
    cerr << "Error: Must supply ServiceKey on the command line" << endl;
    cerr << endl << usage;
    exit(1);
  }
  char* serviceKey = *argv;
  argc--;
  argv++;

  if (argc > 1) {
    cerr << "Error: Too many arguments on the command line" << endl;
    cerr << endl << usage;
    exit(1);
  }

  // If a string is present, use that as the requestStr, otherwise
  // slurp in all of STDIN and use that.

  string requestStr;
  if (argc == 1) {
    requestStr.assign(*argv);
    argc--;
    argv++;
  }
  else {
    stringstream os;
    os << cin.rdbuf();
    requestStr = os.str();
  }

  pluton::client C("plSend");
  C.setDebug(debugFlag);
  if (timeoutSeconds > 0) C.setTimeoutMilliSeconds(timeoutSeconds * 1000);

  if (!C.initialize(lookupMap)) {
    cerr << C.getFault().getMessage("Error: plSend initialize: ", true) << endl;
    exit(1);
  }

  //////////////////////////////////////////////////////////////////////
  // Construct the rest of the request (context is done at getopt()
  //////////////////////////////////////////////////////////////////////

  if (oneShotFlag)  R.setAttribute(pluton::noRetryAttr);
  R.setRequestData(requestStr.data(), requestStr.length());

  if (!C.addRequest(serviceKey, R)) {
    cerr << C.getFault().getMessage("Error: plSend addRequest: ", true) << endl;
    exit(1);
  }

  struct timeval startTime;
  gettimeofday(&startTime, 0);

  int res = C.executeAndWaitAll();

  struct timeval endTime;
  gettimeofday(&endTime, 0);

  if (res == 0) {
    cerr << "Error: plSend timed out" << endl;
    exit(1);
  }

  if (res < 0) {
    cerr << C.getFault().getMessage("Error: plSend executeAndWaitAll: ", true) << endl;
    exit(1);
  }

  if (R.hasFault()) {
    cerr << "Error: plSend Fault: " << R.getFaultCode() << ":" << R.getFaultText() << endl;
  }
  else {
    std::string r;
    R.getResponseData(r);
    cout << r;
    if (newlineFlag) cout << endl;
  }

  if (elapseTimeFlag) {
    timeval diff;
    util::timevalDiff(diff, endTime, startTime);
    cerr << "Elapsed Time: " << diff.tv_sec << "." << setfill('0') << setw(6) << diff.tv_usec << endl;
  }

  return(R.hasFault() ? 1 : 0);
}
