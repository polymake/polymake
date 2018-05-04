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


#ifndef PRIMITIVITY_SGS_TEST_H_
#define PRIMITIVITY_SGS_TEST_H_

#include <permlib/transversal/orbit_set.h>
#include <permlib/transversal/transversal.h>

#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

#include <boost/version.hpp>

#if BOOST_VERSION / 100 >= 1067
#include <boost/next_prior.hpp>
#else
#include <boost/utility.hpp>
#endif

#include <vector>
#include <list>

namespace permlib {

/// Tests a transitive group for which a strong generating set is availble for primitivity.
/**
 * If group is not primitive, it can compute a minimal block.
 * 
 * This class implements the algorithm described in
 * Seress: Permutation Group Algorithms, 2003. Chapter 5.5
 */
template<typename TRANS>
class PrimitivitySGSTest {
private:
	typedef typename TRANS::PERMtype PERM;
public:
	/**
	 * Sets up the test
	 * 
	 * @param genBegin iterator<PERM::ptr> begin for group generators
	 * @param genEnd iterator<PERM::ptr> end for group generators
	 * @param alpha an arbitrary element of the domain the group acts on
	 * @param genStabBegin iterator<PERM::ptr> begin for generators of the stabilizer of alpha in the group
	 * @param genStabEnd iterator<PERM::ptr> end for generators of the stabilizer of alpha in the group
	 * @param U transversal for group modulo the stabilizer of alpha
	 */
	template<typename InputIterator>
	PrimitivitySGSTest(InputIterator genBegin, InputIterator genEnd, dom_int alpha, InputIterator genStabBegin, InputIterator genStabEnd, const TRANS& U);
	
	/**
	 * @param minimalBlock If not null, this vector will be filled a (non-sorted) minimal block for the group.
	 *                     If the group is primitive, the vector will contain all elements of the domain.
	 * @return true iff group is primitive
	 */
	bool blockOfImprimitivity(std::vector<dom_int>* minimalBlock) const;
	
