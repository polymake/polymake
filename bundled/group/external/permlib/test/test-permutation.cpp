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
#define BOOST_TEST_MODULE permutation test
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>

#include <permlib/permutation.h>
#include <permlib/permutationword.h>
#include <permlib/bsgs.h>

using namespace permlib;

template<class PERM>
void testPermutation() {
	const unsigned int n = 10;
	PERM a(n, "1 2 5,3 6 7 8, 9 4 10");
	PERM b(n, "1 2, 5 3 6, 7 8 9 4 10");
	
	BOOST_CHECK_EQUAL(a / 1, 4);
	BOOST_CHECK_EQUAL(a % 4, 1);
	BOOST_CHECK_EQUAL((~a) / 4, 1);

	BOOST_CHECK_EQUAL(b / 1, 0);
	BOOST_CHECK_EQUAL(b % 0, 1);
	BOOST_CHECK_EQUAL((~b) / 0, 1);
	
	for (unsigned int i = 0; i < n; ++i) {
		BOOST_CHECK_EQUAL( a / (~a / i), i);
		BOOST_CHECK_EQUAL( a % (~a % i), i);
	}
	
	PERM c = a * b;
	PERM d(a);
	d *= b;
	
	BOOST_CHECK(c == d);
	
	d.invertInplace();
	
	for (unsigned int i = 0; i < n; ++i) {
		BOOST_CHECK_EQUAL( c / (d / i), i);
		BOOST_CHECK_EQUAL( c % (d % i), i);
	}
	
	c *= d;
	
	BOOST_CHECK(c.isIdentity());
	
	PERM a2(a);
	PERM e(a * a);
	a2 *= a2;
	BOOST_CHECK( a2 == e );
}

BOOST_AUTO_TEST_CASE( test_permutation )
{
    testPermutation<Permutation>();
}

BOOST_AUTO_TEST_CASE( test_permutationword )
{
    testPermutation<PermutationWord>();
}

void testPermutationOrder(const Permutation& p) {
	Permutation q(p);
	
	for (boost::uint64_t i = 0; i < p.order() - 1; ++i) {
		BOOST_CHECK( ! q.isIdentity() );
		q *= p;
	}
	BOOST_CHECK( q.isIdentity() );
}

BOOST_AUTO_TEST_CASE( test_permutation_order ) {
	const unsigned int n = 10;
	Permutation a(n, "1 2 5,3 6 7 8, 9 4 10");
	Permutation b(n, "1 2, 5 3 6, 7 8 4 10");
	Permutation c(n, "1 2, 5 3 6, 7 8 9 4 10");
	Permutation d(n, "1 2 3, 5 6 7, 8 9 10");
	
	BOOST_CHECK_EQUAL( a.order(), 12 );
	testPermutationOrder(a);
	
	BOOST_CHECK_EQUAL( b.order(), 12 );
	testPermutationOrder(b);
	
	BOOST_CHECK_EQUAL( c.order(), 30 );
	testPermutationOrder(c);
	
	BOOST_CHECK_EQUAL( d.order(), 3 );
	testPermutationOrder(d);
}

BOOST_AUTO_TEST_CASE( test_permutation_cycles )
{
	const unsigned int n = 10;
	Permutation a(n, "1 2 5,3 6 7 8, 9 4 10");
	Permutation b(n, "1 2, 5 3 6, 7 8 4 10");
	
	typedef std::pair<dom_int, unsigned int> CyclePair;
	CyclePair c;
	
	std::list<CyclePair> cyclesA = a.cycles();
	BOOST_REQUIRE_EQUAL(cyclesA.size(), 3);
	
	c = cyclesA.front();
	cyclesA.pop_front();
	BOOST_CHECK_EQUAL(c.first, 0);
	BOOST_CHECK_EQUAL(c.second, 3);
	
	c = cyclesA.front();
	cyclesA.pop_front();
	BOOST_CHECK_EQUAL(c.first, 2);
	BOOST_CHECK_EQUAL(c.second, 4);
	
	c = cyclesA.front();
	cyclesA.pop_front();
	BOOST_CHECK_EQUAL(c.first, 3);
	BOOST_CHECK_EQUAL(c.second, 3);
	
	
	// include trivial cycles
	std::list<CyclePair> cyclesB = b.cycles(true);
	BOOST_REQUIRE_EQUAL(cyclesB.size(), 4);
	
	c = cyclesB.front();
	cyclesB.pop_front();
	BOOST_CHECK_EQUAL(c.first, 0);
	BOOST_CHECK_EQUAL(c.second, 2);
	
	c = cyclesB.front();
	cyclesB.pop_front();
	BOOST_CHECK_EQUAL(c.first, 2);
	BOOST_CHECK_EQUAL(c.second, 3);
	
	c = cyclesB.front();
	cyclesB.pop_front();
	BOOST_CHECK_EQUAL(c.first, 3);
	BOOST_CHECK_EQUAL(c.second, 4);
	
	c = cyclesB.front();
	cyclesB.pop_front();
	BOOST_CHECK_EQUAL(c.first, 8);
	BOOST_CHECK_EQUAL(c.second, 1);
	
	
	cyclesB = b.cycles(false);
	BOOST_REQUIRE_EQUAL(cyclesB.size(), 3);
	
	c = cyclesB.front();
	cyclesB.pop_front();
	BOOST_CHECK_EQUAL(c.first, 0);
	BOOST_CHECK_EQUAL(c.second, 2);
	
	c = cyclesB.front();
	cyclesB.pop_front();
	BOOST_CHECK_EQUAL(c.first, 2);
	BOOST_CHECK_EQUAL(c.second, 3);
	
	c = cyclesB.front();
	cyclesB.pop_front();
	BOOST_CHECK_EQUAL(c.first, 3);
	BOOST_CHECK_EQUAL(c.second, 4);
}


BOOST_AUTO_TEST_CASE( test_permutation_projection )
{
	std::string line1("1 2 5 9,3 6 7 8, 4 10");
	const unsigned int n = 10;
	Permutation a(n, line1);
	
	const dom_int projDomainSize = 4;
	const dom_int projDomain[projDomainSize] = { 2, 5, 6, 7 };
	Permutation* p = a.project(projDomainSize, projDomain, projDomain+projDomainSize);
	
	BOOST_ASSERT( p );
	
	BOOST_CHECK( ! p->isIdentity() );
	
	for (unsigned int i = 0; i < projDomainSize; ++i)
		BOOST_CHECK_EQUAL( *p / i, (i + 1) % projDomainSize );
	
	delete p;
}

BOOST_AUTO_TEST_CASE( test_exotic_permutations )
{
  Permutation* p = new Permutation(1, "");
  BOOST_ASSERT( p );
  BOOST_CHECK_EQUAL( p->size(), 1 );
  delete p;
  
  p = new Permutation(0, "");
  BOOST_ASSERT( p );
  BOOST_CHECK_EQUAL( p->size(), 0 );
  delete p;
}


BOOST_AUTO_TEST_CASE( test_identity )
{
	std::string line1("1 2 5 9,3 6 7 8, 4 10");
	const unsigned int n = 10;
	Permutation a(n, line1);
	
	BOOST_CHECK( ! a.isIdentity() );
	
	const dom_int lineArray[] = { 1, 4, 5, 9, 8, 6, 7, 2, 0, 3 };
	Permutation b(lineArray, lineArray + n);
	BOOST_CHECK( ! b.isIdentity() );
	
	Permutation c(a);
	c = ~a * b;
	BOOST_CHECK( c.isIdentity() );
}
