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

#ifndef _REPORTINGCHANNEL_H
#define _REPORTINGCHANNEL_H

//////////////////////////////////////////////////////////////////////
// The reporting channel is where the process writes events back to
// the plutonManager. Events are a fixed size so that: a) they are
// small enough to be a contiguous write and b) so that the reader can
// be a little lazy.
//
// Performance reports occur a lot, so the reports are aggregated into
// an array to dramatically reduce the number of context switches and
// system calls. The price for this optimization is that the manager
// is mostly behind in the performance state of a service. Since this
// data is mostly used for aggregate reporting, that's no big deal and
// for timely information, the service API and manager use shm.
//
// The whole structure *must* not be larger than PIPE_BUF (stdio.h)
// otherwise the write through the reporting channel is not guaranteed
// to be contiguous. Since multiple service instances write to the
// same pipe, this is important.
//////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <limits.h>
#include <sys/types.h>

namespace pluton {
  namespace reportingChannel {

    static const int maximumPerformanceEntries = 15;
    enum recordType { unknown=1, performanceReport };
    enum reportReason { readError=1, writeError, fault, ok };

    typedef struct {
      reportReason 	reason;
      unsigned int	durationMicroSeconds;
      int		requestLength;
      int		responseLength;
      char	function[16];
    } performanceDetails;

    class performanceData {
      public:
      int			entries;
      performanceDetails	rqL[pluton::reportingChannel::maximumPerformanceEntries];
    };

    class idleData {
    public:
    };

    class allBusyData {
    public:
    };

    class record {
    public:
      record() { assert(PIPE_BUF > sizeof(pluton::reportingChannel::record)); }
      recordType 	type;
      int		pid;

      union {
	performanceData	pd;
	idleData	id;
	allBusyData	ab;
      } U;
    };
  }
}

#endif
