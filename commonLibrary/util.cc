#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "util.h"


////////////////////////////////////////////////////////////////////////////////
// Convert a long to a C string. Return a pointer to the
// string. Caller must supply a buffer big enough to handle the
// largest string. The returned point will point somewhere within the
// buffer.
////////////////////////////////////////////////////////////////////////////////

const char*
util::ltoa(IA& iap, long ival)
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


////////////////////////////////////////////////////////////////////////////////
// Convert a long to an std::string. Inefficient, but easy.
////////////////////////////////////////////////////////////////////////////////

std::string
util::ltos(long i)
{
  IA	ia;

  const char* cp = ltoa(ia, i);

  std::string s(cp);

  return s;
}


//////////////////////////////////////////////////////////////////////
// Convert seconds to WDXhYmZs for human consumption. Eg, 87304 will
// results in an output of 1D2h3m4s.
//////////////////////////////////////////////////////////////////////

const std::string&
util::durationToEnglish(int ss, std::string& result)
{
  result.erase();
  int days = ss / (24 * 60 * 60);
  ss -= days * 24 * 60 * 60;

  int hh = ss / (60 * 60);
  ss -= hh * 60 * 60;

  int mm = ss / 60;
  ss -= mm * 60;

  if (days > 0) {
    result += ltos(days);
    result += "D";
  }

  if (hh > 0) {
    result += ltos(hh);
    result += "h";
  }

  if (mm > 0) {
    result += ltos(mm);
    result += "m";
  }

  if ((ss > 0) || result.empty()) {
    result += ltos(ss);
    result += "s";
  }

  return result;
}


////////////////////////////////////////////////////////////////////////////////
// Convert a string like 1h to an int of seconds. Ditto for 2m, 3d and 4s.
////////////////////////////////////////////////////////////////////////////////

extern	int
util::englishToDuration(std::string duration, std::string& em)
{
  em.erase();
  int seconds = atoi(duration.c_str());

  std::string::size_type multiplier = duration.find_first_of("shmd");
  if (multiplier != std::string::npos) {
    switch (duration[multiplier]) {
    case 's': break;
    case 'm': seconds *= 60; break;
    case 'h': seconds *= 60 * 60; break;
    case 'd': seconds *= 60 * 60 * 24; break;
    default:
      em += "Error: Time multiplier of '";
      em += duration[multiplier];
      em += "' must be one of 's', 'm', 'h' or 'd'";
      return -1;
    }
  }

  return seconds;
}

//////////////////////////////////////////////////////////////////////
// Convert an integer value to a rounded human K, M, G type
// value. Accurate to within 10% due to displaying one decimal place.
//////////////////////////////////////////////////////////////////////

const std::string&
util::intToEnglish(long val, std::string& result)
{
  long point;

  result.erase();
  if (val < 1000) {
    result = ltos(val);
    return result;
  }

  point = val % 1000;
  val /= 1000;
  if (val < 1000) {
    result = ltos(val);
    if ((val < 10) && (point >= 100)) {
      result += ".";
      result += ltos(point / 100);
    }
    result += "K";
    return result;
  }

  point = val % 1000;
  val /= 1000;
  if (val < 1000) {
    result = ltos(val);
    if ((val < 10) && (point >= 100)) {
      result += ".";
      result += ltos(point / 100);
    }
    result += "M";
    return result;
  }

  point = val % 1000;
  val /= 1000;
  result = ltos(val);
  if ((val < 10) && (point >= 100)) {
    result += ".";
    result += ltos(point / 100);
  }
  result += "G";
  return result;
}


//////////////////////////////////////////////////////////////////////
// Caculate a percentage and be aware of the fact that the supplied
// numbers may be very close to the limits of long (thus denom * 100
// may overflow).
//////////////////////////////////////////////////////////////////////

long
util::outPercentage(long initDenom, long initNumer)
{
  if (initNumer <= 0) return 0;
  if (initDenom == initNumer) return 100;
  if (initDenom == 0) return 0;

  long denom = initDenom;
  long numer = initNumer;

  // Scale depending on which end of the int range we're at

  if (numer < 10000) {
    denom *= 100;
  }
  else {
    numer /= 100;
  }

  denom /= numer;

  if (denom == 100) denom = 99;		// Equality optimized earlier
  if (denom == 0) denom = 1;		// Zeros caught earlier

  return denom;
}


////////////////////////////////////////////////////////////////////////////////
// Construct a string containing the message and the errno in decimal
////////////////////////////////////////////////////////////////////////////////

const std::string&
util::messageWithErrno(std::string& em, const char* message, const std::string& path)
{
  return util::messageWithErrno(em, message, path.c_str(), 0);
}

const std::string&
util::messageWithErrno(std::string& em, const char* message, const char* path, int additionalRC)
{
  std::ostringstream os;

  if (message && *message) os << message;

  if (path && *path) {
    if (os.str().length()) os << ": ";
    os << "Path=" << path;
  }

  if (os.str().length()) os << ": ";
  os << "errno=" << errno << " " << strerror(errno);

  if (additionalRC != 0) os << "/" << additionalRC;

  em = os.str();

  return em;
}


//////////////////////////////////////////////////////////////////////
// A = B - C (B must be greater than C for sensible results)
//////////////////////////////////////////////////////////////////////

