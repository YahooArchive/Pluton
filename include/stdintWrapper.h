#ifndef _STDINTWRAPPER_H
#define _STDINTWRAPPER_H

//////////////////////////////////////////////////////////////////////
// Encapsulate OS differences for the include that defines fixed sized
// types such as uint32_t.
//////////////////////////////////////////////////////////////////////

#if defined(__FreeBSD__) && (__FreeBSD__ < 6)
	#include <sys/inttypes.h>
#else
	#include <stdint.h>
#endif

#endif
