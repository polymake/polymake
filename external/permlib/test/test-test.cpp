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
#define BOOST_TEST_MODULE group test tests
#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp> 
using boost::test_tools::output_test_stream;

#include <permlib/permutation.h>
#include <permlib/transversal/schreier_tree_transversal.h>
#include <permlib/test/giant_test.h>
#include <permlib/test/primitivity_test.h>
#include <permlib/test/primitivity_sgs_test.h>
#include <permlib/test/type_recognition.h>


#include "test-common.h"
#include <permlib/test/type_recognition_heuristic.h>

using namespace permlib;
using namespace permlib::test;

BOOST_AUTO_TEST_CASE( giant_test )
{
	typedef Permutation PERM;
	
	const unsigned int n = 10;
	const double eps = 1e-4;
	std::list<PERM::ptr> generators;
	GiantTest<PERM> giantTest;
	
	PERM::ptr a(new PERM(n, "1 2 3 4 5 6 7 8 9 10"));
	PERM::ptr b(new PERM(n, "1 2"));
	
	generators.push_back(a);
	generators.push_back(b);
	
	GiantTestBase::GiantGroupType type = giantTest.determineGiantType(eps, n, generators.begin(), generators.end());
	BOOST_CHECK_EQUAL(type, GiantTestBase::Symmetric);
	
	generators.clear();
	a.reset(new PERM(n, "1 2 3 4 5 6 7 8 9"));
	b.reset(new PERM(n, "8 9 10"));
	
	generators.push_back(a);
	generators.push_back(b);
	
	type = giantTest.determineGiantType(eps, n, generators.begin(), generators.end());
	BOOST_CHECK_EQUAL(type, GiantTestBase::Alternating);
	
	generators.clear();
	a.reset(new PERM(n, "1 3 8,2 6 9 10 7 4"));
	b.reset(new PERM(n, "1 5 6 3 9 7 2 10"));
	
	generators.push_back(a);
	generators.push_back(b);
	
	type = giantTest.determineGiantType(eps, n, generators.begin(), generators.end());
	BOOST_CHECK_EQUAL(type, GiantTestBase::None);
}


std::pair<bool, std::vector<dom_int> > testMinimalBlock(const GroupInformation& info) {
	typedef Permutation PERM;
	typedef ExplicitTransversal<PERM> TRANS;
	
	BSGS<PERM, TRANS> bsgs = construct<PERM, TRANS>(info);
	
	std::list<PERM::ptr> S1;
	PointwiseStabilizerPredicate<PERM> stabPred(bsgs.B[0]);
	BOOST_FOREACH(const PERM::ptr& p, bsgs.S) {
		if (stabPred(p))
			S1.push_back(p);
	}
	
	PrimitivitySGSTest<TRANS> primitivityTest(bsgs.S.begin(), bsgs.S.end(), bsgs.B[0], S1.begin(), S1.end(), bsgs.U[0]);
	std::vector<dom_int> minimalBlock;
	const bool isPrimitive = primitivityTest.blockOfImprimitivity(&minimalBlock);
	std::sort(minimalBlock.begin(), minimalBlock.end());
	return std::make_pair(isPrimitive, minimalBlock);
}

std::pair<bool, std::vector<dom_int> > testMinimalBlock2(const GroupInformation& info) {
	typedef Permutation PERM;
	typedef ExplicitTransversal<PERM> TRANS;
	
	BSGS<PERM, TRANS> bsgs = construct<PERM, TRANS>(info);
	
	PrimitivityTest<PERM> primitivityTest(bsgs.n, bsgs.S.begin(), bsgs.S.end());
	std::vector<dom_int> minimalBlock;
	const bool isPrimitive = primitivityTest.blockOfImprimitivity(&minimalBlock);
	std::sort(minimalBlock.begin(), minimalBlock.end());
	return std::make_pair(isPrimitive, minimalBlock);
}

