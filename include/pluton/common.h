#ifndef _PLUTON_COMMON_H
#define _PLUTON_COMMON_H

//////////////////////////////////////////////////////////////////////
// Define values that need to be visible to user code
//////////////////////////////////////////////////////////////////////

namespace pluton {

  //////////////////////////////////////////////////////////////////////
  // Serialization types are available to services
  //////////////////////////////////////////////////////////////////////

  enum serializationType {
    unknownSerialization='?',
    COBOL='c',		// COmon Business Oriented Language
    HTML='h',		// HyperText Markup Language
    JMS='m',		// Jave Messaging Service
    JSON='j',		// JavaScript Object Notation
    NETSTRING='n',	// Yahoo extended version of DJB's format
    PHP='p',		// PHP Serializer
    SOAP='s',		// Simple Object Access Protocol
    XML='x',		// eXtensible Markup Language
    raw='r'		// Undifferentated binary data
  };


  //////////////////////////////////////////////////////////////////////
  // requestAttributes influence the way a request is handled. They
  // are ints rather than enums as the caller may wish to "or" them
  // together.
  //////////////////////////////////////////////////////////////////////

  typedef int requestAttributes;

  static const int noWaitAttr		= 0x0001;
  static const int noRemoteAttr		= 0x0002;
  static const int noRetryAttr		= 0x0004;

  static const int keepAffinityAttr	= 0x0008;
  static const int needAffinityAttr	= 0x0010;

  static const int allAttrs		= 0xFFFF;
}

#endif
