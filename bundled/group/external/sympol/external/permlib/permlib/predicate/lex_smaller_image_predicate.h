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


#ifndef LEXSMALLERIMAGE_PREDICATE_H_
#define LEXSMALLERIMAGE_PREDICATE_H_

#include <permlib/predicate/subgroup_predicate.h>

#include <set>
#include <boost/foreach.hpp>
#include <boost/dynamic_bitset.hpp>

namespace permlib {

/// coset-type predicate for group elements that map one set of zeros and ones to a lex-smaller set (w.r.t. to the indices)
/**
 * holds for a \f$g\f$ such that \f$val(\Delta^g) <_{lex} val(\Delta)\f$
 */
template <class PERM>
class LexSmallerImagePredicate : public SubgroupPredicate<PERM> {
public:
	/// constructor
	/**
	 * @param n          number of points the permutations act on
	 * @param zerosBegin begin iterator to the list of indices with 0-value
	 * @param zerosEnd   end   iterator to the list of indices with 0-value
	 * @param onesBegin  begin iterator to the list of indices with 1-value
	 * @param onesEnd    end   iterator to the list of indices with 1-value
	 */
	template<class ForwardIterator>
	LexSmallerImagePredicate(unsigned int n, ForwardIterator zerosBegin, ForwardIterator zerosEnd, ForwardIterator onesBegin, ForwardIterator onesEnd);

	virtual bool operator()(const PERM &p) const;
	virtual bool childRestriction(const PERM &h, unsigned int i, unsigned long beta_i) const;
	virtual unsigned int limit() const;
private:
	boost::dynamic_bitset<> m_zeros;
	boost::dynamic_bitset<> m_ones;
	unsigned long m_fixed;
};

//
//     ----       IMPLEMENTATION
//

template <class PERM>
template<class ForwardIterator>
LexSmallerImagePredicate<PERM>::LexSmallerImagePredicate(unsigned int n, ForwardIterator zerosBegin, ForwardIterator zerosEnd, ForwardIterator onesBegin, ForwardIterator onesEnd)
	: m_zeros(n), m_ones(n), m_fixed(0)
{ 
	while (zerosBegin != zerosEnd) {
		m_zeros.set(*zerosBegin++, 1);
		++m_fixed;
	}
	while (onesBegin != onesEnd) {
		m_ones.set(*onesBegin++, 1);
		++m_fixed;
	}
}


template <class PERM>
bool LexSmallerImagePredicate<PERM>::operator()(const PERM &p) const {
	for (unsigned int i = 0; i < p.size(); ++i) {
		const dom_int pi = p / i;
		if (pi == i)
			continue;
		if (m_ones[i] && m_zeros[pi]) {
			PERMLIB_DEBUG( std::cout << i << " , " << pi << "  !0 ->  0  TRUE" << std::endl; )
			return true;
		}
		if (m_zeros[i] && !m_zeros[pi]) {
			PERMLIB_DEBUG( std::cout << i << " , " << pi << "  0  -> !0  FALSE" << std::endl; )
			return false;
		}
	}
	return false;
}

template <class PERM>
bool LexSmallerImagePredicate<PERM>::childRestriction(const PERM &h, unsigned int i, unsigned long beta_i) const {
	// Because limit() does not depend on h, we have to check at every node
	// whether we have already found a element, which maps the given sets to a
	// lexicographically smaller set
	if ((*this)(h)) {
		PERMLIB_DEBUG( std::cout << h << " is already the desired element; TRUE" << std::endl; )
		return true;
	}
	if (m_zeros[beta_i] && !m_zeros[h / beta_i]) {
		PERMLIB_DEBUG( std::cout << (h / beta_i) << " mapping zero " << beta_i << " to non-zero; FALSE" << std::endl; )
		return false;
	}
	return true;
}

template <class PERM>
unsigned int LexSmallerImagePredicate<PERM>::limit() const {
	return m_fixed;
}

}

#endif // -- LEXSMALLERIMAGE_PREDICATE_H_
