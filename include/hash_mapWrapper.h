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

#ifndef P_HASHMAPWRAPPER_H
#define P_HASHMAPWRAPPER_H

//////////////////////////////////////////////////////////////////////
// Encapsulate the differences between various versions of STL
// defining a hash_map. Includers must have included config.h
//////////////////////////////////////////////////////////////////////

#ifdef HAVE_UNORDERED_MAP
	#include <unordered_map>
	#define P_STLMAP std::unordered_map
#elif defined(HAVE_TR1_UNORDERED_MAP)
	#include <tr1/unordered_map>
	#define P_STLMAP std::tr1::unordered_map
#elif defined(HAVE_HASH_MAP)
	#include <hash_map>
        #define P_STLMAP std::hash_map
#elif defined(HAVE_EXT_HASH_MAP)
	#include <ext/hash_map>
	#define P_STLMAP __gnu_cxx::hash_map
#else
	#error "No hash_map or unordered_map available - whimper"
#endif

#endif
