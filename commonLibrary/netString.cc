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

#include <string>
#include "ostreamWrapper.h"

#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "netString.h"


//////////////////////////////////////////////////////////////////////
// netstrings are a simple, fast, data serialization format that are
// particularly suited to network transfers. netstrings are intended
// to be: easy to create by the sender; easy to parse by the receiver
// and efficient. See: http://cr.yp.to/proto/netstrings.txt
//
// A netstring consists of a length a delimiter, the contents and a
// terminator. Here's an example: 10:abcdefghij,
//
// This example netstring contains 10 bytes of content. The ':'
// separates the length from the contents and the trailing ','
// provides a simple check that data hasn't been truncated.
// netstrings are intended to be sent over a reliable data stream,
// such as a TCP socket. The length attribute means that data does not
// have to be scanned or parsed.
//
//
// Most of the classes in this module focus on managing a collection
// of netstrings.
//
//
// Note that this particular implementation of netstrings has two
// variations from the standard netstring.
//
// First, the delimiter that separates the length from the content can
// be any non-digit printable character (isprint()) rather than just a
// colon. This acts as a type selector. This gives the decoder the
// ability to identify the contents of a netstring. This is of
// particular use to many protocols that want to serialize in random
// order and optional strings.
//
// The second variation is that the terminating comma can be
// substituted with a trailing \n. It still achieves the goal of
// checking that the netstring is well formed and terminated
// correctly, but that variation allows netstrings to be printed and
// understood by humans much more easily.
//
//
// The classes in this module are:
//
// - netStringGenerate() creates a container of netstrings
//
// - netStringParse() extracts netstrings from a static buffer
//
// - netStringStreamParse() extracts netstrings from a streaming
//   buffer, this only ever touches data as it's passed in, it never
//   touches previous data, even if it's in the same netstring.
//
// - netStringFactory() derived from netStringStreamParse() manages a
//   user supplied I/O buffer.
//
// - netStringFactoryManaged() derived from netStringFactory() with an
//   internally managed buffer.
//////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Convert an integer to a string. Return a pointer to the
// string. Caller must supply a buffer big enough to handle the
// largest string. The returned point will point somewhere within the
// buffer.
////////////////////////////////////////////////////////////////////////////////

typedef struct {
  char  buf[20];
} IA;   // Integer conversion area


static const char*
ltoa(IA& iap, long ival)
{
  bool negative = false;
  int strsize = sizeof(iap.buf);
  char* str = iap.buf;

  if (strsize < 2) return "0";

  --strsize;
  str += strsize;
  *str-- = '\0'; --strsize;

  if (ival < 0) {
    ival = - ival;
    negative = true;
  }

  do {
    *str-- = '0' + (ival % 10);
    ival /= 10;
  } while ((strsize > 0) && (ival > 0));

  if ((strsize > 0) && negative) *str-- = '-';

  return str+1;
}


//////////////////////////////////////////////////////////////////////
// Calculate the number of encapsulation bytes required.
//////////////////////////////////////////////////////////////////////

unsigned int
netStringGenerate::determineOverhead(unsigned int length)
{
  int overhead = 1 + 1;		// Type + trailing comma
  do {
    if (length > 1000) {
      overhead += 3;
      length /= 1000;
    }
    ++overhead;
    length /= 10;
  } while (length > 0);

  return overhead;
}


//////////////////////////////////////////////////////////////////////
// The netStringGenerate class creates netstrings. Given the supplied
// parameters, create a netstring and append it to the internal
// buffer.
//////////////////////////////////////////////////////////////////////

netStringGenerate::netStringGenerate(std::string* alternateStringPtr, bool useAlternateTerminator)
  : _sPtr(alternateStringPtr), _terminator(useAlternateTerminator ? '\n' : ','),
    _defaultBaseString(0)
{
  if (_sPtr == 0) {
    _defaultBaseString = new std::string;
    _sPtr = _defaultBaseString;			// Use default if none supplied
  }
}


netStringGenerate::netStringGenerate(std::string& alternateStringRef, bool useAlternateTerminator)
  : _sPtr(&alternateStringRef), _terminator(useAlternateTerminator ? '\n' : ','),
    _defaultBaseString(0)
{
}


netStringGenerate::~netStringGenerate()
{
  if (_defaultBaseString) delete _defaultBaseString;
}


//////////////////////////////////////////////////////////////////////
// The appenders simply take various data types and append them in
// netstring format.
//////////////////////////////////////////////////////////////////////

int
netStringGenerate::append(const char addType, const char* addPtr, int addLen)
{
  if (addLen < 0) return -1;
  if (!isprint(addType)) return -2;	// Type must be non-digit printable
  if (isdigit(addType)) return -3;
  if (addPtr == NULL) return -4;

  // Convert length to ascii

  IA ia;
  const char* cp = ltoa(ia, addLen);
  (*_sPtr) += cp;

  (*_sPtr) += addType;
  _sPtr->append(addPtr, addLen);
  (*_sPtr) += _terminator;

  return _sPtr->length();
}


//////////////////////////////////////////////////////////////////////
// Non-macro versions where a NULL pointer needs to be checked first
//////////////////////////////////////////////////////////////////////