BOOST_AUTO_TEST_CASE( primitivity_test )
{
	std::pair<bool, std::vector<dom_int> > minimalBlock;
	
	minimalBlock = testMinimalBlock(info_cyclic10);
	BOOST_CHECK( !minimalBlock.first );
	BOOST_REQUIRE_EQUAL( minimalBlock.second.size(), 5 );
	for (unsigned int i = 0 ; i < 5; ++i)
		BOOST_CHECK_EQUAL( minimalBlock.second[i], 2 * i );
	
	
	minimalBlock = testMinimalBlock(info_1997);
	// group is 2-transitive, hence primitive
	BOOST_CHECK( minimalBlock.first );
	BOOST_REQUIRE_EQUAL( minimalBlock.second.size(), info_1997.n );
	
	
	minimalBlock = testMinimalBlock(info_test33);
	// group is 2-transitive, hence primitive
	BOOST_CHECK( minimalBlock.first );
	BOOST_REQUIRE_EQUAL( minimalBlock.second.size(), info_test33.n );
	
	
	minimalBlock = testMinimalBlock(info_S3wrS5);
	BOOST_CHECK( !minimalBlock.first );
	BOOST_REQUIRE_EQUAL( minimalBlock.second.size(), 3 );
	for (unsigned int i = 0 ; i < 3; ++i)
		BOOST_CHECK_EQUAL( minimalBlock.second[i], i );
}

BOOST_AUTO_TEST_CASE( primitivity2_test )
{
	std::pair<bool, std::vector<dom_int> > minimalBlock;
	
	minimalBlock = testMinimalBlock2(info_cyclic10);
	BOOST_CHECK( !minimalBlock.first );
	BOOST_CHECK_LT( minimalBlock.second.size(), info_cyclic10.n );
	BOOST_CHECK_EQUAL( info_cyclic10.n % minimalBlock.second.size(), 0 );
	
	
	minimalBlock = testMinimalBlock2(info_1997);
	// group is 2-transitive, hence primitive
	BOOST_CHECK( minimalBlock.first );
	BOOST_CHECK_EQUAL( minimalBlock.second.size(), info_1997.n );
	
	
	minimalBlock = testMinimalBlock2(info_test33);
	// group is 2-transitive, hence primitive
	BOOST_CHECK( minimalBlock.first );
	BOOST_CHECK_EQUAL( minimalBlock.second.size(), info_test33.n );
	
	
	minimalBlock = testMinimalBlock2(info_S3wrS5);
	BOOST_CHECK( !minimalBlock.first );
	BOOST_CHECK_LT( minimalBlock.second.size(), info_S3wrS5.n );
	BOOST_CHECK_EQUAL( info_S3wrS5.n % minimalBlock.second.size(), 0 );
}

