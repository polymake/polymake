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
#define BOOST_TEST_MODULE transversal test
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>

#include <permlib/permutation.h>
#include <permlib/permutationword.h>
#include <permlib/bsgs.h>

#include <permlib/transversal/shallow_schreier_tree_transversal.h>
#include <permlib/transversal/schreier_tree_transversal.h>
#include <permlib/transversal/explicit_transversal.h>
//#include <permlib/construct/schreier_sims_construction.h>

#include "test-common.h"

using namespace permlib;
using namespace permlib::test;

template<class PERM, class TRANS>
void checkTransversal(const GroupInformation &info) {
	std::list<typename PERM::ptr> generators;
	const unsigned int n = readGroup<PERM>(info.filename, generators);

	for (unsigned long alpha = 0; alpha < n; ++alpha) {
		//std::cout << "alpha = " << alpha << std::endl;
		//std::cout << "elements: " << PERM::elementsNumber() << std::endl;
		boost::scoped_ptr<TRANS> U(new TRANS(n));
		U->orbit(alpha, generators);
		BOOST_CHECK_LE(U->size(), n);
		
		std::list<unsigned long>::const_iterator it;
		for (it = U->begin(); it != U->end(); ++it) {
			const unsigned long &beta = *it;
			boost::scoped_ptr<PERM> u_beta(U->at(beta));
			BOOST_REQUIRE(u_beta);
			
			BOOST_CHECK_EQUAL(*u_beta / alpha, beta);
			
			BOOST_CHECK_LE(U->m_statMaxDepth, n);
		}
		BOOST_CHECK_GE(U->m_statMaxDepth, 1);
		//std::cout << "depth " << U->m_statMaxDepth << " for " << U->size() << " orbit length" << std::endl;
		//std::cout << "STAT " << U->m_statAlphaCheck << std::endl;
	}
}

template<class PERM, class TRANS1, class TRANS2>
void compareTransversal(const GroupInformation &info) {
	std::list<typename PERM::ptr> generators;
	const unsigned int n = readGroup<PERM>(info.filename, generators);
	
	for (unsigned long alpha = 0; alpha < n; ++alpha) {
		boost::scoped_ptr<TRANS1> U1(new TRANS1(n));
		U1->orbit(alpha, generators);
		
		boost::scoped_ptr<TRANS2> U2(new TRANS2(n));
		U2->orbit(alpha, generators);
		
		BOOST_CHECK_EQUAL(U1->size(), U2->size());
		
		std::list<unsigned long>::const_iterator it;
		for (it = U1->begin(); it != U1->end(); ++it) {
			const unsigned long &beta = *it;
			boost::scoped_ptr<PERM> u_beta(U2->at(beta));
			BOOST_CHECK(u_beta);
		}
	}
}

BOOST_AUTO_TEST_CASE( transversal_schreier )
{
	typedef PermutationWord PERM;
    typedef SchreierTreeTransversal<PERM> TRANS;
	
	checkTransversal<PERM,TRANS>(info_cyclic500);
	checkTransversal<PERM,TRANS>(info_1997);
}


BOOST_AUTO_TEST_CASE( transversal_shallow_schreier )
{
	typedef PermutationWord PERM;
    typedef ShallowSchreierTreeTransversal<PERM> TRANS;
	
	checkTransversal<PERM,TRANS>(info_cyclic500);
	checkTransversal<PERM,TRANS>(info_1997);
}

BOOST_AUTO_TEST_CASE( transversal_compare_exact_shallow_schreier )
{
	typedef PermutationWord PERM;
	typedef ExplicitTransversal<PERM> TRANS1;
    typedef ShallowSchreierTreeTransversal<PERM> TRANS2;
	
	compareTransversal<PERM,TRANS1,TRANS2>(info_1997);
	compareTransversal<PERM,TRANS1,TRANS2>(info_cyclic500);
}




