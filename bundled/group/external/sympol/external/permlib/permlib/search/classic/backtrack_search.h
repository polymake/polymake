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


#ifndef BACKTRACKSEARCH_H_
#define BACKTRACKSEARCH_H_

#include <permlib/bsgs.h>
#include <permlib/predicate/subgroup_predicate.h>
#include <permlib/predicate/pointwise_stabilizer_predicate.h>
#include <permlib/predicate/group_intersection_predicate.h>

#include <permlib/search/base_search.h>

#include <boost/scoped_ptr.hpp>

namespace permlib {
namespace classic {

/// searching in a group with classical backtracking
template <class BSGSIN, class TRANSRET>
class BacktrackSearch : public BaseSearch<BSGSIN,TRANSRET> {
public:
	typedef typename BaseSearch<BSGSIN,TRANSRET>::PERM PERM;
	typedef typename BaseSearch<BSGSIN,TRANSRET>::TRANS TRANS;
	
	/// constructor
	/**
	 * @param bsgs BSGS to search in
	 * @param pruningLevelDCM prune levels smaller than pruningLevelDCM by double coset minimality with base change
	 * @param breakAfterChildRestriction true iff the rest of a search level can be skipped when one element has been skipped due to child restriction
	 * @param stopAfterFirstElement true iff the search can be stopped after the first element found with the desired property
	 */
	BacktrackSearch(const BSGSIN& bsgs, unsigned int pruningLevelDCM, bool breakAfterChildRestriction = false, bool stopAfterFirstElement = false);

	/// searches for a subgroup and stores it into groupK
	void search(BSGS<PERM,TRANSRET> &groupK);
	
	using BaseSearch<BSGSIN,TRANSRET>::searchCosetRepresentative;
	virtual typename BaseSearch<BSGSIN,TRANSRET>::PERM::ptr searchCosetRepresentative(BSGS<PERM,TRANSRET> &groupK, BSGS<PERM,TRANSRET> &groupL);
protected:
	virtual const std::vector<dom_int>& subgroupBase() const;
	
