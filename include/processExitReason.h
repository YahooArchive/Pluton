#ifndef _PROCESSEXITREASON_H
#define _PROCESSEXITREASON_H

//////////////////////////////////////////////////////////////////////
// Services and the manager use a common set of exit reasons to
// communicate to each other.
//////////////////////////////////////////////////////////////////////

namespace processExit {
  enum  reason { noReason=0,
		 serviceShutdown, unresponsive, abnormalExit, lostIO, runawayChild,
		 maxRequests, excessProcesses, idleTimeout, acceptFailed,
		 maxReasonCount };
}

#endif
