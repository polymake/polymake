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


#ifndef PARTITION_SET_STABILIZER_SEARCH_H_
#define PARTITION_SET_STABILIZER_SEARCH_H_

#include <permlib/search/partition/r_base.h>
#include <permlib/predicate/setwise_stabilizer_predicate.h>
#include <permlib/search/partition/set_stabilize_refinement.h>
#include <permlib/search/partition/refinement_family.h>

namespace permlib {
namespace partition {

/// subgroup search for a set stabilizer based on partition backtracking
template<class BSGSIN,class TRANSRET>
class SetStabilizerSearch : public RBase<BSGSIN,TRANSRET> {
public:
	typedef typename RBase<BSGSIN,TRANSRET>::PERM PERM;
	
	/// constructor
	/**
	 * @param bsgs BSGS of group
	 * @param pruningLevelDCM level up to which expensive double coset minimality pruning is performed; zero to disable
	 */
	SetStabilizerSearch(const BSGSIN& bsgs, unsigned int pruningLevelDCM);
	
	/// initializes search
	/**
	 * @param begin iterator(unsigned long) begin of the set to be stabilized
	 * @param end iterator(unsigned long) end of the set to be stabilized
	 */
	template<class InputIterator>
	void construct(InputIterator begin, InputIterator end);
protected:
	virtual unsigned int processNewFixPoints(const Partition& pi, unsigned int backtrackCount);
private:
	std::vector<unsigned long> toStab;
};

template<class BSGSIN,class TRANSRET>
SetStabilizerSearch<BSGSIN,TRANSRET>::SetStabilizerSearch(const BSGSIN& bsgs, unsigned int pruningLevelDCM) 
	: RBase<BSGSIN,TRANSRET>(bsgs, pruningLevelDCM)
{ }

template<class BSGSIN,class TRANSRET>
template<class InputIterator>
void SetStabilizerSearch<BSGSIN,TRANSRET>::construct(InputIterator begin, InputIterator end) {
	SetwiseStabilizerPredicate<PERM>* stabPred = new SetwiseStabilizerPredicate<PERM>(begin, end);
	toStab.insert(toStab.begin(), begin, end);
	
	SetStabilizeRefinement<PERM> ssr(RBase<BSGSIN,TRANSRET>::m_bsgs.n, toStab.begin(), toStab.end());
	ssr.initializeAndApply(RBase<BSGSIN,TRANSRET>::m_partition);
	PERM empty(RBase<BSGSIN,TRANSRET>::m_bsgs.n);
	ssr.apply2(RBase<BSGSIN,TRANSRET>::m_partition2, empty);
	
	RBase<BSGSIN,TRANSRET>::construct(stabPred, 0);
}

template<class BSGSIN,class TRANSRET>
unsigned int SetStabilizerSearch<BSGSIN,TRANSRET>::processNewFixPoints(const Partition& pi, unsigned int level) {
	const unsigned int basePos = RBase<BSGSIN,TRANSRET>::processNewFixPoints(pi, level);
	if (!this->m_limitInitialized) {
		bool allFound = true;
		BOOST_FOREACH(unsigned long alpha, toStab) {
			if (std::find(pi.fixPointsBegin(), pi.fixPointsEnd(), alpha) == pi.fixPointsEnd()) {
				allFound = false;
				break;
			}
		}
		if (allFound) {
			this->m_limitLevel = level;
			this->m_limitBase = basePos;
			this->m_limitInitialized = true;
		}
	}
	return basePos;
}

}
}

#endif // -- PARTITION_SET_STABILIZER_SEARCH_H_