int
netStringGenerate::append(const char* addPtr)
{
  if (addPtr == NULL) return -4;
  return append(':', addPtr, strlen(addPtr));
}


int
netStringGenerate::append(const char addType, const char* addPtr)
{
  if (addPtr == NULL) return -4;
  return append(addType, addPtr, strlen(addPtr));
}


//////////////////////////////////////////////////////////////////////
// Multiple implementations with different footprints exist for
// efficiency. I don't want to use c_str() etc on strings, especially
// on the server side where CPU is a valuable resource. Code
// duplication is a pain, but these are such simple routines that I
// don't expect more than one or two latent bugs...
//////////////////////////////////////////////////////////////////////

int
netStringGenerate::append(const char addType, const std::string& addString)
{
  if (!isprint(addType)) return -2;
  if (isdigit(addType)) return -3;

  // Convert length to ascii

  IA ia;
  const char* cp = ltoa(ia, addString.length());
  (*_sPtr) += cp;

  (*_sPtr) += addType;
  (*_sPtr) += addString;
  (*_sPtr) += _terminator;
  return _sPtr->length();
}


//////////////////////////////////////////////////////////////////////
// This routine appends the netstring prefix for the provided
// length. It is usually used in conjunction with a caller that wants
// to do in-situ writev() calls to avoid memory copy costs.
//////////////////////////////////////////////////////////////////////

int
netStringGenerate::appendRawPrefix(const char addType, int len)
{
  if (!isprint(addType)) return -2;
  if (isdigit(addType)) return -3;

  // Convert length to ascii

  IA ia;
  const char* cp = ltoa(ia, len);
  (*_sPtr) += cp;
  (*_sPtr) += addType;

  return _sPtr->length();
}

int
netStringGenerate::appendRawTerminator()
{
  (*_sPtr) += _terminator;
  return _sPtr->length();
}

int 
netStringGenerate::append(const char addType, int addInt)
{
  return append(addType, static_cast<long>(addInt));
}

int
netStringGenerate::append(const char addType, long addInt)
{
  IA iaData;					// Convert signed int to a C string
  const char* cp = ltoa(iaData, addInt);

  return netStringGenerate::append(addType, cp, strlen(cp));
}

int
netStringGenerate::append(const char addType, long long addInt)
{
  IA iaData;					// Convert signed int to a C string
  const char* cp = ltoa(iaData, addInt);

  return netStringGenerate::append(addType, cp, strlen(cp));
}

int 
netStringGenerate::append(const char addType, unsigned int addInt)
{
  return append(addType, static_cast<unsigned long>(addInt));
}

int
netStringGenerate::append(const char addType, unsigned long addInt)
{

  IA iaData;
  const char* cp = ltoa(iaData, addInt);	// Convert unsigned int to a C string

  return netStringGenerate::append(addType, cp, strlen(cp));
}

int
netStringGenerate::append(const char addType, unsigned long long addBigInt)
{
  IA iaData;
  const char* cp = ltoa(iaData, addBigInt);	// Convert unsigned long long to a C string

  return netStringGenerate::append(addType, cp, strlen(cp));
}

int
netStringGenerate::append(const char addType, char addChar)
{
  if (!isprint(addType)) return -2;
  if (isdigit(addType)) return -3;

  (*_sPtr) += '1';
  (*_sPtr) += addType;
  (*_sPtr) += addChar;
  (*_sPtr) += _terminator;
  return _sPtr->length();
}


int
netStringGenerate::append(const char addType, unsigned char addChar)
{
  if (!isprint(addType)) return -2;
  if (isdigit(addType)) return -3;

  (*_sPtr) += '1';
  (*_sPtr) += addType;
  (*_sPtr) += addChar;
  (*_sPtr) += _terminator;
  return _sPtr->length();
}


int
netStringGenerate::append(const char addType)
{
  if (!isprint(addType)) return -2;
  if (isdigit(addType)) return -3;

  (*_sPtr) += '0';
  (*_sPtr) += addType;
  (*_sPtr) += _terminator;
  return _sPtr->length();
}

int
netStringGenerate::appendNL(const char addType)
{
  if (!isprint(addType)) return -2;
  if (isdigit(addType)) return -3;

  (*_sPtr) += '0';
  (*_sPtr) += addType;
  (*_sPtr) += '\n';		// Force this terminator
  return _sPtr->length();
}


int
netStringGenerate::append(const char addType, bool trueOrFalse)
{
  if (!isprint(addType)) return -2;
  if (isdigit(addType)) return -3;

  (*_sPtr) += '1';
  (*_sPtr) += addType;
  (*_sPtr) += (trueOrFalse ? 'y' : 'n');
  (*_sPtr) += _terminator;
  return _sPtr->length();
}


std::ostream&
netStringGenerate::put(std::ostream& s) const
{
  return s << "netStringGenerate: L=" << _sPtr->length() << ", C=" << (*_sPtr);
}

std::ostream&
operator<<(std::ostream& s, const netStringGenerate& nsg)
{
  return nsg.put(s);
}