BOOST_AUTO_TEST_CASE( recognition_test ) 
{
	typedef Permutation PERM;
	typedef SchreierTreeTransversal<PERM> TRANS;
	typedef BSGS<PERM,TRANS> PermutationGroup;
	typedef boost::shared_ptr<PermutationGroup> PermutationGroupPtr;
	typedef TypeRecognition<PERM,TRANS> TypeRec;
	output_test_stream ots;
	GroupType* type;
	NamedGroupType* namedType;
	WreathSymmetricGroupType* wreathType;
	TypeRec* typeRecognition;

	PermutationGroupPtr bsgsS6 = PermutationGroupPtr(new PermutationGroup(construct<PERM,TRANS>( info_S6 )));
	typeRecognition = new TypeRec(bsgsS6);
	type = typeRecognition->analyze();
	delete typeRecognition;
	BOOST_REQUIRE( type );
	BOOST_REQUIRE_EQUAL( type->type(), GroupType::Named );
	namedType = dynamic_cast<NamedGroupType*>(type);
	BOOST_CHECK( strcmp(namedType->name(), "S") == 0 );
	BOOST_CHECK( namedType->isNaturalAction() );
	BOOST_CHECK_EQUAL( namedType->typeDegree(), info_S6.n );
	BOOST_CHECK_EQUAL( namedType->realDegree(), info_S6.n );
	type->writeToStream(ots);
	BOOST_CHECK( ! ots.is_empty(false) );
	BOOST_CHECK( ots.is_equal("S_6") );
	delete type;
	
	PermutationGroupPtr bsgsA9 = PermutationGroupPtr(new PermutationGroup(construct<PERM,TRANS>( info_A9 )));
	typeRecognition = new TypeRec(bsgsA9);
	type = typeRecognition->analyze();
	delete typeRecognition;
	BOOST_REQUIRE( type );
	BOOST_REQUIRE_EQUAL( type->type(), GroupType::Named );
	namedType = dynamic_cast<NamedGroupType*>(type);
	BOOST_CHECK( strcmp(namedType->name(), "A") == 0 );
	BOOST_CHECK( namedType->isNaturalAction() );
	BOOST_CHECK_EQUAL( namedType->typeDegree(), info_A9.n );
	BOOST_CHECK_EQUAL( namedType->realDegree(), info_A9.n );
	type->writeToStream(ots);
	BOOST_CHECK( ! ots.is_empty(false) );
	BOOST_CHECK( ots.is_equal("A_9") );
	delete type;

	PermutationGroupPtr bsgsA12 = PermutationGroupPtr(new PermutationGroup(construct<PERM,TRANS>( info_A12 )));
	typeRecognition = new TypeRec(bsgsA12);
	type = typeRecognition->analyze();
	delete typeRecognition;
	BOOST_REQUIRE( type );
	BOOST_REQUIRE_EQUAL( type->type(), GroupType::Named );
	namedType = dynamic_cast<NamedGroupType*>(type);
	BOOST_CHECK( strcmp(namedType->name(), "A") == 0 );
	BOOST_CHECK( namedType->isNaturalAction() );
	BOOST_CHECK_EQUAL( namedType->typeDegree(), info_A12.n );
	BOOST_CHECK_EQUAL( namedType->realDegree(), info_A12.n );
	type->writeToStream(ots);
	BOOST_CHECK( ! ots.is_empty(false) );
	BOOST_CHECK( ots.is_equal("A_12") );
	delete type;

	PermutationGroupPtr bsgsS6_3 = PermutationGroupPtr(new PermutationGroup(construct<PERM,TRANS>( info_S6_3 )));
	typeRecognition = new TypeRec(bsgsS6_3);
	type = typeRecognition->analyze();
	delete typeRecognition;
	BOOST_REQUIRE( type );
	BOOST_REQUIRE_EQUAL( type->type(), GroupType::Named );
	namedType = dynamic_cast<NamedGroupType*>(type);
	BOOST_CHECK( strcmp(namedType->name(), "S") == 0 );
	BOOST_CHECK( ! namedType->isNaturalAction() );
	BOOST_CHECK_EQUAL( namedType->typeDegree(), 6 );
	BOOST_CHECK_EQUAL( namedType->realDegree(), info_S6_3.n );
	type->writeToStream(ots);
	BOOST_CHECK( ! ots.is_empty(false) );
	BOOST_CHECK( ots.is_equal("ISO( S_6 , 18 )") );
	delete type;
	
	PermutationGroupPtr bsgsC37 = PermutationGroupPtr(new PermutationGroup(constructCyclicGroup<PERM, TRANS>( 37 )));
	typeRecognition = new TypeRec(bsgsC37);
	type = typeRecognition->analyze();
	delete typeRecognition;
	BOOST_REQUIRE( type );
	BOOST_REQUIRE_EQUAL( type->type(), GroupType::Named );
	namedType = dynamic_cast<NamedGroupType*>(type);
	BOOST_CHECK( strcmp(namedType->name(), "C") == 0 );
	BOOST_CHECK( namedType->isNaturalAction() );
	BOOST_CHECK_EQUAL( namedType->typeDegree(), 37 );
	BOOST_CHECK_EQUAL( namedType->realDegree(), 37 );
	type->writeToStream(ots);
	BOOST_CHECK( ! ots.is_empty(false) );
	BOOST_CHECK( ots.is_equal("C_37") );
	delete type;
	
	PermutationGroupPtr bsgsC37_2 = PermutationGroupPtr(new PermutationGroup(construct<PERM,TRANS>( info_cyclic37_2 )));
	typeRecognition = new TypeRec(bsgsC37_2);
	type = typeRecognition->analyze();
	delete typeRecognition;
	BOOST_REQUIRE( type );
	BOOST_REQUIRE_EQUAL( type->type(), GroupType::Named );
	namedType = dynamic_cast<NamedGroupType*>(type);
	BOOST_CHECK( strcmp(namedType->name(), "C") == 0 );
	BOOST_CHECK( ! namedType->isNaturalAction() );
	BOOST_CHECK_EQUAL( namedType->typeDegree(), info_cyclic37_2.order );
	BOOST_CHECK_EQUAL( namedType->realDegree(), info_cyclic37_2.n );
	type->writeToStream(ots);
	BOOST_CHECK( ! ots.is_empty(false) );
	BOOST_CHECK( ots.is_equal("ISO( C_37 , 74 )") );
	delete type;
	
	PermutationGroupPtr bsgsS3wrS5 = PermutationGroupPtr(new PermutationGroup(construct<PERM,TRANS>( info_S3wrS5 )));
	typeRecognition = new TypeRec(bsgsS3wrS5);
	type = typeRecognition->analyze();
	delete typeRecognition;
	BOOST_REQUIRE( type );
	BOOST_REQUIRE_EQUAL( type->type(), GroupType::WreathSymmetric);
	wreathType = dynamic_cast<WreathSymmetricGroupType*>(type);
	BOOST_CHECK( wreathType->isNaturalAction() );
	BOOST_CHECK_EQUAL( wreathType->degreeG(), 3 );
	BOOST_CHECK_EQUAL( wreathType->degreeH(), 5 );
	BOOST_CHECK_EQUAL( wreathType->realDegree(), info_S3wrS5.n );
	type->writeToStream(ots);
	BOOST_CHECK( ! ots.is_empty(false) );
	BOOST_CHECK( ots.is_equal("S_3 wr S_5") );
	delete type;
	
	PermutationGroupPtr bsgsS5wrS3 = PermutationGroupPtr(new PermutationGroup(construct<PERM,TRANS>( info_S5wrS3 )));
	typeRecognition = new TypeRec(bsgsS5wrS3);
	type = typeRecognition->analyze();
	delete typeRecognition;
	BOOST_REQUIRE( type );
	BOOST_REQUIRE_EQUAL( type->type(), GroupType::WreathSymmetric );
	wreathType = dynamic_cast<WreathSymmetricGroupType*>(type);
	BOOST_CHECK( wreathType->isNaturalAction() );
	BOOST_CHECK_EQUAL( wreathType->degreeG(), 5 );
	BOOST_CHECK_EQUAL( wreathType->degreeH(), 3 );
	BOOST_CHECK_EQUAL( wreathType->realDegree(), info_S5wrS3.n );
	type->writeToStream(ots);
	BOOST_CHECK( ! ots.is_empty(false) );
	BOOST_CHECK( ots.is_equal("S_5 wr S_3") );
	delete type;
	
	PermutationGroupPtr bsgsKlein4 = PermutationGroupPtr(new PermutationGroup(construct<PERM,TRANS>( info_Klein4 )));
	typeRecognition = new TypeRec(bsgsKlein4);
	type = typeRecognition->analyze();
	delete typeRecognition;
	BOOST_REQUIRE( type );
	BOOST_REQUIRE_EQUAL( type->type(), GroupType::DirectProduct );
	BOOST_CHECK( type->isNaturalAction() );
	BOOST_CHECK_EQUAL( type->realDegree(), info_Klein4.n );
	type->writeToStream(ots);
	BOOST_CHECK( ! ots.is_empty(false) );
	BOOST_CHECK( ots.is_equal("S_2 x S_2") );
	delete type;
	
	PermutationGroupPtr bsgsC4 = PermutationGroupPtr(new PermutationGroup(constructCyclicGroup<PERM, TRANS>( 4 )));
	typeRecognition = new TypeRec(bsgsC4);
	type = typeRecognition->analyze();
	delete typeRecognition;
	BOOST_REQUIRE( type );
	BOOST_REQUIRE_EQUAL( type->type(), GroupType::Named );
	namedType = dynamic_cast<NamedGroupType*>(type);
	BOOST_CHECK( strcmp(namedType->name(), "C") == 0 );
	BOOST_CHECK( namedType->isNaturalAction() );
	BOOST_CHECK_EQUAL( namedType->typeDegree(), 4 );
	BOOST_CHECK_EQUAL( namedType->realDegree(), 4 );
	type->writeToStream(ots);
	BOOST_CHECK( ! ots.is_empty(false) );
	BOOST_CHECK( ots.is_equal("C_4") );
	delete type;
	
	PermutationGroupPtr bsgsRout = PermutationGroupPtr(new PermutationGroup(construct<PERM,TRANS>( info_rout )));
	typeRecognition = new TypeRec(bsgsRout);
	type = typeRecognition->analyze();
	delete typeRecognition;
	BOOST_REQUIRE( type );
	BOOST_REQUIRE_EQUAL( type->type(), GroupType::Named );
	namedType = dynamic_cast<NamedGroupType*>(type);
	BOOST_CHECK( strcmp(namedType->name(), "S") == 0 );
	BOOST_CHECK( ! namedType->isNaturalAction() );
	BOOST_CHECK_EQUAL( namedType->typeDegree(), 5 );
	BOOST_CHECK_EQUAL( namedType->realDegree(), 555 );
	type->writeToStream(ots);
	BOOST_CHECK( ! ots.is_empty(false) );
	BOOST_CHECK( ots.is_equal("ISO( S_5 , 555 )") );
	delete type;
}

