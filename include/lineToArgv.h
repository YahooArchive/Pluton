#ifndef _LINETOARGV_H
#define _LINETOARGV_H

//////////////////////////////////////////////////////////////////////
// Tokenize a line of tokens into an argv. Return # argv entries of
// -ve column at which the error was detected.
//////////////////////////////////////////////////////////////////////

namespace util {
  extern int lineToArgv(const char* inputLine, int len, char* buffer, char** argv, int maxArgv,
			const char*& err);
}

#endif
