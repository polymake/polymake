// ---------------------------------------------------------------------------
//
//  This file is part of PermLib.
//
// Copyright (c) 2009-2011 Thomas Rehn <thomas@carmen76.de>
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ---------------------------------------------------------------------------


#ifndef COMMON_H_
#define COMMON_H_

#include <iostream>
#include <cstdlib>
#include <vector>

#if __cplusplus < 201103L
namespace std {
	/// copies elements of (begin to end) to destBegin if they match the given predicate
	/**
	 * due to Meyers, Effective STL, p. 157 (Item 36)
	 */
	template<typename InputIterator,
			 typename OutputIterator,
			 typename Predicate>
	OutputIterator copy_if(InputIterator begin, InputIterator end, OutputIterator destBegin, Predicate p) {
		while (begin != end) {
			if (p(*begin))
				*destBegin++ = *begin;
			++begin;
		}
		return destBegin;
	}
}
#endif

namespace permlib {
#ifdef PERMLIB_DOMAIN_INT
	typedef unsigned int dom_int;
#else
	typedef unsigned short int dom_int;
#endif

	/// returns random integer 0 <= x < upperBound 
	inline unsigned int randomInt(unsigned int upperBound) {
		return std::rand() % upperBound;
	}

	template<class Iterator>
	inline void print_iterable(Iterator begin, Iterator end, int offset, const char* name) {
		std::cout << name << " : ";
		while (begin != end) {
			std::cout << (*begin++) + offset << ",";
		}
		std::cout << std::endl;
	}

	/// callable object to delete a pointer
	struct delete_object {
	  template <typename T>
	  void operator()(T *ptr){ delete ptr; }
	};

#ifdef PERMLIB_DEBUG_OUTPUT
#define PERMLIB_DEBUG(X) X
#else
#define PERMLIB_DEBUG(X)
#endif

}

#endif // -- COMMON_H_
