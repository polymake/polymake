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


#ifndef PARTITION_VECTOR_STABILIZER_SEARCH_H_
#define PARTITION_VECTOR_STABILIZER_SEARCH_H_

#include <permlib/search/partition/r_base.h>
#include <permlib/predicate/vector_stabilizer_predicate.h>
#include <permlib/search/partition/set_stabilize_refinement.h>
#include <permlib/search/partition/refinement_family.h>

namespace permlib {
namespace partition {

/// subgroup search for a stabilizer of an integer vector based on partition backtracking
template<class BSGSIN,class TRANSRET>
class VectorStabilizerSearch : public RBase<BSGSIN,TRANSRET> {
public:
	typedef typename RBase<BSGSIN,TRANSRET>::PERM PERM;
	
	/// constructor
	/**
	 * @param bsgs BSGS of group
	 * @param pruningLevelDCM level up to which expensive double coset minimality pruning is performed; zero to disable
	 */
	VectorStabilizerSearch(const BSGSIN& bsgs, unsigned int pruningLevelDCM);
	
	/// initializes search
	/**
	 * assume that integer vector has entries 0, 1, ..., maxEntries-1
	 * @param begin iterator(unsigned long) begin of the integer vector to be stabilized
	 * @param end iterator(unsigned long) end of the integer vector to be stabilized
	 * @param maxEntries value of maximal entry of integer vector plus 1
	 */
	template<class InputIterator>
	void construct(InputIterator begin, InputIterator end, unsigned int maxEntries);
protected:
	virtual unsigned int processNewFixPoints(const Partition& pi, unsigned int backtrackCount);
private:
	std::vector<unsigned int> toStab;
	unsigned int m_maxEntries;
};

template<class BSGSIN,class TRANSRET>
VectorStabilizerSearch<BSGSIN,TRANSRET>::VectorStabilizerSearch(const BSGSIN& bsgs, unsigned int pruningLevelDCM) 
	: RBase<BSGSIN,TRANSRET>(bsgs, pruningLevelDCM)
{ }

template<class BSGSIN,class TRANSRET>
template<class InputIterator>
void VectorStabilizerSearch<BSGSIN,TRANSRET>::construct(InputIterator begin, InputIterator end, unsigned int maxEntries) {
	VectorStabilizerPredicate<PERM>* stabPred = new VectorStabilizerPredicate<PERM>(begin, end);
	m_maxEntries = maxEntries;
	toStab.insert(toStab.begin(), begin, end);
	std::vector<unsigned int> stabC(toStab.size());
	
#ifdef PERMLIB_DEBUGMODE
	BOOST_FOREACH(const unsigned int v, stabC) {
		BOOST_ASSERT( v < maxEntries );
	}
#endif
	
	// we can ignore the highest entries because these are automatically stabilized
	// if all other entries are stabilized
	for (unsigned int c = 0; c < maxEntries - 1; ++c) {
		unsigned int i = 0;
		std::vector<unsigned int>::iterator stabIt = toStab.begin(), cIt = stabC.begin();
		for (; stabIt != toStab.end(); ++stabIt) {
			BOOST_ASSERT( cIt != stabC.end() );
			if (*stabIt == c)
				*cIt++ = i;
			++i;
		}
		SetStabilizeRefinement<PERM> ssr(this->m_bsgs.n, stabC.begin(), cIt);
		ssr.initializeAndApply(this->m_partition);
		PERM empty(this->m_bsgs.n);
		ssr.apply2(this->m_partition2, empty);
	}
	RBase<BSGSIN,TRANSRET>::construct(stabPred, 0);
}

template<class BSGSIN,class TRANSRET>
unsigned int VectorStabilizerSearch<BSGSIN,TRANSRET>::processNewFixPoints(const Partition& pi, unsigned int level) {
	const unsigned int basePos = RBase<BSGSIN,TRANSRET>::processNewFixPoints(pi, level);
	if (!this->m_limitInitialized) {
		bool allFound = true;
		int pos = -1;
		BOOST_FOREACH(unsigned int alpha, toStab) {
			++pos;
			if (alpha == m_maxEntries - 1)
				continue;
			if (std::find(pi.fixPointsBegin(), pi.fixPointsEnd(), static_cast<unsigned int>(pos)) == pi.fixPointsEnd()) {
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

#endif // -- PARTITION_VECTOR_STABILIZER_SEARCH_H_
