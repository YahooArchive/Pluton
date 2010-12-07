#ifndef _STREAMBUFWRAPPER_H
#define _STREAMBUFWRAPPER_H

//////////////////////////////////////////////////////////////////////
// Because this code has to work on new and legacy systems, some of
// the OS dependent stuff - particularly STL related - is moved to a
// seperate module so that #ifdef testing doesn't proliferate.
//////////////////////////////////////////////////////////////////////

#if defined(__FreeBSD__) && (__FreeBSD__ < 6)
	#include <streambuf.h>
#else
	#include <streambuf>
#endif

#endif
