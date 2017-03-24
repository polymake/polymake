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


#ifndef REDUNDANT_BASE_POINT_INSERTION_STRATEGY_H_
#define REDUNDANT_BASE_POINT_INSERTION_STRATEGY_H_

namespace permlib {

template <class PERM, class TRANS>
struct BSGS;

/// strategy for redundant base point insertion
template <class PERM, class TRANS>
class RedundantBasePointInsertionStrategy {
public:
	/// constructor
	/**
	 * @param bsgs BSGS to work on
	 */
	RedundantBasePointInsertionStrategy(const BSGS<PERM,TRANS> &bsgs) : m_bsgs(bsgs) {}

	// virtual destructor
	virtual ~RedundantBasePointInsertionStrategy() {}
	
	/// finds possible insertion point for base point
	/**
	 * @param beta base point to be inserted
	 * @param S_i generators for i-th fundamental orbit where i is the insert position found
	 * @return insert position; if negative then beta is already base point at position -$retVal-1
	 */
	virtual int findInsertionPoint(dom_int beta, std::list<typename PERM::ptr> &S_i) const = 0;
protected:
	/// BSGS to work on
	const BSGS<PERM,TRANS> &m_bsgs;
};

/// insertion position after first non-trivial transversal
template <class PERM, class TRANS>
class TrivialRedundantBasePointInsertionStrategy : public RedundantBasePointInsertionStrategy<PERM,TRANS> {
public:
	/// constructor
	TrivialRedundantBasePointInsertionStrategy(const BSGS<PERM,TRANS> &bsgs) : RedundantBasePointInsertionStrategy<PERM,TRANS>(bsgs) {}
	
	virtual int findInsertionPoint(dom_int beta, std::list<typename PERM::ptr> &S_i) const {
		const std::vector<dom_int> &B = RedundantBasePointInsertionStrategy<PERM,TRANS>::m_bsgs.B;
		const std::vector<TRANS> &U = RedundantBasePointInsertionStrategy<PERM,TRANS>::m_bsgs.U;
		for (unsigned int i=0; i<B.size(); ++i) {
			if (beta == B[i])
				return -i-1;
		}
		int pos = B.size();
		while (pos > 0 && U[pos-1].size() == 1)
			--pos;
		return pos;
	}
};

/// insertion position at first position i such that \f$G^{[i]}\f$ stabilizes beta
template <class PERM, class TRANS>
class FirstRedundantBasePointInsertionStrategy : public RedundantBasePointInsertionStrategy<PERM,TRANS> {
public:
	/// constructor
	FirstRedundantBasePointInsertionStrategy(const BSGS<PERM,TRANS> &bsgs) : RedundantBasePointInsertionStrategy<PERM,TRANS>(bsgs) {}
	
	virtual int findInsertionPoint(dom_int beta, std::list<typename PERM::ptr> &S_i) const {
		const std::vector<dom_int> &B = RedundantBasePointInsertionStrategy<PERM,TRANS>::m_bsgs.B;
		const std::list<typename PERM::ptr> &S = RedundantBasePointInsertionStrategy<PERM,TRANS>::m_bsgs.S;
		typename std::vector<dom_int>::const_iterator bIt = B.begin();
		int pos = B.size();
		for (unsigned int i=0; i<B.size(); ++i) {
			if (beta == B[i])
				return -i-1;
			
			++bIt;
			const PointwiseStabilizerPredicate<PERM> stab_i(B.begin(), bIt);
			S_i.clear();

			//TODO: don't create temporary copy
			//      place directly into predicate
			std::copy_if(S.begin(), S.end(), std::back_inserter(S_i), stab_i);

			StabilizesPointPredicate<PERM> stab_beta(S_i.begin(), S_i.end());
			if (stab_beta(beta)) {
				pos = i+1;
				break;
			}
		}
		return pos;
	}
};

}

#endif // -- REDUNDANT_BASE_POINT_INSERTION_STRATEGY_H_
