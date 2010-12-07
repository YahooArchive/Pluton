#include <iostream>

#include <fcntl.h>
#include <stdlib.h>

#include <pluton/service.h>

using namespace std;


int
main(int argc, char** argv)
{
  pluton::service S("fopenLoop");

  if (!S.initialize()) {
    clog << S.getFault().getMessage("fopenLoop initialize") << std::endl;
    exit(1);
  }

  while (S.getRequest()) {

    int fcount = 0;
    while (++fcount) {
      if (open(".", O_RDONLY, 0) == -1) {
	std::cerr << "Opened " << fcount << std::endl;
	return(1);
      }
    }
  }


  return(0);
}
