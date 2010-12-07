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
