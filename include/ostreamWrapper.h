#ifndef _OSTREAMWRAPPER_H
#define _OSTREAMWRAPPER_H

//////////////////////////////////////////////////////////////////////
// Encapsulate OS differences for defining an STL ostream.
//////////////////////////////////////////////////////////////////////

#if defined(__FreeBSD__) && (__FreeBSD__ < 6)
	#include <ostream.h>
#else
	#include <ostream>
#endif

#endif