template<class PERM, class TRANS>
void conjugateGroup(const GroupInformation &info) {
	std::list<typename PERM::ptr> generators;
	BSGS<PERM,TRANS> bsgs = ConstructDeterministic1<PERM, TRANS>()(info);
	BSGS<PERM,TRANS> bsgsCopy(bsgs);

	const unsigned int n = bsgs.n;
	Permutation::perm permC(n);
	for (unsigned int i = 0; i < n; ++i)
		permC[i] = i;

	// introduce some random transpositions
	const unsigned int numberOfRandomTranspositions = 5;
	for (unsigned int i = 0; i < numberOfRandomTranspositions; ++i) {
		const dom_int a = randomInt(bsgs.n);
		const dom_int b = randomInt(bsgs.n);
		std::swap(permC[a], permC[b]);
	}
	const PERM c(permC);
	bsgsCopy.conjugate(c);

	for (unsigned int i = 0; i < bsgs.U.size(); ++i) {
		BOOST_REQUIRE_EQUAL(bsgsCopy.U[i].size(), bsgs.U[i].size());

		// test that all transversal values are correct
		for (unsigned int beta = 0; beta < bsgsCopy.n; ++beta) {
			boost::scoped_ptr<PERM> u_beta(bsgsCopy.U[i].at(beta));
			if (u_beta)
				BOOST_CHECK_EQUAL(*u_beta / bsgsCopy.B[i], beta);
		}

		// test that the orbit stored in the transversal is correct
		BOOST_FOREACH(unsigned long beta, bsgsCopy.U[i].pairIt()) {
			BOOST_REQUIRE_LT(beta, bsgsCopy.n);
			boost::scoped_ptr<PERM> u_beta(bsgsCopy.U[i].at(beta));
			BOOST_CHECK(u_beta);
		}
	}
	
	const PERM cInv(~c);
	for (unsigned int i = 0; i < 5; ++i) {
		const PERM bsgsCopyElement(cInv * bsgs.random() * c);
		BOOST_CHECK(bsgsCopy.sifts(bsgsCopyElement));
	}
}

template<class PERM, class TRANS>
void testConjugateGroup() {
	conjugateGroup<PERM,TRANS>(info_1997);
	conjugateGroup<PERM,TRANS>(info_cyclic500);
	conjugateGroup<PERM,TRANS>(info_test33);
}

BOOST_AUTO_TEST_CASE( transversal_conjugate_schreier )
{
	typedef Permutation PERM;
	typedef SchreierTreeTransversal<PERM> TRANS;
	// 50 runs with random conjugation
	for (unsigned int i = 0; i < 30; ++i)
		testConjugateGroup<PERM,TRANS>();
}

BOOST_AUTO_TEST_CASE( transversal_conjugate_explicit )
{
	typedef Permutation PERM;
	typedef ExplicitTransversal<PERM> TRANS;
	// 50 runs with random conjugation
	for (unsigned int i = 0; i < 100; ++i)
		testConjugateGroup<PERM,TRANS>();
}




template<class PERM, class TRANS>
void generateAllGroupElements(const BSGS<PERM,TRANS>& bsgs, std::list<typename PERM::ptr>& all, int k) {
	typedef typename PERM::ptr PERMptr;
	const std::list<PERMptr> tempList(all.begin(), all.end());
	all.clear();
	BOOST_FOREACH(const PERMptr& p, tempList) {
		BOOST_FOREACH(unsigned long beta, bsgs.U[k].pairIt()) {
			PERMptr g(new PERM(*p));
			boost::scoped_ptr<PERM> u_beta(bsgs.U[k].at(beta));
			BOOST_ASSERT(u_beta);
			*g *= *u_beta;
			all.push_back(g);
		}
	}
	if (k > 0)
		generateAllGroupElements(bsgs, all, k-1);
}

template<class PERM, class TRANS>
void checkGroupElements(const GroupInformation& info) {
	typedef typename PERM::ptr PERMptr;
	BSGS<PERM,TRANS> bsgs = ConstructDeterministic1<PERM, TRANS>()(info);
	std::list<PERMptr> allPerms;
	PERMptr identity(new PERM(bsgs.n));
	allPerms.push_back(identity);
	generateAllGroupElements(bsgs, allPerms, bsgs.U.size()-1);
	
	BOOST_REQUIRE_EQUAL(allPerms.size(), bsgs.order());
	
	boost::uint64_t fixSum = 0;
	BOOST_FOREACH(const PERMptr& p, allPerms) {
		for (dom_int i = 0; i < bsgs.n; ++i) 
			if (p->at(i) == i)
				++fixSum;
	}
	
	BOOST_CHECK_EQUAL(fixSum, bsgs.order());
}

void checkTransitiveGroupElements(const GroupInformation& info) {
	checkGroupElements<Permutation, ExplicitTransversal<Permutation> >(info);
	checkGroupElements<Permutation, SchreierTreeTransversal<Permutation> >(info);
}

BOOST_AUTO_TEST_CASE( group_elements ) 
{
	checkTransitiveGroupElements(info_1997);
	checkTransitiveGroupElements(info_test33);
	checkTransitiveGroupElements(info_cyclic500);
}
