#include <iostream>

#include <assert.h>
#include <stdlib.h>

#include "rateLimit.h"

using namespace std;

int
main()
{
  util::rateLimit	rl1;			// One per second
  util::rateLimit	rl2(3, 2);		// Three in two seconds
  util::rateLimit	rl3;
  util::rateLimit 	rl4(1, 3);		// One in three seconds

  rl3.setRate(7);
  rl3.setTimeUnit(3);			// Seven in three seconds

  assert(rl1.getRate() == 1);
  assert(rl1.getTimeUnit() == 1);

  assert(rl2.getRate() == 3);
  assert(rl2.getTimeUnit() == 2);

  assert(rl3.getRate() == 7);
  assert(rl3.getTimeUnit() == 3);

  // Tick

  int sec = 0;
  assert(rl1.allowed(sec));		// 1
  assert(rl2.allowed(sec));		// 1
  assert(rl3.allowed(sec));		// 1
  assert(rl4.allowed(sec));		// 1

  assert(!rl1.allowed(sec));		// >1
  assert(rl2.allowed(sec));		// 2
  assert(rl3.allowed(sec));		// 2
  assert(!rl4.allowed(sec));		// >1

  assert(!rl1.allowed(sec));		// >1
  assert(rl2.allowed(sec));		// 2
  assert(rl3.allowed(sec));		// 3
  assert(!rl4.allowed(sec));		// >1

  // Tick

  sec = 1;
  assert(rl1.allowed(sec));		// 1
  assert(!rl2.allowed(sec));		// >2
  assert(rl3.allowed(sec));		// 4
  assert(!rl4.allowed(sec));		// >1

  // Tick

  sec = 2;
  assert(rl1.allowed(sec));		// 1
  assert(rl2.allowed(sec));		// 1
  assert(rl3.allowed(sec));		// 5
  assert(rl3.allowed(sec));		// 6
  assert(rl3.allowed(sec));		// 7
  assert(!rl3.allowed(sec));		// >7
  assert(!rl4.allowed(sec));		// 1

  // Tick

  sec = 3;
  assert(rl4.allowed(sec));		// 1

  cout << "RateLimit tests ok" << endl;
}