template<class PERM>
SymmetricGroupRecognitionHeuristic<PERM>* getGroupHeuristic(const GroupInformation& info) {
	GroupReader<PERM> reader;
	reader.read(info.filename);
	
	SymmetricGroupRecognitionHeuristic<PERM>* sHeur = new SymmetricGroupRecognitionHeuristic<PERM>(reader.n(), true);
	
	BOOST_FOREACH(typename PERM::ptr p, reader.generators()) {
		sHeur->addGenerator(*p);
	}
	return sHeur;
}

BOOST_AUTO_TEST_CASE( symmetric_group_heuristic )
{
	typedef Permutation PERM;
	SymmetricGroupRecognitionHeuristic<PERM>* sHeur = 0;
	
	sHeur = getGroupHeuristic<PERM>(info_S6);
	std::vector<std::list<dom_int> > orbits_S6;
	sHeur->symmetricGroupOrbits(orbits_S6);
	BOOST_CHECK_EQUAL( sHeur->unusedGenerators().size(), 0 );
	delete sHeur;
	
	BOOST_REQUIRE_EQUAL(orbits_S6.size(), 1);
	BOOST_CHECK_EQUAL(orbits_S6[0].size(), 6);
	
	sHeur = getGroupHeuristic<PERM>(info_tanglegram2);
	std::vector<std::list<dom_int> > orbits_tanglegram2;
	sHeur->symmetricGroupOrbits(orbits_tanglegram2);
	BOOST_CHECK_EQUAL( sHeur->unusedGenerators().size(), 38 );
	delete sHeur;
	
	std::vector<unsigned int> orbitSizeHistogram(info_tanglegram2.n);
	BOOST_FOREACH(const std::list<dom_int>& o, orbits_tanglegram2) {
		BOOST_ASSERT( o.size() < orbitSizeHistogram.size() );
		++orbitSizeHistogram[o.size()];
	}
	
	// check for orbits of a quite characteristic size
	BOOST_CHECK_EQUAL( orbitSizeHistogram[425], 1 );
	BOOST_CHECK_EQUAL( orbitSizeHistogram[297], 1 );
	BOOST_CHECK_EQUAL( orbitSizeHistogram[287], 1 );
	BOOST_CHECK_EQUAL( orbitSizeHistogram[236], 1 );
	BOOST_CHECK_EQUAL( orbitSizeHistogram[130], 1 );
	BOOST_CHECK_EQUAL( orbitSizeHistogram[108], 1 );
}