std::ostream&
netStringGenerateEncapsulated::put(std::ostream& s) const
{
  return s << "netStringGenerateEncapsulated: L=" << _sPtr->length()-_reservedSpace
	   << ", R=" << _reservedSpace
	   << ", C=" << (*_sPtr).c_str()+_reservedSpace;
}

std::ostream&
operator<<(std::ostream& s, const netStringGenerateEncapsulated& nsg)
{
  return nsg.put(s);
}


//////////////////////////////////////////////////////////////////////

netStringGenerateEncapsulated::netStringGenerateEncapsulated(int setEncapsulationDepth,
							     std::string* alternateStringPtr,
							     bool useAlternateTerminator)
  : netStringGenerate(alternateStringPtr, useAlternateTerminator),
    _reservedSpace((10+1+1)*setEncapsulationDepth),
    _originalReservedSpace(_reservedSpace)
{
  if (_sPtr->empty()) {
    _sPtr->assign(_reservedSpace, '_');
  }
  else {
    for (unsigned int ix=0; ix<_reservedSpace; ++ix) _sPtr->insert(0, "_");
  }
      
  //  clog << "nsGEL1=" << _sPtr->length() << " from " << _reservedSpace << endl;
}

netStringGenerateEncapsulated::netStringGenerateEncapsulated(int setEncapsulationDepth,
							     std::string& alternateStringRef,
							     bool useAlternateTerminator)
  : netStringGenerate(alternateStringRef, useAlternateTerminator),
    _reservedSpace((10+1+1)*setEncapsulationDepth),
    _originalReservedSpace(_reservedSpace)
{
  if (_sPtr->empty()) {
    _sPtr->assign(_reservedSpace, '_');
  }
  else {
    for (unsigned int ix=0; ix<_reservedSpace; ++ix) _sPtr->insert(0, "_");
  }
  //  clog << "nsGEL2=" << _sPtr->length() << " from " << _reservedSpace << endl;
}

void
netStringGenerateEncapsulated::erase()
{
  _reservedSpace = _originalReservedSpace;
  _sPtr->assign(_reservedSpace, '_');
}


//////////////////////////////////////////////////////////////////////
// Encapsulate the current netString with the nominated type. This
// involves the following steps:
//
// 1) Determine how many bytes are needed for the length
// 2) If there is not enough reserved space, return false
// 3) Replace filler bytes with length and type
// 4) Append the terminator
// 5) Reduce the reserved space
//////////////////////////////////////////////////////////////////////

bool
netStringGenerateEncapsulated::encapsulate(const char addType)
{
  unsigned int overHead = netStringGenerate::determineOverhead(_sPtr->length() - _reservedSpace);
  overHead++;	// Need a byte for the type		- step 1)
  if (overHead > _reservedSpace) return false;	//	- step 2)

  IA ia;
  const char* lenPtr = ltoa(ia, _sPtr->length() - _reservedSpace);
  unsigned int len = strlen(lenPtr);

  if ((len+1) > _reservedSpace) return false;	//	should never occur but be safe

  // - step 3) Replace filler bytes with the length and type

  unsigned startIx = _reservedSpace - len - 1;
  for (unsigned int ix = 0; ix < len; ++ix, ++startIx) {
    (*_sPtr)[startIx] = lenPtr[ix];
  }

  (*_sPtr)[_reservedSpace -1] = addType;
  (*_sPtr) += _terminator;				// - step 4)

  _reservedSpace -= (len + 1);				// - step 5)

  return true;
}

const char*
netStringGenerateEncapsulated::data() const
{
  return _sPtr->data() + _reservedSpace;
}

unsigned int
netStringGenerateEncapsulated::length() const
{
  return _sPtr->length() - _reservedSpace;
}

bool
netStringGenerateEncapsulated::empty() const
{
  return _sPtr->length() == _reservedSpace;
}



//////////////////////////////////////////////////////////////////////
// The netStringParse class examines the user supplied buffer for each
// netstring. If a validly formatted netstring is found, the getNext()
// extracts the length, type and returns them along with a pointer to
// the start of the data. A caller can safely touch all of the
// netstring after a good return from this routine on the basis that
// the caller supplies the initial buffer so the caller is responsible
// for ensuring that the data is valid across the life of the
// netStringParse() object.
//////////////////////////////////////////////////////////////////////

