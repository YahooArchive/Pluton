#include <iostream>

#include <string.h>

using namespace std;

#include <stdlib.h>

#include "lineToArgv.h"

// Test out lineToArgv.cc


static const int maxArgv = 100;

static void
runit(const char* line, int expectedArgv, const char* errMsg, int newMaxArgv=maxArgv)
{
  char buffer[2048];
  char* argv[maxArgv];
  const char* err;
  int argc = util::lineToArgv(line, strlen(line), buffer, argv, newMaxArgv, err);
  if (argc != expectedArgv) {
    cout << "Error: " << errMsg << ":" << argc;
    if (argc < 0) cout << ":" << err;
    cout << endl;
    cout << "Line: " << line << endl;
  }
  if (argc <= 0) return;

  for (int ix=0; ix < argc; ++ix) {
    cout << ix << ":" << argv[ix] << "< ";
  }
  cout << endl;
}

int
main(int realArgc, char** realArgv)
{

  runit("", 0, "1 Empty Line Fails");

  runit("aaaaa", 1, "2 One simple token");

  runit("a bb", 2, "3 Two simple tokens");

  runit(" a bb", 2, "4a Leading whitespace");
  runit("	  a bb", 2, "4b Leading whitespace");

  runit(" a bb ", 2, "5a Trailing whitespace");
  runit(" a bb	 ", 2, "5b Trailing whitespace");
  runit(" a bb 	  ", 2, "5c Trailing whitespace");

  runit("a bb ccc dddd eeeee ffffff ggggggg hhhhhhhh iiiiiiiii", 9, "6a too many args", 9);
  runit("a b c d e f g h j k", -17, "6b too many args", 8);
  runit("a b c d e f g h j     k", -23, "6c too many args", 9);

  runit("a 'bb' \"ccc\" 'dddd'  \"eeeee\" ffffff", 6, "7a quotes");
  runit("a 'ee\"eee'", 2, "7b quote double quotes");
  runit("a \"ee'eee\"", 2, "7c quote single quotes");
  runit("'ggggggg'", 1, "7d Ends in single quote");
  runit("\"ggggggg\"", 1, "7e Ends in double quote");

  runit("a ee\\\"eee", 2, "8a escape double quote");
  runit("a ee\\\'eee", 2, "8b escape single quote");
  runit("a ee\\\\eee", 2, "8c escape escape");

  runit("a '  b b' \"c  c  c  \"", 3, "9a white spaced tokens");

  exit(0);
}