	/**
	 * @return true iff group is primitive
	 */
	bool isPrimitive() const { return blockOfImprimitivity(NULL); }
	
private:
	const TRANS& m_U;
	const dom_int m_alpha;
	const std::list<typename PERM::ptr> m_generators;
	const std::list<typename PERM::ptr> m_generatorsStab;
};



template<typename TRANS>
template<typename InputIterator>
PrimitivitySGSTest<TRANS>::PrimitivitySGSTest(InputIterator genBegin, InputIterator genEnd, dom_int alpha, InputIterator genStabBegin, InputIterator genStabEnd, const TRANS& U)
	: m_U(U), m_alpha(alpha), m_generators(genBegin, genEnd), m_generatorsStab(genStabBegin, genStabEnd)
{}


template<typename TRANS>
bool PrimitivitySGSTest<TRANS>::blockOfImprimitivity(std::vector<dom_int>* minimalBlock) const {
	const unsigned int n = m_U.n();
	std::vector<dom_int> AllLambdas(n);
	std::vector<dom_int> LambdaReverse(n, n);
	unsigned int LambdaLastIndex = 0;
	std::list<unsigned int> LambdaBegin;
	std::list<unsigned int> LambdaSize;
	
	dom_int omega;
	for (omega = 0; omega < n; ++omega)
		if (m_alpha != omega) 
			break;
	
	BOOST_ASSERT( omega < n );
	
	OrbitSet<PERM, dom_int> omegaOrbit;
	omegaOrbit.orbit(omega, m_generatorsStab, typename Transversal<PERM>::TrivialAction());
	for (typename OrbitSet<PERM, dom_int>::const_iterator oit = omegaOrbit.begin(); oit != omegaOrbit.end(); ++oit) {
		AllLambdas[LambdaLastIndex++] = *oit;
		LambdaReverse[*oit] = 0;
	}
	LambdaBegin.push_back(0);
	LambdaSize.push_back(omegaOrbit.size());
	
	while (true) {
		const dom_int lambda = AllLambdas[LambdaBegin.back()];
		std::vector<dom_int>::const_iterator LambdaItBegin = AllLambdas.begin() + LambdaBegin.back();
		std::vector<dom_int>::const_iterator LambdaItEnd = LambdaItBegin + LambdaSize.back();
		bool needAnotherIteration = false;
		
		PERMLIB_DEBUG( std::cout << "lambda = " << lambda << std::endl; )
		
		for (std::vector<dom_int>::const_iterator lit = LambdaItBegin; lit != LambdaItEnd; ++lit) {
			boost::scoped_ptr<PERM> u_lambda(m_U.at(lambda));
			BOOST_ASSERT( u_lambda );
			
			const dom_int gamma = *lit;
			const dom_int mu = *u_lambda / gamma;
			
			PERMLIB_DEBUG( std::cout << " u_lambda = " << *u_lambda << ", gamma = " << gamma << ", mu = " << mu << std::endl; )
			
			const unsigned int muIndex = LambdaReverse[mu];
			if (mu != m_alpha && muIndex == n) {
				PERMLIB_DEBUG( std::cout << " add orbit of mu = " << mu << " at " << LambdaBegin.size() << std::endl; )
				OrbitSet<PERM, dom_int> muOrbit;
				muOrbit.orbit(mu, m_generatorsStab, typename Transversal<PERM>::TrivialAction());
				LambdaBegin.push_back(LambdaLastIndex);
				LambdaSize.push_back(muOrbit.size());
				for (typename OrbitSet<PERM, dom_int>::const_iterator oit = muOrbit.begin(); oit != muOrbit.end(); ++oit) {
					AllLambdas[LambdaLastIndex++] = *oit;
					LambdaReverse[*oit] = LambdaBegin.size() - 1;
				}
				needAnotherIteration = true;
				break;
			} else if (muIndex < LambdaBegin.size() - 1) {
				std::list<unsigned int>::const_reverse_iterator sizeIt = LambdaSize.rbegin();
				std::list<unsigned int>::const_reverse_iterator reprIt = LambdaBegin.rbegin();
				unsigned int largestReprIndex = n;
				unsigned int largestLambdaSize = 0;
				for (dom_int i = muIndex; i < LambdaBegin.size(); ++i) {
					if (*sizeIt > largestLambdaSize) {
						largestLambdaSize = *sizeIt;
						largestReprIndex = *reprIt;
					}
					++sizeIt;
					++reprIt;
				}
				PERMLIB_DEBUG( std::cout << " merge sets from i = " << muIndex << " with representative " << AllLambdas[largestReprIndex] << std::endl; )
				
				std::swap(AllLambdas[*boost::next(LambdaBegin.begin(), muIndex)], AllLambdas[largestReprIndex]);
				for (dom_int i = LambdaBegin.size() - 1; i > muIndex ; --i) {
					const unsigned int oldSize = LambdaSize.back();
					LambdaSize.pop_back();
					LambdaBegin.pop_back();
					LambdaSize.back() += oldSize;
				}
				for (unsigned int i = 0; i < n; ++i)
					if (LambdaReverse[i] > muIndex && LambdaReverse[i] < n)
						LambdaReverse[i] = muIndex;
				
				PERMLIB_DEBUG( std::cout << " now there are " << LambdaBegin.size() << " sets left" << std::endl; )
				needAnotherIteration = true;
				break;
			}
		}
		
		BOOST_ASSERT( LambdaBegin.size() == LambdaSize.size() );
		
		if (!needAnotherIteration)
			break;
	}
	
	if (minimalBlock) {
		minimalBlock->clear();
		minimalBlock->resize(LambdaSize.back() + 1);
		minimalBlock->at(0) = m_alpha;
		for (unsigned int i = 1; i < minimalBlock->size(); ++i)
			minimalBlock->at(i) = AllLambdas[LambdaBegin.back() + i - 1];
	}
	
	return LambdaSize.back() == m_U.size() - 1;
}

}

#endif
