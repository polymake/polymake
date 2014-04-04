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


#ifndef SETIMAGEREFINEMENT_H_
#define SETIMAGEREFINEMENT_H_

#include <permlib/predicate/pointwise_stabilizer_predicate.h>

#include <permlib/search/partition/partition.h>
#include <permlib/search/partition/refinement.h>

#include <algorithm>
#include <boost/dynamic_bitset.hpp>
#include <boost/foreach.hpp>

namespace permlib {
namespace partition {

/// concrete \f$\mathcal P\f$-refinements for set image
/// @see SetImagePredicate
template<class PERM>
class SetImageRefinement : public Refinement<PERM> {
public:
	/// constructor
	/**
	 * @param n cardinality of the set the group acts on
	 * @param begin iterator(unsigned long) begin of the set \f$\Delta\f$
	 * @param end iterator(unsigned long) end of the set \f$\Delta\f$
	 * @param beginImg iterator(unsigned long) begin of the set \f$\Gamma\f$
	 * @param endImg iterator(unsigned long) end of the set \f$\Gamma\f$
	 */
	template<class InputIterator>
	SetImageRefinement(unsigned long n, InputIterator begin, InputIterator end, InputIterator beginImg, InputIterator endImg);
	
	virtual unsigned int apply(Partition& pi) const;
	virtual unsigned int apply2(Partition& pi, const PERM& t) const;
	
	virtual bool init(Partition& pi);
private:
	std::vector<unsigned long> delta;
	std::vector<unsigned long> gamma;
};

template<class PERM>
template<class InputIterator>
SetImageRefinement<PERM>::SetImageRefinement(unsigned long n, InputIterator begin, InputIterator end, InputIterator beginImg, InputIterator endImg) 
	: Refinement<PERM>(n, Default), delta(begin, end), gamma(beginImg, endImg)
{
	std::sort(delta.begin(), delta.end());
	std::sort(gamma.begin(), gamma.end());
}

template<class PERM>
unsigned int SetImageRefinement<PERM>::apply(Partition& pi) const {
	BOOST_ASSERT( this->initialized() );
	unsigned int ret = 0;
	BOOST_FOREACH(unsigned int cell, Refinement<PERM>::m_cellPairs) {
		PERMLIB_DEBUG(std::cout << "apply set image1 " << cell << std::endl;)
		if (pi.intersect(delta.begin(), delta.end(), cell))
			++ret;
	}
	return ret;
}

template<class PERM>
unsigned int SetImageRefinement<PERM>::apply2(Partition& pi, const PERM& t) const {
	BOOST_ASSERT( this->initialized() );
	unsigned int ret = 0;
	BOOST_FOREACH(unsigned int cell, Refinement<PERM>::m_cellPairs) {
		PERMLIB_DEBUG(std::cout << "apply set image2 " << cell << std::endl;)
		if (pi.intersect(gamma.begin(), gamma.end(), cell))
			++ret;
	}
	return ret;
}

template<class PERM>
bool SetImageRefinement<PERM>::init(Partition& pi) {
	for (unsigned int c = 0; c < pi.cells(); ++c) {
		if (pi.intersect(delta.begin(), delta.end(), c))
			Refinement<PERM>::m_cellPairs.push_back(c);
	}
	if (!Refinement<PERM>::m_cellPairs.empty()) {
		typename Refinement<PERM>::RefinementPtr ref(new SetImageRefinement<PERM>(*this));
		Refinement<PERM>::m_backtrackRefinements.push_back(ref);
		return true;
	}
	return false;
}

}
}

#endif // -- SETIMAGEREFINEMENT_H_
