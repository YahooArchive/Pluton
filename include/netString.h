#ifndef _NETSTRING_H
#define _NETSTRING_H

#include <string>

#include "ostreamWrapper.h"

#include <string.h>


//////////////////////////////////////////////////////////////////////
// This library implements a variant of
// http://cr.yp.to/proto/netstrings.txt - see the .cc file for
// details.
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// netStringGenerate creates a container of netStrings. There are a
// plethora of "append" signatures to handle many of the types that
// might be used as data for a netString. This class uses std::string
// as the container, so what is left is largely the creation of the
// netString length, type, data, terminator syntax.
//////////////////////////////////////////////////////////////////////


class netStringGenerate {
 public:
  netStringGenerate(std::string* alternateStringPtr=0, bool useAlternateTerminator=false);
  netStringGenerate(std::string& alternateStringRef, bool useAlternateTerminator=false);

  virtual ~netStringGenerate();

  static unsigned int determineOverhead(unsigned int length);		// Return extra bytes needs

  // Append the content as a netString

  int append(const char addType, const char* addPtr, int addLen);
  int append(const char addType, const char* addPtr);
  int append(const char addType, const std::string& addString);
  int append(const char addType, char addChar);
  int append(const char addType, unsigned char addChar);
  int append(const char addType);
  int appendNL(const char addType);
  int append(const char addType, bool trueOrFalse);
  int append(const char addType, int addInt);
  int append(const char addType, long addInt);
  int append(const char addType, long long addInt);
  int append(const char addType, unsigned int uInt);
  int append(const char addType, unsigned long uInt);
  int append(const char addType, unsigned long long uInt);

  int appendRawPrefix(const char addType, int len);
  int appendRaw(const std::string addString) { _sPtr->append(addString); return _sPtr->length(); };
  int appendRaw(const char* cp, int len) { _sPtr->append(cp, len); return _sPtr->length(); };
  int appendRawTerminator();
  void reserve(int s) { _sPtr->reserve(s); }

  int append(const char addType, const netStringGenerate& addNS)
    {
      return append(addType, addNS.getString());
    }
  int append(const char* addPtr, int addLen) { return append(':', addPtr, addLen); }
  int append(const char* addPtr);
  int append(const std::string& addString) { return append(':', addString); }
  int append(const netStringGenerate& addNS) { return append(':', addNS.getString()); }
  int append(const netStringGenerate* addNS) { return append(':', addNS->getString()); }

  virtual void			erase() { _sPtr->erase(); }
  void				clear() { _sPtr->erase(); }
  virtual const std::string	getString() const { return (*_sPtr); }
  virtual const char*		data() const { return _sPtr->data(); }
  virtual unsigned int		length() const { return _sPtr->length(); }
  virtual bool			empty() const { return _sPtr->empty(); }

  virtual std::ostream& put(std::ostream& s) const;

 protected:
  std::string*	_sPtr;
  unsigned char	_terminator;

 private:
  netStringGenerate&	operator=(const netStringGenerate& rhs);	// Assign not ok
  netStringGenerate(const netStringGenerate& rhs);			// Copy not ok

  std::string*	_defaultBaseString;
};

std::ostream&        operator<<(std::ostream&, const netStringGenerate&);


//////////////////////////////////////////////////////////////////////
// An encapsulated netString is an netString in which a series of
// netstrings will be encapsulated into another netstring. Typically
// as a packet of netStrings. To do this, space is reserved at the
// beginning of the string so that the type and length can be
// prepended without moving the string. It's an optimization.
// //////////////////////////////////////////////////////////////////////.

class netStringGenerateEncapsulated : public netStringGenerate {
 public:
  netStringGenerateEncapsulated(int setEncapsulationDepth=1,
				std::string* alternateStringPtr=0,
				bool useAlternateTerminator=false);

  netStringGenerateEncapsulated(int setEncapsulationDepth,
				std::string& alternateStringRef,
				bool useAlternateTerminator=false);

  ~netStringGenerateEncapsulated() {}

  bool		encapsulate(const char addType);
  const char*	data() const;
  unsigned int	length() const;
  void		erase();
  bool		empty() const;

  virtual const std::string	getString() const { return std::string(data(), length()); }

  std::ostream& 	put(std::ostream& s) const;

