#ifndef _FAULTIMPL_H
#define _FAULTIMPL_H

//////////////////////////////////////////////////////////////////////
// The fault class is returned to the caller so they can get extensive
// information about the fault returned. Mainly it's a container of
// return codes, errno values, textual matter and the code location of
// the fault.
//////////////////////////////////////////////////////////////////////

#include "pluton/fault.h"

namespace pluton {

  class faultImpl {
  public:
    faultImpl();
    ~faultImpl();
    void inline clear(const char* entry)
      {
	_hasFault = false;
	if (entry && *entry) _entryName = entry;
      }

    bool	set(faultCode setF, const char* setMN, int setLN,
		    int setSC=0, int setEN=0,
		    const char* setTX1=0, const char* setTX2=0);

    bool	hasFault() const { return _hasFault; }
    faultCode 	getFaultCode() const { return _hasFault ? _code : pluton::noFault; }

    const std::string	getMessage(const char* pre=0, bool longFormat=true) const;

  private:
    bool	_hasFault;
    const char*	_entryName;
    faultCode	_code;
    int		_subCode;	// Returned from subroutine within caller
    int		_errno;		// Copy of the system value in <errno.h>

    int		_tx1Size;	// Max size of space in tx1, tx2
    int		_tx2Size;
    bool	_tx1Relevant;	// If tx1 and tx2 are relevant to current fault
    bool	_tx2Relevant;
    char*	_tx1;
    char*	_tx2;

    const char*	_moduleName;	// Defaults to __FILE__ and __LINE__
    int		_lineNumber;
  };

  class faultInternal : public pluton::fault {
  public:
    ~faultInternal() {};

    bool	set(faultCode setF, const char* setMN, int setLN,
		    int setSC=0, int setEN=0,
		    const char* setTX1=0, const char* setTX2=0)
      {
	return _impl->set(setF, setMN, setLN, setSC, setEN, setTX1, setTX2);
      }

    void inline clear(const char* entry) { _impl->clear(entry); }
  };
}

#endif
