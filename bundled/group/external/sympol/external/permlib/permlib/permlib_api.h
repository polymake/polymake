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

#ifndef PERMLIB_API_H
#define PERMLIB_API_H

#include <permlib/common.h>
#include <permlib/permutation.h>
#include <permlib/bsgs.h>
#include <permlib/transversal/schreier_tree_transversal.h>
#include <permlib/transversal/orbit_set.h>
#include <permlib/construct/schreier_sims_construction.h>
#include <permlib/change/conjugating_base_change.h>
#include <permlib/search/partition/vector_stabilizer_search.h>
#include <permlib/search/classic/set_stabilizer_search.h>
#include <permlib/search/classic/set_image_search.h>
#include <permlib/search/classic/lex_smaller_image_search.h>
#include <permlib/search/orbit_lex_min_search.h>

#include <boost/shared_ptr.hpp>
#include <boost/iterator/counting_iterator.hpp>

namespace permlib {

// ---------------------------------------------------------------------
// useful type definitions
//

typedef Permutation PERMUTATION;
typedef SchreierTreeTransversal<PERMUTATION> TRANSVERSAL;
typedef BSGS<PERMUTATION,TRANSVERSAL> PermutationGroup;
typedef OrbitSet<PERMUTATION,unsigned long> OrbitAsSet;


// ---------------------------------------------------------------------
// BSGS construction
//

template<class InputIterator>
boost::shared_ptr<PermutationGroup> construct(unsigned long n, InputIterator begin, InputIterator end) {
	SchreierSimsConstruction<PERMUTATION, TRANSVERSAL> schreierSims(n);
	boost::shared_ptr<PermutationGroup> group(new PermutationGroup(schreierSims.construct(begin, end)));
	return group;
}

template<class InputIteratorGen, class InputIteratorBase>
boost::shared_ptr<PermutationGroup> construct(unsigned long n, InputIteratorGen beginGen, InputIteratorGen endGen,
            InputIteratorBase beginBase, InputIteratorBase endBase) {
	SchreierSimsConstruction<PERMUTATION, TRANSVERSAL> schreierSims(n);
	boost::shared_ptr<PermutationGroup> group(new PermutationGroup(schreierSims.construct(beginGen, endGen, beginBase, endBase)));
	return group;
}


// ---------------------------------------------------------------------
// setwise stabilizer
//

template<class InputIterator>
boost::shared_ptr<PermutationGroup> setStabilizer(const PermutationGroup& group, InputIterator begin, InputIterator end) {
	if (begin == end)
		return boost::shared_ptr<PermutationGroup>(new PermutationGroup(group));
	
	PermutationGroup copy(group);
	// change the base so that is prefixed by the set
	ConjugatingBaseChange<PERMUTATION,TRANSVERSAL,
		RandomBaseTranspose<PERMUTATION,TRANSVERSAL> > baseChange(copy);
	baseChange.change(copy, begin, end);
	
	// prepare search without DCM pruning
	classic::SetStabilizerSearch<BSGS<PERMUTATION,TRANSVERSAL>, TRANSVERSAL> backtrackSearch(copy, 0);
	backtrackSearch.construct(begin, end);
	
	// start the search
	boost::shared_ptr<PermutationGroup> stabilizer(new PermutationGroup(copy.n));
	backtrackSearch.search(*stabilizer);
	return stabilizer;
}


// ---------------------------------------------------------------------
// set image
//

template<class InputIterator>
boost::shared_ptr<Permutation> setImage(const PermutationGroup& group, InputIterator begin, InputIterator end, InputIterator begin2, InputIterator end2) {
	PermutationGroup copy(group);
	// change the base so that is prefixed by the set
	ConjugatingBaseChange<PERMUTATION,TRANSVERSAL,
		RandomBaseTranspose<PERMUTATION,TRANSVERSAL> > baseChange(copy);
	baseChange.change(copy, begin, end);
	
	// prepare search without DCM pruning
	classic::SetImageSearch<BSGS<PERMUTATION,TRANSVERSAL>, TRANSVERSAL> backtrackSearch(copy, 0);
	backtrackSearch.construct(begin, end, begin2, end2);
	
	// start the search
	return backtrackSearch.searchCosetRepresentative();
}


// ---------------------------------------------------------------------
// vector stabilizer
//

template<class InputIterator>
boost::shared_ptr<PermutationGroup> vectorStabilizer(const PermutationGroup& group, InputIterator begin, InputIterator end, unsigned int maxEntry = 0) {
	std::vector<unsigned int> vector(begin, end);
	if (maxEntry == 0) {
		BOOST_FOREACH(const unsigned int& v, vector) {
			if (v > maxEntry)
				maxEntry = v;
		}
	}
	BOOST_ASSERT( maxEntry );
	++maxEntry;
	
	std::list<unsigned int> nonMaxEntries;
	unsigned int i = 0;
	BOOST_FOREACH(const unsigned int& v, vector) {
		if (v < maxEntry-1)
			nonMaxEntries.push_back(i);
		++i;
	}
	
	PermutationGroup copy(group);
	// change the base so that is prefixed by the set
	ConjugatingBaseChange<PERMUTATION,TRANSVERSAL,
		RandomBaseTranspose<PERMUTATION,TRANSVERSAL> > baseChange(copy);
	baseChange.change(copy, nonMaxEntries.begin(), nonMaxEntries.end());
	
	// prepare search without DCM pruning
	partition::VectorStabilizerSearch<BSGS<PERMUTATION,TRANSVERSAL>, TRANSVERSAL> backtrackSearch(copy, 0);
	backtrackSearch.construct(vector.begin(), vector.end(), maxEntry);
	
	// start the search
	boost::shared_ptr<PermutationGroup> stabilizer(new PermutationGroup(copy.n));
	backtrackSearch.search(*stabilizer);
	return stabilizer;
}


// ---------------------------------------------------------------------
// orbits
//

template<typename PDOMAIN,typename ACTION,typename InputIterator>
std::list<boost::shared_ptr<OrbitSet<PERMUTATION,PDOMAIN> > > orbits(const PermutationGroup& group, InputIterator begin, InputIterator end) {
	typedef boost::shared_ptr<OrbitSet<PERMUTATION,PDOMAIN> > ORBIT;
	std::list<ORBIT> orbitList;
	
	for (; begin != end; ++begin) {
		const PDOMAIN& alpha = *begin;
		bool knownElement = false;
		BOOST_FOREACH(const ORBIT& orb, orbitList) {
			if (orb->contains(alpha)) {
				knownElement = true;
				break;
			}
		}
		
		if (knownElement)
			continue;
		
		ORBIT orbit(new OrbitSet<PERMUTATION,PDOMAIN>());
		orbit->orbit(alpha, group.S, ACTION());
		orbitList.push_back(orbit);
	}

	return orbitList;
}

inline std::list<boost::shared_ptr<OrbitAsSet> > orbits(const PermutationGroup& group) {
	return orbits<unsigned long, Transversal<PERMUTATION>::TrivialAction>(group, boost::counting_iterator<unsigned long>(0), boost::counting_iterator<unsigned long>(group.n));
}

// ---------------------------------------------------------------------
// smallest orbit element
//

inline dset smallestSetImage(const PermutationGroup& group, const dset& set) {
	OrbitLexMinSearch<PermutationGroup>  orbLexMin(group);
	return orbLexMin.lexMin(set);
}


template<class InputIteratorB, class InputIteratorZ, class InputIteratorO>
inline bool isNotLexMinSet(PermutationGroup& group,
		InputIteratorB baseBegin,  InputIteratorB baseEnd,
		InputIteratorZ zerosBegin, InputIteratorZ zerosEnd,
		InputIteratorO onesBegin,  InputIteratorO onesEnd
		)
{
	// change the base so that is prefixed by the set
	ConjugatingBaseChange<PERMUTATION,TRANSVERSAL,
		RandomBaseTranspose<PERMUTATION,TRANSVERSAL> > baseChange(group);
	baseChange.change(group, baseBegin, baseEnd);

	classic::LexSmallerImageSearch<PermutationGroup, TRANSVERSAL> backtrackSearch(group, 0);
	backtrackSearch.construct(zerosBegin, zerosEnd, onesBegin, onesEnd);

	// start the search
	typename PERMUTATION::ptr g = backtrackSearch.searchCosetRepresentative();
	if (g) {
		return true;
	}
	return false;
}

template<class InputIteratorB, class InputIteratorZ, class InputIteratorO>
inline bool isNotLexMinSet(const PermutationGroup& group,
		InputIteratorB baseBegin,  InputIteratorB baseEnd,
		InputIteratorZ zerosBegin, InputIteratorZ zerosEnd,
		InputIteratorO onesBegin,  InputIteratorO onesEnd
		)
{
	PermutationGroup copy(group);
	return isNotLexMinSet(copy, baseBegin, baseEnd, zerosBegin, zerosEnd, onesBegin, onesEnd);
}


} // namespace permlib


#endif // PERMLIB_API_H