extern void
util::timevalDiff(timeval& A, const timeval& B, const timeval& C)
{
  A = B;
  A.tv_sec -= C.tv_sec;
  A.tv_usec -= C.tv_usec;
  if (A.tv_usec < 0) {
    A.tv_sec--;
    A.tv_usec += util::MICROSECOND;
  }
}


//////////////////////////////////////////////////////////////////////
// Return <0 if A < B, == 0 if A == B, > 0 if A > B
//////////////////////////////////////////////////////////////////////

extern int
util::timevalCompare(const timeval& A, const timeval& B)
{
  if (A.tv_sec < B.tv_sec) return -1;
  if (A.tv_sec > B.tv_sec) return 1;

  if (A.tv_usec < B.tv_usec) return -1;
  if (A.tv_usec > B.tv_usec) return 1;

  return 0;
}


//////////////////////////////////////////////////////////////////////
// A += B
//////////////////////////////////////////////////////////////////////

extern void
util::timevalAdd(timeval& A, const timeval& B)
{
  A.tv_sec += B.tv_sec;
  A.tv_usec += B.tv_usec;
  while (A.tv_usec > util::MICROSECOND) {
    ++A.tv_sec;
    A.tv_usec -= util::MICROSECOND;
  }
}


//////////////////////////////////////////////////////////////////////

extern void
util::timevalNormalize(timeval& A)
{
  if (A.tv_usec >= util::MICROSECOND) {
    A.tv_sec += A.tv_usec / util::MICROSECOND;
    A.tv_usec %= util::MICROSECOND;
  }
}


//////////////////////////////////////////////////////////////////////
// Return A = B - C (B must be greater than C for sensible results)
//////////////////////////////////////////////////////////////////////

extern long
util::timevalDiffMS(const timeval& B, const timeval& C)
{
  timeval A;
  util::timevalDiff(A, B, C);
  return A.tv_sec * MILLISECOND + A.tv_usec / MILLISECOND;
}


//////////////////////////////////////////////////////////////////////
// Return A = B - C in microseconds (B must be greater than C for
// sensible results)
//////////////////////////////////////////////////////////////////////

extern long
util::timevalDiffuS(const timeval& B, const timeval& C)
{
  timeval A;
  util::timevalDiff(A, B, C);
  return A.tv_sec * MICROSECOND + A.tv_usec;
}


//////////////////////////////////////////////////////////////////////
// Given a binary stream, return a hex string
//////////////////////////////////////////////////////////////////////

static char hexList[] = "0123456789ABCDEF";

void
util::hex(const unsigned char* source, unsigned int len, std::string& destination)
{
  destination.erase();
  unsigned int ix;
  for (ix=0; ix < len; ++ix) {
    destination += hexList[(source[ix] & 0xF0) >> 4];
    destination += hexList[source[ix] & 0xF];
  }
}


//////////////////////////////////////////////////////////////////////
// Undo util::hex. This routines assume a contiguous range from '0'
// to '9' and 'A' to 'F'. ASCII, EBCDIC and FIELDDATA all meet that
// constraint. If you're running this code on a machine with another
// character set, all bets are off!
//////////////////////////////////////////////////////////////////////

int
util::dehex(unsigned char* destination, unsigned int destinationSize,
	    const unsigned char* source, unsigned int sourceLength)
{
  int destinationLen = 0;
  while ((destinationSize > 0) && (sourceLength > 1)) {
    unsigned char h1 = *source++;
    unsigned char h2 = *source++;
    char c = 0;
    sourceLength -= 2;		// It'd be cute to use --sourceLength--; Oh well.
    if ((h1 >= '0') && (h1 <= '9')) c = (h1 - '0') << 4;
    if ((h1 >= 'A') && (h1 <= 'F')) c = (h1 - 'A' + 10) << 4;
    if ((h2 >= '0') && (h2 <= '9')) c |= h2 - '0';
    if ((h2 >= 'A') && (h2 <= 'F')) c |= h2 - 'A' + 10;

    *destination++ = c;
    ++destinationLen;
    --destinationSize;
  }

  return destinationLen;
}


//////////////////////////////////////////////////////////////////////
// Set the send and recv buffers as close to the supplied limit as
// possible.
//////////////////////////////////////////////////////////////////////

void
util::increaseOSBuffer(int fd, int maxRecvSize, int maxSendSize)
{
  int sz;
  socklen_t msl = sizeof(sz);

  int res = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &sz, &msl);
  if ((res == -1) || (sz < maxRecvSize)) {	// only change if it results in an increase
    while (maxRecvSize > sz) {
      msl = sizeof(maxRecvSize);
      res = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &maxRecvSize, msl);
      if (res == 0) break;
      maxRecvSize /= 2;
    }
  }

  msl = sizeof(sz);
  res = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sz, &msl);
  if ((res == -1) || (sz < maxSendSize)) {	// only change if it results in an increase
    while (maxSendSize > sz) {
      msl = sizeof(maxSendSize);
      res = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &maxSendSize, msl);
      if (res == 0) break;
      maxSendSize /= 2;
    }
  }
}


int
util::setNonBlocking(int fd)
{
  return fcntl(fd, F_SETFL, O_NONBLOCK);
}

int
util::setCloseOnExec(int fd)
{
  return fcntl(fd, F_SETFD, FD_CLOEXEC);
}
