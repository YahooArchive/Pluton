#ifndef _PLUTON_FAULT_H
#define _PLUTON_FAULT_H

#include <string>

namespace pluton {

  //////////////////////////////////////////////////////////////////////
  // faultCodes are used throughout the various pluton APIs. They are
  // returned with client::request results as well as calls to the
  // APIs.
  //
  // Negative faultCodes are reserved for system faults.
  // Zero indicates no fault.
  // Positive faultCodes are reserved for user faults.
  //
  // In addition, the first 10 positive faultCodes are pre-defined and
  // services are encouraged to use these where possible rather than
  // inventing their own.
  //
  // The pluton::fault class provides detailed information about a
  // fault so the exact reason and code location of the fault can be
  // easily determined. The pluton::fault class is available to both
  // the client and service APIs.
  //
  // The "// E: " pattern is used by a script to auto-generate the
  // code->english array used in the pluton::fault class.
  //////////////////////////////////////////////////////////////////////

  enum faultCode {
    noFault = 0,

    // User reserved faultCodes

    deserializeFailed = 1,		// E: De-serialization of netString packet failed
    unknownFunction = 2,		// E: Service wildcard request with invalid function
    requestTooLarge = 3,		// E: Request too large
    remoteConnectFailed = 4,		// E: Could not connect to remote host
    remoteTransferFailed = 5,		// E: Write to remote socket failed mid-stream
    lastReservedUserFault = 10,		// E: If you see this - worry!

    // Client API errors -1 to -50

    alreadyInitialized = -1,		// E: Multiple calls to ::initialize()
    notInitialized = -2,		// E: Must call ::initialize() prior to all others
    requestNotAdded = -3,		// E: Waiting on a request that is not in progress
    requestAlreadyAdded = -4,		// E: Request has already been added
    requestInProgress = -5,		// E: Request is still being sent to the service
    responseInProgress = -6,		// E: Response has not yet been read from service
    unimplemented = -7,			// E: Called an unimplemented method
    badRequestLength = -8,		// E: Request Length is less than zero
    noAffinity = -9,			// E: needAffinityAttr set on non-connected request
    noWaitNotAllowed = -10,		// E: Cannot have noWaitAttr and keepAffinityAttr
    needNoRetry = -11,			// E: Must have noRetryAttr with keepAffinityAttr

    openSocketFailed = -15,		// E: -1 return from socket()
    connectFailed = -16,		// E: -1 return from connect()
    socketWriteFailed = -17,		// E: -1 return from socket write()
    socketReadFailed = -18,		// E: -1 return from socket read()
    serviceTimeout = -19,		// E: Caller timeout expired
    incompleteResponse = -20,		// E: Service closed socket with response pending
    responsePacketFormatError = -21,	// E: Response Packet format is invalid
    contextFormatError = -22,		// E: Context data in packet is incorrect
    contextReservedNamespace = -23,	// E: Context key cannot start with .pluton
    setsockoptFailed = -24,		// E: -1 return from setsockopt()
    fcntlFailed = -25,			// E: -1 return from fcntl()

    // Lookup errors

    serviceNotFound = -30,		// E: ServiceKey is not in lookupMap
    serviceKeyBad = -31,		// E: Format of serviceKey is invalid
    mapInitializationFailed = -32, 	// E: Could not open/mmap the lookup map
    openForMmapFailed = -33,		// E: open() prior to mmap() of lookup Path failed
    lookupButNoMap = -34,		// E: Attempted lookup on non-mapped lookup Path
    persistentRemapFlag = -35,		// E: Remap flag set after remap attempt

    // Usage or other client internal errors

    exceededRetryLimit = -43,		// E: Configured retry limit has been exceeded
    retryNotAllowed = -44,		// E: noRetryAttr set on a request that failed
    remoteNotAllowed = -45,		// E: noRemoteAttr set on a request to be relayed
    writeEventForWrongFD = -46,		// E: pluton::clientImpl::sendCanWriteEvent gave a bad fd
    readEventForWrongFD = -47,		// E: pluton::clientImpl::sendCanReadEvent gave a bad fd
    writeEventWrongState = -48,		// E: pluton::clientImpl::writeEvent with wrong state
    readEventWrongState = -49,		// E: pluton::clientImpl::readEvent with wrong state


    // Service errors -51 to -100

    openAcceptSocketFailed = -51,	// E: Could not open getenv('plutonAcceptSocket')
    mmapInheritedShmFDFailed = -52,	// E: Could not mmap fd passed by manager
    getRequestNotNext = -53,		// E: Out of sequence call to getRequest()
    acceptFailed = -54,			// E: accept() on listening socket failed
    noFauxSTDIN = -55,			// E: Service needs a faux STDIN when run from the shell
    wrongNSType = -56,			// E: Unexpected netString type
    requestDecodeFailed = -57,		// E: Could not decode request packet
    requestNetStringParseError = -58,	// E: Parse of request packet netstring failed
    sendResponseNotNext = -59,		// E: Out of sequence call to sendResponse()
    sendRawResponseNotNext = -60,	// E: Out of sequence call to sendRawResponse()
    netStringTooLarge = -61,		// E: buffer cannot expand to fit inbound netString
    unexpectedEOF = -62,		// E: EOF mid-way through a packet read
    bindFailed = -63,			// E: -1 return from bind()
    unlinkFailed = -64,			// E: -1 return from unlink() of named pipe
    listenFailed = -65,			// E: -1 returned from listen()
    emptyIdentifier = -66,		// E: Empty parameters given to openService()
    pollInterrupted = -67,		// E: -1 and EINTR returned from poll() of pipe

    // mmap errors -101 to -110

    shmFstatFailed = -101,		// E: fstat() return -1 on mmap fd
    shmMmapFailed = -102,		// E: mmap() returned MAP_FAILED
    shmVersionMisMatch = -103,		// E: shm verion differs from expected
    shmThreadConfigNotService = -104,	// E: Threaded Config mis-matches service
    shmThreadServiceNotConfig = -105,	// E: Threaded Service mis-matches config
    shmPreWriteFailed = -106,		// E: Pre-write of mmap file failed
    shmImpossiblySmall = -107,		// E: Lookup Map file too small to be valid

    // Unexpected system errors

    seriousInternalOSError = -998,	// E: syscall failure
    seriousInternalRoutingError = -999	// E: Response returned to wrong client
  };

  class faultImpl;

  class fault {
  public:
    fault();
    virtual ~fault() = 0;

    bool	hasFault() const;
    faultCode 	getFaultCode() const;

    const std::string	getMessage(const char* pre=0, bool longFormat=true) const;
    const std::string	getMessage(const std::string& pre, bool longFormat=true) const;

  private:
    fault&    operator=(const fault& rhs);          // Assign not ok
    fault(const fault& rhs);                        // Copy not ok

  protected:
    faultImpl*	_impl;
  };
}

#endif
