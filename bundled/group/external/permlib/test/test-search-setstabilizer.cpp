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


#define BOOST_TEST_DYN_LINK 
#define BOOST_TEST_MODULE Backtrack search test - set stabilization and set images
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>

#include <permlib/permutation.h>
#include <permlib/generator/product_replacement_generator.h>
#include <permlib/bsgs.h>
#include <permlib/construct/schreier_sims_construction.h>
#include <permlib/generator/bsgs_generator.h>
#include <permlib/transversal/schreier_tree_transversal.h>
#include <permlib/construct/schreier_sims_construction.h>
#include <permlib/change/conjugating_base_change.h>
#include <permlib/search/classic/set_stabilizer_search.h>
#include <permlib/search/partition/set_stabilizer_search.h>
#include <permlib/search/partition/vector_stabilizer_search.h>
#include <permlib/search/classic/set_image_search.h>
#include <permlib/search/partition/set_image_search.h>

#include "test-common.h"

#include <boost/foreach.hpp>
#include <boost/iterator/counting_iterator.hpp>

using namespace permlib;
using namespace permlib::test;

template<class TRANSVERSAL>
void checkStabilizer(const GroupInformation& info, unsigned int setSize, unsigned int numberOfSets) {
	BOOST_CHECK_LE(setSize, info.n);
	
	typedef typename TRANSVERSAL::PERMtype PERM;
	BSGS<PERM, TRANSVERSAL> bsgs = ConstructDeterministic1<PERM, TRANSVERSAL>()(info);
	
	for (unsigned int i = 0; i < numberOfSets; ++i) {
		std::set<dom_int> testSet;
		while (testSet.size() < setSize) {
			testSet.insert(randomInt(info.n));
		}
		
		// change the base so that is prefixed by the set
		ConjugatingBaseChange<PERM,TRANSVERSAL,
			RandomBaseTranspose<PERM,TRANSVERSAL> > baseChange(bsgs);
		baseChange.change(bsgs, testSet.begin(), testSet.end());
		
		BSGS<PERM,TRANSVERSAL> classicStab(bsgs.n);
		// prepare search without DCM pruning
		classic::SetStabilizerSearch<BSGS<PERM,TRANSVERSAL>,TRANSVERSAL> backtrackSearch(bsgs, 0);
		backtrackSearch.construct(testSet.begin(), testSet.end());
		backtrackSearch.search(classicStab);
		
		BSGS<PERM,TRANSVERSAL> partitionStab(bsgs.n);
		// prepare search without DCM pruning
		partition::SetStabilizerSearch<BSGS<PERM,TRANSVERSAL>,TRANSVERSAL> partitionBacktrackSearch(bsgs, 0);
		partitionBacktrackSearch.construct(testSet.begin(), testSet.end());
		partitionBacktrackSearch.search(partitionStab);
		
		BOOST_CHECK_EQUAL( classicStab.order(), partitionStab.order() );
		
		// check stabilizer order by iterating over all group elements
		BSGSGenerator<TRANSVERSAL> bsgsGen(bsgs.U);
		SetwiseStabilizerPredicate<PERM> stabPred(testSet.begin(), testSet.end());
		std::list<PERM> stabilizingGroupElements;
		while (bsgsGen.hasNext()) {
			const PERM p = bsgsGen.next();
			if (stabPred(p))
				stabilizingGroupElements.push_back(p);
		}
		
		BOOST_CHECK_EQUAL( classicStab.order(), stabilizingGroupElements.size() );
		
		// check that stabilizer contains all required elements
		BOOST_FOREACH(const PERM& p, stabilizingGroupElements) {
			BOOST_CHECK(classicStab.sifts(p));
			BOOST_CHECK(partitionStab.sifts(p));
		}
	}
}

