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


#ifndef GIANT_TEST_H_
#define GIANT_TEST_H_

#include <permlib/generator/product_replacement_generator.h>
#include <permlib/transversal/explicit_transversal.h>
#include <permlib/bsgs.h>
#include <permlib/construct/schreier_sims_construction.h>
#include <permlib/prime_helper.h>

#include <boost/foreach.hpp>
#include <boost/math/common_factor_rt.hpp>
#include <cmath>
#include <algorithm>

namespace permlib {


struct GiantTestBase {
	/// Enumeration of "giant" groups, i.e. Alternating and Symmetric group
	enum GiantGroupType { None, Alternating, Symmetric };
};

/// Tests a group given by generators for being an Alternating Group or a Symmetric Group
/**
 * This an implementation of the algorithm given in
 * Holt, Eick, O'Brien: Handbook of Computational Group Theory, 2005. Chapter 4.2
 */
template<typename PERM>
class GiantTest : public GiantTestBase {
public:
	/// tests whether group given by generators is an Alternating or a Symmetric Group
	/**
	 * The test is deterministic for n < 8 and randomized for n >= 8.
	 * The randomized test assumes that n is smaller than
	 * 
	 * @param eps If randomized, 1-eps is the probability that the decision "None" is wrong
	 * @param n   Degree of the group
	 * @param begin iterator of Permutation::ptr, group generators
	 * @param end iterator of Permutation::ptr, group generators
	 * @param bsgs if for the test a BSGS is computed, it is stored into this variable
	 * @param isKnownPrimitive true if group is known to be primitive
	 * @return Result "None" may be wrong if n >= 8 with probability eps
	 */
	template<typename ForwardIterator, typename TRANS>
	GiantGroupType determineGiantType(double eps, unsigned int n, ForwardIterator begin, ForwardIterator end, BSGS<PERM, TRANS>& bsgs, bool isKnownPrimitive = false) const;
	
	/// tests whether group given by generators is an Alternating or a Symmetric Group
	/**
	 * The test is deterministic for n < 8 and randomized for n >= 8.
	 * The randomized test assumes that n is smaller than
	 * 
	 * @param eps If randomized, 1-eps is the probability that the decision "None" is wrong
	 * @param n   Degree of the group
	 * @param begin iterator of Permutation::ptr, group generators
	 * @param end iterator of Permutation::ptr, group generators
	 * @param isKnownPrimitive true if group is known to be primitive
	 * @return Result "None" may be wrong if n >= 8 with probability eps
	 */
	template<typename ForwardIterator>
	GiantGroupType determineGiantType(double eps, unsigned int n, ForwardIterator begin, ForwardIterator end, bool isKnownPrimitive = false) const {
		typedef ExplicitTransversal<PERM> TRANS;
		BSGS<PERM, TRANS> bsgs(n);
		return determineGiantType<ForwardIterator, TRANS>(eps, n, begin, end, bsgs, isKnownPrimitive);
	}
	
	/// tests whether group given by generators is a subgroup of an Alternating Group
	/**
	 * Tests subgroup property by computing parity of all generators.
	 * @param begin iterator of Permutation::ptr, group generators
	 * @param end iterator of Permutation::ptr, group generators
	 * @return true if the group is a subgroup of an alternating group
	 */
	template<typename ForwardIterator>
	static bool isSubgroupOfAlternatingGroup(ForwardIterator begin, ForwardIterator end);
	
private:
	template<class T>
	static GiantGroupType giantTypeByOrder(const T& order, const T& symOrder);
};


template<typename PERM>
template<typename ForwardIterator>
bool GiantTest<PERM>::isSubgroupOfAlternatingGroup(ForwardIterator begin, ForwardIterator end) {
	typedef std::pair<dom_int, unsigned int> CyclePair;
	for (ForwardIterator pit = begin; pit != end; ++pit) {
		unsigned int parity = 0;
		std::list<CyclePair> genCycles = (*pit)->cycles();
		BOOST_FOREACH(const CyclePair& c, genCycles) {
			if (c.second % 2 == 0)
				++parity;
		}
		if (parity % 2 != 0)
			return false;
	}
	return true;
}

template<typename PERM>
template<typename T>
GiantTestBase::GiantGroupType GiantTest<PERM>::giantTypeByOrder(const T& order, const T& symOrder) {
	if (order == symOrder / 2)
		return Alternating;
	if (order == symOrder)
		return Symmetric;
	return None;
}
	
template<typename PERM>
template<typename ForwardIterator, typename TRANS>
GiantTestBase::GiantGroupType GiantTest<PERM>::determineGiantType(double eps, unsigned int n, ForwardIterator begin, ForwardIterator end, BSGS<PERM, TRANS>& bsgs, bool isKnownPrimitive) const {
	BOOST_ASSERT(n > 1);
	
	// special cases for n < 8
	
	if (n == 2) {
		for (ForwardIterator pit = begin; pit != end; ++pit) {
			if ( ! (*pit)->isIdentity() )
				return Symmetric;
		}
		return None;
	} else if (n < 8) {
		SchreierSimsConstruction<PERM, TRANS> ssc(n);
		bsgs = ssc.construct(begin, end);
		const boost::uint64_t order = bsgs.order();
		switch (n) {
			case 3:
				return giantTypeByOrder(order, static_cast<boost::uint64_t>(6));
			case 4:
				return giantTypeByOrder(order, static_cast<boost::uint64_t>(24));
			case 5:
				return giantTypeByOrder(order, static_cast<boost::uint64_t>(120));
			case 6:
				return giantTypeByOrder(order, static_cast<boost::uint64_t>(720));
			case 7:
				return giantTypeByOrder(order, static_cast<boost::uint64_t>(5040));
			default:
				// should not happen
				BOOST_ASSERT(false);
				return None;
		}
	}
	
	// This constant 0.395 comes from 0.57 * log(2)
	const unsigned int randomRuns = static_cast<unsigned int>(-std::log(eps) * std::log(n) / 0.395);
	
	ProductReplacementGenerator<PERM> rng(n, begin, end);
	for (unsigned int i = 0; i < randomRuns; ++i) {
		PERM randPerm = rng.next();
		typedef std::pair<dom_int, unsigned int> CyclePair;
		std::list<CyclePair> cycleList = randPerm.cycles();
		std::vector<unsigned int> cycleLength(cycleList.size());
		unsigned int j = 0;
		BOOST_FOREACH(const CyclePair& c, cycleList) {
			cycleLength[j++] = c.second;
		}
		for (j = 0; j < cycleLength.size(); ++j) {
			const unsigned int len = cycleLength[j];
			if (len < n-2 && PrimeHelper::isPrimeNumber(len, true) && (isKnownPrimitive || len > n/2)) {
				// check whether $len is co-prime to all other cycle length.
				// if so, the group contains a real cycle of length $len
				bool isCoprime = true;
				for (unsigned int k = 0; k < cycleLength.size(); ++k) {
					if (j == k)
						continue;
					if (boost::math::gcd(cycleLength[j], cycleLength[k]) != 1) {
						isCoprime = false;
						break;
					}
				}
				if ( ! isCoprime )
					continue;

				if (isSubgroupOfAlternatingGroup(begin, end))
					return Alternating;
				else
					return Symmetric;
			}
		}
	}
	
	return None;
}


} // end NS permlib

#endif
