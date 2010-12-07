#ifndef _LOCALEWRAPPER_H
#define _LOCALEWRAPPER_H

//////////////////////////////////////////////////////////////////////
// Encapsulate OS differences for defining access to locale.
//////////////////////////////////////////////////////////////////////

#if defined(__FreeBSD__) && (__FreeBSD__ < 6)
	#include <locale.h>
	typedef int_int type;
#else
	#include <locale>
#endif


#endif
