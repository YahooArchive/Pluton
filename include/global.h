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
