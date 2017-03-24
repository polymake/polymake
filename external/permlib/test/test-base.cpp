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
#define BOOST_TEST_MODULE BSGS construction and change test
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>
#include <cmath>

#include <permlib/permutation.h>
#include <permlib/permutationword.h>
#include <permlib/bsgs.h>

#include <permlib/transversal/schreier_tree_transversal.h>
#include <permlib/transversal/explicit_transversal.h>
#include <permlib/construct/schreier_sims_construction.h>

#include <permlib/construct/cyclic_group_construction.h>

#include <permlib/change/simple_base_change.h>
#include <permlib/change/conjugating_base_change.h>
#include <permlib/change/deterministic_base_transpose.h>
#include <permlib/change/random_base_transpose.h>

#include "test-common.h"
#include <permlib/generator/bsgs_random_generator.h>

using namespace permlib;
using namespace permlib::test;

template<class CONSTRUCTOR>
void checkGroup(const GroupInformation &info) {   
	typedef typename CONSTRUCTOR::PERMtype PERM;
	typedef typename CONSTRUCTOR::TRANStype TRANS;
	
	BSGS<PERM,TRANS> bsgs = CONSTRUCTOR()(info);
	BOOST_CHECK_EQUAL( bsgs.n, info.n );
	BOOST_CHECK_EQUAL( bsgs.order(), info.order );
	
	unsigned int transversalProduct = 1;
	BOOST_FOREACH(const TRANS &U, bsgs.U) {
			transversalProduct *= U.size();
	}
	BOOST_CHECK_EQUAL( transversalProduct, bsgs.order() );
	
	for (unsigned int i = 0; i < bsgs.n; ++i) {
		BSGS<PERM,TRANS> bsgs2(bsgs);
		bsgs2.insertRedundantBasePoint(i);
		
		transversalProduct = 1;
		BOOST_FOREACH(const TRANS &U, bsgs2.U) {
			transversalProduct *= U.size();
		}
		BOOST_CHECK_EQUAL( transversalProduct, bsgs2.order() );
		BOOST_CHECK_EQUAL( bsgs.order(), bsgs2.order() );
	}
}

void checkGroup(const GroupInformation &info) {
	checkGroup<ConstructDeterministic1<Permutation, ExplicitTransversal<Permutation> > >(info);
	checkGroup<ConstructDeterministic1<Permutation, SchreierTreeTransversal<Permutation> > >(info);
	checkGroup<ConstructDeterministic1<PermutationWord, SchreierTreeTransversal<PermutationWord> > >(info);
	
	checkGroup<ConstructDeterministic2<Permutation, ExplicitTransversal<Permutation> > >(info);
	checkGroup<ConstructDeterministic2<Permutation, SchreierTreeTransversal<Permutation> > >(info);
	
	checkGroup<ConstructRandomized<Permutation, ExplicitTransversal<Permutation> > >(info);
	checkGroup<ConstructRandomized<Permutation, SchreierTreeTransversal<Permutation> > >(info);
}


template<class PERM,class TRANS,class TRANSPOSE>
void checkTranspose(const GroupInformation &info, const BSGS<PERM,TRANS> &bsgs) {
	const unsigned int m = bsgs.B.size();
	for (unsigned int i=0; i<m-1; ++i) {
		BSGS<PERM,TRANS> bsgs_copy(bsgs);
		TRANSPOSE bt;
		bt.transpose(bsgs_copy, i);
		
		BOOST_REQUIRE_EQUAL( bsgs_copy.B.size(), bsgs.B.size() );
		BOOST_CHECK_EQUAL( bsgs_copy.order(), bsgs.order() );
		unsigned int transversalProduct = 1;
		BOOST_FOREACH(const TRANS &U, bsgs_copy.U) {
			transversalProduct *= U.size();
		}
		BOOST_CHECK_EQUAL( transversalProduct, bsgs.order() );
		
		BOOST_CHECK_EQUAL( bsgs_copy.B[i], bsgs.B[i+1] );
		BOOST_CHECK_EQUAL( bsgs_copy.B[i+1], bsgs.B[i] );
		for (unsigned int j=0; j<m; ++j) {
			if (j == i || j == i+1)
				continue;
			BOOST_CHECK_EQUAL( bsgs_copy.B[j], bsgs.B[j] );
		}
	}
}