template<int N>
struct Factorial {
	enum FacHelper { value = N * Factorial<N - 1>::value };
};
 
template <>
struct Factorial<0> {
	enum FacHelper { value = 1 };
};

template<int DiagonalDegree>
void diagonalGroupTest(const GroupInformation& info, const char* diagonalTypeString) {
	typedef Permutation PERM;
	typedef SchreierTreeTransversal<PERM> TRANS;
	typedef BSGS<PERM,TRANS> PermutationGroup;
	typedef boost::shared_ptr<PermutationGroup> PermutationGroupPtr;
	typedef TypeRecognition<PERM,TRANS> TypeRec;
	output_test_stream ots;
	GroupType* type = 0;
	GroupType* subtype = 0;
	NamedGroupType* namedType;
	TypeRec* typeRecognition = 0;
	TypeRec* subtypeRecognition = 0;

	PermutationGroupPtr bsgs = PermutationGroupPtr(new PermutationGroup(construct<PERM,TRANS>( info )));
	typeRecognition = new TypeRec(bsgs);
	std::vector<dom_int> orbitCharacteristic;
	type = typeRecognition->largeSymmetricDiagonalSubgroup(orbitCharacteristic);
	delete typeRecognition;
	BOOST_REQUIRE( type );
	BOOST_REQUIRE_EQUAL( type->type(), GroupType::Named );
	namedType = dynamic_cast<NamedGroupType*>(type);
	BOOST_CHECK( strcmp(namedType->name(), "S") == 0 );
	BOOST_CHECK( ! namedType->isNaturalAction() );
	BOOST_CHECK_EQUAL( namedType->typeDegree(), DiagonalDegree );
	BOOST_CHECK_EQUAL( namedType->realDegree(), info.n );
	type->writeToStream(ots);
	BOOST_CHECK( ! ots.is_empty(false) );
	BOOST_CHECK( ots.is_equal(diagonalTypeString) );
	BOOST_REQUIRE_EQUAL( orbitCharacteristic.size(), info.n );
	
	std::vector<unsigned int> orbitHistogram(namedType->realDegree() / namedType->typeDegree());
	for (unsigned int i = 0; i < bsgs->n; ++i) {
		BOOST_REQUIRE_LT(orbitCharacteristic[i], orbitHistogram.size());
		++orbitHistogram[orbitCharacteristic[i]];
	}
	BOOST_FOREACH(const unsigned int h, orbitHistogram) {
		BOOST_REQUIRE_EQUAL(h, namedType->typeDegree());
	}
	
	PermutationGroupPtr sub = vectorStabilizer(*bsgs, orbitCharacteristic.begin(), orbitCharacteristic.end(), orbitHistogram.size() - 1);
	BOOST_REQUIRE( sub );
	BOOST_CHECK_GE(sub->order(), Factorial<DiagonalDegree>::value);
	
	subtypeRecognition = new TypeRec(sub);
	subtype = subtypeRecognition->analyze();
	BOOST_REQUIRE( subtype );
	BOOST_CHECK( subtype->equals(type) );
	
	delete type;
	delete subtype;
	delete subtypeRecognition;
}

BOOST_AUTO_TEST_CASE( largest_diagonal_test )
{
	diagonalGroupTest<4>(info_myciel4sub, "ISO( S_4 , 20 )");
	diagonalGroupTest<3>(info_S3wrS5,     "ISO( S_3 , 15 )");
}
