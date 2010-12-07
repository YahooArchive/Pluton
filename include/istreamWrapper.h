#ifndef _ISTREAMWRAPPER_H
#define _ISTREAMWRAPPER_H

//////////////////////////////////////////////////////////////////////
// Encapsulate OS differences for defining an STL istream.
//////////////////////////////////////////////////////////////////////

#if defined(__FreeBSD__) && (__FreeBSD__ < 6)
	#include <istream.h>
#else
	#include <istream>
#endif

#endif
