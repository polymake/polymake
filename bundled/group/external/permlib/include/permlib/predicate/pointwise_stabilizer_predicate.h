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


#ifndef POINTWISESTABILIZERPREDICATE_H_
#define POINTWISESTABILIZERPREDICATE_H_

#include <boost/foreach.hpp>

namespace permlib {

/// predicate matching a permutation if it stabilizes a given list of points pointwise
template <class PERM>
class PointwiseStabilizerPredicate : public std::unary_function<typename PERM::ptr, bool> {
public:
	/// constructor
	/**
	 * @param begin begin iterator of points (unsigned long) that the permutation is checked against
	 * @param end   end   iterator of points (unsigned long) that the permutation is checked against
	 */
	template<class InputIterator>
	PointwiseStabilizerPredicate(InputIterator begin, InputIterator end) 
		: m_toStabilize(begin, end)
	{ }
	
	explicit PointwiseStabilizerPredicate(unsigned int x) 
		: m_toStabilize(1, x)
	{ }

	/// evaluate predicate
	bool operator()(const typename PERM::ptr &p) const {
		BOOST_FOREACH(unsigned long beta, m_toStabilize) {
			if (*p / beta != beta)
				return false;
		}
		return true;
	};
private:
	std::vector<dom_int> m_toStabilize;
};

}

#endif // -- POINTWISESTABILIZERPREDICATE_H_
