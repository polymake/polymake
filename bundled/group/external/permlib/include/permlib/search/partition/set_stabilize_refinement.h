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


#ifndef SETSTABILIZEREFINEMENT_H_
#define SETSTABILIZEREFINEMENT_H_

#include <permlib/predicate/pointwise_stabilizer_predicate.h>

#include <permlib/search/partition/partition.h>
#include <permlib/search/partition/refinement.h>

#include <algorithm>
#include <boost/dynamic_bitset.hpp>
#include <boost/foreach.hpp>

namespace permlib {
namespace partition {

/// concrete \f$\mathcal P\f$-refinements for set stabilization
template<class PERM>
class SetStabilizeRefinement : public Refinement<PERM> {
public:
	/// constructor
	template<class InputIterator>
	SetStabilizeRefinement(unsigned long n, InputIterator begin, InputIterator end);
	
	virtual unsigned int apply(Partition& pi) const;
	
	virtual bool init(Partition& pi);
private:
	std::vector<unsigned long> toStab;
};

template<class PERM>
template<class InputIterator>
SetStabilizeRefinement<PERM>::SetStabilizeRefinement(unsigned long n, InputIterator begin, InputIterator end) 
	: Refinement<PERM>(n, Default), toStab(begin, end)
{
	std::sort(toStab.begin(), toStab.end());
	PERMLIB_DEBUG(print_iterable(toStab.begin(), toStab.end(), 0, "to stab");)
}

template<class PERM>
unsigned int SetStabilizeRefinement<PERM>::apply(Partition& pi) const {
	BOOST_ASSERT( this->initialized() );
	unsigned int ret = 0;
	BOOST_FOREACH(unsigned int cell, Refinement<PERM>::m_cellPairs) {
		PERMLIB_DEBUG(std::cout << "apply set stab " << cell << std::endl;)
		if (pi.intersect(toStab.begin(), toStab.end(), cell))
			++ret;
	}
	return ret;
}

template<class PERM>
bool SetStabilizeRefinement<PERM>::init(Partition& pi) {
	for (unsigned int c = 0; c < pi.cells(); ++c) {
		if (pi.intersect(toStab.begin(), toStab.end(), c))
			Refinement<PERM>::m_cellPairs.push_back(c);
	}
	if (!Refinement<PERM>::m_cellPairs.empty()) {
		typename Refinement<PERM>::RefinementPtr ref(new SetStabilizeRefinement<PERM>(*this));
		Refinement<PERM>::m_backtrackRefinements.push_back(ref);
		return true;
	}
	return false;
}

}
}

#endif // -- SETSTABILIZEREFINEMENT_H_