template<class PERM,class TRANS>
void checkTranspose(const GroupInformation &info) {
	BSGS<PERM,TRANS> bsgs = construct<PERM,TRANS>(info);
	
	checkTranspose<PERM,TRANS,DeterministicBaseTranspose<PERM,TRANS> >(info, bsgs);
	checkTranspose<PERM,TRANS,RandomBaseTranspose<PERM,TRANS> >(info, bsgs);
}

template<class PERM,class TRANS,class BASECHANGE>
void checkChange(const GroupInformation &info) {
	BSGS<PERM,TRANS> bsgs = construct<PERM,TRANS>(info);
	BSGS<PERM,TRANS> bsgs_copy(bsgs);
    
	BASECHANGE bc(bsgs);
	std::vector<unsigned long> newBase(5);
	newBase[0] = 0;
	newBase[1] = 2;
	newBase[2] = 4;
	newBase[3] = 6;
	newBase[4] = 8;
	bc.change(bsgs_copy, newBase.begin(), newBase.end());
    
	BOOST_REQUIRE_GE( bsgs_copy.B.size(), newBase.size() );
	BOOST_CHECK_EQUAL( bsgs_copy.order(), bsgs.order() );
	unsigned int transversalProduct = 1;
	BOOST_FOREACH(const TRANS &U, bsgs_copy.U) {
		transversalProduct *= U.size();
	}
	BOOST_CHECK_EQUAL( transversalProduct, bsgs.order() );
	
	for (unsigned int i=0; i<newBase.size(); ++i)
		BOOST_CHECK_EQUAL( newBase[i], bsgs_copy.B[i] );
}

BOOST_AUTO_TEST_CASE( construction_test33 )
{
	checkGroup(info_test33);
}

BOOST_AUTO_TEST_CASE( construction_trivial )
{
	checkGroup(info_trivial);
}

BOOST_AUTO_TEST_CASE( construction_test1997 )
{
	checkGroup(info_1997);
}

BOOST_AUTO_TEST_CASE( construction_psu4_3 )
{
	checkGroup(info_psu4_3);
}

BOOST_AUTO_TEST_CASE( construction_S6 )
{
	checkGroup(info_S6);
}

BOOST_AUTO_TEST_CASE( transpose_test1997 )
{
	typedef Permutation PERM;
	typedef ExplicitTransversal<PERM> TRANS;
	checkTranspose<PERM,TRANS>(info_1997);
}

BOOST_AUTO_TEST_CASE( transpose_psu4_3 )
{
	typedef PermutationWord PERM;
	typedef SchreierTreeTransversal<PERM> TRANS;
	checkTranspose<PERM,TRANS>(info_psu4_3);
}


BOOST_AUTO_TEST_CASE( change_test1997_simple )
{
	typedef Permutation PERM;
	typedef ExplicitTransversal<PERM> TRANS;
	checkChange<PERM,TRANS,SimpleBaseChange<PERM,TRANS,DeterministicBaseTranspose<PERM,TRANS> > >(info_1997);
	checkChange<PERM,TRANS,SimpleBaseChange<PERM,TRANS,RandomBaseTranspose<PERM,TRANS> > >(info_1997);
}

BOOST_AUTO_TEST_CASE( change_test1997_conjugating )
{
	typedef Permutation PERM;
	typedef ExplicitTransversal<PERM> TRANS;
	checkChange<PERM,TRANS,ConjugatingBaseChange<PERM,TRANS,DeterministicBaseTranspose<PERM,TRANS> > >(info_1997);
	checkChange<PERM,TRANS,ConjugatingBaseChange<PERM,TRANS,RandomBaseTranspose<PERM,TRANS> > >(info_1997);
}

