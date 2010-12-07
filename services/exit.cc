#include <iostream>
#include <string>

#include <stdlib.h>

using namespace std;

#include "pluton/service.h"


int
main(int argc, char** argv)
{
  pluton::service S("exit");

  if (!S.initialize()) {
    cerr << "exit: Cannot initialize as a service: " << S.getFault().getMessage() << std::endl;
    exit(42);
  }

  if (!S.getRequest()) exit(1);
  std::string exitCodeStr;
  S.getContext("exit.code", exitCodeStr);
  int exitCode = atoi(exitCodeStr.c_str());

  exit(exitCode);
}