template<class TRANSVERSAL>
void checkSetImage(const GroupInformation& info, unsigned int setSize, unsigned int numberOfSets) {
	BOOST_CHECK_LE(setSize, info.n);
	
	typedef typename TRANSVERSAL::PERMtype PERM;
	BSGS<PERM, TRANSVERSAL> bsgs = ConstructDeterministic1<PERM, TRANSVERSAL>()(info);
	
	for (unsigned int i = 0; i < numberOfSets; ++i) {
		std::set<dom_int> testSet1;
		std::set<dom_int> testSet2;
		while (testSet1.size() < setSize) {
			testSet1.insert(randomInt(info.n));
		}
		while (testSet2.size() < setSize) {
			testSet2.insert(randomInt(info.n));
		}
		
		// change the base so that is prefixed by the set
		ConjugatingBaseChange<PERM,TRANSVERSAL,
			RandomBaseTranspose<PERM,TRANSVERSAL> > baseChange(bsgs);
		baseChange.change(bsgs, testSet1.begin(), testSet1.end());
		
		// prepare search without DCM pruning
		classic::SetImageSearch<BSGS<PERM,TRANSVERSAL>,TRANSVERSAL> backtrackSearch(bsgs, 0);
		backtrackSearch.construct(testSet1.begin(), testSet1.end(), testSet2.begin(), testSet2.end());
		typename PERM::ptr classicRep = backtrackSearch.searchCosetRepresentative();
		
		BSGS<PERM,TRANSVERSAL> partitionStab(bsgs.n);
		// prepare search without DCM pruning
		partition::SetImageSearch<BSGS<PERM,TRANSVERSAL>,TRANSVERSAL> partitionBacktrackSearch(bsgs, 0);
		partitionBacktrackSearch.construct(testSet1.begin(), testSet1.end(), testSet2.begin(), testSet2.end());
		typename PERM::ptr partitionRep = partitionBacktrackSearch.searchCosetRepresentative();

		bool mappingExists = false;
		// check set image result by iterating over all group elements
		BSGSGenerator<TRANSVERSAL> bsgsGen(bsgs.U);
		SetImagePredicate<PERM> stabPred(testSet1.begin(), testSet1.end(), testSet2.begin(), testSet2.end());
		while (bsgsGen.hasNext()) {
			const PERM p = bsgsGen.next();
			if (stabPred(p)) {
				mappingExists = true;
				break;
			}
		}
		
		BOOST_CHECK_EQUAL( mappingExists, static_cast<bool>(classicRep) );
		BOOST_CHECK_EQUAL( mappingExists, static_cast<bool>(partitionRep) );
		
		if (classicRep)
			BOOST_CHECK( stabPred(*classicRep) );
		if (partitionRep)
			BOOST_CHECK( stabPred(*partitionRep) );
	}
}

BOOST_AUTO_TEST_CASE( setstabilizer )
{
	// we use elementary permutations
	typedef Permutation PERM;
	// and Schreier tree transversals
	typedef SchreierTreeTransversal<PERM> TRANSVERSAL;
	
	checkStabilizer<TRANSVERSAL>(info_testThesis, 3, 10);
	checkStabilizer<TRANSVERSAL>(info_1997, 3, 10);
	checkStabilizer<TRANSVERSAL>(info_cyclic10, 2, 30);
	checkStabilizer<TRANSVERSAL>(info_S6_3, 4, 30);
	checkStabilizer<TRANSVERSAL>(info_e6, 4, 15);
}

BOOST_AUTO_TEST_CASE( setimage )
{
	// we use elementary permutations
	typedef Permutation PERM;
	// and Schreier tree transversals
	typedef SchreierTreeTransversal<PERM> TRANSVERSAL;
	
	checkSetImage<TRANSVERSAL>(info_testThesis, 2, 10);
	checkSetImage<TRANSVERSAL>(info_testThesis, 3, 10);
	checkSetImage<TRANSVERSAL>(info_testThesis, 5, 10);
	checkSetImage<TRANSVERSAL>(info_1997, 2, 10);
	checkSetImage<TRANSVERSAL>(info_1997, 3, 10);
	checkSetImage<TRANSVERSAL>(info_1997, 5, 10);
	checkSetImage<TRANSVERSAL>(info_cyclic10, 2, 30);
	checkSetImage<TRANSVERSAL>(info_S6_3, 4, 30);
	checkSetImage<TRANSVERSAL>(info_e6, 4, 15);
	checkSetImage<TRANSVERSAL>(info_e6, 11, 3);
}

