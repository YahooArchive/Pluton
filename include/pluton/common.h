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

#ifndef PLUTON_COMMON_H
#define PLUTON_COMMON_H

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