const char*
netStringParse::getNext(char& returnType, const char*& returnPtr, int& returnLen)
{
  returnPtr = NULL;
  returnLen = 0;

  if (eof()) return "already at EOF";

  int	len = 0;

  //////////////////////////////////////////////////////////////////////
  // Decode the length until alpha separator, no more characters or
  // length too large. The very first character must be a digit. Even
  // a zero length netstring has a leading '0'.
  //////////////////////////////////////////////////////////////////////

  if (_ptr == NULL) return "pointer is NULL";

  if (! isdigit(*_ptr)) return "leading character is not a digit";

  while (_remaining > 0) {

    if (! isdigit(*_ptr)) {		// End of length must be a valid type
      if (isprint(*_ptr)) break;		// which is an isprint() char
      _remaining = 0;
      return "invalid separator type (not isprint())";
    }


    len *= 10;				// It is a digit, continue to accumulate length
    if (len > 999999999) {		// ... upto a ridiculously large size, but we
      _remaining = 0;			// need to defend against int overflow.
      return "length value greater than 999999999";
    }

    len += *_ptr - '0';			// Process length digit
    --_remaining; ++_ptr;			// Consume length digit
  }

  // The type character broke the length decode loop

  if (_remaining <= 0) return "length delimiter missing";	// No data left, error

  returnType = *_ptr;			// Extract Type
  --_remaining; ++_ptr;			// Consume type

  if (len >= _remaining) {		// Make sure length is not greater than remaining
    _remaining = 0;			// == catches the 1 byte needed for the trailing comma
    return "length longer than data";
  }

  if ((_ptr[len] != ',') && (_ptr[len] != '\n')) {	// Check for trailing terminator
    _remaining = 0;
    return "incorrect trailing terminator - comma or \\n expected";
  }

  returnPtr = _ptr;			// Good, transfer return values to caller
  returnLen = len;

  _ptr += len + 1;			// Consume data + terminating comma
  _remaining -= len + 1;

  return 0;
}


//////////////////////////////////////////////////////////////////////
// find is a wrapper routine around getNext that finds a particular
// netString in a parseable stream of netStrings. It presents a good
// example of how to use getNext() and for code that doesn't care
// about the inefficiency of re-scanning each time it's a code-cheap
// way of extracting netStrings. Generally a caller should use
// getNext() or find() but not both as find() resets the pointers each
// time.
//////////////////////////////////////////////////////////////////////

bool
netStringParse::find(char findType, std::string& result)
{
  _remaining = _origRemaining;		// Reset parsing to the beginning
  _ptr = _origPtr;

  char 		nextType;
  const char* 	nextData;
  int 		nextLength;

  while (! eof()) {
    if (getNext(nextType, nextData, nextLength)) return false;

    if (nextType == findType) {
      result.assign(nextData, nextLength);
      return true;
    }
  }

  return false;
}


//////////////////////////////////////////////////////////////////////
// Say how much of the data has been consumed
//////////////////////////////////////////////////////////////////////

int
netStringParse::consumed() const
{
  return _origRemaining - _remaining;
}


//////////////////////////////////////////////////////////////////////

std::ostream&
netStringParse::put(std::ostream& s) const
{
  return s << "netStringParse: remaining=" << _remaining << ", C=" << _ptr;
}

std::ostream&
operator<<(std::ostream& s, const netStringParse& nsp)
{
  return nsp.put(s);
}


//////////////////////////////////////////////////////////////////////
// The netStringStreamParse extracts netstrings from an on-going
// stream of bytes. Typically it is used as part of an I/O routine
// that is reading netstrings from a socket or file of somesort where
// all of the data is not necessarily immediately available.
//
// netStringFactory is supplied with a buffer which it manages internally,
// relocating data as needed to create a contiguous netstring.
//
// netStringFactoryManaged manages an internal buffer.
//////////////////////////////////////////////////////////////////////

netStringStreamParse::netStringStreamParse()
  : _ignoreCRLF(false),
    _parseState(checkLeadingDigit), _dataLength(0), _bytesToSkip(0),
    _nsType(0), _netStringsFound(0), _bytesSkipped(0)
{
}

netStringStreamParse::~netStringStreamParse()
{
}


void
netStringStreamParse::reset()
{
  _parseState = checkLeadingDigit;
  _nsType = 0;
  _dataLength = 0;
  _bytesToSkip = 0;
}

char
netStringStreamParse::getType() const
{
  if (_parseState <= checkType) return 0;

  return _nsType;
}


//////////////////////////////////////////////////////////////////////
// Return the details of the current netString in that parsed
// stream. This call effectively consumes the netString as the parser
// state is changed to start looking for the next netString.
//
// Return: true if netString has been returned
//////////////////////////////////////////////////////////////////////

bool
netStringStreamParse::getNetString(char& returnType, int& returnDataLength)
{
  if (_parseState != gotit) {
    returnType = 0;
    returnDataLength = -1;
    return false;
  }

  returnType = _nsType;
  returnDataLength = _dataLength;			// modify their own buffer

  // Reset internal state now that they have the NS

  _parseState = checkLeadingDigit;
  _nsType = 0;
  _dataLength = 0;
  _bytesToSkip = 0;

  return true;
}


//////////////////////////////////////////////////////////////////////
// Parse the data looking for a netstring. Return an error if the
// contents don't look like a valid netstring. A netstring should be
// consumed by getNetString() after each true return from this method.
//
// This routine differs from netStringParse::getNext() in that it can
// handle a netstring coming in piece-meal, eg, from a socket.
//
// The variables nsLengthOffset and nsDataOffset are set as the length
// and data locations are discovered. parseOffset and unparsedBytes
// are adjusted as the bytes are consumed by the parser.
//
// By the nature of a stream parser, the value of the returned data is
// only valid at the time it is returned and is relative to the
// baseAddress *at that time*.
//////////////////////////////////////////////////////////////////////

