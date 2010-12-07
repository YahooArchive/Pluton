#include <stdlib.h>

#include <string>
#include <iostream>

using namespace std;

#include "pluton/client.h"


int
main(int argc, char** argv)
{
  pluton::client        C("tc");
  pluton::clientRequest R1;
  pluton::clientRequest R2;
  pluton::clientRequest R3;

  cout << C.getAPIVersion() << endl;
  if (!C.initialize("./lookup.map")) {
    cout << C.getFault().getMessage("Fatal: pluton::initialize() failed")  << endl;
    exit(1);
  }

  R1.setRequestData("This is the first request");
  R2.setRequestData("This is the second request");
  R3.setRequestData("This is the third");

  C.setTimeoutMilliSeconds(2000);

  C.addRequest("Mail.getFolder.XML.1", &R1);
  C.addRequest("AB.addEvent.PHP.0", &R2);
  C.addRequest("AB.addEvent.PHP0", &R3);
  if (C.executeAndWaitAll() <= 0) {
    cout << "execute failed" << endl;
  }

  cout << "R1 Fault Code=" << R1.getFaultCode() << " Text=" << R1.getFaultText() << endl;
  cout << "R2 Fault Code=" << R2.getFaultCode() << " Text=" << R2.getFaultText() << endl;
  cout << "R3 Fault Code=" << R3.getFaultCode() << " Text=" << R3.getFaultText() << endl;

  string s;

  cin >> s;             // Block until input

  return 0;
}
