#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <string>
#include <iostream>

using namespace std;

#include "pluton/client.h"

//////////////////////////////////////////////////////////////////////
// Test that the asserts work
//////////////////////////////////////////////////////////////////////

static pluton::thread_t
tSelf(const char* who)
{
  return 0;
}

static int
pollProxy(struct pollfd *fds, int nfds, unsigned long long timeout)
{
  return 0;
}


int
main(int argc, char** argv)
{
  if (argc < 2) {
    cerr << "Usage: tThread3 scipr" << endl;
    exit(1);
  }

  for (const char* cp = argv[1]; *cp; ++cp) {

    switch (*cp) {
    case 's':
      cout << "s pluton::client::setThreadHandlers(tSelf)" << endl;
      pluton::client::setThreadHandlers(tSelf);
      cout << "s ok" << endl;
      break;

    case 'c':
      {
	cout << "c pluton::client* c = new pluton::client()" << endl;
	pluton::client* c = new pluton::client("c");
	cout << "c ok " << c << endl;
      }
      break;

    case 'i':
      {
	cout << "i pluton::client* c = new pluton::client()" << endl;
	pluton::client* c = new pluton::client("c");
	cout << "i c->initialize()" << endl;
	c->initialize("");
	cout << "i ok" << endl;
      }
      break;

    case 'p':
      {
	cout << "p pluton::client* c = new pluton::client()" << endl;
	pluton::client* c = new pluton::client("c");
	cout << "p c->initialize()" << endl;
	c->initialize("");
	cout << "p c->setPollProxy(pollProxy)" << endl;
	c->setPollProxy(pollProxy);
	cout << "p ok" << endl;
      }
      break;

    case 'r':
      {
	cout << "r pluton::client* c = new pluton::client()" << endl;
	pluton::client* c = new pluton::client("c");
	cout << "r c->initialize()" << endl;
	c->initialize("");
	cout << "r c->setPollProxy(0)" << endl;
	c->setPollProxy(0);
	cout << "r ok" << endl;
      }
      break;

    default:
      cerr << "Don't know optionLetter '" << *cp << "'. Try scip or r" << endl;
      exit(2);
    }
  }

  exit(0);
}
