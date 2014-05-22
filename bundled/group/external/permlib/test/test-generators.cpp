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
#define BOOST_TEST_MODULE BSGS generators test
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>

#include <permlib/permutation.h>
#include <permlib/generator/product_replacement_generator.h>
#include <permlib/bsgs.h>
#include <permlib/construct/schreier_sims_construction.h>
#include <permlib/transversal/explicit_transversal.h>
#include <permlib/generator/bsgs_generator.h>

#include "test-common.h"

#include <boost/foreach.hpp>

using namespace permlib;
using namespace permlib::test;

BOOST_AUTO_TEST_CASE( product_replacement_generator )
{
	typedef Permutation PERM;
	typedef std::list<PERM::ptr> PERMlist;
	
	PERMlist generators;
	const unsigned int n = readGroup<PERM>(info_1997.filename, generators);
	
	boost::indirect_iterator<PERMlist::iterator> begin(generators.begin()), end(generators.end());
	std::vector<PERM> originalGenerators(begin, end);
	
	ProductReplacementGenerator<PERM> rng(n, generators.begin(), generators.end());
	for (unsigned int i=0; i<100; ++i)
		rng.next();
	
	unsigned int index = 0;
	BOOST_FOREACH(boost::shared_ptr<PERM> p, generators) {
		BOOST_CHECK( *p == originalGenerators[index] );
		++index;
	}
}

void testBSGS(const GroupInformation& info) {
	typedef Permutation PERM;
	typedef ExplicitTransversal<PERM> TRANS;
	
	BSGS<PERM, TRANS> bsgs = construct<PERM,TRANS>(info);
	BSGSGenerator<TRANS> bsgsGen(bsgs.U);
	
	unsigned int c = 0;
	while (bsgsGen.hasNext()) {
		PERM p = bsgsGen.next();
		++c;
	}
	BOOST_CHECK_EQUAL( c, info.order );
}

BOOST_AUTO_TEST_CASE( bsgs_generator )
{
	testBSGS(info_Klein4);
	testBSGS(info_1997);
	testBSGS(info_cyclic500);
	testBSGS(info_S6_3);
}

