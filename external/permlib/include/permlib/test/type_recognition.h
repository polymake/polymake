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

#ifndef TYPE_RECOGNITION_H_
#define TYPE_RECOGNITION_H_

#include <permlib/prime_helper.h>
#include <permlib/transversal/schreier_tree_transversal.h>
#include <permlib/generator/bsgs_generator.h>
#include <permlib/construct/schreier_sims_construction.h>
#include <permlib/transversal/orbit_set.h>
#include <permlib/test/giant_test.h>
#include <permlib/test/group_type.h>
#include <permlib/test/primitivity_sgs_test.h>
#include <permlib/test/primitivity_test.h>
#include <permlib/permlib_api.h>

#include <boost/shared_ptr.hpp>
#include <boost/math/common_factor_rt.hpp>
#include <iostream>


namespace permlib {

/// Class for a basic type recognition of permutation groups
/**
 * The algorithms used for recognition are partially non-deterministic,
 * ie. they may report unknown for an otherwise known group type.
 */
template<class PERM, class TRANSVERSAL = SchreierTreeTransversal<PERM> >
class TypeRecognition {
public:
	/// abbreviation for a pointer to a BSGS structure
	typedef boost::shared_ptr<BSGS<PERM, TRANSVERSAL> > PermutationGroupPtr;
	
	/**
	 * @param n the degree of the permutation group to analyze
	 * @param genBegin begin-iterator to PERM::ptr-typed generators of the group to analyze
	 * @param genEnd end-iterator to PERM::ptr-typed generators of the group to analyze
	 */
	template<class InputIterator>
	TypeRecognition(unsigned int n, InputIterator genBegin, InputIterator genEnd);
	
	/**
	 * @param bsgs group to analyze
	 */
	TypeRecognition(const PermutationGroupPtr& bsgs);
	
	/// analyzes the given group and attempts to determine the group type
	GroupType* analyze() { return analyze(true); }
	/// returns a BSGS if one was constructed during the analysis
	PermutationGroupPtr bsgs() const { return m_bsgs; }
	
	/// attempts to find a large diagonal subgroup of symmetric groups
	/**
	 * @param orbitCharacteristic filled in by method. For each domain element there is an index.
	 *                            The value at the corresponding index is the same for all elements
	 *                            in the same orbit under the constructed diagonal subgroup.
	 * @return diagonal subgroup of given group, may be trivial
	 */
	GroupType* largeSymmetricDiagonalSubgroup(std::vector<dom_int>& orbitCharacteristic) const;
private:
	const unsigned int m_n;
	std::list<typename PERM::ptr> m_generators;
	const PermutationGroupPtr m_externalBSGS;
	PermutationGroupPtr m_bsgs;
	
	/**
	 * @param allowRecursiveCalls true iff analysis is allowed to be recursive 
	 */
	GroupType* analyze(bool allowRecursiveCalls);
	
	/// analyzes the group with action restricted to its orbit
	/**
	 * @return group type if actions restricted to each group orbit are equal
	 */
	GroupType* checkOrbitActions();
	