	/// initializes the search
	void construct(SubgroupPredicate<PERM>* pred, bool addPredRefinement);
	/// recursive backtrack search
	unsigned int search(const PERM& t, unsigned int level, unsigned int& completed, BSGS<PERM,TRANSRET> &groupK, BSGS<PERM,TRANSRET> &groupL);
private:
	/// true iff we can skip a node after the first failing child node
	/**
	 * this is currently only important for set stabilizers
	 */
	const bool m_breakAfterChildRestriction;
};

//
// IMPLEMENTATION
//


template <class BSGSIN, class TRANSRET>
BacktrackSearch<BSGSIN,TRANSRET>::BacktrackSearch(const BSGSIN& bsgs, unsigned int pruningLevelDCM, bool breakAfterChildRestriction, bool stopAfterFirstElement) 
	: BaseSearch<BSGSIN,TRANSRET>(bsgs, pruningLevelDCM, stopAfterFirstElement), 
	  m_breakAfterChildRestriction(breakAfterChildRestriction)
{ }

template <class BSGSIN, class TRANSRET>
void BacktrackSearch<BSGSIN,TRANSRET>::search(BSGS<PERM,TRANSRET> &groupK) {
	BOOST_ASSERT(this->m_pred != 0);
	
	this->setupEmptySubgroup(groupK);
	
	this->m_order = BaseSorterByReference::createOrder(this->m_bsgs.n, this->m_bsgs.B.begin(), this->m_bsgs.B.end());
	this->m_sorter.reset(new BaseSorterByReference(this->m_order));

	unsigned int completed = this->m_bsgs.n;
	BSGS<PERM,TRANSRET> groupL(groupK);
	search(PERM(this->m_bsgs.n), 0, completed, groupK, groupL);

	groupK.stripRedundantBasePoints();
}

template <class BSGSIN, class TRANSRET>
typename BaseSearch<BSGSIN,TRANSRET>::PERM::ptr BacktrackSearch<BSGSIN,TRANSRET>::searchCosetRepresentative(BSGS<PERM,TRANSRET> &groupK, BSGS<PERM,TRANSRET> &groupL) {
	BOOST_ASSERT(this->m_pred != 0);
		
	this->setupEmptySubgroup(groupK);
	this->setupEmptySubgroup(groupL);
	
	this->m_order = BaseSorterByReference::createOrder(this->m_bsgs.n, this->m_bsgs.B.begin(), this->m_bsgs.B.end());
	this->m_sorter.reset(new BaseSorterByReference(this->m_order));

	unsigned int completed = this->m_bsgs.n;
	search(PERM(this->m_bsgs.n), 0, completed, groupK, groupL);

	return BaseSearch<BSGSIN,TRANSRET>::m_lastElement;
}
	
template <class BSGSIN, class TRANSRET>
unsigned int BacktrackSearch<BSGSIN,TRANSRET>::search(const PERM& g, unsigned int level, unsigned int& completed, BSGS<PERM,TRANSRET> &groupK, BSGS<PERM,TRANSRET> &groupL) {
	const std::vector<dom_int> &B = this->m_bsgs.B;
	std::vector<TRANS > &U = this->m_bsgs.U;

	PERMLIB_DEBUG(std::cout << "starting with " << g <<  "    @ " << level << std::endl;)
	++this->m_statNodesVisited;

	if (level == B.size() || this->checkLeaf(level)) {
		PERMLIB_DEBUG(std::cout << "limit reached for " << g << " // " << (*this->m_pred)(g) << std::endl;)
		return this->processLeaf(g, level, level, completed, groupK, groupL);
	}


	std::vector<unsigned long> orbit(U[level].begin(), U[level].end());
	BOOST_FOREACH(unsigned long &alpha, orbit) {
		alpha = g / alpha;
	}
	std::sort(orbit.begin(), orbit.end(), *this->m_sorter);
	unsigned int s = orbit.size();
	
	std::vector<unsigned long>::const_iterator orbIt;
	for (orbIt = orbit.begin(); orbIt != orbit.end(); ++orbIt) {
		if (s < groupK.U[level].size()) {
			PERMLIB_DEBUG(std::cout << "PRUNE the rest:  s=" << s << " < " << groupK.U[level].size() << std::endl;)
			this->m_statNodesPrunedCosetMinimality += s;
			// skip the rest due to double coset minimality
			break;
		}
		
		--s;
		unsigned long beta = g % *orbIt;
		PERMLIB_DEBUG(std::cout << " BETA = " << beta << " <-- " << B[level] << std::endl;)
		boost::scoped_ptr<PERM> u_beta_ptr(U[level].at(beta));
		*u_beta_ptr *= g;

		if (!this->m_pred->childRestriction(*u_beta_ptr, level, B[level])) {
			++this->m_statNodesPrunedChildRestriction;
			if (m_breakAfterChildRestriction)
				break;
			continue;
		}
		if (this->m_pruningLevelDCM && this->pruneDCM(*u_beta_ptr, level, groupK, groupL)) {
			++this->m_statNodesPrunedCosetMinimality2;
			continue;
		}

		unsigned int ret = search(*u_beta_ptr, level+1, completed, groupK, groupL);
		if (BaseSearch<BSGSIN,TRANSRET>::m_stopAfterFirstElement && ret == 0)
			return 0;
		if (ret < level) {
			PERMLIB_DEBUG(std::cout << "^^ MULTI BACKTRACK! leave " << level << " to " << ret << std::endl;)
			return ret;
		}
	}
	completed = std::min(completed, level);

	return level;
}

template<class BSGSIN,class TRANSRET>
void BacktrackSearch<BSGSIN,TRANSRET>::construct(SubgroupPredicate<PERM>* pred, bool addPredRefinement) {
	this->m_pred.reset(pred);
}

template<class BSGSIN,class TRANSRET>
const std::vector<dom_int>& BacktrackSearch<BSGSIN,TRANSRET>::subgroupBase() const {
	return this->m_bsgs.B;
}

}
}

#endif // -- BACKTRACKSEARCH_H_
