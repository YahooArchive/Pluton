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

#include <stdlib.h>
#include <unistd.h>

#include "pluton/fault.h"
#include "pluton/service.h"
#include "pluton/client.h"

#include "global.h"
#include "version.h"

using namespace std;


static const char* usage =
"Usage: plVersion [-h]\n"
"\n"
"Print out Pluton versions and other static values\n"
"\n"
" -h   Print this usage message on STDOUT and exit(0)\n"
"\n"
"See also: http://localhost/docs/pluton/\n"
"\n";

//////////////////////////////////////////////////////////////////////

int
main(int argc, char **argv)
{
  char	optionChar;

  while ((optionChar = getopt(argc, argv, "h")) != -1) {
    switch (optionChar) {
    case 'h':
      cout << usage << version::rcsid << endl;
      exit(0);

    default:
      cerr << usage << version::rcsid << endl;
      exit(1);
    }
  }

  string manager = version::rcsid;
  string client = pluton::client::getAPIVersion();
  string service = pluton::service::getAPIVersion();

  cout << "plutonManager: " << manager << endl;
  cout << "clientAPI: " << client << endl;
  cout << "serviceAPI: " << service << endl;
  cout << "Lookup Map: " << pluton::lookupMapPath << endl;
  cout << "shmLookupMapVersion: " << plutonGlobal::shmLookupMapVersion << endl;
  cout << "shmServiceVersion: " << plutonGlobal::shmServiceVersion << endl;

  if (client != manager) cerr << "Warning: Client version does not match plutonManager" << endl;
  if (service != manager) cerr << "Warning: Service version does not match plutonManager" << endl;
  if (client != service) cerr << "Warning: Client version does not match Service" << endl;

  return(0);
}
