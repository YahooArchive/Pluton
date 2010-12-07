#include <iostream>

#include <stdlib.h>
#include <unistd.h>

#include "pluton/fault.h"
#include "pluton/service.h"
#include "pluton/client.h"

#include "global.h"
#include "version.h"

using namespace std;


static const char* usage =
"Usage: plVersion [-h]\n"
"\n"
"Print out Pluton versions and other static values\n"
"\n"
" -h   Print this usage message on STDOUT and exit(0)\n"
"\n"
"See also: http://localhost/docs/pluton/\n"
"\n";

//////////////////////////////////////////////////////////////////////

int
main(int argc, char **argv)
{
  char	optionChar;

  while ((optionChar = getopt(argc, argv, "h")) != -1) {
    switch (optionChar) {
    case 'h':
      cout << usage << version::rcsid << endl;
      exit(0);

    default:
      cerr << usage << version::rcsid << endl;
      exit(1);
    }
  }

  string manager = version::rcsid;
  string client = pluton::client::getAPIVersion();
  string service = pluton::service::getAPIVersion();

  cout << "plutonManager: " << manager << endl;
  cout << "clientAPI: " << client << endl;
  cout << "serviceAPI: " << service << endl;
  cout << "Lookup Map: " << pluton::lookupMapPath << endl;
  cout << "shmLookupMapVersion: " << plutonGlobal::shmLookupMapVersion << endl;
  cout << "shmServiceVersion: " << plutonGlobal::shmServiceVersion << endl;

  if (client != manager) cerr << "Warning: Client version does not match plutonManager" << endl;
  if (service != manager) cerr << "Warning: Service version does not match plutonManager" << endl;
  if (client != service) cerr << "Warning: Client version does not match Service" << endl;

  return(0);
}