BOOST_AUTO_TEST_CASE( vectorstabilizer_group )
{
	// we use elementary permutations
	typedef Permutation PERM;
	// and Schreier tree transversals
	typedef SchreierTreeTransversal<PERM> TRANSVERSAL;
	typedef std::list<PERM::ptr> PERMlist;
	typedef partition::VectorStabilizerSearch<BSGS<PERM,TRANSVERSAL>,TRANSVERSAL> VecStab;
	
	// our group will have degree 10
	const unsigned long n = 10;
	
	// group generators
	PERMlist groupGenerators;
	boost::shared_ptr<PERM> gen1(new PERM(n, std::string("1 3 5 7 9 10 2 4 6 8")));
	groupGenerators.push_back(gen1);
	boost::shared_ptr<PERM> gen2(new PERM(n, std::string("1 5")));
	groupGenerators.push_back(gen2);
	
	// BSGS construction
	SchreierSimsConstruction<PERM, TRANSVERSAL> schreierSims(n);
	BSGS<PERM, TRANSVERSAL> group = schreierSims.construct(groupGenerators.begin(),
														  groupGenerators.end());
	
	std::vector<unsigned int> v(n);
	v[0] = 0;
	v[1] = 0;
	v[2] = 0;
	v[3] = 0;
	v[4] = 1;
	v[5] = 1;
	v[6] = 1;
	v[7] = 2;
	v[8] = 2;
	v[9] = 2;
	
	ConjugatingBaseChange<PERM,TRANSVERSAL,
			RandomBaseTranspose<PERM,TRANSVERSAL> > baseChange(group);
	
	baseChange.change(group, boost::counting_iterator<unsigned int>(0), boost::counting_iterator<unsigned int>(7));
	
	BSGS<PERM,TRANSVERSAL>* partitionStab = NULL;
	VecStab* partitionVectorSearch = NULL;
	
	partitionStab = new BSGS<PERM,TRANSVERSAL>(n);
	partitionVectorSearch = new VecStab(group, 0);
	BOOST_ASSERT(partitionStab);
	BOOST_ASSERT(partitionVectorSearch);
	partitionVectorSearch->construct(v.begin(), v.end(), 3);
	partitionVectorSearch->search(*partitionStab);
	VectorStabilizerPredicate<PERM> vecStabPred(v.begin(), v.end());
	BOOST_FOREACH(const PERM::ptr& g, partitionStab->S) {
		BOOST_CHECK( vecStabPred(*g) );
	}
	// correct value 16 computed with GAP
	BOOST_CHECK_EQUAL(partitionStab->order(), 16);
	delete partitionStab;
	delete partitionVectorSearch;
}