	/// degree of the group up to which a BSGS construction is attempted
	static const unsigned int m_bsgsComputationDegreeLimit = 50;
};

template<class PERM, class TRANSVERSAL>
template<class InputIterator>
TypeRecognition<PERM,TRANSVERSAL>::TypeRecognition(unsigned int n, InputIterator genBegin, InputIterator genEnd) 
	: m_n(n), m_generators(genBegin, genEnd)
{ }

template<class PERM, class TRANSVERSAL>
TypeRecognition<PERM,TRANSVERSAL>::TypeRecognition(const PermutationGroupPtr& bsgs_) 
	: m_n(bsgs_->n), m_generators(bsgs_->S.begin(), bsgs_->S.end()), m_externalBSGS(bsgs_), m_bsgs(m_externalBSGS)
{ }


template<class PERM, class TRANSVERSAL>
GroupType* TypeRecognition<PERM,TRANSVERSAL>::analyze(bool allowRecursiveCalls) {
	if (m_n < m_bsgsComputationDegreeLimit && ! m_bsgs) {
		SchreierSimsConstruction<PERM,TRANSVERSAL> ssc(m_n);
		m_bsgs = PermutationGroupPtr(new BSGS<PERM, TRANSVERSAL>(ssc.construct(m_generators.begin(), m_generators.end())));
	}

	// error probability for randomized algorithms
	const double eps = 1e-3;
	
	if (m_bsgs) {
		const unsigned int groupIntOrder = m_bsgs->template order<unsigned int>();
		
		if (groupIntOrder == 1)
			return new TrivialGroupType(m_n);

		if (groupIntOrder == 2)
			return new SymmetricGroupType(groupIntOrder, m_n);
		
		//
		// check for cyclic groups
		//
		if (PrimeHelper::isPrimeNumber(groupIntOrder, false)) {
			BOOST_ASSERT( m_n % groupIntOrder == 0 );
			return new CyclicGroupType(groupIntOrder, m_n);
		}
		
		if (groupIntOrder == 4) {
			// distinguish between C_4 and the Klein four group
			BSGSGenerator<TRANSVERSAL> bsgsGen(m_bsgs->U);
			while (bsgsGen.hasNext()) {
				PERM el = bsgsGen.next();
				if (el.order() == 4)
					return new CyclicGroupType(4, m_n);
			}
			return new DirectProductGroupType(new SymmetricGroupType(2,2), new SymmetricGroupType(2,2), m_n);
		}
		
		//
		// check for symmetric groups, natural action
		//
		unsigned int symmetricGroupCandidateDegree = 0;
		
		std::vector<unsigned int> transversalSizes(m_bsgs->U.size());
		for (unsigned int i = 0; i < m_bsgs->U.size(); ++i) {
			transversalSizes[i] = m_bsgs->U[i].size();
		}
		std::sort(transversalSizes.begin(), transversalSizes.end());
		
		// test whether group order is a factorial of some natural number
		
		unsigned int expectedFactor = 2;
		for (std::vector<unsigned int>::const_iterator it = transversalSizes.begin(); it != transversalSizes.end(); ++it) {
			if (*it == 1)
				continue;
			if (*it == expectedFactor)
				++expectedFactor;
			else {
				expectedFactor = 0;
				break;
			}
		}
		
		if (expectedFactor > 0)
			symmetricGroupCandidateDegree = expectedFactor - 1;
		
		if (symmetricGroupCandidateDegree == m_n)
			return new SymmetricGroupType(symmetricGroupCandidateDegree, m_n);
		
		// check for wreath products of symmetric groups if group is transitive
		if (m_bsgs->U[0].size() == m_n) {
			std::list<typename PERM::ptr> S1;
			PointwiseStabilizerPredicate<PERM> stabPred(m_bsgs->B[0]);
			BOOST_FOREACH(const typename PERM::ptr& p, m_bsgs->S) {
				if (stabPred(p))
					S1.push_back(p);
			}
			PrimitivitySGSTest<TRANSVERSAL> primitivityTest(m_bsgs->S.begin(), m_bsgs->S.end(), m_bsgs->B[0], S1.begin(), S1.end(), m_bsgs->U[0]);
			std::vector<dom_int> minimalBlock;
			bool groupIsPrimitive = primitivityTest.blockOfImprimitivity(&minimalBlock); 
			if ( ! groupIsPrimitive ) {
				// TODO: avoid integer overflow in order computation
				unsigned int degreeG = minimalBlock.size();
				unsigned int degreeH = m_n / degreeG;
				if (m_n % degreeG == 0) {
					boost::uint64_t wreathSize = 1;
					// group order must equal   factorial(deg G)^(deg H) * factorial(deg H)
					for (unsigned int i = 1; i <= degreeH; ++i) {
						for (unsigned int j = 2; j <= degreeG; ++j) {
							wreathSize *= j;
						}
						wreathSize *= i;
					}
					if (wreathSize == m_bsgs->order())
						return new WreathSymmetricGroupType(degreeG, degreeH, m_n);
				}
			} else {
				GiantTest<PERM> giantTest;
				GiantTestBase::GiantGroupType giant = giantTest.determineGiantType(eps, 
					m_n, m_bsgs->S.begin(), m_bsgs->S.end(), true);
				if (giant == GiantTestBase::Symmetric)
					return new SymmetricGroupType(m_n, m_n);
				else if (giant == GiantTestBase::Alternating)
					return new AlternatingGroupType(m_n, m_n);
			}
		}
		
		
		if (allowRecursiveCalls) {
			// check for multiple linked copies of S_k
			// TODO: check for inner direct products of S_k \times S_l
			GroupType* t = checkOrbitActions();
			if (t) {
				return t;
			}
		}
	} // end if m_bsgs
	else // else if ! m_bsgs
	{
		GiantTest<PERM> giantTest;
		GiantTestBase::GiantGroupType giant = giantTest.determineGiantType(eps, 
			m_n, m_generators.begin(), m_generators.end());
		if (giant == GiantTestBase::Symmetric)
			return new SymmetricGroupType(m_n, m_n);
		else if (giant == GiantTestBase::Alternating)
			return new AlternatingGroupType(m_n, m_n);
		
		if (allowRecursiveCalls) {
			GroupType* t = checkOrbitActions();
			if (t)
				return t;
		}
	}
	
	if (m_bsgs)
		return new AnonymousGroupType<>(m_n, m_bsgs->order());
	return new AnonymousGroupType<>(m_n);
}

template<class PERM, class TRANSVERSAL>
GroupType* TypeRecognition<PERM,TRANSVERSAL>::checkOrbitActions() {
	if (m_generators.empty())
		return NULL;
	
	typedef typename PERM::ptr PERMptr;
	boost::dynamic_bitset<> worked(m_n);
	GroupType* candidateType = 0;
	unsigned int groupSupport = m_n;
	
	for (unsigned int i = 0; i < m_n; ++i) {
		if (worked[i])
			continue;
		worked.set(i, true);
		OrbitSet<PERM, dom_int> orbit;
		orbit.orbit(i, m_generators, typename Transversal<PERM>::TrivialAction());
		for (typename OrbitSet<PERM, dom_int>::const_iterator it = orbit.begin(); it != orbit.end(); ++it) {
			worked.set(*it, true);
		}
		// ignore fix points
		if (orbit.size() == 1) {
			--groupSupport;
			continue;
		}
		
		// restrict group to this orbit
		std::list<PERMptr> orbitGenerators;
		BOOST_FOREACH(const PERMptr& p, m_generators) {
			orbitGenerators.push_back(PERMptr(p->project(orbit.size(), orbit.begin(), orbit.end())));
		}
		TypeRecognition<PERM, TRANSVERSAL> orbitTypeRecognition(orbit.size(), orbitGenerators.begin(), orbitGenerators.end());
		GroupType* orbitType = orbitTypeRecognition.analyze(false);
		if ( ! candidateType )
			candidateType = orbitType;
		else {
			const bool isEqualType = candidateType->equals(orbitType);
			delete orbitType;
			if ( ! isEqualType ) 
				return NULL;
		}
	}
	if (candidateType)
		candidateType->setNonNaturalAction(groupSupport);
	return candidateType;
}

template<typename PERM>
struct BlockVectorAction {
	std::vector<dom_int> operator()(const PERM& p, const std::vector<dom_int>& v) const {
		std::vector<dom_int> ret(v.size());
		for (unsigned int i = 0; i < v.size(); ++i)
			ret[i] = p / v[i];
		std::sort(ret.begin(), ret.end());
		return ret;
	}
};

template<class PERM, class TRANSVERSAL>
GroupType* TypeRecognition<PERM,TRANSVERSAL>::largeSymmetricDiagonalSubgroup(std::vector<dom_int>& orbitCharacteristic) const {
	if (m_generators.empty())
		return NULL;
	
	typedef boost::shared_ptr<OrbitSet<PERM, dom_int> > OrbitPtr;
	typedef typename PERM::ptr PERMptr;

	orbitCharacteristic.clear();
	orbitCharacteristic.resize(m_n);
	unsigned int orbitCharacteristicCounter = 0;
	
	std::list<OrbitPtr> orbits;
	boost::dynamic_bitset<> worked(m_n);
	for (unsigned int i = 0; i < m_n; ++i) {
		if (worked[i])
			continue;
		worked.set(i, true);
		OrbitPtr orbit(new OrbitSet<PERM, dom_int>());
		orbit->orbit(i, m_generators, typename Transversal<PERM>::TrivialAction());
		for (typename OrbitSet<PERM, dom_int>::const_iterator it = orbit->begin(); it != orbit->end(); ++it) {
			worked.set(*it, true);
		}
		orbits.push_back(orbit);
	}
	
	size_t orbitGCD = orbits.front()->size();
	BOOST_FOREACH(const OrbitPtr& orbit, orbits) {
		orbitGCD = boost::math::gcd(orbitGCD, orbit->size());
	}
	
	GroupType* lastType = 0;
	BOOST_FOREACH(const OrbitPtr& orbit, orbits) {
		GroupType* orbitType = 0;
		// restrict group to this orbit
		std::list<PERMptr> orbitGenerators;
		// copy orbit elements into vector because we need a fixed order to recover from 'project'ion
		std::vector<dom_int> orbitV(orbit->begin(), orbit->end());
		BOOST_FOREACH(const PERMptr& p, m_generators) {
			orbitGenerators.push_back(PERMptr(p->project(orbit->size(), orbitV.begin(), orbitV.end())));
			//TODO: filter identities
		}
		PermutationGroupPtr orbitGroup = construct(orbit->size(), orbitGenerators.begin(), orbitGenerators.end());
		
		// compute all minimal blocks
		PrimitivityTest<PERM> primitivityTest(orbit->size(), orbitGenerators.begin(), orbitGenerators.end());
		std::list<std::vector<dom_int> > blocks;
		bool groupIsPrimitive = primitivityTest.allBlocks(&blocks);
		
		if (!groupIsPrimitive) {
			BOOST_FOREACH(const std::vector<dom_int>& b, blocks) {
				// if group is transitive (#orbits == 1), then we do not need the gcd condition
				if (orbits.size() != 1 && b.size() % orbitGCD != 0)
					continue;
				
				// look at group action within block b
				// 1. compute stabilizer of b
				// 2. project action of stabilizer onto b
				// 3. check whether induced action is a symmetric group 
				PermutationGroupPtr stab = setStabilizer(*orbitGroup, b.begin(), b.end());
				std::list<PERMptr> suborbitGenerators;
				BOOST_FOREACH(const PERMptr& p, stab->S) {
					suborbitGenerators.push_back(PERMptr(p->project(b.size(), b.begin(), b.end())));
				}
				TypeRecognition<PERM, TRANSVERSAL> suborbitRecognition(b.size(), suborbitGenerators.begin(), suborbitGenerators.end());
				GroupType* subType = suborbitRecognition.analyze(false);
				if (subType->type() == GroupType::Named && subType->isNaturalAction()) {
					NamedGroupType* namedType = reinterpret_cast<NamedGroupType*>(subType);
					if (strcmp(namedType->name(), "S") == 0) {
						orbitType = subType;
					}
				}
				if (orbitType) {
					OrbitSet<PERM, std::vector<dom_int> > blockOrbit;
					blockOrbit.orbit(b, orbitGenerators, BlockVectorAction<PERM>());
					BOOST_FOREACH(const std::vector<dom_int>& block, std::make_pair(blockOrbit.begin(), blockOrbit.end())) {
						BOOST_FOREACH(const dom_int j, block) {
							orbitCharacteristic[orbitV[j]] = orbitCharacteristicCounter;
						}
						++orbitCharacteristicCounter;
					}
					break;
				} else {
					delete subType;
				}
			}
		} else {
			TypeRecognition<PERM, TRANSVERSAL> suborbitRecognition(orbit->size(), orbitGenerators.begin(), orbitGenerators.end());
			orbitType = suborbitRecognition.analyze(false);
			BOOST_FOREACH(const dom_int& o, orbitV) {
				orbitCharacteristic[o] = orbitCharacteristicCounter;
			}
			++orbitCharacteristicCounter;
		}
		if (!orbitType) {
			delete lastType;
			return NULL;
		}
		
		if (!lastType) {
			lastType = orbitType;
		} else { 
			if (!lastType->equals(orbitType)) {
				delete orbitType;
				delete lastType;
				return NULL;
			}
			delete orbitType;
		}
	}
	
	if (lastType)
		lastType->setNonNaturalAction(m_n);
	
	return lastType;
}

}

#endif

