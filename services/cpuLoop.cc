#include <iostream>

#include <stdlib.h>

#include <pluton/service.h>

using namespace  std;

int
main(int argc, char** argv)
{
  pluton::service S("cpuLoop");

  if (!S.initialize()) {
    clog << S.getFault().getMessage("cpuLoop initialize") << std::endl;
    exit(1);
  }

  while (S.getRequest()) {
    while (true);
  }

  return(0);
}
