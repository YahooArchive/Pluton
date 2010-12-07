#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#include <pluton/service.h>

using namespace std;

int
main(int argc, char** argv)
{
  pluton::service S("mallocLoop");

  if (!S.initialize()) {
    clog << S.getFault().getMessage("mallocLoop initialize") << std::endl;
    exit(1);
  }

  while (S.getRequest()) {
    while (true) {
      char* p = new char[1000000];	// 1Meg
      if (p) p++;			// Shut the compiler up
    }
  }

  return(0);
}
