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

#include <algorithm>
#include <map>
#include <permlib/abstract_permutation_group.h>

#ifndef ABSTRACT_SYMMETRIC_PRODUCT_H_
#define ABSTRACT_SYMMETRIC_PRODUCT_H_

namespace permlib {

/// A high level interface implementing a direct product of symmetric groups
class AbstractSymmetricProduct : public AbstractPermutationGroup {
public:
	/// constructor
	/**
	 * The group is constructed from a list of lists.
	 * Each inner list contains an orbit of indices on which a symmetric group acts.
	 *
	 * @param begin begin iterator which in turn points to iterators
	 * @param end   end iterator which in turn points to iterators
	 */
	template<typename InputIterator>
	AbstractSymmetricProduct(InputIterator begin, InputIterator end) {
		for (InputIterator it = begin; it != end; ++it) {
			m_indices.push_back(std::set<dom_int>((*it).begin(), (*it).end()));
		}
	}
	
	virtual AbstractPermutationGroup* setStabilizer(const std::vector<dom_int>& s) const;
	virtual OrbitList* orbits() const;
	// TODO: must s be sorted?
	virtual OrbitList* orbits(const std::vector<dom_int>& s) const;
	
	virtual bool isLexMinSet(const std::vector<dom_int>& setIndices, const std::vector<dom_int>& rankIndices) const;
	
	virtual AbstractGroupType type() const { return AGT_SymmetricProduct; }
protected:
	virtual void transversalSizes(std::vector<unsigned long>& sizes) const;

private:
	AbstractSymmetricProduct() {}
	
	typedef std::list<std::set<dom_int> > IndexList;
	std::list<std::set<dom_int> > m_indices;
	mutable std::map<dom_int, dom_int> m_indicesReverse;
	
	int getOrbitRank(dom_int x) const;
};

inline void AbstractSymmetricProduct::transversalSizes(std::vector<unsigned long>& sizes) const {
	sizes.clear();
	sizes.reserve(m_indices.size());
	BOOST_FOREACH(const std::set<dom_int>& ind, m_indices) {
		for (unsigned long x = ind.size(); x > 1; --x)
			sizes.push_back(x);
	}
}

inline AbstractPermutationGroup* AbstractSymmetricProduct::setStabilizer(const std::vector<dom_int>& svec) const {
	std::set<dom_int> s(svec.begin(), svec.end());
	
	AbstractSymmetricProduct* stabilizer = new AbstractSymmetricProduct();
	BOOST_FOREACH(const std::set<dom_int>& ind, m_indices) {
		std::set<dom_int> sA, sB;
		std::set_difference(ind.begin(), ind.end(), s.begin(), s.end(), std::inserter(sA, sA.begin()));
		if (sA.size() > 1) {
			stabilizer->m_indices.push_back(sA);
		}
		std::set_intersection(ind.begin(), ind.end(), s.begin(), s.end(), std::inserter(sB, sB.begin()));
		if (sB.size() > 1) {
			stabilizer->m_indices.push_back(sB);
		}
	}
	return stabilizer;
}

inline AbstractPermutationGroup::OrbitList* AbstractSymmetricProduct::orbits() const {
	OrbitList* retList = new OrbitList(m_indices);
	return retList;
}

inline AbstractPermutationGroup::OrbitList* AbstractSymmetricProduct::orbits(const std::vector<dom_int>& s) const {
	OrbitList* retList = new OrbitList();
	BOOST_FOREACH(const std::set<dom_int>& ind, m_indices) {
		std::set<dom_int>::const_iterator indIt = std::find_first_of(ind.begin(), ind.end(), s.begin(), s.end());
		if (indIt != ind.end()) {
			retList->push_back(ind);
		}
	}
	return retList;
}

inline bool AbstractSymmetricProduct::isLexMinSet(const std::vector<dom_int>& setIndices, const std::vector<dom_int>& rankIndices) const {
	std::vector<unsigned int> expectedPosition(m_indices.size());
	
	BOOST_FOREACH(dom_int x, setIndices) {
		// if x is not at expectedPosition of its orbit
		//   return false
		const int rank = getOrbitRank(x);
		if (rank < 0)
			continue;
		
		dom_int position = 0;
		BOOST_FOREACH(dom_int el, rankIndices) {
			if (el == x)
				break;
			if (getOrbitRank(el) == rank)
				++position;
		}
		
		if (expectedPosition[rank] != position)
			return false;
		
		++expectedPosition[rank];
	}
	
	return true;
}

inline int AbstractSymmetricProduct::getOrbitRank(dom_int x) const {
	if (m_indicesReverse.empty()) {
		if (m_indices.empty())
			return -1;
		
		dom_int rank = 0;
		BOOST_FOREACH(const std::set<dom_int>& orb, m_indices) {
			BOOST_FOREACH(dom_int el, orb) {
				m_indicesReverse[el] = rank;
			}
			++rank;
		}
	}
	
	std::map<dom_int, dom_int>::const_iterator pos = m_indicesReverse.find(x);
	if (pos == m_indicesReverse.end())
		return -1;
	
	return (*pos).second;
}

} // end NS

#endif
