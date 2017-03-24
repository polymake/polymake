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
#define BOOST_TEST_MODULE Test high level permutation group classes
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>

#include <permlib/permutation.h>
#include <permlib/permlib_api.h>
#include <permlib/abstract_bsgs.h>
#include <permlib/abstract_symmetric_product.h>

#include "test-common.h"

#include <boost/foreach.hpp>
#include <boost/iterator/counting_iterator.hpp>

using namespace permlib;
using namespace permlib::test;

template<class TRANSVERSAL>
void checkStabilizerBSGS(const GroupInformation& info, unsigned int setSize, unsigned int numberOfSets, bool computeSupport) {
	BOOST_CHECK_LE(setSize, info.n);
	
	typedef typename TRANSVERSAL::PERMtype PERM;
	boost::shared_ptr<BSGS<PERM, TRANSVERSAL> > bsgs(new BSGS<PERM, TRANSVERSAL>(ConstructDeterministic1<PERM, TRANSVERSAL>()(info)));
	AbstractBSGS<TRANSVERSAL> abstractBSGS(bsgs, computeSupport);
	
	for (unsigned int i = 0; i < numberOfSets; ++i) {
		std::set<dom_int> testSet;
		while (testSet.size() < setSize) {
			testSet.insert(randomInt(info.n));
		}
		
		boost::shared_ptr<BSGS<PERM, TRANSVERSAL> > stabilizerAPI = setStabilizer(*bsgs, testSet.begin(), testSet.end());
		
		boost::shared_ptr<AbstractPermutationGroup> stabilizer(abstractBSGS.setStabilizer(std::vector<dom_int>(testSet.begin(), testSet.end())));
		BOOST_CHECK_EQUAL( stabilizerAPI->order(), stabilizer->order() );
		
		boost::shared_ptr<AbstractPermutationGroup> stabilizer2(stabilizer->setStabilizer(std::vector<dom_int>(testSet.begin(), testSet.end())));
		BOOST_CHECK_EQUAL( stabilizerAPI->order(), stabilizer2->order() );
	}
}

AbstractSymmetricProduct* constructTestSymProduct1() {
	std::vector<dom_int> orb1;
	orb1.push_back(0);
	orb1.push_back(2);
	orb1.push_back(4);
	orb1.push_back(5);
	
	std::vector<dom_int> orb2;
	orb2.push_back(6);
	orb2.push_back(7);
	
	std::list<std::vector<dom_int> > symOrbits;
	symOrbits.push_back(orb1);
	symOrbits.push_back(orb2);
	
	return new AbstractSymmetricProduct(symOrbits.begin(), symOrbits.end());
}

AbstractSymmetricProduct* constructTestSymProduct2() {
	std::vector<std::vector<dom_int> > indices(2);
	indices[0].push_back(0);
	indices[0].push_back(1);
	indices[0].push_back(2);
	indices[1].push_back(3);
	indices[1].push_back(4);
	indices[1].push_back(5);
	return new AbstractSymmetricProduct(indices.begin(), indices.end());
}


template<class TRANSVERSAL>
void checkOrbits(const GroupInformation& info) {
	typedef typename TRANSVERSAL::PERMtype PERM;
	boost::shared_ptr<BSGS<PERM, TRANSVERSAL> > bsgs(new BSGS<PERM, TRANSVERSAL>(ConstructDeterministic1<PERM, TRANSVERSAL>()(info)));
	AbstractBSGS<TRANSVERSAL> abstractBSGS(bsgs);
	
	AbstractPermutationGroup::OrbitList* orbitList = abstractBSGS.orbits();
	
	BOOST_REQUIRE( orbitList );
	
	BOOST_CHECK_EQUAL( orbitList->size(), 1 );
	
	delete orbitList;
}

template<class TRANSVERSAL>
void checkOrder(const GroupInformation& info) {
	typedef typename TRANSVERSAL::PERMtype PERM;
	boost::shared_ptr<BSGS<PERM, TRANSVERSAL> > bsgs(new BSGS<PERM, TRANSVERSAL>(ConstructDeterministic1<PERM, TRANSVERSAL>()(info)));
	AbstractBSGS<TRANSVERSAL> abstractBSGS(bsgs);
	
	BOOST_CHECK_EQUAL( abstractBSGS.order(), bsgs->order() );
}

BOOST_AUTO_TEST_CASE( setstabilizer )
{
	// we use elementary permutations
	typedef Permutation PERM;
	// and Schreier tree transversals
	typedef SchreierTreeTransversal<PERM> TRANSVERSAL;
	
	const bool computeSupport[2] = { true, false };

	for (int i = 0; i < 2; ++i) {
		checkStabilizerBSGS<TRANSVERSAL>(info_testThesis, 3, 10, computeSupport[i]);
		checkStabilizerBSGS<TRANSVERSAL>(info_1997, 3, 10, computeSupport[i]);
		checkStabilizerBSGS<TRANSVERSAL>(info_cyclic10, 2, 30, computeSupport[i]);
		checkStabilizerBSGS<TRANSVERSAL>(info_S6_3, 4, 30, computeSupport[i]);
		checkStabilizerBSGS<TRANSVERSAL>(info_e6, 4, 15, computeSupport[i]);
	}
}

BOOST_AUTO_TEST_CASE( orbitsComputation )
{
	// we use elementary permutations
	typedef Permutation PERM;
	// and Schreier tree transversals
	typedef SchreierTreeTransversal<PERM> TRANSVERSAL;
	
	checkOrbits<TRANSVERSAL>(info_testThesis);
	checkOrbits<TRANSVERSAL>(info_1997);
	checkOrbits<TRANSVERSAL>(info_cyclic10);
}

