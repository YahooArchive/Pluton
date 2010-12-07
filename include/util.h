#ifndef _UTIL_H
#define	_UTIL_H


//////////////////////////////////////////////////////////////////////
// This file defines a bunch of generic routes and values that are not
// specific to any one application.
//////////////////////////////////////////////////////////////////////

#include <string>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/param.h>

#include <errno.h>
#include <inttypes.h>

namespace util {

  static const int	MICROSECOND = 1000000;			// Microseconds in a second
  static const int	MILLISECOND = 1000;			// Millis in a second
  static const int	MICROMILLI = 1000;			// Micros in a milli

  typedef struct {
    char	buf[40];	// 10^39 is > 2^128 so good until 256bit computing
  } IA;

#if !defined(ETIME)
#if defined(ETIMEDOUT)
	#define ETIME ETIMEDOUT
  #else
	#define ETIME EAGAIN
  #endif
#endif

  inline bool   retryNonBlockIO(int e)
  {
    return (e == EINTR) || (e == EAGAIN) || (e == ENOBUFS) || (e == ETIME);
  }

  std::string	ltos(long i);				// Long to std::string
  const	char* 	ltoa(IA& iap, long ival);		// Long to IA

  //////////////////////////////////////////////////////////////////////
  // Convert values to nice "English" looking strings. Eg, convert
  // values like 121 to "2m1s".
  //////////////////////////////////////////////////////////////////////

  const std::string& 	durationToEnglish(int ss, std::string& result);
  const std::string&	intToEnglish(long val, std::string& resut);
  int		englishToDuration(std::string duration, std::string &message);
  long		outPercentage(long initDenom, long initNumer);

  //////////////////////////////////////////////////////////////////////
  // Generate error messages that include the errno value and message.
  //////////////////////////////////////////////////////////////////////

  const std::string& 	messageWithErrno(std::string &em, const char *, const char* path=0,
					 int additionalRC=0);
  const std::string& 	messageWithErrno(std::string &em, const char *, const std::string& path);

  void		splitInterface(const char* inputstr, std::string& Interface, int& port,
			       const char* defaultInterface, int defaultPort);

		// A = B - C
  void		timevalDiff(timeval& A, const timeval& B, const timeval& C);

		// A += B
  void		timevalAdd(timeval& A, const timeval& B);

		// Return: <0 if A < B, == 0 if A == B, > 0 if A > B
  int		timevalCompare(const timeval& A, const timeval& B);

		// milliseconds = endTime - startTime
  long		timevalDiffMS(const timeval& endTime, const timeval& startTime);

		// Return: microseconds = endTime - startTime
  long		timevalDiffuS(const timeval& endTime, const timeval& startTime);

		// Adjust so that usecs is always lt a second
  void		timevalNormalize(timeval& A);

  void 		hex(const unsigned char* source, unsigned int sourceLength,
		    std::string& destination);
  int 		dehex(unsigned char* destination, unsigned int destinationSize,
		      const unsigned char* source, unsigned int sourceLength);

  void		increaseOSBuffer(int fd, int maxRecvSize, int maxSendSize);

  int		setNonBlocking(int fd);
  int		setCloseOnExec(int fd);
}

#endif
