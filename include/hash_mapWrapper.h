#ifndef _HASHMAPWRAPPER_H
#define _HASHMAPWRAPPER_H

//////////////////////////////////////////////////////////////////////
// Encapsulate the differences between various OSes for defining an
// STL hash_map.
//////////////////////////////////////////////////////////////////////

#if defined(__FreeBSD__) && (__FreeBSD__ < 6)
	#include <hash_map>
        using std::hash_map;
#else
	#include <ext/hash_map>
	using __gnu_cxx::hash_map;
#endif

#endif
