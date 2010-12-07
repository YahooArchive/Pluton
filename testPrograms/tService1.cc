#include <iostream>
#include <string>

#include <stdlib.h>

#include "pluton/service.h"

using namespace std;

// Invoked as test.service1.3.HTML
// and test.wildcard.3.JSON (configured test..wildcard.3.JSON)

int
main(int argc, char** argv)
{
  pluton::service S("tService1");

  if (!S.initialize()) {
    cerr << S.getFault().getMessage("tService1", true) << endl;
    exit(1);
  }

  while (S.getRequest()) {

    string sk;
    S.getServiceKey(sk);

    if ((sk != "test.service1.3.HTML") && (sk != "test.wildcard.3.JSON")) {
      clog << "Error tService1: getServiceKey mismatch=" << sk << endl;
      S.sendFault(11, "sk bad");
      continue;
    }

    string sa;
    S.getServiceApplication(sa);
    if (sa != "test") {
      clog << "Error tService1: getServiceApplication mismatch=" << sa << endl;
      S.sendFault(12, "sa bad");
      continue;
    }

    string sf;
    S.getServiceFunction(sf);
    if ((sf != "service1") && (sf != "wildcard"))  {
      clog << "Error tService1: getServiceFunction mismatch=" << sf << endl;
      S.sendFault(pluton::unknownFunction, "sf bad");
      continue;
    }

    int version = S.getServiceVersion();
    if (version != 3) {
      clog << "Error tService1: getServiceVersion mismatch=" << version << endl;
      S.sendFault(13, "version bad");
      continue;
    }

    pluton::serializationType st = S.getSerializationType();
    if ((st != pluton::HTML) && (st != pluton::JSON)) {
      clog << "Error tService1: getSerializationType mismatch=" << st << endl;
      S.sendFault(14, "st bad");
      continue;
    }

    string cn;
    S.getClientName(cn);
    clog << "Client: " << cn << endl;
    S.sendResponse("");
  }

  if (S.hasFault()) clog << "Error: " << S.getFault().getMessage() << endl;

  return 0;
}
