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


#ifndef CONJUGATINGBASECHANGE_H_
#define CONJUGATINGBASECHANGE_H_

#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/cstdint.hpp>

#include <permlib/change/base_change.h>

namespace permlib {

template<class PERM>
struct SymmetricGroup;

template<class PERM,class TRANS>
struct BSGS;

/// base change by conjugation and, if necessary, transpositions
template<class PERM, class TRANS, class BASETRANSPOSE>
class ConjugatingBaseChange : public BaseChange<PERM,TRANS> {
public:
	/// constructor
	explicit ConjugatingBaseChange(const BSGSCore<PERM,TRANS>&);
	
	/// changes base of bsgs so that it starts with the sequence given by baseBegin to baseEnd
	template <class InputIterator>
	unsigned int change(BSGS<PERM,TRANS> &bsgs, InputIterator baseBegin, InputIterator baseEnd, bool skipRedundant = false) const;
	
	/// changes base of symmetric group so that it starts with the sequence given by baseBegin to baseEnd
	template <class InputIterator>
	unsigned int change(SymmetricGroup<PERM> &bsgs, InputIterator baseBegin, InputIterator baseEnd, bool skipRedundant = false) const;
};

template<class PERM, class TRANS, class BASETRANSPOSE>
ConjugatingBaseChange<PERM,TRANS,BASETRANSPOSE>::ConjugatingBaseChange(const BSGSCore<PERM,TRANS>&) 
	: BaseChange<PERM,TRANS>() 
{ }

template<class PERM, class TRANS, class BASETRANSPOSE>
template<class InputIterator>
unsigned int ConjugatingBaseChange<PERM,TRANS,BASETRANSPOSE>::change(BSGS<PERM,TRANS> &bsgs, InputIterator baseBegin, InputIterator baseEnd, bool skipRedundant) const {
	if (baseBegin == baseEnd)
		return 0;
	
	const boost::uint64_t origOrder __attribute__((unused)) = bsgs.order();
	BASETRANSPOSE trans;
	PERM c(bsgs.n);
	PERM cInv(bsgs.n);
	/// true iff we multiply c with another permutation (and thus c is no longer with absolute certainty the identity)
	bool touchedC = false;
	
	unsigned int baseTargetPos = 0;
	while (baseBegin != baseEnd && baseTargetPos < bsgs.B.size()) {
		const unsigned long alpha = cInv.at(*baseBegin);
		const unsigned long beta = bsgs.B[baseTargetPos];
		const bool redundant = skipRedundant && this->isRedundant(bsgs, baseTargetPos, alpha);
		
		if (!redundant && beta != alpha) {
			boost::scoped_ptr<PERM> r(bsgs.U[baseTargetPos].at(alpha));
			if (r) {
				c ^= *r;
				cInv = ~c;
				touchedC = true;
			} else {
				unsigned int pos = bsgs.insertRedundantBasePoint(alpha, baseTargetPos);
				for (; pos > baseTargetPos; --pos) {
					trans.transpose(bsgs, pos-1);
					++BaseChange<PERM,TRANS>::m_statTranspositions;
				}
			}
		}
		if (!redundant)
			++baseTargetPos;
		++baseBegin;
	}
	
	// insert remaining base points
	while (!skipRedundant && baseBegin != baseEnd) {
		const unsigned long alpha = cInv.at(*baseBegin);
		bsgs.insertRedundantBasePoint(alpha, baseTargetPos);
		
		++baseBegin;
		++baseTargetPos;
	}
	
	if (touchedC) {
		// correct generators by conjugation
		BOOST_FOREACH(typename PERM::ptr& g, bsgs.S) {
			*g ^= cInv;
			*g *= c;
			g->flush();
		}
		
		// correct base points
		BOOST_FOREACH(dom_int& beta, bsgs.B) {
			beta = c.at(beta);
		}
	}
	
	// always strip redundant base points from the end of the new base
	bsgs.stripRedundantBasePoints(baseTargetPos);
	BaseChange<PERM,TRANS>::m_statScheierGeneratorsConsidered += trans.m_statScheierGeneratorsConsidered;
	
	if (touchedC) {
		for (unsigned int i=0; i<bsgs.U.size(); ++i) {
			bsgs.U[i].permute(c, cInv);
		}
	}

	BOOST_ASSERT(bsgs.B.size() == bsgs.U.size());
	BOOST_ASSERT(origOrder == bsgs.order());
	
	return baseTargetPos;
}

template<class PERM, class TRANS, class BASETRANSPOSE>
template<class InputIterator>
unsigned int ConjugatingBaseChange<PERM,TRANS,BASETRANSPOSE>::change(SymmetricGroup<PERM> &bsgs, InputIterator baseBegin, InputIterator baseEnd, bool skipRedundant) const {
	unsigned int basePos = 0;
	while (baseBegin != baseEnd) {
		//std::cout << "base prefix " << *baseBegin << std::endl;
		for (unsigned int i = basePos; i < bsgs.B.size(); ++i) {
			if (bsgs.B[i] == *baseBegin) {
				std::swap(bsgs.B[i], bsgs.B[basePos]);
				//std::cout << "  swap " << i << " and " << basePos << std::endl;
				break;
			}
		}
		++basePos;
		++baseBegin;
	}
	return basePos;
}

}

#endif // -- CONJUGATINGBASECHANGE_H_
