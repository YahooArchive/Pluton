#include <string>
#include <iostream>

#include <stdlib.h>

using namespace std;

#include "pluton/client.h"

// mutex tests were not comprehensive in clientWrapper

int
main(int argc, char** argv)
{
  const char* map = argv[1];
  pluton::client        C("t12345");

  cout << C.getAPIVersion() << endl;
  if (!C.initialize(map)) {
    cout << C.getFault().getMessage("Fatal: pluton::initialize() failed")  << endl;
    exit(1);
  }

  C.executeAndWaitSent();
  C.executeAndWaitSent();	// Pre-fix will assert() here

  pluton::clientRequest R1;
  C.executeAndWaitOne(R1);	// Generates fault

  pluton::clientRequest R2;
  C.executeAndWaitOne(R2);	// Pre-fix will assert() here

  return 0;
}
