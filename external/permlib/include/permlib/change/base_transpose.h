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


#ifndef BASETRANSPOSE_H_
#define BASETRANSPOSE_H_

#include <permlib/predicate/pointwise_stabilizer_predicate.h>
#include <permlib/generator/generator.h>

#include <boost/scoped_ptr.hpp>
#include <boost/iterator/indirect_iterator.hpp>

namespace permlib {

/// abstract base class for base transposition
/**
 * A base transposition is a base change where two adjacent base points are exchanged.
 */
template<class PERM, class TRANS>
class BaseTranspose {
public:
	/// constructor
    BaseTranspose();
	/// virtual destructor
    virtual ~BaseTranspose() {}

	/// performs a base transposition on bsgs between bsgs.B[i] and bsgs.B[i+1]
	/**
	 * @param bsgs the BSGS that the transposition is performed on
	 * @param i exchanges i-th and (i+1)-st base point
	 */
    void transpose(BSGS<PERM,TRANS> &bsgs, unsigned int i) const;

	/// number of Schreier generators that have been considered during the last transpose call
    mutable unsigned int m_statScheierGeneratorsConsidered;
	/// number of new strong generators that have been added during the last transpose call
    mutable unsigned int m_statNewGenerators;
protected:
	typedef std::list<typename PERM::ptr> PERMlist;
	
	/// initializes the specific Schreier Generator that is used for the BaseTranpose implementation
	/** 
	 * @param bsgs the BSGS that the generator is contructed for
	 * @param i setup Schreier Generator for the i-th base element
	 * @param S_i group generators for Schreier generator
	 * @param U_i transversal for Schreier generator
	 */
    virtual Generator<PERM>* setupGenerator(BSGS<PERM,TRANS> &bsgs, unsigned int i, const PERMlist &S_i, const TRANS &U_i) const = 0;
};

//
//     ----       IMPLEMENTATION
//

template<class PERM, class TRANS>
BaseTranspose<PERM,TRANS>::BaseTranspose() 
	: m_statScheierGeneratorsConsidered(0), m_statNewGenerators(0)
{ }

template<class PERM, class TRANS>
void BaseTranspose<PERM,TRANS>::transpose(BSGS<PERM,TRANS> &bsgs, unsigned int i) const {
	std::vector<dom_int> &B = bsgs.B;
	std::vector<TRANS> &U = bsgs.U;

	if (i+1 >= B.size())
		// illegal transpose index
		return;

	PERMlist S_i;
	std::copy_if(bsgs.S.begin(), bsgs.S.end(), std::back_inserter(S_i), PointwiseStabilizerPredicate<PERM>(B.begin(), B.begin() + i));

	std::swap(B[i], B[i+1]);

	PERMlist S_i1;
	std::copy_if(bsgs.S.begin(), bsgs.S.end(), std::back_inserter(S_i1), PointwiseStabilizerPredicate<PERM>(B.begin(), B.begin() + (i+1)));

	unsigned int targetTransversalSize = U[i+1].size() * U[i].size();

	// new transversal
	TRANS U_i(U[i].n());
	U_i.orbit(B[i], S_i);
	targetTransversalSize /= U_i.size();

	m_statScheierGeneratorsConsidered = 0;
	m_statNewGenerators = 0;
	TRANS U_i1(U[i+1].n());
	U_i1.orbit(B[i+1], S_i1);
    boost::scoped_ptr<Generator<PERM> > generator(setupGenerator(bsgs, i, S_i, U_i));
    BOOST_ASSERT(generator != 0);
    
	while (U_i1.size() < targetTransversalSize) {
		bool newGeneratorFound = false;
		while (generator->hasNext()) {
			PERM g = generator->next();
			++m_statScheierGeneratorsConsidered;
			boost::indirect_iterator<typename PERMlist::iterator> sBegin(S_i1.begin()), sEnd(S_i1.end());
			if (!U_i1.contains(g / B[i+1]) && std::find(sBegin, sEnd, g) == sEnd) {
				g.flush();
				boost::shared_ptr<PERM> gen(new PERM(g));
				S_i1.push_front(gen);
				++m_statNewGenerators;
				U_i1.orbitUpdate(B[i+1], S_i1, gen);
				newGeneratorFound = true;
				break;
			}
		}
		if (!newGeneratorFound)
			// we have exhausted all available generators, and we won't find any new ones in the loop
			break;
	}
	BOOST_ASSERT(U_i1.size() >= targetTransversalSize);

	bsgs.S.insert(bsgs.S.end(), S_i1.begin(), boost::next(S_i1.begin(), m_statNewGenerators));
	U[i] = U_i;
	U[i+1] = U_i1;
}

}

#endif // -- BASETRANSPOSE_H_