BOOST_AUTO_TEST_CASE( change_trivial_conjugating )
{
	typedef Permutation PERM;
	typedef ExplicitTransversal<PERM> TRANS;
	checkChange<PERM,TRANS,ConjugatingBaseChange<PERM,TRANS,DeterministicBaseTranspose<PERM,TRANS> > >(info_trivial);
	checkChange<PERM,TRANS,ConjugatingBaseChange<PERM,TRANS,RandomBaseTranspose<PERM,TRANS> > >(info_trivial);
}

BOOST_AUTO_TEST_CASE( cyclic_construction )
{
	typedef Permutation PERM;
	typedef SchreierTreeTransversal<PERM> TRANS;
	for (unsigned int n = 2; n < 13; ++n) {
		BSGS<PERM, TRANS> cyclicBSGS = constructCyclicGroup<PERM, TRANS>(n);
		BOOST_CHECK_EQUAL( cyclicBSGS.n, n );
		BOOST_CHECK_EQUAL( cyclicBSGS.order<unsigned int>(), n );
		// a base of one point suffices with a cyclic group
		BOOST_CHECK_EQUAL( cyclicBSGS.B.size(), 1 );
		BOOST_CHECK_EQUAL( cyclicBSGS.U.size(), 1 );
	}
}


template<class PERM,class TRANS>
void checkRedundantStrongGenerators(const GroupInformation& info) {
	BSGS<PERM,TRANS> bsgs = ConstructDeterministic1<PERM, TRANS>()(info);
	BSGS<PERM,TRANS> bsgsCopy(bsgs);
	BSGSRandomGenerator<PERM,TRANS> randGen(bsgs);
	
	for (unsigned int i = 0; i < info.n; ++i) {
		typename PERM::ptr p(new PERM(randGen.next()));
		bsgsCopy.S.push_front(p);
	}
	
	bsgsCopy.stripRedundantStrongGenerators();
	
	SchreierSimsConstruction<PERM,TRANS> ssc(bsgsCopy.n);
	BSGS<PERM,TRANS> bsgsStripped = ssc.construct(bsgsCopy.S.begin(), bsgsCopy.S.end(), bsgs.B.begin(), bsgs.B.end());
	
	BOOST_CHECK_EQUAL(bsgs.order(), bsgsStripped.order());
	BOOST_CHECK_GE(bsgsCopy.S.size(), bsgsStripped.S.size());
	BOOST_CHECK_LE(bsgsCopy.S.size(), std::log(bsgs.order()) / std::log(2));
	
	// stripRedundantStrongGenerators has to be idempotent
	const unsigned int oldGeneratorNumber = bsgsCopy.S.size();
	bsgsCopy.stripRedundantStrongGenerators();
	BOOST_CHECK_EQUAL(oldGeneratorNumber, bsgsCopy.S.size());
}

BOOST_AUTO_TEST_CASE( strip_redundant_generators )
{
	typedef Permutation PERM;
	typedef SchreierTreeTransversal<PERM> TRANS;
	
	checkRedundantStrongGenerators<PERM,TRANS>(info_1997);
	checkRedundantStrongGenerators<PERM,TRANS>(info_S6);
	checkRedundantStrongGenerators<PERM,TRANS>(info_A9);
	checkRedundantStrongGenerators<PERM,TRANS>(info_S3wrS5);
	checkRedundantStrongGenerators<PERM,TRANS>(info_testThesis);
	checkRedundantStrongGenerators<PERM,TRANS>(info_test33);
	checkRedundantStrongGenerators<PERM,TRANS>(info_cyclic37_2);
	checkRedundantStrongGenerators<PERM,TRANS>(info_e6);
	checkRedundantStrongGenerators<PERM,TRANS>(info_myciel3);
	checkRedundantStrongGenerators<PERM,TRANS>(info_cov1075);
	checkRedundantStrongGenerators<PERM,TRANS>(info_metric5);
}
