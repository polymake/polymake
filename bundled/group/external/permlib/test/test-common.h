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


#ifndef TEST_COMMON_H_
#define TEST_COMMON_H_

#include <list>
#include <boost/shared_ptr.hpp>

#include <permlib/common.h>
#include <permlib/bsgs.h>
#include <permlib/construct/schreier_sims_construction.h>
#include <permlib/construct/random_schreier_sims_construction.h>
#include <permlib/construct/cyclic_group_construction.h>
#include <permlib/generator/product_replacement_generator.h>

#include <iostream>
#include <fstream>

#include "group_data.h"

namespace permlib { namespace test {

template<class PERM,class TRANS>
struct Constructor {
	typedef PERM PERMtype;
	typedef TRANS TRANStype;
};

template<class PERM,class TRANS>
struct ConstructDeterministic1 : public Constructor<PERM,TRANS> {
	BSGS<PERM, TRANS> operator()(const GroupInformation& info) {
		std::list<typename PERM::ptr> groupGenerators;
		const unsigned int n = readGroup<PERM>(info.filename, groupGenerators);
		BOOST_REQUIRE_GT(n, 0);
		
		SchreierSimsConstruction<PERM, TRANS> ssc(n);
		BSGS<PERM, TRANS> bsgs = ssc.construct(groupGenerators.begin(), groupGenerators.end());
		
		return bsgs;
	}
};

template<class PERM,class TRANS>
struct ConstructDeterministic2 : public Constructor<PERM,TRANS> {
	BSGS<PERM, TRANS> operator()(const GroupInformation& info) {
		std::list<typename PERM::ptr> groupGenerators;
		const unsigned int n = readGroup<PERM>(info.filename, groupGenerators);
		BOOST_REQUIRE_GT(n, 0);
		
		SchreierSimsConstruction<PERM, TRANS> ssc(n);
		BSGS<PERM, TRANS> bsgs(n);
		bsgs = ssc.construct(groupGenerators.begin(), groupGenerators.end());
		
		return bsgs;
	}
};

template<class PERM,class TRANS>
struct ConstructRandomized : public Constructor<PERM,TRANS> {
	BSGS<PERM, TRANS> operator()(const GroupInformation& info) {
		std::list<typename PERM::ptr> groupGenerators;
		const unsigned int n = readGroup<PERM>(info.filename, groupGenerators);
		BOOST_REQUIRE_GT(n, 0);
		
		ProductReplacementGenerator<PERM> rng(n, groupGenerators.begin(), groupGenerators.end());
		RandomSchreierSimsConstruction<PERM, TRANS> ssc(n, &rng, info.order);
		bool result = false;
		BSGS<PERM, TRANS> bsgs = ssc.construct(groupGenerators.begin(), groupGenerators.end(), result);
		BOOST_REQUIRE(result);
		
		return bsgs;
	}
};

template<class PERM,class TRANS>
BSGS<PERM, TRANS> construct(const GroupInformation& info) {
	return ConstructDeterministic1<PERM,TRANS>()(info);
}

template<class PERM, class TRANS>
BSGS<PERM, TRANS> constructCyclicGroup(unsigned int n) {
	CyclicGroupConstruction<TRANS> cycConstruct(n);
	return cycConstruct.construct();
}

template<class PERM>
void checkImage(const unsigned long* Delta, unsigned long DeltaSize, const unsigned long* Phi, const boost::shared_ptr<PERM> &repr) {
	std::list<unsigned long> imgList(Phi, Phi+DeltaSize);
	for (unsigned int j = 0; j < DeltaSize; ++j) {
		std::list<unsigned long>::iterator it = std::find(imgList.begin(), imgList.end(), *repr / Delta[j]);
		BOOST_REQUIRE( it != imgList.end() );
		imgList.erase(it);
	}
	BOOST_CHECK( imgList.size() == 0 );
}

} } // end NS

#endif // - TEST_COMMON_H_
