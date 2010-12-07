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

#include <iostream>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "netString.h"

using namespace std;


static const char* usage =
"Usage: plNetStringGrep [-hnv] [typeList]\n"
"\n"
"Read netStrings from STDIN and write matching types to STDOUT.\n"
"\n"
" -h   Display usage and exit\n"
"\n"
" -n   Convert the trailing netString terminator to a newline\n"
"\n"
" -v   Invert the test and only output types that do not match.\n"
"       This option is particularly useful for removing variable\n"
"       valued netstrings - such as timestamps and process ids - to\n"
"       simplify regression test comparisons.\n"
"\n"
" typeList is a series of netString types to match. If not present\n"
" all types match.\n"
"\n"
"Examples:\n"
"\n"
" $ plNetStringGrep <filein aFnb: >fileout\n"
"\n"
"      fileout only contains 'a', 'F', 'n', 'b' and ':' types.\n"
"\n"
" $ plNetStringGrep -v <filein aFnB: >fileoutNot\n"
"\n"
"      fileoutNot contains everything not in fileout.\n"
"\n"
" $ plNetStringGrep -v a <results >results.stripped\n"
" $ plNetStringGrep -v a <expected | cmp --s - -\n"
" $ [ $? -ne 0 ] && echo Comparison failed\n"
"\n"
"      Remove the 'a' netString type and compare the rest with\n"
"      a previously created regression comparison file to see\n"
"      whether results have changed or not.\n"
"\n"
"If the input file does not contain an invalid netString\n"
"an error message is written to STDERR and the program exits.\n"
"\n"
"See also: http://localhost/docs/pluton/\n"
"\n";

//////////////////////////////////////////////////////////////////////

int
main(int argc, char **argv)
{
  bool conditionRequired = true;
  bool convertTerminate = false;

  char optionChar;
  while ((optionChar = getopt(argc, argv, "hnv")) != -1) {
    switch (optionChar) {
    case 'h':
      cout << usage;
      exit(0);

    case 'n': convertTerminate = true; break;

    case 'v': conditionRequired = false; break;

    default:
      cerr << endl << usage;
      exit(1);
    }
  }

  argc -= optind;
  argv += optind;
  if (argc > 1) {
    cerr << "Error: Unexpected argument on command line." << endl;
    cerr << endl << usage;
    exit(1);
  }

  const char* typeList = 0;
  if (argc == 1) typeList = argv[0];
  --argc; ++argv;


  netStringFactoryManaged nsf;
  do {
    char* bufPtr;
    int minRequired, maxAllowed;
    if (nsf.getReadParameters(bufPtr, minRequired, maxAllowed) == -1) {
      cerr << "Size Error: netString is bigger than maximum buffer size" << endl;
      exit(1);
    }

    cin.read(bufPtr, maxAllowed);
    int bytes = cin.gcount();
    if (bytes > 0) nsf.addBytesRead(bytes);

    const char* err = 0;
    while (nsf.haveNetString(err)) {
      const char* rawPtr;		// Get the raw version first for subsequent writing
      int rawLength;
      nsf.getRawString(rawPtr, rawLength);

      char nsType;			// Only really need the type, but nsf also
      const char* nsDataPtr;		// garbage collects when getNetString is
      int nsDataLength;			// called.
      int nsOffset;
      nsf.getNetString(nsType, nsDataPtr, nsDataLength, &nsOffset);

      bool matchCondition = true;
      if (typeList) matchCondition = strchr(typeList, nsType);
      if (matchCondition == conditionRequired) {
	if (convertTerminate) {
	  cout.write(rawPtr, rawLength-1);
	  cout << endl;
	}
	else {
	  cout.write(rawPtr, rawLength);
	}
      }
    }
    if (err) {
      cerr << "Parse Error: " << err << endl;
      exit(1);
    }

  } while (!cin.eof());

  return(0);
}