bool
netStringStreamParse::haveNetString(const char* baseAddress, int& parseOffset, int& unparsedBytes,
				     int& nsLengthOffset, int& nsDataOffset, const char*& error)
{
  error = 0;
  if ((_parseState != gotit) && (unparsedBytes > 0)) {
    error = parse(baseAddress, parseOffset, unparsedBytes, nsLengthOffset, nsDataOffset);
    if (error) return false;
  }
  return _parseState == gotit;
}


//////////////////////////////////////////////////////////////////////
// Stop the scanning precisely when the location of the first byte of
// the netString content is known.
//////////////////////////////////////////////////////////////////////

bool
netStringStreamParse::haveDataStart(const char* baseAddress, int& parseOffset,
				    int& unparsedBytes,
				    int& nsLengthOffset, int& nsDataOffset, const char*& error)
{
  error = 0;
  if ((_parseState != skippingData) && (unparsedBytes > 0)) {
    error = parse(baseAddress, parseOffset, unparsedBytes, nsLengthOffset, nsDataOffset, true);
  }
  return _parseState == skippingData;
}


//////////////////////////////////////////////////////////////////////
// Stop the scanning precisely when all data has been scanned.
//////////////////////////////////////////////////////////////////////

bool
netStringStreamParse::haveDataEnd(const char* baseAddress, int& parseOffset,
				  int& unparsedBytes,
				  int& nsLengthOffset, int& nsDataOffset, const char*& error)
{
  error = 0;
  if ((_parseState != checkingTerminator) && (unparsedBytes > 0)) {
    error = parse(baseAddress, parseOffset, unparsedBytes, nsLengthOffset, nsDataOffset,
		  false, true);
  }
  return _parseState == checkingTerminator;
}


//////////////////////////////////////////////////////////////////////
// This helper routine decodes the netstring piece-meal.
//////////////////////////////////////////////////////////////////////

