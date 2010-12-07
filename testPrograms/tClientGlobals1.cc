//////////////////////////////////////////////////////////////////////
// Try and call globals twice
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
  pluton::client::setThreadHandlers(my_thread_self);
  pluton::client::setThreadHandlers(my_thread_self);

  exit(0);
}
