#include <iostream>

#include <stdlib.h>
#include <string.h>

#include "netString.h"

using namespace std;

typedef struct {
  int	resetSize;		// Input to addBytes
  string	readData;

  int	expectedExpandTo;	// Expected from getReadParameters
  int	expectedBufferOffset;
  int	expectedMaxToRead;

  char	expectedNSType;		// '.' == not expected
  string	expectedValue;

} testSrc;


int
main()
{

  char*	buffer = 0;
  netStringFactory* nsf = 0;

  string testID;
  while (!cin.eof()) {

    testSrc	S;

    testID.erase();
    cin >> testID;
    if (cin.eof()) break;
    if ((testID.length() == 0) || (testID[0] == '#')) {
      getline(cin, testID);	// Slurp to end of line
      continue;
    }

    cin >> S.resetSize
	>> S.readData
	>> S.expectedExpandTo
	>> S.expectedBufferOffset
	>> S.expectedMaxToRead
	>> S.expectedNSType
	>> S.expectedValue;

    if (S.resetSize == -1) break;

    if (S.resetSize) {
      if (nsf) delete nsf;
      if (buffer) delete [] buffer;
      buffer = new char[S.resetSize];
      nsf = new netStringFactory(buffer, S.resetSize);
    }

    char* bufPtr;
    int minToRead, maxToRead;		// How much is needed?
    int expandTo = nsf->getReadParameters(bufPtr, minToRead, maxToRead);

    if (expandTo != S.expectedExpandTo) {
      cerr << testID << ": Unexpected expandTo of " << expandTo
	   << " expected " << S.expectedExpandTo << endl;
      exit(1);
    }

    int bufferOffset = bufPtr - buffer;
    if (bufferOffset != S.expectedBufferOffset) {
      cerr << testID << ": Unexpected offset of " << bufferOffset
	   << " expected " << S.expectedBufferOffset << endl;
      exit(1);
    }

    if (maxToRead != S.expectedMaxToRead) {
      cerr << testID << ": Unexpected maxToRead of " << maxToRead
	   << " expected " << S.expectedMaxToRead << endl;
      exit(1);
    }

    // Ok, good to read, read in any data

    if (S.readData.length()) {
      strcpy(bufPtr, S.readData.c_str());
      nsf->addBytesRead(S.readData.length());
    }


    // See if there is a netstring available

    const char* err;
    bool haveNSFlag = nsf->haveNetString(err);
    if (err) {
      cerr << testID << ": Unexpected err=" << err << endl;
      exit(1);
    }

    if ((S.expectedNSType != '.') != haveNSFlag) {
      cerr << testID << ": Expected haveNS state != "
	   << ((haveNSFlag) ? "true" : "false")
	   << " expected " << S.expectedNSType << endl;
      exit(1);
    }

    if (haveNSFlag) {
      char nsType;
      const char* nsPtr;
      int nsLength;

      nsf->getNetString(nsType, nsPtr, nsLength);
      if (nsType != S.expectedNSType) {
	cerr << testID << ": Unexpected nsType of " << nsType
	     << " expected " << S.expectedNSType << endl;
	exit(1);
      }

      if (nsLength != (signed) S.expectedValue.length()) {
	cerr << testID << ": Unexpected nsLength of " << nsLength
	     << " expected " << S.expectedValue.length() << endl;
	exit(1);
      }

      if (strncmp(nsPtr, S.expectedValue.c_str(), nsLength) != 0) {
	cerr << testID << ": Unexpected nsValue of " << string(nsPtr, nsLength)
	     << " expected " << S.expectedValue.c_str() << endl;
	exit(1);
      }
    }
    cout << testID << ": ok" << endl;
  }

  int a, b, c, d, e, f;

  nsf->getStatistics(a, b, c, d, e, f);

  cout << "NS stats: bytesRead=" << a
       << " bytesRelocated=" << b
       << " netStringsFound=" << c
       << " bytesSkipped=" << d
       << " ZcostEesets=" << e
       << " UserReallocs=" << f
       << endl;
}
