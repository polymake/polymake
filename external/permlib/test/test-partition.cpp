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
#define BOOST_TEST_MODULE partition magic
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>
#include <algorithm>

#include <permlib/permutation.h>
#include <permlib/generator/product_replacement_generator.h>
#include <permlib/bsgs.h>
#include <permlib/transversal/explicit_transversal.h>
#include <permlib/construct/schreier_sims_construction.h>

#include "test-common.h"
#include <permlib/search/partition/partition.h>
//#include <permlib/search/partition/r_base.h>

using namespace permlib::partition;

BOOST_AUTO_TEST_CASE( partition_intersection )
{
	const unsigned long n = 5;
	Partition P(n);
	
	const unsigned long set[5] = {0,1,2,3,4};
	
	Partition Pprime(P);

    BOOST_CHECK( Pprime.intersect(set, set+3, 0) );
    BOOST_CHECK( Pprime.cells() == 2 );
    BOOST_CHECK( ! Pprime.intersect(set, set+3, 0) );
    BOOST_CHECK( ! Pprime.intersect(set, set+3, 1) );
    
    BOOST_CHECK( Pprime.intersect(set+2, set+4, 0) );
    BOOST_CHECK( Pprime.cells() == 3 );
    BOOST_CHECK( Pprime.intersect(set+2, set+4, 1) );
    BOOST_CHECK( Pprime.cells() == 4 );
    BOOST_CHECK( Pprime.intersect(set+1, set+3, 2) );
    BOOST_CHECK( Pprime.cells() == 5 );

    const unsigned long fixSizes[4] = {5,3,1,0};
    const unsigned long fixContain[4] = {0,4,2,0};
    unsigned int i = 0;
    while (Pprime.fixPointsSize() && i < 6) {
	    std::vector<unsigned long> Pfix(Pprime.fixPointsBegin(), Pprime.fixPointsEnd());
        BOOST_CHECK( Pfix.size() == Pprime.fixPointsSize() );
        BOOST_CHECK( Pfix.size() == fixSizes[i] );
        BOOST_CHECK( std::find(Pfix.begin(), Pfix.end(), fixContain[i]) != Pfix.end() );

        BOOST_CHECK( Pprime.undoIntersection() );
        ++i;
    }
    BOOST_CHECK( i == 3 );
    
    BOOST_CHECK( Pprime.undoIntersection() );
    BOOST_CHECK( ! Pprime.undoIntersection() );
}

