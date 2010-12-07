#include <iostream>

#include <stdlib.h>
#include <string.h>

#include "netString.h"

using namespace std;

static int
shouldBe(const char* testName, netStringGenerate& ns, const char* expectedStr)
{
  bool failed = false;

  cout << "testnsgenerate: " << testName << " ";
  int	expectedLength = strlen(expectedStr);
  int	nsLength = ns.length();
  const char* nsPtr = ns.data();

  if (nsLength != expectedLength) {
    cout << "failed. Expected Length=" << expectedLength << ", actual=" << nsLength << endl;
    failed = true;
  }

  if (memcmp(nsPtr, expectedStr, expectedLength) != 0) {
    cout << "failed. Mis-matched Data" << endl;
    cout << "\tExpected data=" << expectedStr << endl;
    cout << "\tActual data  =";
    cout.write(nsPtr, nsLength);
    cout << endl;
    failed = true;
  }

  if (failed) return 1;

  cout << "good." << endl;
  return 0;
}


//////////////////////////////////////////////////////////////////////

int
main()
{
  int errorCount = 0;

  {
    string s = "This is a string";
    netStringGenerate ns(s);
    ns.append("Four");
    errorCount += shouldBe("one - Supplied string constructor", ns, "This is a string4:Four,");
  }

  {
    netStringGenerate ns;
    ns.append("Four");
    errorCount += shouldBe("two - Internal string constructor", ns, "4:Four,");
  }

  {
    netStringGenerate ns;
    string expected;
    ns.append('A', "aa", 2);			expected += "2Aaa,";
    ns.append('B', "bb");			expected += "2Bbb,";

    char c = 'c';
    ns.append('C', c);				expected += "1Cc,";
    unsigned char d = 'd';
    ns.append('D', d);				expected += "1Dd,";

    ns.append('E');				expected += "0E,";

    ns.append('f', false);			expected += "1fn,";
    ns.append('t', true);			expected += "1ty,";

    unsigned int i2 = 456;
    ns.append('j', i2);				expected += "3j456,";
    int	i1 = -123;
    ns.append('i', i1);				expected += "4i-123,";
    long long i4 = -987654321;
    ns.append('n', i4);				expected += "10n-987654321,";
    unsigned long long i3 = 67890;
    ns.append('l', i3);				expected += "5l67890,";

    errorCount += shouldBe("three - different appenders", ns, expected.c_str());
    cout << ns << endl;
  }

////////////////////////////////////////
// Test error conditions
////////////////////////////////////////

  {
    netStringGenerate ns;

    if (ns.append('\t', "NO Questionmark allowed") >= 0) {
      cout << "Error: Accepts non-printable types" << endl;
      errorCount++;
    }
    else {
      cout << "testgenerate: four - bad type detected good." << endl;
    }

    if (ns.append('5', "NO numerics allowed") >= 0) {
      cout << "Error: Accepts numeric types" << endl;
      errorCount++;
    }
    else {
      cout << "testgenerate: five - numeric type detected good." << endl;
    }

    if (ns.append('x', (char *) NULL) >= 0) {
      cout << "Error: Accepts NULL ptr" << endl;
      errorCount++;
    }
    else {
      cout << "testgenerate: six - NULL ptr detected good." << endl;
    }

    if (ns.append((char *) NULL) >= 0) {
      cout << "Error: Accepts NULL ptr" << endl;
      errorCount++;
    }
    else {
      cout << "testgenerate: six - NULL ptr detected good." << endl;
    }

  }


  if (errorCount > 0) {
    cout << "FAILED=" << errorCount << " netStringGenerate." << endl;
    exit(1);
  }

  return(0);
}
