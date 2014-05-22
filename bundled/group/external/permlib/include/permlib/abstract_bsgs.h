// ---------------------------------------------------------------------------
//
//  This file is part of PermLib.
//
// Copyright (c) 2009-2012 Thomas Rehn <thomas@carmen76.de>
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

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/iterator/counting_iterator.hpp>

#include <algorithm>

#include <permlib/abstract_permutation_group.h>
#include <permlib/abstract_bsgs_helpers.h>

#include <permlib/change/random_base_transpose.h>
#include <permlib/change/conjugating_base_change.h>
#include <permlib/search/classic/set_stabilizer_search.h>
#include <permlib/search/orbit_lex_min_search.h>

#ifndef ABSTRACT_BSGS_H_
#define ABSTRACT_BSGS_H_

namespace permlib {

/// A high level interface implementing a group represented by a BSGS data structure
template<typename TRANS>
class AbstractBSGS : public AbstractPermutationGroup {
public:
	/// typedef for the BSGS type associated with this group
	typedef BSGS<typename TRANS::PERMtype, TRANS> PermutationGroup;
	/// constructor
	/**
	 * @param bsgs_ the BSGS data structure that represents this group
	 * @param computeSupport if true, the support of the group is computed to accelerate stabilizer and lexMin computations
	 */
	AbstractBSGS(const boost::shared_ptr<PermutationGroup>& bsgs_, bool computeSupport = true);
	
	virtual AbstractPermutationGroup* setStabilizer(const std::vector<dom_int>& s) const;
	virtual OrbitList* orbits() const;
	virtual OrbitList* orbits(const std::vector<dom_int>& s) const;
	virtual bool isLexMinSet(const std::vector<dom_int>& setIndices, const std::vector<dom_int>& rankIndices) const;
	virtual AbstractGroupType type() const { return AGT_BSGS; }

	/// strong generating set of this permutation group
	std::list<typename TRANS::PERMtype::ptr> generators() const;

	/// BSGS data structure for this permutation group
	const boost::shared_ptr<PermutationGroup> bsgs() const { return m_bsgs; }
protected:
	virtual void transversalSizes(std::vector<unsigned long>& sizes) const;
	
	template<typename Iterator>
	OrbitList* orbits(Iterator begin, Iterator end) const;

