//////////////////////////////////////////////////////////////////////
// Try and call globals after construction of pluton::client
//////////////////////////////////////////////////////////////////////

#include <stdlib.h>

#include "pluton/client.h"

static pluton::thread_t
my_thread_self(const char* who)
{
  return 0;
}

int
main(int argc, char **argv)
{
  pluton::client C("tClientGlobals2");
  pluton::client::setThreadHandlers(my_thread_self);

  exit(0);
}