 private:
  unsigned int	_reservedSpace;		// Before the start of the current netString
  unsigned int	_originalReservedSpace;	// Needed for clear
};

std::ostream&        operator<<(std::ostream&, const netStringGenerate&);


////////////////////////////////////////////////////////////////////////////////
// netStringParse is complementary to the netStringGenerate family. It
// parses a netString making the type, data and length available to
// the caller.  NetStringParse expects the original string to remain
// untouched during the parse process, leastwise that part of the
// string that has yet to be passed back to the caller.
////////////////////////////////////////////////////////////////////////////////

class netStringParse {
 public:
  netStringParse() : _ptr(""), _remaining(-1), _origPtr(0), _origRemaining(-1) {}
  ~netStringParse() {}

  netStringParse(const char* initPtr, int initLen)
    : _ptr(initPtr), _remaining(initLen),
    _origPtr(_ptr), _origRemaining(_remaining) {}

  netStringParse(const char* initPtr)
    : _ptr(initPtr), _remaining(strlen(_ptr)),
    _origPtr(_ptr), _origRemaining(_remaining) {}

  netStringParse(const std::string& initString) :
    _ptr(initString.data()), _remaining(initString.length()),
    _origPtr(_ptr), _origRemaining(_remaining) {}

  netStringParse(const netStringGenerate& nsg) :
    _ptr(nsg.getString().data()), _remaining(nsg.getString().length()),
    _origPtr(_ptr), _origRemaining(_remaining) {}

  void set(const char* initPtr, int initLen)
    {
      _origPtr = _ptr = initPtr;
      _origRemaining = _remaining = initLen;
    };
  void set(const char* initPtr)
    {
      _origPtr = _ptr = initPtr;
      _origRemaining = _remaining = strlen(_ptr);
    };
  void set(const std::string& initString)
    {
      _origPtr = _ptr = initString.data();
      _origRemaining = _remaining = initString.length();
    };

  ////////////////////////////////////////
  // The main routine for netStringParse
  ////////////////////////////////////////

  const char*	getNext(char& returnType, const char*& returnPtr, int& returnLength);

  bool 	eof() const { return _remaining == 0; };
  bool 	isPresent() const { return _remaining > -1; };
  void	erase() { _remaining = -1; };
  bool	find(char type, std::string& returnValue);

  int	consumed() const;		// Return how much of the data have been parsed

  std::string getOrigString() const	// Debug only
    {
      if (isPresent()) return std::string(_origPtr, _origRemaining);
      return "";
    };

  std::ostream& put(std::ostream& s) const;

 private:
  netStringParse&	operator=(const netStringParse& rhs);	// Assign not ok
  netStringParse(const netStringParse& rhs);			// Copy not ok

  const char*	_ptr;
  int		_remaining;

  const char*	_origPtr;
  int		_origRemaining;
};

std::ostream&        operator<<(std::ostream& s, const netStringParse &nsp);


//////////////////////////////////////////////////////////////////////
// netStringStreamParse and it's derivatives scans a stream of bytes
// extracting netStrings. Typically used in conjunction with code that
// is reading in netstrings from, say, a socket.
//
// The important feature of this family of routines is that they do
// not have to have access to all of the netString at once. The caller
// feeds data piecemeal as it becomes available.
//////////////////////////////////////////////////////////////////////

class netStringStreamParse {
 public:
  netStringStreamParse();
  virtual ~netStringStreamParse();

  bool	haveNetString(const char* bufferAddress, int& parseOffset, int& unparsedBytes,
		      int& nsLengthOffset, int& nsDataOffset, const char*& error);

  bool	haveDataStart(const char* bufferAddress, int& parseOffset, int& unparsedBytes,
		      int& nsLengthOffset, int& nsDataOffset, const char*& error);

  bool	haveDataEnd(const char* bufferAddress, int& parseOffset, int& unparsedBytes,
		    int& nsLengthOffset, int& nsDataOffset, const char*& error);

  int	getDataLength() const { return (_parseState > parsingLength) ? _dataLength : -1; }
  bool	getNetString(char& returnType, int& returnDataLength);
  void	setIgnoreCRLF(bool newFlag) { _ignoreCRLF = newFlag; }	// Allows telnet for testing
  char  getType() const;

