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


#ifndef PRIMITIVITY_TEST_H_
#define PRIMITIVITY_TEST_H_

#include <permlib/prime_helper.h>

#include <boost/foreach.hpp>
#include <boost/utility.hpp>
#include <vector>
#include <list>
#include <set>

namespace permlib {

/// Tests a transitive group is availble for primitivity.
/**
 * If group is not primitive, it can compute a minimal block.
 * Note that PrimitivitySGSTest may be faster if a strong generating set is known.
 * 
 * This class implements the algorithm described in
 * Holt, Eick, O'Brien: Handbook of Computational Group Theory, 2005. Chapter 4.3
 */
template<typename PERM>
class PrimitivityTest {
public:
	/**
	 * Sets up the test
	 * 
	 * @param n number of elements in the group domain
	 * @param genBegin iterator<PERM::ptr> begin for group generators
	 * @param genEnd iterator<PERM::ptr> end for group generators
	 */
	template<typename InputIterator>
	PrimitivityTest(const unsigned int n, InputIterator genBegin, InputIterator genEnd);
	
	/**
	 * attempts to find a minimal block for the group
	 *
	 * @param minimalBlock If not null, this vector will be filled with a (non-sorted) minimal block for the group.
	 *                     If the group is primitive, the vector will contain all elements of the domain.
	 * @return true iff group is primitive
	 */
	bool blockOfImprimitivity(std::vector<dom_int>* minimalBlock) const;
	
	/**
	 * finds a list of all minimal blocks for the group
	 *
	 * @param blocks If not null, this list will be filled with all (non-sorted) minimal blocks for the group.
	 *               If the group is primitive, the list will contain one vector all elements of the domain.
	 * @return true iff group is primitive
	 */
	bool allBlocks(std::list<std::vector<dom_int> >* blocks, bool findOnlyOneBlock = false) const;
	
	/**
	 * @return true iff group is primitive
	 */
	bool isPrimitive() const { return blockOfImprimitivity(NULL); }
	
private:
	const unsigned int m_n;
	unsigned int m_primeLimit;
	const std::list<typename PERM::ptr> m_generators;
	
	bool fillTrivialBlock(std::list<std::vector<dom_int> >* block) const;
	
	static dom_int rep(dom_int kappa, std::vector<dom_int>& p);
	
	bool merge(dom_int kappa, dom_int lambda, std::vector<dom_int>& c, std::vector<dom_int>& p, std::vector<dom_int>& q, unsigned int& l) const;
};



template<typename PERM>
template<typename InputIterator>
PrimitivityTest<PERM>::PrimitivityTest(const unsigned int n, InputIterator genBegin, InputIterator genEnd)
	: m_n(n), m_primeLimit(m_n), m_generators(genBegin, genEnd)
{
	for (const unsigned int* p = PrimeHelper::firstPrime(); p != PrimeHelper::lastPrime(); ++p) {
		if (m_n % (*p) == 0) {
			m_primeLimit = m_n / (*p);
			break;
		}
	}
}

template<typename PERM>
bool PrimitivityTest<PERM>::blockOfImprimitivity(std::vector<dom_int>* minimalBlock) const {
	if (minimalBlock) {
		std::list<std::vector<dom_int> > blocks;
		const bool is_primitive = allBlocks(&blocks, true);
		minimalBlock->clear();
		minimalBlock->reserve(blocks.front().size());
		std::copy(blocks.front().begin(), blocks.front().end(), std::back_inserter(*minimalBlock));
		return is_primitive;
	}
	return allBlocks(0, true);
}


template<typename PERM>
bool PrimitivityTest<PERM>::allBlocks(std::list<std::vector<dom_int> >* blocks, bool findOnlyOneBlock) const {
	std::vector<dom_int> alphas(2);
	std::set<dom_int> workedAlphas;
	alphas[0] = 0;
	
	for (dom_int a = 1; a < m_n; ++a) {
		if (workedAlphas.count(a))
			continue;
		alphas[1] = a;
		
		const unsigned int k = alphas.size();
		unsigned int l = k - 1;
		std::vector<dom_int> p(m_n);
		std::vector<dom_int> q(m_n);
		std::vector<dom_int> c(m_n);
		
		for (unsigned int i = 0; i < m_n; ++i) {
			c[i] = 1;
			p[i] = i;
		}
		
		for (unsigned int i = 0; i < k - 1; ++i) {
			p[alphas[i+1]] = alphas[0];
			q[i] = alphas[i+1];
		}
		
		c[alphas[0]] = k;
		for (unsigned int i = 0; i < l; ++i) {
			const dom_int gamma = q[i];
			BOOST_FOREACH(const typename PERM::ptr& x, m_generators) {
				const dom_int delta = rep(gamma, p);
				if (merge(x->at(gamma), x->at(delta), c, p, q, l)) {
					goto TRY_NEXT_ALPHA;
				}
			}
		}
		
		for (unsigned int i = 0; i < m_n; ++i)
			rep(i, p);
		
		if (c[rep(alphas[0], p)] < m_n) {
			if (blocks) {
				std::vector<dom_int> block;
				block.reserve(c[rep(alphas[0], p)]);
				for (unsigned int i = 0; i < m_n; ++i) {
					if (p[i] == p[alphas[0]]) {
						block.push_back(i);
						if (i != alphas[0])
							workedAlphas.insert(i);
					}
				}
				BOOST_ASSERT( block.size() == c[rep(alphas[0], p)] );
				blocks->push_back(block);
			}
			if (findOnlyOneBlock)
				return false;
		}
		
		// end of alpha loop
TRY_NEXT_ALPHA:
		continue;
	}
	
	if (!blocks->empty())
		return false;
	return fillTrivialBlock(blocks);
}

template<typename PERM>
bool PrimitivityTest<PERM>::fillTrivialBlock(std::list<std::vector<dom_int> >* blocks) const {
	if (blocks) {
		std::vector<dom_int> minimalBlock(m_n);
		for (unsigned int i = 0; i < m_n; ++i)
			minimalBlock[i] = i;
		blocks->push_back(minimalBlock);
	}
	return true;
}

template<typename PERM>
dom_int PrimitivityTest<PERM>::rep(dom_int kappa, std::vector<dom_int>& p) {
	dom_int lambda = kappa;
	dom_int rho = p[lambda];
	while (rho != lambda) {
		lambda = rho;
		rho = p[lambda];
	}
	
	dom_int mu = kappa;
	rho = p[mu];
	while (rho != lambda) {
		p[mu] = lambda;
		mu = rho;
		rho = p[mu];
	}
	
	return lambda;
}

template<typename PERM>
bool PrimitivityTest<PERM>::merge(dom_int kappa, dom_int lambda, std::vector<dom_int>& c, std::vector<dom_int>& p, std::vector<dom_int>& q, unsigned int& l) const {
	dom_int phi = rep(kappa, p);
	dom_int psi = rep(lambda, p);
	
	if (phi != psi) {
		dom_int mu, nu;
		if (c[phi] >= c[psi]) {
			mu = phi;
			nu = psi;
		} else {
			mu = psi;
			nu = phi;
		}
		p[nu] = mu;
		c[mu] += c[nu];
		if (c[mu] > m_primeLimit)
			return true;
		
		q[l] = nu;
		++l;
	}
	
	return false;
}

}

#endif
