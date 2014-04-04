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
#define BOOST_TEST_MODULE symmetric group magic
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <set>

#include <permlib/permutation.h>

#include "test-common.h"
#include <permlib/symmetric_group.h>
#include <permlib/change/deterministic_base_transpose.h>
#include <permlib/change/conjugating_base_change.h>





using namespace permlib;

BOOST_AUTO_TEST_CASE( symmetric_group )
{
	typedef Permutation PERM;
	typedef SymmetricGroup<PERM>::TRANS TRANS;
	typedef DeterministicBaseTranspose<PERM,TRANS> BASETRANSPOSE;
	
	const unsigned long n = 19;
	
	SymmetricGroup<PERM> s_n(n);

	BOOST_REQUIRE( s_n.B.size() >= n-1 );
	
	for (unsigned int i=0; i<n; ++i) {
		PERM* p = s_n.U[0].at(i);
		BOOST_REQUIRE( p );
		BOOST_CHECK( *p / s_n.B[0] == i );
		BOOST_CHECK( *p / i == s_n.B[0] );
		delete p;
	}

	ConjugatingBaseChange<PERM, TRANS, BASETRANSPOSE> cbc(s_n);
	const unsigned long newBaseLength = 5;
	unsigned long newBase[newBaseLength] = { 3, 1, 0, 9, 17 };
	cbc.change(s_n, newBase, newBase+newBaseLength);
	
	BOOST_REQUIRE( s_n.B.size() >= n-1 );
	
	for (unsigned int i=0; i<newBaseLength; ++i) {
		BOOST_CHECK(s_n.B[i] == newBase[i]);
	}
	
	std::set<unsigned long> seenBaseElements;
	for (unsigned int i=0; i<n; ++i) {
		seenBaseElements.insert(s_n.B[i]);
	}
	
	BOOST_CHECK( s_n.B.size() == seenBaseElements.size() );
}

