#include <iostream>

using namespace std;

#include <stdlib.h>

#include "pluton/service.h"


int
main(int argc, char** argv)
{
  pluton::service S("ts");

  if (!S.initialize()) {
    cerr << S.getFault().getMessage("ts") << endl;
    exit(1);
  }

  while (S.getRequest()) {
    string req;
    string con;
    string key;
    cout << "SK = " << S.getServiceKey(key);
    S.getRequestData(req);
    S.getContext("context", con);
    cerr << "Mas Request: " << req << " C=" << con << endl;
    string sa;
    string sf;
    S.getServiceApplication(sa);
    S.getServiceFunction(sf);

    S.sendFault(1, "ERROR");
    S.sendResponse(sf);
  }

  if (S.hasFault()) cerr << "Error: " << S.getFault().getMessage() << endl;

  return 0;
}
