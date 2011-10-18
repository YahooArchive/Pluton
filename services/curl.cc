/*
Copyright (c) 2010, Yahoo! Inc. All rights reserved.

Redistribution and use of this software in source and binary forms, with or
without modification, are permitted provided that the following conditions are
met: 

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation and/or 
other materials provided with the distribution.

* Neither the name of Yahoo! Inc. nor the names of its contributors may be used 
to endorse or promote products derived from this software without specific prior 
written permission of Yahoo! Inc.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

#include "config.h"

#include <iostream>

#include <string>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include <pluton/service.h>

#include "lineToArgv.h"

using namespace std;

#if HAVE_DECL_OPTRESET
extern int optreset;		// Needed for FBSD & OS/X
#else
static int optreset = 0;	// but meaningless on others (Linux/Solaris)
#endif

extern int optind;
extern int opterr;

static const int MAXARGS = 100;

static const char* usage =
"Usage: pluton::system.curl [-ihL] [-A UserAgent] [-b cookieValue]\n"
"                           [-e referrer] [-H header] [-U UserPass] URL\n"
"\n"
"Make a libcurl request to the nominated URL for a pluton client.\n"
"\n"
"The available options emulate those of the curl command excepting\n"
"that options only set rather than toggled and filename alternatives\n"
"where allowed with the curl command are not recognized here.\n";

static const bool curlit(CURL*, int, char * const argv[], string& response, string& errorMessage);
static size_t acceptData(void *buffer, size_t size, size_t nmemb, void *userp);

static bool debugFlag = false;

static void
dumpArgs(int argc, const char* const argv[])
{
  clog << "dumpArgs: ind=" << optind << " C=" << argc;
  for (int ix=0; ix < argc; ix++) {
    clog << " " << ix << ":" << argv[ix];
  }
  clog << std::endl;
}

int
main(int realArgc, char** realArgv)
{
  if (realArgc > 1) {
    if (strcmp(realArgv[1], "-h") == 0) {
      cout << usage << endl;
      exit(0);
    }
    debugFlag = true;
  }

  curl_global_init(CURL_GLOBAL_ALL);
  CURL* curl = curl_easy_init();	// Create this once and re-use

  pluton::service S("curl");
  if (!S.initialize()) {
    clog << S.getFault().getMessage("curl initialize") << std::endl;
    clog << endl << usage << endl;
    exit(1);
  }

  while (S.getRequest()) {
    const char* p;
    int l = 0;
    S.getRequestData(p, l);

    if (l <= 0) {
      S.sendFault(18, "Zero length request cannot possible contain a URL");
      continue;
    }

    // Tokenize into a psuedo command line.

    char* argv[MAXARGS+1];
    const char* err;
    char* tokenBuffer = (char*) malloc(l+1);	// Copied by tokenizer

    int argc = util::lineToArgv(p, l, tokenBuffer, argv+1, MAXARGS, err);
    if (argc <= 0) {
      string em = "Syntax error parsing arguments: ";
      em += err;
      free(tokenBuffer);
      S.sendFault(19, em.c_str());
      continue;
    }

    // Fake out the curl command as argv[0]

    char a0[] = "curl";
    argv[0] = a0;
    ++argc;

    string response;
    string errorMessage;
    if (!curlit(curl, argc, argv, response, errorMessage)) {
      if (debugFlag) clog << "curlit error: " << errorMessage << endl;
      S.sendFault(20, errorMessage.c_str());
    }
    else {
      if (debugFlag) clog << "curlit good" << endl;
      S.sendResponse(response);
    }
    free(tokenBuffer);
    if (S.hasFault()) break;
  }

  if (S.hasFault()) clog << S.getFault().getMessage("curl fault: ") << endl;
  S.terminate();

  curl_easy_cleanup(curl);
  curl_global_cleanup();

  return 0;
}


//////////////////////////////////////////////////////////////////////
// Treat as as shell command-type interface
//////////////////////////////////////////////////////////////////////

static const bool
curlit(CURL* curl, int argc, char * const argv[], string& response, string& errorMessage)
{

  if (debugFlag) dumpArgs(argc, argv);

  curl_easy_reset(curl);
  if (curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1) != CURLE_OK) {
    errorMessage = "curl_easy_setopt(CURLOPT_TCP_NODELAY) failed";
    return false;
  }

  if (curl_easy_setopt(curl, CURLOPT_MAXCONNECTS, 20) != CURLE_OK) {
    errorMessage = "curl_easy_setopt(CURLOPT_TCP_NODELAY, 20) failed";
    return false;
  }

  if (curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1) != CURLE_OK) {
    errorMessage = "curl_easy_setopt(CURLOPT_NOPROGRESS, 1) failed";
    return false;
  }

  // Wrap header list in a class so we can catch the destruction

  class headerList {
  public:
    headerList() : headers(0) {}
    ~headerList() { curl_slist_free_all(headers); }
    struct curl_slist *headers;
  };

  headerList HL;

  char errorBuffer[CURL_ERROR_SIZE+1];

  if (curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer) != CURLE_OK) {
    errorMessage = "curl_easy_setopt(CURLOPT_ERRORBUFFER) failed";
    return false;
  }


  optreset = 1;		// getopt uses internal statics to reset
  optind = 1;
  opterr = 0;		// No error messages to stderr please
  int optChar;
  while ((optChar = getopt(argc, argv, "A:b:e:H:iLU:")) != -1) {

    if (debugFlag) clog << "getopt c=" << optChar << " optarg=" << optarg << endl;
    switch (optChar) {
    case 'A':
      if (curl_easy_setopt(curl, CURLOPT_USERAGENT, optarg) != CURLE_OK) {
	errorMessage = "CURLOPT_USERAGENT failed: ";
	errorMessage += errorBuffer;
	return false;
      }
      break;

    case 'b':
      if (curl_easy_setopt(curl, CURLOPT_COOKIE, optarg) != CURLE_OK) {
	errorMessage = "CURLOPT_COOKIE failed: ";
	errorMessage += errorBuffer;
	return false;
      }
      break;

    case 'e':
      if (curl_easy_setopt(curl, CURLOPT_REFERER, optarg) != CURLE_OK) {
	errorMessage = "CURLOPT_REFERER failed: ";
	errorMessage += errorBuffer;
	return false;
      }
      break;

    case 'H':
      HL.headers = curl_slist_append(HL.headers, optarg);
      break;

    case 'i':
      if (curl_easy_setopt(curl, CURLOPT_HEADER, 1) != CURLE_OK) {
        errorMessage = "CURLOPT_HEADER failed: ";
	errorMessage += errorBuffer;
	return false;
      }
      break;

    case 'L':
      if (curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1) != CURLE_OK) {
        errorMessage = "CURLOPT_FOLLOWLOCATION failed: ";
	errorMessage += errorBuffer;
	return false;
      }
      break;

    case 'U':
      if (curl_easy_setopt(curl, CURLOPT_USERPWD, optarg) != CURLE_OK) {
	errorMessage = "CURLOPT_USERPWD failed: ";
	errorMessage += errorBuffer;
	return false;
      }
      break;

    default:
      errorMessage = "Unrecognized option: Must be A:b:e:H:iLU:";
      return false;
    }
  }

  argc -= optind;
  argv += optind;

  if (argc < 1) {
    errorMessage = "No URL after parsing options";	// URL should be solitary argument
    return false;
  }

  if (argc > 1) {
    errorMessage = "More than one URL after parsing options";
    return false;
  }

  if (curl_easy_setopt(curl, CURLOPT_HTTPHEADER, HL.headers) != CURLE_OK) {
    errorMessage = "curl_easy_setopt(CURLOPT_HTTPHEADER) failed: ";
    errorMessage += errorBuffer;
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_URL, *argv);	// Set the URL

  --argc; ++argv;

  if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, acceptData) != CURLE_OK) {
    errorMessage = "CURLOPT_WRITEFUNCTION failed: ";
    errorMessage += errorBuffer;
    return false;
  }

  if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &response) != CURLE_OK) {
    errorMessage = "CURLOPT_WRITEDATA failed: ";
    errorMessage += errorBuffer;
    return false;
  }


  // Issue the curl request

  if (curl_easy_perform(curl) != CURLE_OK) {
    errorMessage = "curl_easy_perform failed: ";
    errorMessage += errorBuffer;
    return false;
  }

  return true;
}


//////////////////////////////////////////////////////////////////////
// curl returns data via a callback routine
//////////////////////////////////////////////////////////////////////

static size_t
acceptData(void *buffer, size_t size, size_t nmemb, void *resultingString)
{
  string* rs = (string*) resultingString;

  rs->append((char*) buffer, size * nmemb);

  return size * nmemb;
}