BOOST_AUTO_TEST_CASE( order )
{
	// we use elementary permutations
	typedef Permutation PERM;
	// and Schreier tree transversals
	typedef SchreierTreeTransversal<PERM> TRANSVERSAL;
	
	checkOrder<TRANSVERSAL>(info_testThesis);
	checkOrder<TRANSVERSAL>(info_1997);
	checkOrder<TRANSVERSAL>(info_cyclic10);
	checkOrder<TRANSVERSAL>(info_S6_3);
	checkOrder<TRANSVERSAL>(info_e6);
}


BOOST_AUTO_TEST_CASE( order_sym )
{
	AbstractSymmetricProduct* asp = 0;
	
	asp = constructTestSymProduct1();
	BOOST_REQUIRE( asp );
	BOOST_CHECK_EQUAL( asp->order(), 48 );
	delete asp;
	
	asp = constructTestSymProduct2();
	BOOST_REQUIRE( asp );
	BOOST_CHECK_EQUAL( asp->order(), 36 );
	delete asp;
}

BOOST_AUTO_TEST_CASE( setstabilizer_sym )
{
	AbstractSymmetricProduct* asp = 0;
	AbstractPermutationGroup* stabilizer = 0;
	
	asp = constructTestSymProduct2();
	BOOST_REQUIRE( asp );
	
	std::vector<dom_int> delta(3);
	delta[0] = 0;
	delta[1] = 3;
	delta[2] = 5;
	stabilizer = asp->setStabilizer(delta);
	
	BOOST_REQUIRE( stabilizer );
	BOOST_CHECK_EQUAL( stabilizer->order(), 4 );
	
	delete stabilizer;
	delete asp;
	
	asp = constructTestSymProduct1();
	delta[0] = 3;
	delta[1] = 6;
	delta[2] = 7;
	stabilizer = asp->setStabilizer(delta);
	
	BOOST_REQUIRE( stabilizer );
	// set is already stabilized
	BOOST_CHECK_EQUAL( stabilizer->order(), asp->order() );
	delete stabilizer;
	
	delete asp;
}

void testLexmin2(const AbstractPermutationGroup* group) {
	BOOST_REQUIRE( group );
	
	std::vector<dom_int> delta(3);
	delta[0] = 0;
	delta[1] = 5;
	delta[2] = 3;
	
	std::vector<dom_int> rank(4);
	rank[0] = 0;
	rank[1] = 3;
	rank[2] = 4;
	rank[3] = 5;
	
	BOOST_CHECK( ! group->isLexMinSet(delta, rank) );
	std::swap(rank[1], rank[3]);
	BOOST_CHECK( ! group->isLexMinSet(delta, rank) );
	std::swap(rank[2], rank[3]);
	BOOST_CHECK( group->isLexMinSet(delta, rank) );
}

void testLexmin1(const AbstractPermutationGroup* group) {
	BOOST_REQUIRE( group );
	
	std::vector<dom_int> delta(3);
	delta[0] = 4;
	delta[1] = 3;
	delta[2] = 6;
	
	std::vector<dom_int> rank(boost::counting_iterator<dom_int>(0), boost::counting_iterator<dom_int>(8));
	BOOST_CHECK( ! group->isLexMinSet(delta, rank) );
	delta[0] = 0;
	delta[2] = 7;
	BOOST_CHECK( ! group->isLexMinSet(delta, rank) );
	delta[2] = 6;
	BOOST_CHECK( group->isLexMinSet(delta, rank) );
	
	rank = std::vector<dom_int>(rank.rbegin(), rank.rend());
	BOOST_CHECK( ! group->isLexMinSet(delta, rank) );
	delta[2] = 7;
	BOOST_CHECK( ! group->isLexMinSet(delta, rank) );
	delta[0] = 5;
	BOOST_CHECK( group->isLexMinSet(delta, rank) );
	std::swap(delta[0], delta[2]);
	BOOST_CHECK( group->isLexMinSet(delta, rank) );


	// check set that is not moved by the group

	delta.clear();
	delta.resize(2);
	delta[0] = 1;
	delta[1] = 3;

	rank = std::vector<dom_int>(boost::counting_iterator<dom_int>(0), boost::counting_iterator<dom_int>(4));
	BOOST_CHECK( group->isLexMinSet(delta, rank) );
}


BOOST_AUTO_TEST_CASE( lexmin_sym )
{
	AbstractPermutationGroup* group = 0;
	group = constructTestSymProduct2();
	testLexmin2(group);
	delete group;
	
	typedef SchreierTreeTransversal<Permutation> TRANSVERSAL;
	typedef TRANSVERSAL::PERMtype PERM;
	boost::shared_ptr<BSGS<PERM, TRANSVERSAL> > bsgs(new BSGS<PERM, TRANSVERSAL>(ConstructDeterministic1<PERM, TRANSVERSAL>()(info_lexminTest2)));
	group = new AbstractBSGS<TRANSVERSAL>(bsgs);
	testLexmin2(group);
	delete group;
	
	group = constructTestSymProduct1();
	BOOST_REQUIRE( group );
	testLexmin1(group);
	delete group;
	
	bsgs.reset(new BSGS<PERM, TRANSVERSAL>(ConstructDeterministic1<PERM, TRANSVERSAL>()(info_lexminTest1)));
	group = new AbstractBSGS<TRANSVERSAL>(bsgs);
	testLexmin1(group);
	delete group;
}
