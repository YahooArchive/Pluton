#include "lineToArgv.h"

//////////////////////////////////////////////////////////////////////
// A routine to convert a line of text into a set of argv tokens using
// the same tokenizing rules used by a shell.
//
// Rules:
//
// 1. Tokens are separated by whitespace
//
// 2. Leading and trailing whitespace are ignored
//
// 3. A token can have a single quote or a double quote and is
//    then terminated solely the matching quote.
//
// 4. Backslash escaping works in unquoted tokens and means to never
//    treat the next character syntactically. That means it can be
//    used to escape whitespace, single quotes, double quotes and a
//    backslash.
//
//
// On completetion, return the number of token pointers placed into
// argv. Negative indicates an error and the negative column number
// where the error was detected. Eg, -5 means an error at column
// 5. Column one is the first column.
//
// inputLine and len define the string to tokenize.
//
// buffer is used to store the resulting tokens, do not destroy the
// contents until you have finished with argv. buffer must be size
// len+1 or greater.
//
// argv and maxArgv defines the pointer array.
//
// err contains an error message if the return is < 0, otherwise it's
// NULL.
//////////////////////////////////////////////////////////////////////

int
util::lineToArgv(const char* inputLine, int len, char* buffer, char** argv, int maxArgv,
		 const char*& err)
{
  err = 0;

  int column = 0;
  int argc = 0;
  char quote = 0;

  enum { skippingWS, copyingToken, lookingForQuote, escapeNext } state = skippingWS;

  // Ensure no residual goop

  for (int ix=0; ix < maxArgv; ++ix) argv[ix] = 0;

  while (len > 0) {
    char cc = *inputLine;
    ++inputLine; --len; ++column;

    switch (state) {
    case skippingWS:
      if ((cc == ' ') || (cc == '\t')) break;

      state = copyingToken;
      if (argc == maxArgv) {
	err = "argv is not big enough for all the tokens";
	return -column;
      }
      argv[argc] = buffer;		// Token will start here

      /* FALLTHRU */

    case copyingToken:
      if ((cc == '\'') || (cc == '"')) {
	quote = cc;
	state = lookingForQuote;
	break;
      }

      if ((cc == ' ') || (cc == '\t')) {// WS terminates this token
	*buffer++ = '\0';
	state = skippingWS;
	++argc;				// Added to argv list
	break;
      }

      if (cc == '\\') {			// Arbitrarily accept the next char
	state = escapeNext;
	break;
      }

      *buffer++ = cc;			// Nothing special - append to token buffer
      break;


    case lookingForQuote:
      if (cc == quote) {		// Nothing stops a quote except itself
	state = copyingToken;
	quote = 0;
      }
      else {
	*buffer++ = cc;
      }
      break;


    case escapeNext:
      *buffer++ = cc;
      state = quote ? lookingForQuote : copyingToken;
      break;
    }					// End of switch(state)
  }					// End of while(len)

  // Did the scan finish in an acceptable state?

  switch (state) {
  case skippingWS: break;

  case copyingToken:
    *buffer++ = '\0';
    ++argc;
    break;

  case lookingForQuote:
    err = "Unterminated quote";
    return -column;

  case escapeNext:
    err = "Unterminated escape character";
    return -column;
  }

  return argc;
}