	/// returns a strategy to decide whether the action of this group is trivial on /s/
	helpers::BaseSupportRestriction* supportRestriction(const std::vector<dom_int>& s) const;
private:
	const boost::shared_ptr<PermutationGroup> m_bsgs;
	boost::shared_ptr<std::set<dom_int> > m_support;
};


template<typename TRANS>
AbstractBSGS<TRANS>::AbstractBSGS(const boost::shared_ptr<PermutationGroup>& bsgs_, bool computeSupport)
	: m_bsgs(bsgs_)
{
	if ( ! computeSupport )
		return;

	m_support.reset( new std::set<dom_int>() );
	BOOST_FOREACH(const typename TRANS::PERMtype::ptr& p, m_bsgs->S) {
		for (dom_int i = 0; i < m_bsgs->n; ++i) {
			if (p->at(i) != i)
				m_support->insert(i);
		}
	}
}

template <class TRANS>
void AbstractBSGS<TRANS>::transversalSizes(std::vector<unsigned long>& sizes) const {
	sizes.clear();
	sizes.reserve(m_bsgs->U.size());
	BOOST_FOREACH(const TRANS &Ui, m_bsgs->U) {
		sizes.push_back(Ui.size());
	}
}

template <class TRANS>
AbstractPermutationGroup* AbstractBSGS<TRANS>::setStabilizer(const std::vector<dom_int>& s) const {
	if (s.empty())
		return new AbstractBSGS<TRANS>(*this);
	
	boost::scoped_ptr<helpers::BaseSupportRestriction> supRestriction( supportRestriction(s) );
	if ( supRestriction->canBeIgnored() )
		return new AbstractBSGS<TRANS>(*this);
	const std::vector<dom_int>* setToStabilize = supRestriction->set();
	BOOST_ASSERT( setToStabilize );
	
	typedef typename TRANS::PERMtype PERM;
	PermutationGroup copy(*m_bsgs);
	// change the base so that is prefixed by the set
	ConjugatingBaseChange<PERM, TRANS,
		RandomBaseTranspose<PERM, TRANS> > baseChange(copy);
	baseChange.change(copy, setToStabilize->begin(), setToStabilize->end());
	
	// prepare search without DCM pruning
	classic::SetStabilizerSearch<BSGS<PERM, TRANS>, TRANS> backtrackSearch(copy, 0);
	backtrackSearch.construct(setToStabilize->begin(), setToStabilize->end());
	
	// start the search
	boost::shared_ptr<PermutationGroup> stabilizer(new PermutationGroup(copy.n));
	backtrackSearch.search(*stabilizer);
	return new AbstractBSGS<TRANS>(stabilizer, m_support);
}

template <class TRANS>
AbstractPermutationGroup::OrbitList* AbstractBSGS<TRANS>::orbits() const {
	return this->orbits(boost::counting_iterator<dom_int>(0), boost::counting_iterator<dom_int>(m_bsgs->n));
}

template <class TRANS>
AbstractPermutationGroup::OrbitList* AbstractBSGS<TRANS>::orbits(const std::vector<dom_int>& s) const {
	return this->orbits(s.begin(), s.end());
}

template <class TRANS>
template<typename Iterator>
AbstractPermutationGroup::OrbitList* AbstractBSGS<TRANS>::orbits(Iterator begin, Iterator end) const {
	OrbitList* retList = new OrbitList();
	
	for (Iterator it = begin; it != end; ++it) {
		const dom_int& alpha = *it;
		bool knownElement = false;
		BOOST_FOREACH(const std::set<dom_int>& orb, *retList) {
			if (orb.find(alpha) != orb.end()) {
				knownElement = true;
				break;
			}
		}
		
		if (knownElement)
			continue;
		
		typedef typename TRANS::PERMtype PERM;
		OrbitSet<PERM,dom_int> orbit;
		orbit.orbit(alpha, m_bsgs->S, typename Transversal<PERM>::TrivialAction());
		retList->push_back(std::set<dom_int>(orbit.begin(), orbit.end()));
	}

	return retList;
}

template <class TRANS>
bool AbstractBSGS<TRANS>::isLexMinSet(const std::vector<dom_int>& setIndices, const std::vector<dom_int>& rankIndices) const {
	if (setIndices.empty())
		return true;
	
	boost::scoped_ptr<helpers::BaseSupportRestriction> supRestriction( supportRestriction(setIndices) );
	if ( supRestriction->canBeIgnored() )
		return true;
	const std::vector<dom_int>* setToLexMin = supRestriction->set();
	BOOST_ASSERT( setToLexMin );
	
	typedef typename TRANS::PERMtype PERM;
	const unsigned int n = m_bsgs->n;
	
	// compute a permutation that we can use for conjugation 
	typename PERM::perm conjugatingPerm(n);

	// rank indices shall be mapped to 1,2,3,4,5,... 
	unsigned int i = 0;
	dset rankSet(n);
	for (std::vector<dom_int>::const_iterator it = rankIndices.begin(); it != rankIndices.end(); ++it)
	{
		conjugatingPerm[*it] = i;
		rankSet[*it] = 1;
		++i;
	}

	// fill up the rest arbitrarily so that we get a proper permutation
	unsigned int v = 0;
	for ( ; i < n; ++i )
	{
		while (rankSet[v]) 
			++v;
		conjugatingPerm[v] = i;
		++v;
	}
	
	PERM c(conjugatingPerm);
	PermutationGroup conjugatedBSGS(*m_bsgs);
	conjugatedBSGS.conjugate(c);
	
	dset rankedTestSet(n);
	for (std::vector<dom_int>::const_iterator it = setToLexMin->begin(); it != setToLexMin->end(); ++it)
	{
		rankedTestSet[c / *it] = 1;
	}
	
	OrbitLexMinSearch<PermutationGroup>  orbLexMin(conjugatedBSGS, true);
	const bool t  =  ( orbLexMin.lexMin(rankedTestSet) == rankedTestSet );
	return t;
}

template <class TRANS>
std::list<typename TRANS::PERMtype::ptr> AbstractBSGS<TRANS>::generators() const {
	return m_bsgs->S;
}

template <class TRANS>
helpers::BaseSupportRestriction* AbstractBSGS<TRANS>::supportRestriction(const std::vector<dom_int>& s) const {
	BOOST_ASSERT( m_bsgs );
	if ( ! m_support )
		return new helpers::BaseSupportRestriction(m_support, s);
	
	// don't use full support restriction if the group base is small
	if (m_bsgs->B.size() <= 10 )
		return new helpers::ReducedSupportRestriction(m_support, s);
	
	return new helpers::FullSupportRestriction(m_support, s);
}

} // end NS permlib

#endif