BOOST_AUTO_TEST_CASE( vectorstabilizer_cyclic )
{
	typedef Permutation PERM;
	typedef SchreierTreeTransversal<PERM> TRANS;
	typedef std::vector<unsigned int> vec;
	typedef partition::VectorStabilizerSearch<BSGS<PERM,TRANS>,TRANS> VecStab;
	
	const GroupInformation& info = info_cyclic500;
	BSGS<PERM,TRANS> group = construct<PERM,TRANS>(info);
	const unsigned int n = info.n;
	
	const unsigned int maxV = 2;
	const unsigned int maxW = 5;
	const unsigned int maxX = 25;
	vec v(n), w(n), x(n);
	vec stabV(n/2);
	for (unsigned int i = 0; i < n; ++i) {
		v[i] = i % maxV;
		w[i] = i % maxW;
		x[i] = i % maxX;
		if (i % 2)
			stabV[i/2] = i;
	}
	
	BSGS<PERM,TRANS>* partitionStab = NULL;
	VecStab* partitionVectorSearch = NULL;
	VectorStabilizerPredicate<PERM>* vecStabPred = NULL;
	
	partitionStab = new BSGS<PERM,TRANS>(n);
	BOOST_ASSERT(partitionStab);
	partition::SetStabilizerSearch<BSGS<PERM,TRANS>,TRANS> partitionSetStabSearch(group, 0);
	partitionSetStabSearch.construct(stabV.begin(), stabV.end());
	partitionSetStabSearch.search(*partitionStab);
	BOOST_CHECK_EQUAL(partitionStab->order(), n/2);
	delete partitionStab;
	delete partitionVectorSearch;
	
	partitionStab = new BSGS<PERM,TRANS>(n);
	partitionVectorSearch = new VecStab(group, 0);
	BOOST_ASSERT(partitionStab);
	BOOST_ASSERT(partitionVectorSearch);
	partitionVectorSearch->construct(v.begin(), v.end(), maxV);
	partitionVectorSearch->search(*partitionStab);
	BOOST_CHECK_EQUAL(partitionStab->order(), n/maxV);
	vecStabPred = new VectorStabilizerPredicate<PERM>(v.begin(), v.end());
	BOOST_FOREACH(const PERM::ptr& g, partitionStab->S) {
		BOOST_CHECK( (*vecStabPred)(*g) );
	}
	delete vecStabPred;
	delete partitionStab;
	delete partitionVectorSearch;
	

	partitionStab = new BSGS<PERM,TRANS>(n);
	partitionVectorSearch = new VecStab(group, 0);
	BOOST_ASSERT(partitionStab);
	BOOST_ASSERT(partitionVectorSearch);
	partitionVectorSearch->construct(w.begin(), w.end(), maxW);
	partitionVectorSearch->search(*partitionStab);
	BOOST_CHECK_EQUAL(partitionStab->order(), n/maxW);
	vecStabPred = new VectorStabilizerPredicate<PERM>(w.begin(), w.end());
	BOOST_FOREACH(const PERM::ptr& g, partitionStab->S) {
		BOOST_CHECK( (*vecStabPred)(*g) );
	}
	delete vecStabPred;
	delete partitionStab;
	delete partitionVectorSearch;
	
	partitionStab = new BSGS<PERM,TRANS>(n);
	partitionVectorSearch = new VecStab(group, 0);
	BOOST_ASSERT(partitionStab);
	BOOST_ASSERT(partitionVectorSearch);
	partitionVectorSearch->construct(x.begin(), x.end(), maxX);
	partitionVectorSearch->search(*partitionStab);
	BOOST_CHECK_EQUAL(partitionStab->order(), n/maxX);
	vecStabPred = new VectorStabilizerPredicate<PERM>(x.begin(), x.end());
	BOOST_FOREACH(const PERM::ptr& g, partitionStab->S) {
		BOOST_CHECK( (*vecStabPred)(*g) );
	}
	delete vecStabPred;
	delete partitionStab;
	delete partitionVectorSearch;
}

BOOST_AUTO_TEST_CASE( vectorstabilizer_group2 )
{
	// we use elementary permutations
	typedef Permutation PERM;
	// and Schreier tree transversals
	typedef SchreierTreeTransversal<PERM> TRANSVERSAL;
	typedef partition::VectorStabilizerSearch<BSGS<PERM,TRANSVERSAL>,TRANSVERSAL> VecStab;
	
	BSGS<PERM,TRANSVERSAL> group = construct<PERM,TRANSVERSAL>(info_myciel3);
	const unsigned long n = group.n;
	
	std::vector<unsigned int> v(n,1);
	v[0] = 0;
	
	// computed with GAP
	const unsigned int stabilizerOrder = 4;
	
	ConjugatingBaseChange<PERM,TRANSVERSAL,
			RandomBaseTranspose<PERM,TRANSVERSAL> > baseChange(group);
	
	baseChange.change(group, boost::counting_iterator<unsigned int>(0), boost::counting_iterator<unsigned int>(1));
	
	// point stabilizer, we can read its order from the transversal sizes
	BOOST_CHECK_EQUAL(group.order() / group.U[0].size(), stabilizerOrder);
	
	BSGS<PERM,TRANSVERSAL>* partitionStab = NULL;
	VecStab* partitionVectorSearch = NULL;
	
	partitionStab = new BSGS<PERM,TRANSVERSAL>(n);
	partitionVectorSearch = new VecStab(group, 0);
	BOOST_ASSERT(partitionStab);
	BOOST_ASSERT(partitionVectorSearch);
	partitionVectorSearch->construct(v.begin(), v.end(), 2);
	partitionVectorSearch->search(*partitionStab);
	VectorStabilizerPredicate<PERM> vecStabPred(v.begin(), v.end());
	BOOST_FOREACH(const PERM::ptr& g, partitionStab->S) {
		BOOST_CHECK( vecStabPred(*g) );
	}
	BOOST_CHECK_EQUAL(partitionStab->order(), stabilizerOrder);
	delete partitionStab;
	delete partitionVectorSearch;
}

