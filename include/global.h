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

#ifndef	_GLOBAL_H
#define _GLOBAL_H

class plutonGlobal {
 public:
  static const int plutonPort = 14099;
  static const int inheritedAcceptFD = 3;
  static const int inheritedShmServiceFD = 4;
  static const int inheritedReportingFD = 5;

  static const int inheritedHighestFD = 5;

  static const unsigned int shmLookupMapVersion = 1001;
  static const unsigned int shmServiceVersion = 2001;
};

namespace pluton {

  const char* const lookupMapPath = "/usr/local/var/pluton/lookup.map";

  //////////////////////////////////////////////////////////////////////
  // Packet Types are used to define a subsequent set of netStrings
  // which define the particular action. Packets are *not* nested
  // netStrings, rather they are a series of netStrings with a
  // terminating type.
  //////////////////////////////////////////////////////////////////////

  enum packetType {
    remoteLoginPT = 'L',		// Send credentials
    loginResponsePT = 'M',

    requestPT = 'Q',
    responsePT = 'A'
  };


  //////////////////////////////////////////////////////////////////////
  // Following a Packet Type are netString Types use to define the
  // data items. There is no constraint to using the same netString
  // Types across different packets. The constraint, if any, is solely
  // imposed by the functionalty of the Packet Type.
  //////////////////////////////////////////////////////////////////////

  enum netStringType {

    // Common types used in both directions

    endPacketNT = 'z',

    clientIDNT = 'a',
    requestIDNT = 'b',
    serviceKeyNT = 'c',

    // Types in a client request

    attributeNoWaitNT = 'e',
    attributeNoRemoteNT = 'f',
    attributeNoRetryNT = 'g',

    attributeKeepAffinityNT = 'h',
    attributeNeedAffinityNT = 'i',

    contextNT = 'j',
    requestDataNT = 'k',
    timeoutMSNT = 'l',
    fileDescriptorNT = 'm',

    // Types in a service response

    faultCodeNT = 'p',
    responseDataNT = 'q',
    faultTextNT = 'r',
    serviceIDNT = 's',

    // Types in Remote Login

    credentialsNT = 't',

    // Types in Login Response

    allowedNT = 'u'
  };
}

#endif
