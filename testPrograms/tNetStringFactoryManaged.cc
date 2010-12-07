#include <iostream>

#include <stdlib.h>
#include <string.h>

#include "netString.h"

using namespace std;

typedef struct {
  int	resetSize;		// Input to addBytes
  string	readData;

  int	expectedMaxToRead;

  char	expectedNSType;		// '.' == not expected
  string	expectedValue;

} testSrc;



static void
showStats(netStringFactoryManaged* nsf)
{
  int a, b, c, d, e, f;

  nsf->getStatistics(a, b, c, d, e, f);

  cout << "NS stats: bytesRead=" << a
       << " bytesRelocated=" << b
       << " netStringsFound=" << c
       << " bytesSkipped=" << d
       << " ZcostResets=" << e
       << " UserReallocs=" << f
       << endl;
}

int
main()
{
  netStringFactoryManaged* nsf = 0;

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
	>> S.expectedMaxToRead
	>> S.expectedNSType
	>> S.expectedValue;

    if (S.resetSize == -1) break;

    if (S.resetSize) {
      if (nsf) {
	showStats(nsf);
	delete nsf;
      }
      nsf = new netStringFactoryManaged(S.resetSize);
    }

    char* bufPtr;
    int minToRead;
    int	maxToRead;

    int expandTo = nsf->getReadParameters(bufPtr, minToRead, maxToRead);
    if (expandTo != 0) {
      cerr << testID << ": Cannot getReadParameters" << endl;
      exit(1);
    }

    if (maxToRead < S.expectedMaxToRead) {
      cerr << testID << ": Unexpected maxToRead of " << maxToRead
	   << " expected >= " << S.expectedMaxToRead << endl;
      cout << "Final: " << *nsf << endl;
      exit(1);
    }

    if (maxToRead < (signed) S.readData.length()) {
      cout << testID << ": Warning maxToRead of " << maxToRead
	   << " is less than supplied data " << S.readData.length()
	   << " may cause assert() failure later" << endl;
    }

    // Ok, good to read, read in any data

    if ((maxToRead > 0) && S.readData.length()) {
      strcpy(bufPtr, S.readData.c_str());
      nsf->addBytesRead(S.readData.length());
    }


    // See if there is a netstring available

    const char* err;
    bool haveNSFlag = nsf->haveNetString(err);
    if (err) {
      cerr << testID << ": Unexpected err=" << err << endl;
      cout << "Final: " << *nsf << endl;
      exit(1);
    }

    if ((S.expectedNSType != '.') != haveNSFlag) {
      cerr << testID << ": Expected haveNS state != "
	   << ((haveNSFlag) ? "true" : "false")
	   << " expected " << S.expectedNSType << endl;
      cout << "Final: " << *nsf << endl;
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
	cout << "Final: " << *nsf << endl;
	exit(1);
      }

      if (nsLength != (signed) S.expectedValue.length()) {
	cerr << testID << ": Unexpected nsLength of " << nsLength
	     << " expected " << S.expectedValue.length() << endl;
	cout << "Final: " << *nsf << endl;
	exit(1);
      }

      if (strncmp(nsPtr, S.expectedValue.c_str(), nsLength) != 0) {
	cerr << testID << ": Unexpected nsValue of " << string(nsPtr, nsLength)
	     << " expected " << S.expectedValue.c_str() << endl;
	cout << "Final: " << *nsf << endl;
	exit(1);
      }
      cout << testID << ": ok NS=" << string(nsPtr, nsLength) << endl;
    }
    else {
      cout << testID << ": ok" << endl;
    }
  }

  showStats(nsf);
}
