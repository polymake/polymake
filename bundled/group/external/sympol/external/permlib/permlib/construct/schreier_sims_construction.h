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


#ifndef SCHREIERSIMSCONSTRUCTION_H_
#define SCHREIERSIMSCONSTRUCTION_H_

#include <permlib/construct/base_construction.h>
#include <permlib/bsgs.h>
#include <permlib/generator/schreier_generator.h>

#include <boost/foreach.hpp>

namespace permlib {

/// BSGS construction with classic Schreier-Sims algorithm
template <class PERM, class TRANS>
class SchreierSimsConstruction : public BaseConstruction<PERM, TRANS> {
public:
	/// constructor
	/**
	 * @param n cardinality of the set the group is acting on
	 */
	explicit SchreierSimsConstruction(unsigned int n);

	/// constructs a BSGS for group given by generators with no base prescribed
	/** @see construct(ForwardIterator generatorsBegin, ForwardIterator generatorsEnd, InputIterator prescribedBaseBegin, InputIterator prescribedBaseEnd)
	 */
	template <class ForwardIterator>
	BSGS<PERM, TRANS> construct(ForwardIterator generatorsBegin, ForwardIterator generatorsEnd) const;

	/// constructs a BSGS for group given by generators respecting prescribed base elements
	/**
	 * @param generatorsBegin begin iterator of group generators of type PERM
	 * @param generatorsEnd  end iterator of group generators of type PERM
	 * @param prescribedBaseBegin begin iterator of prescribed base of type unsigned long
	 * @param prescribedBaseEnd  end iterator of prescribed base of type unsigned long
	 */
	template <class ForwardIterator, class InputIterator>
	BSGS<PERM, TRANS> construct(ForwardIterator generatorsBegin, ForwardIterator generatorsEnd, InputIterator prescribedBaseBegin, InputIterator prescribedBaseEnd) const;

	/// number of Schreier generators examined during the last construct call
	mutable unsigned int m_statScheierGeneratorsConsidered;
};

//
//     ----       IMPLEMENTATION
//

template <class PERM, class TRANS>
SchreierSimsConstruction<PERM,TRANS>::SchreierSimsConstruction(unsigned int n) 
	: BaseConstruction<PERM, TRANS>(n), m_statScheierGeneratorsConsidered(0)
{ }

template <class PERM, class TRANS>
template <class ForwardIterator>
inline BSGS<PERM, TRANS> SchreierSimsConstruction<PERM,TRANS>::construct(ForwardIterator generatorsBegin, ForwardIterator generatorsEnd) const {
	return construct(generatorsBegin, generatorsEnd, BaseConstruction<PERM,TRANS>::empty, BaseConstruction<PERM,TRANS>::empty);
}

template <class PERM, class TRANS>
template <class ForwardIterator, class InputIterator>
BSGS<PERM, TRANS> SchreierSimsConstruction<PERM, TRANS>
	::construct(ForwardIterator generatorsBegin, ForwardIterator generatorsEnd, InputIterator prescribedBaseBegin, InputIterator prescribedBaseEnd) const
{
	const dom_int &n = this->m_n;
	BSGS<PERM, TRANS> ret(n);
	std::vector<dom_int> &B = ret.B;
	std::vector<TRANS> &U = ret.U;
	std::vector<std::list<typename PERM::ptr> > S;
	this->setup(generatorsBegin, generatorsEnd, prescribedBaseBegin, prescribedBaseEnd, ret, S);
	
	std::vector<boost::shared_ptr<SchreierGenerator<PERM, TRANS> > > SchreierGens;
	for (unsigned int i = 0; i < B.size(); ++i) {
		BOOST_ASSERT( i < U.size() );
		BOOST_ASSERT( i < S.size() );
		SchreierGens.push_back(boost::shared_ptr<SchreierGenerator<PERM, TRANS> >(new SchreierGenerator<PERM, TRANS>(&U[i], S[i].begin(), S[i].end())));
	}
	
	unsigned int j = B.size();
	bool breakUp = false;
	while (j >= 1) {
		breakUp = false;
		SchreierGenerator<PERM, TRANS> &sg = *SchreierGens[j-1];
		sg.update(&U[j-1], S[j-1].begin(), S[j-1].end());

		while (sg.hasNext()) {
			++m_statScheierGeneratorsConsidered;
			PERMLIB_DEBUG(for(unsigned int jjj=0; jjj<j; ++jjj) std::cout << " ";)
			PERMLIB_DEBUG(std::cout << "schreier j = " << (j-1) << " [" << S[j-1].size() << "," << U[j-1].size() << "]:   ";)
			PERM g = sg.next();
			PERM h(n);
			// sift for S_{j+1}, so use index j here
			unsigned int k = ret.sift(g, h, j);
			if (k < B.size() - j || !h.isIdentity()) {
				// flush it, because we add it as a generator
				h.flush();
				
				if (j == B.size()) {
					dom_int gamma = n+1;
					if (ret.chooseBaseElement(h, gamma)) {
						B.push_back(gamma);
					}
					BOOST_ASSERT(j < B.size());
					S.push_back(std::list<typename PERM::ptr>());
					U.push_back(TRANS(n));
				}
				boost::shared_ptr<PERM> hPtr(new PERM(h));
				S[j].insert(S[j].end(), hPtr);

				ret.orbitUpdate(j, S[j], hPtr);
				if (j >= SchreierGens.size()) {
					boost::shared_ptr<SchreierGenerator<PERM, TRANS> > localVar(new SchreierGenerator<PERM, TRANS>(&U[j], S[j].begin(), S[j].end()));
					SchreierGens.push_back(localVar);
				} else {
					SchreierGens[j]->update(S[j].size() - 1);
				}

				breakUp = true;
				++j;
				break;
			}
		}
		if (!breakUp)
			--j;
	}

	this->mergeGenerators(S, ret);

	return ret;
}

}

#endif // -- SCHREIERSIMSCONSTRUCTION_H_
