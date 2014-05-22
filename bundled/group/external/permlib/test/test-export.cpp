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
#include <permlib/bsgs.h>
#include <permlib/transversal/schreier_tree_transversal.h>
#include <permlib/export/bsgs_schreier_export.h>

#include "test-common.h"

using namespace permlib;
using namespace permlib::exports;
using namespace permlib::test;

void testGroup(const GroupInformation& info) {
	typedef Permutation PERM;
	typedef SchreierTreeTransversal<PERM> TRANSVERSAL;
	
	BSGS<PERM,TRANSVERSAL> bsgs = construct<PERM,TRANSVERSAL>(info);
	
	BSGSSchreierExport exporter;
	BSGSSchreierImport importer;
	
	BSGSSchreierData* data = exporter.exportData(bsgs);
	
	BOOST_REQUIRE_EQUAL(bsgs.B.size(), data->baseSize);
	BOOST_REQUIRE_EQUAL(bsgs.S.size(), data->sgsSize);
	BOOST_REQUIRE_EQUAL(bsgs.U.size(), data->baseSize);
	
	BSGS<PERM,TRANSVERSAL>* bsgsImp = importer.importData(data);
	
	BOOST_CHECK_EQUAL(bsgs.order(), bsgsImp->order());
	
	// check that imported transversal is a transversal and that
	// it behaves like the original transversal
	unsigned int i = 0;
	BOOST_FOREACH(const TRANSVERSAL& U, bsgsImp->U) {
		for (unsigned int j = 0; j < U.n(); ++j) {
			boost::scoped_ptr<PERM> u_beta(U.at(j));
			if (u_beta)
				BOOST_CHECK_EQUAL(*u_beta / bsgsImp->B[i], j);
			else
				BOOST_CHECK( ! bsgs.U[i].at(j) );
		}
		++i;
	}
	
	delete bsgsImp;
	delete data;
}

BOOST_AUTO_TEST_CASE( test_export )
{
  testGroup(info_1997);
	testGroup(info_cov1075);
	testGroup(info_cyclic37_2);
	testGroup(info_S5wrS3);
	testGroup(info_S6);
}

BOOST_AUTO_TEST_CASE( test_export_empty )
{
  BSGSSchreierData* data = new BSGSSchreierData();
  data->n = 2;
  delete data;
}
