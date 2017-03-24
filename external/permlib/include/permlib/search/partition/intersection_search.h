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


#ifndef PARTITION_INTERSECTION_SEARCH_H_
#define PARTITION_INTERSECTION_SEARCH_H_

#include <permlib/search/partition/r_base.h>
#include <permlib/predicate/group_intersection_predicate.h>

namespace permlib {
namespace partition {

/// subgroup search for a group intersection based on partition backtracking
template<class BSGSIN,class TRANSRET>
class IntersectionSearch : public RBase<BSGSIN,TRANSRET> {
public:
	typedef typename RBase<BSGSIN,TRANSRET>::PERM PERM;
	typedef typename RBase<BSGSIN,TRANSRET>::TRANS TRANS;
	
	/// constructor
	/**
	 * @param bsgs BSGS of first group
	 * @param pruningLevelDCM level up to which expensive double coset minimality pruning is performed; zero to disable
	 */
	IntersectionSearch(const BSGSIN& bsgs, unsigned int pruningLevelDCM);
	
	/// initializes search
	/**
	 * @param bsgs2 BSGS of second group for intersection
	 */
	void construct(BSGSIN* bsgs2);
	
	typedef typename RBase<BSGSIN,TRANSRET>::PartitionIt PartitionIt;
};

template<class BSGSIN,class TRANSRET>
IntersectionSearch<BSGSIN,TRANSRET>::IntersectionSearch(const BSGSIN& bsgs, unsigned int pruningLevelDCM) 
	: RBase<BSGSIN,TRANSRET>(bsgs, pruningLevelDCM)
{ }

template<class BSGSIN,class TRANSRET>
void IntersectionSearch<BSGSIN,TRANSRET>::construct(BSGSIN* bsgs2) {
	GroupIntersectionPredicate<PERM,TRANS>* intersectPred = new GroupIntersectionPredicate<PERM,TRANS>(RBase<BSGSIN,TRANSRET>::m_bsgs, *bsgs2);
	RBase<BSGSIN,TRANSRET>::m_bsgs2 = bsgs2;
	
	RBase<BSGSIN,TRANSRET>::construct(intersectPred, new GroupRefinementFamily<PERM,TRANS>(*bsgs2));
}

}
}

#endif // -- PARTITION_INTERSECTION_SEARCH_H_
