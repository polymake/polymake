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


#ifndef GROUPREFINEMENT_H_
#define GROUPREFINEMENT_H_

#include <permlib/predicate/pointwise_stabilizer_predicate.h>
#include <permlib/search/partition/refinement.h>

namespace permlib {
namespace partition {

/// concrete \f$\mathcal P\f$-refinements for group membership
template<class PERM,class TRANS>
class GroupRefinement : public Refinement<PERM> {
public:
	/// constructor
	explicit GroupRefinement(const BSGSCore<PERM,TRANS>& bsgs);
	
	virtual unsigned int apply(Partition& pi) const;
	virtual unsigned int apply2(Partition& pi, const PERM& t) const;
	
	virtual bool init(Partition& pi);
	
	/// bsgs which membership for is required
	const BSGSCore<PERM,TRANS>& bsgs() const;
private:
	const BSGSCore<PERM,TRANS>& m_bsgs;
	
	std::vector<unsigned int> thetaOrbit;
	std::vector<int> thetaBorder;
	//WARNING: not thread-safe
	/// pre-allocate memory for apply2
	mutable Partition::vector_t m_myTheta;
	
	unsigned int apply2(Partition& pi, const PERM* t) const;
};

template<class PERM,class TRANS>
GroupRefinement<PERM,TRANS>::GroupRefinement(const BSGSCore<PERM,TRANS>& bsgs_) 
	: Refinement<PERM>(bsgs_.n, Group), m_bsgs(bsgs_), thetaOrbit(m_bsgs.n), thetaBorder(m_bsgs.n, -1), m_myTheta(m_bsgs.n)
{
}

template<class PERM,class TRANS>
unsigned int GroupRefinement<PERM,TRANS>::apply(Partition& pi) const {
	return apply2(pi, 0);
}

template<class PERM,class TRANS>
unsigned int GroupRefinement<PERM,TRANS>::apply2(Partition& pi, const PERM& t) const {
	return apply2(pi, &t);
}

template<class PERM,class TRANS>
unsigned int GroupRefinement<PERM,TRANS>::apply2(Partition& pi, const PERM* t) const {
	BOOST_ASSERT( this->initialized() );
	
	Partition::vector_t& myTheta = m_myTheta;
	
	Partition::vector_t::iterator thetaIt;
	Partition::vector_t::iterator thetaBeginIt, thetaEndIt;
	Partition::vector_t::const_iterator thetaOrbitIt;
	std::list<int>::const_iterator cellPairIt = Refinement<PERM>::m_cellPairs.begin();
	unsigned int ret = false;
	while (cellPairIt != Refinement<PERM>::m_cellPairs.end()) {
		const int thetaC = *cellPairIt;
		++cellPairIt;
		if (*cellPairIt < 0) {
			++cellPairIt;
			continue;
		}
		
		int borderLo = 0;
		if (thetaC > 0)
			borderLo = thetaBorder[thetaC-1];
		thetaBeginIt = myTheta.begin() + borderLo;
		thetaEndIt   = myTheta.begin() + thetaBorder[thetaC];
		
		if (t) {
			for (thetaIt = thetaBeginIt, thetaOrbitIt =  thetaOrbit.begin() + borderLo;
					 thetaIt != thetaEndIt && thetaOrbitIt != thetaOrbit.begin() + thetaBorder[thetaC];
					 ++thetaIt, ++thetaOrbitIt) 
			{
				*thetaIt = *t / *thetaOrbitIt;
			}
			std::sort(thetaBeginIt, thetaEndIt);
		}
		
		for (int c = *cellPairIt; c >= 0; c = *(++cellPairIt)) {
			if (t) {
				PERMLIB_DEBUG(std::cout << "apply theta " << thetaC << "," << c << " t = " << *t << " to " << pi << std::endl;)
			} else {
				PERMLIB_DEBUG(std::cout << "apply theta " << thetaC << "," << c << " t = 0 to " << pi << std::endl;)
			}
			//print_iterable(thetaBeginIt, thetaEndIt, 1, "theta apply");
			if (pi.intersect(thetaBeginIt, thetaEndIt, c))
				++ret;
		}
		++cellPairIt;
	}
	
	return ret;
}

template<class PERM,class TRANS>
bool GroupRefinement<PERM,TRANS>::init(Partition& pi) {
	unsigned int fixSize = pi.fixPointsSize();
	if (fixSize > 0) {
		boost::dynamic_bitset<> orbitCharacterstic(m_bsgs.n);
		
		std::vector<dom_int>::const_iterator Bit;
		Partition::vector_t::const_iterator fixIt = pi.fixPointsBegin();
		Partition::vector_t::const_iterator fixEndIt = pi.fixPointsEnd();
		for (Bit = m_bsgs.B.begin(); Bit != m_bsgs.B.end(); ++Bit) {
			while (fixIt != fixEndIt && *fixIt != *Bit) {
				PERMLIB_DEBUG(std::cout << "skip " << (*fixIt + 1) << " for " << (*Bit + 1) << std::endl;)
				++fixIt;
			}
			if (fixIt == fixEndIt)
				break;
			++fixIt;
		}
		//PointwiseStabilizerPredicate<PERM> fixStab(m_bsgs.B.begin(), m_bsgs.B.begin() + std::min(fixSize, static_cast<unsigned int>(m_bsgs.B.size())));
#ifdef PERMLIB_DEBUG_OUTPUT
		print_iterable(m_bsgs.B.begin(), m_bsgs.B.end(), 1, " BSGS ");
		print_iterable(m_bsgs.B.begin(), Bit, 1, "to fix");
		print_iterable(pi.fixPointsBegin(), pi.fixPointsEnd(), 1, "   fix");
#endif
		PointwiseStabilizerPredicate<PERM> fixStab(m_bsgs.B.begin(), Bit);
		std::list<PERM> S_fix;
		BOOST_FOREACH(const typename PERM::ptr& p, m_bsgs.S) {
			if (fixStab(p)) {
				//std::cout << "$ " << *p << " fixes " << std::min(fixSize, static_cast<unsigned long>(m_bsgs.B.size())) << " points" << std::endl;
				S_fix.push_back(*p);
			}
		}
		
		unsigned int thetaIndex = 0;
		std::vector<int>::iterator thetaBorderIt = thetaBorder.begin();
		std::vector<unsigned int>::iterator thetaIt = thetaOrbit.begin();
		for (unsigned long alpha = 0; alpha < m_bsgs.n; ++alpha) {
			if (orbitCharacterstic[alpha])
				continue;
			orbitCharacterstic.flip(alpha);
			std::vector<unsigned int>::iterator thetaOrbitBeginIt = thetaIt;
			*thetaIt = alpha;
			++thetaIt;
			++thetaIndex;
			std::vector<unsigned int>::iterator thetaOrbitEndIt = thetaIt;
			
			std::vector<unsigned int>::iterator it;
			for (it = thetaOrbitBeginIt; it != thetaOrbitEndIt; ++it) {
				unsigned int beta = *it;
				BOOST_FOREACH(const PERM &p, S_fix) {
					unsigned int beta_p = p / beta;
					if (!orbitCharacterstic[beta_p]) {
						*thetaIt = beta_p;
						++thetaIt;
						++thetaOrbitEndIt;
						++thetaIndex;
						orbitCharacterstic.flip(beta_p);
					}
				}
			}
			*thetaBorderIt = thetaIndex;
			++thetaBorderIt;
		}
		
		thetaIt = thetaOrbit.begin();
		std::vector<unsigned int>::iterator thetaItEnd;
		thetaBorderIt = thetaBorder.begin();
		unsigned int thetaC = 0;
		int oldBorder = 0;
		while (thetaBorderIt != thetaBorder.end() && *thetaBorderIt >= 0) {
			thetaItEnd = thetaOrbit.begin() + *thetaBorderIt;
			std::sort(thetaIt, thetaItEnd);
			
			if (*thetaBorderIt - oldBorder != 1 || std::find(pi.fixPointsBegin(), pi.fixPointsEnd(), *thetaIt) == pi.fixPointsEnd()) {
				bool hasTheta = false;
				const unsigned int oldCellNumber = pi.cells();
				for (unsigned int c = 0; c < oldCellNumber; ++c) {
					if (pi.cellSize(c) == 1)
						continue;
					
					//std::cout << "  theta pi = " << pi << std::endl;
					//print_iterable(thetaIt, thetaItEnd, 1, "theta prep");
					if (pi.intersect(thetaIt, thetaItEnd, c)) {
						PERMLIB_DEBUG(std::cout << "prepare theta " << thetaC << "," << c << std::endl;)
						//print_iterable(thetaIt, thetaItEnd, 1, "theta prep");
						if (!hasTheta) {
							Refinement<PERM>::m_cellPairs.push_back(thetaC);
							hasTheta = true;
						}
						Refinement<PERM>::m_cellPairs.push_back(c);
						//std::cout << (thetaIt - thetaOrbit.begin()) << " - " << (thetaItEnd - thetaOrbit.begin()) << std::endl;
					}
					//std::cout << "pii = " << pi << std::endl;
				}
				
				if (hasTheta)
					Refinement<PERM>::m_cellPairs.push_back(-1);
			}
			
			oldBorder = *thetaBorderIt;
			thetaIt = thetaItEnd;
			++thetaC;
			++thetaBorderIt;
		}
		//print_iterable(Refinement<PERM>::m_cellPairs.begin(), Refinement<PERM>::m_cellPairs.end(), 0, "cell pairs");
		if (!Refinement<PERM>::m_cellPairs.empty()) {
			typename Refinement<PERM>::RefinementPtr ref(new GroupRefinement<PERM,TRANS>(*this));
			Refinement<PERM>::m_backtrackRefinements.push_back(ref);
			return true;
		}
	} 
	
	return false;
}

template<class PERM,class TRANS>
const BSGSCore<PERM,TRANS>& GroupRefinement<PERM,TRANS>::bsgs() const {
	return m_bsgs;
}

}
}

#endif // -- GROUPREFINEMENT_H_
