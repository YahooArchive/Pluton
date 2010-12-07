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
