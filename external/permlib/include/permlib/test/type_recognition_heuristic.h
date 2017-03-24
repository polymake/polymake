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


#ifndef TYPERECOGNITIONHEURISTIC_H_
#define TYPERECOGNITIONHEURISTIC_H_

#include <list>
#include <vector>

#include <boost/iterator/counting_iterator.hpp>
#include <boost/pending/disjoint_sets.hpp>

namespace permlib {

/// Fast recognition of symmetric group subgroups
/**
 * This class identifies subgroups that are symmetric groups
 * by analyzing the graph induced by permutations which are transpositions.
 */
template <class PERM>
class SymmetricGroupRecognitionHeuristic {
public:
	/**
	 * @param n size of the group domain
	 * @param storeUnusedGenerators if set to true, all non-identity permutations that are not transpositions are stored for further use
	 */
	SymmetricGroupRecognitionHeuristic(unsigned int n, bool storeUnusedGenerators = false);
	
	/// adds a group generator for recognition
	/**
	 * @param p group generator
	 * @return true iff p is a transposition (and is therefore used for recognition) or is the identity
	 */
	bool addGenerator(const PERM& p);
	/// computes the orbits of recognized non-trivial symmetric subgroups
	/**
	 * @param orbits vector of lists which contain indices belonging to the same orbit
	 */
	void symmetricGroupOrbits(std::vector<std::list<dom_int> >& orbits);
	
	/// returns the list of all group generators that were not used for group recognition
	const std::list<PERM>& unusedGenerators() { return m_unusedGenerators; }
private:
	bool addUnusedGenerator(const PERM& p);
	
	unsigned int m_n;
	bool m_storeUnusedGenerators;
	std::list<PERM> m_unusedGenerators;
	
	std::vector<dom_int> m_rank;
	std::vector<dom_int> m_parent;
	boost::disjoint_sets<dom_int*,dom_int*> m_components;
	
};

//
//     ----       IMPLEMENTATION
//

template <class PERM>
SymmetricGroupRecognitionHeuristic<PERM>::SymmetricGroupRecognitionHeuristic(unsigned int n, bool storeUnusedGenerators) 
	: m_n(n), m_storeUnusedGenerators(storeUnusedGenerators), m_rank(n), m_parent(n), m_components(&m_rank[0], &m_parent[0])
{ 
	for (dom_int i = 0; i < n; ++i)
		m_components.make_set(i);
}


template <class PERM>
bool SymmetricGroupRecognitionHeuristic<PERM>::addGenerator(const PERM& p) {
	BOOST_ASSERT( p.size() == m_n );
	
	dom_int cycle[2];
	unsigned int cycleIndex = 0;
	
	for (unsigned int i = 0; i < p.size(); ++i) {
		if (p.at(i) != i) {
			if (cycleIndex >= 2)
				return addUnusedGenerator(p);
			
			cycle[cycleIndex++] = i;
		}
	}
	
	if (cycleIndex == 0)
		// discard identity
		return true;
	
	BOOST_ASSERT(cycleIndex == 2);
	
	m_components.union_set(cycle[0], cycle[1]);
	return true;
}


template <class PERM>
void SymmetricGroupRecognitionHeuristic<PERM>::symmetricGroupOrbits(std::vector<std::list<dom_int> >& orbits) {
	m_components.compress_sets(boost::counting_iterator<dom_int>(0), boost::counting_iterator<dom_int>(m_n));
	
	std::vector<unsigned int> componentSizes(m_n);
	for (unsigned int i = 0; i < m_n; ++i) {
		++componentSizes[ m_components.find_set(i) ];
	}
	
	std::vector<int> nonTrivialComponentMap(m_n, -1);
	unsigned int nonTrivialIndex = 0;
	for (unsigned int i = 0; i < m_n; ++i) {
		if (componentSizes[i] > 1)
			nonTrivialComponentMap[i] = nonTrivialIndex++;
	}
	
	orbits.clear();
	orbits.resize(nonTrivialIndex);
	
	for (unsigned int i = 0; i < m_n; ++i) {
		unsigned int componentIndex = m_components.find_set(i);
		if (nonTrivialComponentMap[componentIndex] >= 0)
			orbits[ nonTrivialComponentMap[componentIndex] ].push_back(i);
	}
}


template <class PERM>
bool SymmetricGroupRecognitionHeuristic<PERM>::addUnusedGenerator(const PERM& p) {
	m_unusedGenerators.push_back(p);
	return false;
}

} // end NS

#endif // -- TYPERECOGNITIONHEURISTIC_H_
