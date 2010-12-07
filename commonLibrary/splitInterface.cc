#include <string>

#include <stdlib.h>
#include <string.h>

#include "util.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////
// Split a host:port string into component parts.
//
// Valid combinations are:
// *empty string*		defaultInterface:defaultPort
// Ipaddress:Port		Ipaddress:Port
// IPaddress			IPaddress:defaultPort
// IPaddress:			IPaddress:defaultPort
// :Port			defaultInterface:Port
// :				defaultInterface:DefaultPort
////////////////////////////////////////////////////////////////////////////////


void
util::splitInterface(const char* inputstr, string& interface, int& port,
		     const char* defaultInterface, int defaultPort)
{
  interface = defaultInterface;
  port = defaultPort;
  if (!*inputstr) return;

  const char* colon = strchr(inputstr, ':');
  if (colon) {
    if (colon != inputstr) interface.assign(inputstr, colon - inputstr);
    ++colon;	// Skip past :
    if (*colon) port = atoi(colon);
  }
  else {
    interface = inputstr;
  }
}
