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


#ifndef BASECONSTRUCTION_H
#define BASECONSTRUCTION_H

#include <map>
#include <algorithm>

#include <permlib/predicate/pointwise_stabilizer_predicate.h>
#include <permlib/predicate/identity_predicate.h>

namespace permlib {

/// base class for BSGS construction algorithms
template <class PERM, class TRANS>
class BaseConstruction {
public:
	/// constructor
	/**
	 * @param n cardinality of the set the group is acting on
	 */
	explicit BaseConstruction(dom_int n);
protected:
	/// cardinality of the set the group is acting on
	dom_int m_n;
	
	/// initializes BSGS object
	/**
	 * @param generatorsBegin begin iterator of group generators of type PERM
	 * @param generatorsEnd  end iterator of group generators of type PERM
	 * @param prescribedBaseBegin begin iterator of prescribed base of type unsigned long
	 * @param prescribedBaseEnd  end iterator of prescribed base of type unsigned long
	 * @param bsgs  BSGS object to work on
	 * @param S     approximation of strong generating set to fill
	 */
	template <class ForwardIterator, class InputIterator>
	void setup(ForwardIterator generatorsBegin, ForwardIterator generatorsEnd, InputIterator prescribedBaseBegin, InputIterator prescribedBaseEnd, BSGS<PERM, TRANS> &bsgs, std::vector<std::list<typename PERM::ptr> > &S) const;
	
	/// merges all strong generators in S into a single strong generating set ret.S
	void mergeGenerators(std::vector<std::list<typename PERM::ptr> >& S, BSGS<PERM,TRANS>& ret) const;
	
	/// auxilliary element marking an empty iterator
	static const unsigned long *empty;
};

//
//     ----       IMPLEMENTATION
//

template <class PERM, class TRANS>
const unsigned long *BaseConstruction<PERM, TRANS>::empty = static_cast<unsigned long*>(0);


template <class PERM, class TRANS>
BaseConstruction<PERM,TRANS>::BaseConstruction(dom_int n) 
	: m_n(n) 
{ }

template <class PERM, class TRANS>
template <class ForwardIterator, class InputIterator>
void BaseConstruction<PERM,TRANS>::setup(ForwardIterator generatorsBegin, ForwardIterator generatorsEnd, InputIterator prescribedBaseBegin, InputIterator prescribedBaseEnd, BSGS<PERM, TRANS> &bsgs, std::vector<std::list<typename PERM::ptr> > &S) const
{
	std::vector<dom_int> &B = bsgs.B;
	std::vector<TRANS> &U = bsgs.U;
	
	std::list<typename PERM::ptr> nonIdentityGenerators;
	std::remove_copy_if(generatorsBegin, generatorsEnd, std::back_inserter(nonIdentityGenerators), IdentityPredicate<PERM>());

	B.insert(B.begin(), prescribedBaseBegin, prescribedBaseEnd);
	// extend base so that no group element fixes all base elements
	dom_int beta = m_n + 1;
	PointwiseStabilizerPredicate<PERM> stab_k(B.begin(), B.end());
	BOOST_FOREACH(const typename PERM::ptr &gen, nonIdentityGenerators) {
		if (stab_k(gen)) {
			if (bsgs.chooseBaseElement(*gen, beta)) {
				B.push_back(beta);
				stab_k = PointwiseStabilizerPredicate<PERM>(B.begin(), B.end());
			}
		}
	}
	
	if (B.empty()) {
		B.push_back(0);
		U.push_back(TRANS(m_n));
		// the trivial group has an empty generator list
		std::list<typename PERM::ptr> S_0;
		S.push_back(S_0);
		U[0].orbit(B[0], S_0);
		return;
	}
	S.reserve(B.size());
	
	// pre-compute transversals and fundamental orbits for the current base
	unsigned int i = 0;
	std::vector<dom_int>::iterator Bit;
	for (Bit = B.begin(); Bit != B.end(); ++Bit) {
		std::list<typename PERM::ptr> S_i;
		std::copy_if(nonIdentityGenerators.begin(), nonIdentityGenerators.end(),
				std::back_inserter(S_i), PointwiseStabilizerPredicate<PERM>(B.begin(), Bit));

		U.push_back(TRANS(m_n));
		S.push_back(S_i);

		bsgs.orbit(i, S_i);

		++i;
	}
}

template <class PERM, class TRANS>
void BaseConstruction<PERM,TRANS>::mergeGenerators(std::vector<std::list<typename PERM::ptr> >& S, BSGS<PERM,TRANS>& ret) const {
	std::map<PERM*,typename PERM::ptr> generatorMap;
	// merge all generators into one list
	BOOST_FOREACH(std::list<typename PERM::ptr> &S_j, S) {
		BOOST_FOREACH(typename PERM::ptr &gen, S_j) {
			bool found = false;
			BOOST_FOREACH(const typename PERM::ptr& genS, ret.S) {
				if (*genS == *gen) {
					found = true;
					generatorMap.insert(std::make_pair(gen.get(), genS));
					break;
				}
			}
			if (!found) {
				ret.S.push_back(gen);
				generatorMap.insert(std::make_pair(gen.get(), gen));
			}
		}
	}

	BOOST_FOREACH(TRANS& U_i, ret.U) {
		U_i.updateGenerators(generatorMap);
	}
}

}

#endif // -- BASECONSTRUCTION_H