  virtual void	reset();	// Clear a previous error and parsing state

  const char*	stateToEnglish() const;

  std::ostream& put(std::ostream& s) const;

 protected:
  netStringStreamParse&	operator=(const netStringStreamParse& rhs);		// Assign not ok
  netStringStreamParse(const netStringStreamParse& rhs);			// Copy not ok

  const char*	parse(const char* baseAddress, int& parseOffset, int& unparsedBytes,
		      int& nsLengthOffset, int& nsDataOffset,
		      bool stopAtDataFlag=false, bool stopAtTerminatorFlag=false);

  bool	_ignoreCRLF;
  enum { checkLeadingDigit, parsingLength, checkType, setDataOffset, skippingData,
	 checkingTerminator, gotit } _parseState;

  int	_dataLength;		// Computed length of the netstring being parsed
  int 	_bytesToSkip;		// When skipping over data - how many are left
  char	_nsType;		// As a result of parsing the netstring

  int	_netStringsFound;
  int	_bytesSkipped;
};


//////////////////////////////////////////////////////////////////////

class netStringFactory : public netStringStreamParse {
 public:
  netStringFactory(const char* initBaseAddress, int initBufferSize);
  virtual ~netStringFactory();

  bool	getNetString(char& returnType, const char*& returnDataPtr, int& returnDataLength,
		     int* returnDataOffset = 0);
  bool	getRawString(int& returnDataOffset, int& returnDataLength) const;
  bool	getRawString(const char*& returnDataPtr, int& returnDataLength) const;
  int	getRawOffset() const { return _parseOffset; }
  const char* getBasePtr() const { return _baseAddress; }

  virtual int	getReadParameters(char*& bufferPtr, int& minimumBytesToRead,
				  int& maximumBytesToRead);
  void		addBytesRead(int len);

  void		reset();

  bool	haveNetString(const char*& error);
  void	reallocated(const char* newBaseAddress, int newBufferSize);

  void	getStatistics(int& getBytesRead, int& getBytesRelocated,	// get resets
		      int& getNetStringsFound, int& getBytesSkipped,
		      int& getZeroCostRelocates, int& getUserReallocations);

  void	disableCompaction();
  void	reclaimByCompaction();
  bool	isFrozen() { return !_relocationAllowed; }

  std::ostream& put(std::ostream& s) const;

 private:
  netStringFactory&	operator=(const netStringFactory& rhs);		// Assign not ok
  netStringFactory(const netStringFactory& rhs);			// Copy not ok

  bool		tryToCompact();

  bool	_relocationAllowed;	// Allowed to make non zero-cost relocates
  const char* _baseAddress;	// Can move between calls - see relocate()
  int	_bufferSize;		// From the start of baseAddress

  int	_parseOffset;		// Next character to look at for parsing
  int	_unparsedBytes;		// length of available data at parseOffset
  int	_nsLengthOffset;	// Offset to start of length for this netstring
  int	_nsDataOffset;		// Offset to start of data for this netstring

  int	_bytesRead;		// Statistics
  int	_bytesRelocated;
  int	_zeroCostRelocates;
  int	_userReallocations;
};


//////////////////////////////////////////////////////////////////////
// A managed netStringFactory looks after memory allocation for the
// caller.
//////////////////////////////////////////////////////////////////////

class netStringFactoryManaged : public netStringFactory {
 public:
  netStringFactoryManaged(unsigned int initialSize=1024, unsigned int maximumSize=0);
  ~netStringFactoryManaged();

  virtual int	getReadParameters(char*& ptr, int& minToRead, int& maxToRead);
  int		getBufferSize() const { return _inboundBufferSize; }

 private:
  netStringFactoryManaged&	operator=(const netStringFactoryManaged& rhs);	// Assign not ok
  netStringFactoryManaged(const netStringFactoryManaged& rhs);			// Copy not ok

  unsigned int	_inboundBufferSize;
  unsigned int	_maximumSize;
  char* 	_inboundBufferPtr;
};


// Unclassed routines

const char*	netStringPop(const std::string& input, char& nsType,
			     std::string& firstPart, std::string& residual);

std::ostream&        operator<<(std::ostream& s, const netStringFactory &nsio);

#endif