const char*
netStringStreamParse::parse(const char* baseAddress, int& parseOffset, int& unparsedBytes,
			    int& nsLengthOffset, int& nsDataOffset,
			    bool stopAtDataFlag, bool stopAtTerminatorFlag)
{
  while (unparsedBytes > 0) {

    switch (_parseState) {
    case checkLeadingDigit:
      nsLengthOffset = parseOffset;
      if (! isdigit(baseAddress[parseOffset])) {
	if (_ignoreCRLF
	    && (unparsedBytes == 2)
	    && (baseAddress[parseOffset] == '\r')
	    && (baseAddress[parseOffset+1] == '\n')) {
	  unparsedBytes = 0;	// Consume CRLF if allowed and matches the residual
	  break;
	}
	return "leading netString character not a digit";
      }

      _dataLength = baseAddress[parseOffset] - '0';
      ++parseOffset; --unparsedBytes;
      _parseState = parsingLength;
      break;

    case parsingLength:
      if (isdigit(baseAddress[parseOffset])) {
	_dataLength *= 10;
	if (_dataLength > 999999999) return "netString length value greater than 999999999";
	_dataLength += baseAddress[parseOffset] - '0';
	++parseOffset; --unparsedBytes;
	break;
      }
      _parseState = checkType;
      break;

    case checkType:
      if (! isprint(baseAddress[parseOffset])) return "invalid netString type (not isprint())";
      _nsType = baseAddress[parseOffset];
      ++parseOffset; --unparsedBytes;
      _bytesToSkip = _dataLength;
      _bytesSkipped += _dataLength;
      _parseState = setDataOffset;
      break;

    case setDataOffset:
      nsDataOffset = parseOffset;	// Set this when the actual byte is underfoot
      _parseState = skippingData;	// (though it could be the closing terminator too!)
      if (stopAtDataFlag) return 0;
      break;

    case skippingData:
      if (unparsedBytes < _bytesToSkip) {
	_bytesToSkip -= unparsedBytes;
	parseOffset += unparsedBytes;
	unparsedBytes = 0;
      }
      else {
	unparsedBytes -= _bytesToSkip;
	parseOffset += _bytesToSkip;
	_bytesToSkip = 0;
	_parseState = checkingTerminator;
	if (stopAtTerminatorFlag) return 0;
      }
      break;

    case checkingTerminator:
      if ((baseAddress[parseOffset] != ',') && (baseAddress[parseOffset] != '\n'))
	return "incorrect netString terminator - comma or \\n expected";
      ++parseOffset; --unparsedBytes;
      _parseState = gotit;
      ++_netStringsFound;
      return 0;

    case gotit: return "parsing when NetString is ready - caller should call getNext()";
    }
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////

const char*
netStringStreamParse::stateToEnglish() const
{
  switch (_parseState) {
  case checkLeadingDigit: return "checkLeadingDigit";
  case parsingLength: return "parsingLength";
  case checkType: return "checkType";
  case setDataOffset: return "setDataOffset";
  case skippingData: return "skippingData";
  case checkingTerminator: return "checkingTerminator";
  case gotit: return "gotit";
  }

  return "??";
}


//////////////////////////////////////////////////////////////////////

netStringFactory::netStringFactory(const char* initBuffer, int initBufferSize)
  : _relocationAllowed(true), _baseAddress(initBuffer), _bufferSize(initBufferSize),
    _parseOffset(0), _unparsedBytes(0), _nsLengthOffset(0), _nsDataOffset(0),
    _bytesRead(0), _bytesRelocated(0), _zeroCostRelocates(0), _userReallocations(0)
{
}

netStringFactory::~netStringFactory()
{
}


//////////////////////////////////////////////////////////////////////
// Return the just-parsed netstring if it's ready. The netstring data
// only remains valid until the next call (of any type) to this
// object.
//
// Return: true if the netString has been produced.
//////////////////////////////////////////////////////////////////////

bool
netStringFactory::getNetString(char& returnType, const char*& returnDataPtr, int& returnDataLength,
			       int* returnDataOffset)
{
  netStringStreamParse::getNetString(returnType, returnDataLength);
  if (returnType == 0) {
    returnDataPtr = 0;
    return false;
  }

  returnDataPtr = _baseAddress + _nsDataOffset;
  if (returnDataOffset) *returnDataOffset = _nsDataOffset;

  _nsLengthOffset = 0;
  _nsDataOffset = 0;

  ////////////////////////////////////////////////////////////
  // Take the opportunity to reset to the beginning of the buffer if
  // the consumption of this netstring depletes all data in the
  // buffer. This saves on expensive relocations mid-parse and should
  // be a common occurence since ultimately the senders of a stream
  // are likely to stop on a netstring boundary.
  ////////////////////////////////////////////////////////////

  if ((_unparsedBytes == 0) && _relocationAllowed) {
    _parseOffset = 0;
    ++_zeroCostRelocates;
  }

  return true;
}


//////////////////////////////////////////////////////////////////////
// Return pointer+length of the complete netString, including
// terminator. This call does not change state as does
// ::getNetString() indirectly via the Stream Parser.
//////////////////////////////////////////////////////////////////////

bool
netStringFactory::getRawString(const char*& returnDataPtr, int& returnDataLength) const
{
  int offset;
  returnDataPtr = 0;
  if (!getRawString(offset, returnDataLength)) return false;

  returnDataPtr = _baseAddress + offset;

  return true;
}


//////////////////////////////////////////////////////////////////////
// A varient that is offset based rather than an absolute address
//////////////////////////////////////////////////////////////////////

bool
netStringFactory::getRawString(int& returnDataOffset, int& returnDataLength) const
{
  if (_parseState != gotit) {
    returnDataOffset = 0;
    returnDataLength = -1;
    return false;
  }

  returnDataOffset = _nsLengthOffset;
  returnDataLength = _dataLength + (_nsDataOffset - _nsLengthOffset) + 1;

  return true;
}


//////////////////////////////////////////////////////////////////////
// Parse the data looking for a netstring. Return an error if the
// contents don't look like a valid netstring. A netstring should be
// consumed by getNetString() after each true return from this method.
//
// This routine differs from netStringParse::getNext() in that it can
// handle a netstring coming in piece-meal, eg, from a socket.
//////////////////////////////////////////////////////////////////////

bool
netStringFactory::haveNetString(const char*& error)
{
  error = 0;
  if ((_parseState != gotit) && (_unparsedBytes > 0)) {
    error = netStringStreamParse::parse(_baseAddress, _parseOffset, _unparsedBytes,
					_nsLengthOffset, _nsDataOffset);
  }

  return _parseState == gotit;
}


void
netStringFactory::reset()
{
  netStringStreamParse::reset();

  _parseOffset = 0;
  _unparsedBytes = 0;
  _nsLengthOffset = 0;
  _nsDataOffset = 0;
}

//////////////////////////////////////////////////////////////////////
// Tell the caller where to read new data into (bufferOffset) and the
// maximum number of bytes that they can safely read
// (maximumBytesToRead). This is an opportunistic time to relocate the
// netstring to the bottom of the buffer.
//
// If the return value is non-zero, it indicates that the buffer needs
// to expand to at least this size to cater for the current
// netstring. In short, the buffer is too small to ever hold the
// current netstring contiguously. In this case, the offset and
// bytesToRead values are not valid.
//
// The caller should call std::realloc() (or the moral equivalent)
// with the return value and then call netStringFactory::reallocated()
// to inform this class of the new buffer (with intact data as a
// consequence of std::realloc()).
//
// Note that if you do call reallocated() then any previous pointers
// returned by getReadParameters and getNetString are (obviously)
// invalidated.
//
// If you don't want to bother with all this, you should consider
// using the derived class: netStringFactoryManaged.
//
// If maximumBytesToRead is returned as zero, the caller should call
// ::haveNetString() to see if a netString is available to free up
// space in the buffer.
//////////////////////////////////////////////////////////////////////

int
netStringFactory::getReadParameters(char*& bufferPtr, int& minimumBytesToRead,
				    int& maximumBytesToRead)
{

  // It's ok to cast away constness as it's their buffer to read
  // into. It's const internally to compile-detect bugs in this
  // library that might attempt to write into the buffer.

  bufferPtr = const_cast<char*>(_baseAddress) + _parseOffset + _unparsedBytes;
  minimumBytesToRead = 0;
  maximumBytesToRead = _bufferSize - (_parseOffset + _unparsedBytes);

  // Find out, as best we can, what the size of the yet-to-be parsed
  // part of the netstring is.

  switch (_parseState) {

  case checkLeadingDigit:		// We don't know the exact amount yet, but it's
    minimumBytesToRead = 1+1+1;		// at least one length byte + type + ,
    break;

  case parsingLength:			// _dataLength is meaninful though incomplete
    minimumBytesToRead = _dataLength + 1+1;
    break;

  case checkType:
    minimumBytesToRead = _dataLength + 1;
    break;

  case setDataOffset:
  case skippingData:
    minimumBytesToRead = _bytesToSkip + 1;
    break;

  case checkingTerminator:
    minimumBytesToRead = 1;
    break;

  default:
  case gotit:
    return 0;				// Already have a netstring ready to go!
  }

  ////////////////////////////////////////////////////////////
  // If there's enough unparsed data to complete the netstring, then
  // there is no reason to read more data, yet. "Yet" because
  // "minimumBytesToRead" is a minimum that could grow if the data
  // length is not yet know, thus a caller should interpret this
  // return (expand=0, bytesToRead=0) as a directive to call getNext()
  // and then make another call to this routine.
  ////////////////////////////////////////////////////////////

  if (minimumBytesToRead <= _unparsedBytes) {	// Got enough unparsed to do the job?
    return 0;
  }

  ////////////////////////////////////////////////////////////
  // Definitely need to read more data in. If there's enough
  // contiguous space left to read the required data in, then let the
  // caller know, otherwise relocate and try the test a second time.
  ////////////////////////////////////////////////////////////

  do { 
    if ((minimumBytesToRead + _parseOffset) <= _bufferSize) {
      bufferPtr = const_cast<char*>(_baseAddress) + _parseOffset + _unparsedBytes;
      maximumBytesToRead = _bufferSize - (_parseOffset + _unparsedBytes);
      return 0;
    }
  } while (_relocationAllowed && tryToCompact());

  // The netstring is at the beginning of the buffer and there is not
  // enough buffer to fit the full netstring. Time to get the caller
  // to expand.


  return _bufferSize + minimumBytesToRead - _unparsedBytes;
}


//////////////////////////////////////////////////////////////////////
// If the netstring is not at the beginning of the buffer, move it
// down to the beginning and adjust the offsets to match. Return true
// if a compaction occured.
//////////////////////////////////////////////////////////////////////

bool
netStringFactory::tryToCompact()
{
  int moveLength = 0;
  int moveOffset = 0;

  if (_parseState == checkLeadingDigit) {	// nsLengthOffset is not valid
    if (_unparsedBytes == 0) {
      _parseOffset = 0;
      ++_zeroCostRelocates;
      return true;
    }
    moveLength = _unparsedBytes;
    moveOffset = _parseOffset;
  }
  else {					// nsLengthOffset is valid
    moveLength = _parseOffset - _nsLengthOffset + _unparsedBytes;
    moveOffset = _nsLengthOffset;
  }

  if ((moveOffset > 0) && (moveLength > 0)) {
    memmove((void*) _baseAddress, (void*) (_baseAddress + moveOffset), moveLength);
    _bytesRelocated += moveLength;

    _nsLengthOffset -= moveOffset;	// Adjust all offsets to suit, regardless
    _parseOffset -= moveOffset;		// of whether they are currently valid
    _nsDataOffset -= moveOffset;

    return true;
  }

  return false;
}


//////////////////////////////////////////////////////////////////////
// If the caller changes the size of the buffer or reallocates it
// (with data intact) then they need to tell us via this call.
//////////////////////////////////////////////////////////////////////

void
netStringFactory::reallocated(const char* newBaseAddress, int newBufferSize)
{
  if (_bufferSize > 0) ++_userReallocations;

  _baseAddress = newBaseAddress;
  _bufferSize = newBufferSize;
}


//////////////////////////////////////////////////////////////////////
// The caller has read in data, adjust our internal structure and see
// if there is a complete netstring available.
//////////////////////////////////////////////////////////////////////

void
netStringFactory::addBytesRead(int len)
{
  _unparsedBytes += len;
  _bytesRead += len;

  assert((_unparsedBytes + _parseOffset) <= _bufferSize);
}


//////////////////////////////////////////////////////////////////////
// Disabling compaction means that a caller can use the factory to
// produce multiple netstrings and still know that earlier netstrings
// are available at the offset indicated. After an allowCompaction()
// call, all previously returned offsets are likely invalid.
//////////////////////////////////////////////////////////////////////
 
void
netStringFactory::disableCompaction()
{
  _relocationAllowed = false;
}


//////////////////////////////////////////////////////////////////////
// It a caller has turned off auto-compaction, they should call
// reclaimByCompaction() at strategic points in their program when
// they know that they're done with all previously returned offsets.
//////////////////////////////////////////////////////////////////////

void
netStringFactory::reclaimByCompaction()
{
  tryToCompact();
}


//////////////////////////////////////////////////////////////////////

void
netStringFactory::getStatistics(int& getBytesRead, int& getBytesRelocated,
				int& getNetStringsFound, int& getBytesSkipped,
				int& getZeroCostRelocates, int& getUserReallocations)
{
  getBytesRead = _bytesRead;
  getBytesRelocated = _bytesRelocated;
  getNetStringsFound = _netStringsFound;
  getBytesSkipped = _bytesSkipped;
  getZeroCostRelocates = _zeroCostRelocates;
  getUserReallocations = _userReallocations;

  _bytesRead = 0;
  _bytesRelocated = 0;
  _netStringsFound = 0;
  _bytesSkipped = 0;
  _zeroCostRelocates = 0;
  _userReallocations = 0;
}


//////////////////////////////////////////////////////////////////////
// A managed netStringFactory handles reallocations internally rather
// than having the caller manage the space. initialSize+1 is simply to
// ensure that the initial buffer is always greater than zero bytes
// long.
//////////////////////////////////////////////////////////////////////

netStringFactoryManaged::netStringFactoryManaged(unsigned int initialSize,
						 unsigned int setMaximumSize)
  : netStringFactory(0, 0),
    _inboundBufferSize(initialSize+1), _maximumSize(setMaximumSize),
    _inboundBufferPtr(new char[_inboundBufferSize])
{
  reallocated(_inboundBufferPtr, _inboundBufferSize);
  if ((_maximumSize > 0) && (_inboundBufferSize > _maximumSize)) _maximumSize = _inboundBufferSize;
}

netStringFactoryManaged::~netStringFactoryManaged()
{
  delete [] _inboundBufferPtr;
}


//////////////////////////////////////////////////////////////////////
// Managed the expansion for the caller. If the buffer given to the
// netStringFactory isn't big enough to hold the inbound netstring,
// reallocate the buffer and data upto the allowed maximum.
//////////////////////////////////////////////////////////////////////

int
netStringFactoryManaged::getReadParameters(char*& bufferPtr, int& minimumBytesToRead,
					   int& maximumBytesToRead)
{
  int expandTo = netStringFactory::getReadParameters(bufferPtr, minimumBytesToRead,
						     maximumBytesToRead);
  if (expandTo == 0) return 0;

  if ((_maximumSize > 0) && ((unsigned) expandTo > _maximumSize)) return -1;	// Too big

  // Don't nickel and dime the growth of the buffer, rather, make it a
  // decent chunk of change.

  int minimumExpansion = _inboundBufferSize / 2;
  if (minimumExpansion > 16*1024) minimumExpansion = 16*1024;
  minimumExpansion += _inboundBufferSize;
  if ((_maximumSize > 0) && ((unsigned) minimumExpansion > _maximumSize)) {
    minimumExpansion = _maximumSize;
  }
  if (minimumExpansion > expandTo) expandTo = minimumExpansion;

  char* newBuffer = new char[expandTo];
  memcpy(newBuffer, _inboundBufferPtr, _inboundBufferSize);	// Overkill, but easy and rare

  _inboundBufferSize = expandTo;					// Remember new buffer details
  delete [] _inboundBufferPtr;
  _inboundBufferPtr = newBuffer;

  reallocated(_inboundBufferPtr, _inboundBufferSize); 		// Notify factory of new buffer
  expandTo = netStringFactory::getReadParameters(bufferPtr, minimumBytesToRead,
						 maximumBytesToRead);
  assert(expandTo == 0);					// Trust but verify

  return 0;
}



//////////////////////////////////////////////////////////////////////
// Given a string containing a stream of netStrings. "Pop" off the
// first netString and return the separated netStrings. Return error
// or return firstPart, _nsType and residual.
//////////////////////////////////////////////////////////////////////

const char*
netStringPop(const std::string& input, char& nsType, std::string& firstPart, std::string& residual)
{
  netStringParse rk(input);
  const char* nsDataPtr;
  int	nsLength;

  const char* err = rk.getNext(nsType, nsDataPtr, nsLength);
  if (err) return err;

  firstPart.assign(nsDataPtr, nsLength);
  residual.append(input, rk.consumed(), std::string::npos);

  return 0;
}


std::ostream&
netStringFactory::put(std::ostream& s) const
{
  return s << "NSF:"
	   << "Sz=" << _bufferSize
	   << " S=" << stateToEnglish()
	   << " dataLen=" << _dataLength
	   << " T=" << _nsType
	   << " nsLenO=" << _nsLengthOffset
	   << " nsDataO=" << _nsDataOffset
	   << " Skip=" << _bytesToSkip
	   << " unparsed=" << _unparsedBytes
	   << " _parseOffset=" << _parseOffset
	   << " Stats: " << _bytesRead << "/" << _bytesRelocated
	   << "/" << _netStringsFound << "/" << _bytesSkipped
	   << "/" << _zeroCostRelocates << "/" << _userReallocations
	   << " Rok=" << (_relocationAllowed ? "yes" : "no");
}


std::ostream&
operator<<(std::ostream& s, const netStringFactory& nsf)
{
  return nsf.put(s);
}
