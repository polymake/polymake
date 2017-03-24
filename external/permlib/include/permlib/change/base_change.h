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


#ifndef BASECHANGE_H_
#define BASECHANGE_H_

#include <permlib/predicate/pointwise_stabilizer_predicate.h>

namespace permlib {

/// abstract base class for base change algorithms
/**
 * This base class cannot contain a virtual change function unless
 *  the input is given explicitly instead of iterators
 */
template<class PERM,class TRANS>
class BaseChange {
public:
	/// constructor
    BaseChange() : m_statTranspositions(0), m_statScheierGeneratorsConsidered(0) {}
	
	/// nuber of base transpositions needed since construction
	mutable unsigned int m_statTranspositions;
	
	/// nuber of Schreier generators considered in transposition since construction
	mutable unsigned int m_statScheierGeneratorsConsidered;
protected:
	/// checks if insertion of a base point at given position is redundant
	/**
	 * @param bsgs
	 * @param baseTargetPos designated insertion position
	 * @param alpha designated base point
	 */
	bool isRedundant(const BSGSCore<PERM,TRANS>& bsgs, unsigned int baseTargetPos, unsigned long alpha) const;
};

template<class PERM,class TRANS>
bool BaseChange<PERM,TRANS>::isRedundant(const BSGSCore<PERM,TRANS>& bsgs, unsigned int baseTargetPos, unsigned long alpha) const {
	bool redundant = true;
	const PointwiseStabilizerPredicate<PERM> stab_i(bsgs.B.begin(), bsgs.B.begin() + baseTargetPos);
	BOOST_FOREACH(const typename PERM::ptr& g, bsgs.S) {
		if (!stab_i(g))
			continue;
		if (*g / alpha != alpha) {
			redundant = false;
			break;
		}
	}
	
#ifdef PERMLIB_DEBUG_OUTPUT
	if (redundant) {
		std::cout << "skip redundant " << (alpha+1) << std::endl;
		print_iterable(bsgs.B.begin(), bsgs.B.begin() + baseTargetPos, 1, " redundant for");
	} else {
		std::cout << "look for " << (alpha+1) << " at position " << baseTargetPos << std::endl;
	}
#endif

	return redundant;
}

}

#endif // -- BASECHANGE_H_
