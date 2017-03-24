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
#define BOOST_TEST_MODULE API test
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>
#include <vector>

#include <permlib/permlib_api.h>

#include "test-common.h"

using namespace permlib;
using namespace permlib::test;

BOOST_AUTO_TEST_CASE( orbit_ulong_trivial )
{
	const PermutationGroup group = construct<PERMUTATION,TRANSVERSAL>(info_trivial);
	std::list<boost::shared_ptr<OrbitAsSet> > o = orbits(group);
	
	BOOST_CHECK_EQUAL( o.size(), info_trivial.n );
}


BOOST_AUTO_TEST_CASE( orbit_stabilizer_ulong_test1997 )
{
	const PermutationGroup group = construct<PERMUTATION,TRANSVERSAL>(info_1997);
	std::list<boost::shared_ptr<OrbitAsSet> > o = orbits(group);
	
	BOOST_CHECK_EQUAL( o.size(), 1 );
	
	const unsigned long DeltaSize = 3;
	unsigned long Delta[DeltaSize] = {0,1,4};
	boost::shared_ptr<PermutationGroup> stab1 = setStabilizer(group, Delta, Delta+DeltaSize);
	BOOST_REQUIRE( stab1 );
	o = orbits(*stab1);
	
	BOOST_FOREACH(const boost::shared_ptr<OrbitAsSet>& orb, o) {
		if (orb->contains(Delta[0])) {
			BOOST_REQUIRE_EQUAL( orb->size(), DeltaSize );
			for (unsigned int i = 1; i < DeltaSize; ++i) {
				BOOST_CHECK( orb->contains(Delta[i]) );
			}
		}
	}
	
	BOOST_CHECK_EQUAL(o.size(), 3);
}

typedef std::vector<unsigned long> Vector;
template<class PERM>
struct VectorAction {
	Vector operator()(const PERM& p, const Vector& v) {
		Vector ret(v);
		for (unsigned int i = 0; i < v.size(); ++i) {
			ret[i] = v[p / i];
		}
		return ret;
	}
};

BOOST_AUTO_TEST_CASE( orbit_stabilizer_vector_test1997 )
{
	const PermutationGroup group = construct<PERMUTATION,TRANSVERSAL>(info_1997);
	
	Vector v(group.n);
	for (unsigned int i = 0; i < v.size(); ++i)
		v[i] = i;
	
	std::list<Vector> vectors;
	vectors.push_back(v);
	
	std::list<boost::shared_ptr<OrbitSet<PERMUTATION,Vector> > > o = orbits<Vector, VectorAction<PERMUTATION> >(group, vectors.begin(), vectors.end());
	
	BOOST_REQUIRE_EQUAL( o.size(), 1 );
	BOOST_CHECK_EQUAL( o.front()->size(), group.order() );
	BOOST_CHECK( o.front()->contains(v) );
	
	boost::shared_ptr<PermutationGroup> stab1 = setStabilizer(group, v.begin(), v.end());
	BOOST_REQUIRE( stab1 );
	o = orbits<Vector, VectorAction<PERMUTATION> >(*stab1, vectors.begin(), vectors.end());
	
	BOOST_REQUIRE_EQUAL( o.size(), 1 );
	BOOST_CHECK_EQUAL( o.front()->size(), stab1->order() );
	BOOST_CHECK( o.front()->contains(v) );
	
	OrbitSet<PERMUTATION,Vector>::const_iterator orbIt = o.front()->begin();
	const unsigned int magicNumber = 4267;
	BOOST_REQUIRE_GT(o.front()->size(), magicNumber);
	// #magicNumber is a number between 0 and 5616-1
	// the hope is that v != w
	std::advance(orbIt, magicNumber);
	Vector w = *orbIt;
	vectors.push_back(w);
	
	o = orbits<Vector, VectorAction<PERMUTATION> >(group, vectors.begin(), vectors.end());
	
	BOOST_REQUIRE_EQUAL( o.size(), 1 );
	BOOST_CHECK( o.front()->contains(v) );
	BOOST_CHECK( o.front()->contains(w) );
}

BOOST_AUTO_TEST_CASE( vector_stabilizer_test1997 )
{
	const PermutationGroup group = construct<PERMUTATION,TRANSVERSAL>(info_1997);
	
	Vector v(group.n);
	for (unsigned int i = 0; i < v.size(); ++i)
		v[i] = i%3;
	
	boost::shared_ptr<PermutationGroup> stab = vectorStabilizer(group, v.begin(), v.end());
	BOOST_REQUIRE( stab );
	// correct order computed with GAP
	BOOST_CHECK_EQUAL( stab->order(), 2 );
}

BOOST_AUTO_TEST_CASE( notlexmin_test1997 )
{
	const PermutationGroup group = construct<PERMUTATION,TRANSVERSAL>(info_1997);

	const unsigned int base[] = {3,5,6,8,9};
	const unsigned int zeros[] = {0,5,6};
	const unsigned int ones[] = {3,8,9,11};

	// [4,6,7,9,10] is not lex-min in orbit because e.g. [ 1, 12, 3, 11, 13 ] is lex-smaller
	BOOST_CHECK( isNotLexMinSet(group, base, base+5, zeros, zeros+3, ones, ones+4) );
}
