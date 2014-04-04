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
#define BOOST_TEST_MODULE orbit lex min
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>
#include <algorithm>

#include <permlib/permutation.h>
#include <permlib/bsgs.h>
#include <permlib/symmetric_group.h>
#include <permlib/transversal/explicit_transversal.h>
#include <permlib/construct/schreier_sims_construction.h>

#include "test-common.h"
#include <permlib/transversal/orbit_set.h>
#include <permlib/search/orbit_lex_min_search.h>
#include <permlib/search/classic/lex_smaller_image_search.h>

using namespace permlib;
using namespace permlib::test;
using namespace std;

template<class BSGSIN>
void testOneSet(const BSGSIN& bsgs, const unsigned long n, const dset& testSet, bool withStabilizer = false) {
	typedef typename BSGSIN::PERMtype PERM;

	OrbitSet<PERM,dset> orbit;
	orbit.orbit(testSet, bsgs.S, DSetAction<PERM>(n));
	typedef OrbitLexMinSearch<BSGSIN>  OrbitLexMinSearchClass;

	OrbitLexMinSearchClass orbLexMin(bsgs);
	const dset lexMin = orbLexMin.lexMin(testSet);
	//cout << "lexMin = " << lexMin << endl;

	BOOST_REQUIRE(orbit.contains(lexMin));

	BOOST_FOREACH(const dset& el, make_pair(orbit.begin(), orbit.end())) {
		//cout << el << " - " << lexMin << " : " << OrbitLexMinSearch<int>::isLexSmaller(el, lexMin) << endl;
		BOOST_CHECK(!OrbitLexMinSearchClass::isLexSmaller(el, lexMin));
	}
}

void testGroup(const GroupInformation& info, const unsigned long numberOfTestRuns, const long perCent, bool withStabilizer = false) {
	typedef Permutation PERM;
	typedef ExplicitTransversal<PERM> TRANS;

	const BSGS<PERM,TRANS> bsgs = construct<PERM,TRANS>(info);
	const unsigned long n = info.n;

	for (unsigned int i = 0; i < numberOfTestRuns; ++i) {
		dset test(n);
		for (unsigned int j = 0; j < n; ++j) {
			if (rand() % 100 > perCent)
				test.set(j, 1);
		}
		testOneSet(bsgs, n, test, withStabilizer);
	}
}
BOOST_AUTO_TEST_CASE( lex_min_symmetric )
{
	const unsigned long numberOfTestRuns = 10;
	typedef Permutation PERM;

	const unsigned long n = 10;
	const SymmetricGroup<PERM> s_n(n);

	for (unsigned int i = 0; i < numberOfTestRuns; ++i) {
		dset test(n);
		for (unsigned int j = 0; j < n; ++j) {
			if (rand() % 100 > 60)
				test.set(j, 1);
		}
		testOneSet(s_n, n, test);
	}
}

BOOST_AUTO_TEST_CASE( lex_min_leon1997 )
{
	testGroup(info_1997, 30, 60);
}

BOOST_AUTO_TEST_CASE( lex_min_e6 )
{
	testGroup(info_e6, 5, 60);
}

BOOST_AUTO_TEST_CASE( lex_min_cyclic500 )
{
	testGroup(info_cyclic500, 30, 60);
}


void testGroup(const GroupInformation& info, bool isTwoTransitive = true) {
	typedef Permutation PERM;
	typedef ExplicitTransversal<PERM> TRANS;
	const unsigned int baseSize = 5;
	const unsigned int baseOffset = 2;
	BOOST_ASSERT(info.n > baseSize + baseOffset);

	BSGS<PERM,TRANS> bsgs = construct<PERM,TRANS>(info);

	std::vector<unsigned int> base(baseSize);
	for (unsigned int i = 0; i < base.size(); ++i) {
		base[i] = i + baseOffset;
	}

	ConjugatingBaseChange<PERM, TRANS, RandomBaseTranspose<PERM,TRANS> > cbc(bsgs);
	cbc.change(bsgs, base.begin(), base.end());

	std::list<unsigned int> zeros, ones;
	for (unsigned int i = 0; i < baseSize; ++i) {
		if (i == 0)
			zeros.push_back(i + baseOffset);
		else
			ones.push_back(i + baseOffset);
	}
	classic::LexSmallerImageSearch<BSGS<PERM,TRANS>, TRANS> backtrackSearch(bsgs, 0);
	backtrackSearch.construct(zeros.begin(), zeros.end(), ones.begin(), ones.end());
	PERM::ptr g = backtrackSearch.searchCosetRepresentative();
	BOOST_CHECK( !g );

	if (isTwoTransitive) {
		zeros.clear();
		ones.clear();
		for (unsigned int i = 0; i < baseSize; ++i) {
			if (i == 2 || i == 0)
				zeros.push_back(i + baseOffset);
			else
				ones.push_back(i + baseOffset);
		}
		backtrackSearch.construct(zeros.begin(), zeros.end(), ones.begin(), ones.end());
		g = backtrackSearch.searchCosetRepresentative();
		BOOST_CHECK( g );
	}
	zeros.clear();
	ones.clear();
	for (unsigned int i = 0; i < baseSize; ++i) {
		if (i == baseSize - 1)
			zeros.push_back(i + baseOffset);
		else
			ones.push_back(i + baseOffset);
	}
	backtrackSearch.construct(zeros.begin(), zeros.end(), ones.begin(), ones.end());
	g = backtrackSearch.searchCosetRepresentative();
	BOOST_CHECK( g );

	zeros.clear();
	ones.clear();
	for (unsigned int i = 0; i < baseSize; ++i) {
		if (i == baseSize/2)
			zeros.push_back(i + baseOffset);
		else
			ones.push_back(i + baseOffset);
	}
	backtrackSearch.construct(zeros.begin(), zeros.end(), ones.begin(), ones.end());
	g = backtrackSearch.searchCosetRepresentative();
	BOOST_CHECK( g );
}

BOOST_AUTO_TEST_CASE( not_lex_min )
{
	typedef Permutation PERM;
	typedef ExplicitTransversal<PERM> TRANS;

	testGroup(info_cyclic10, false);
	testGroup(info_e6);
	testGroup(info_1997);
}
