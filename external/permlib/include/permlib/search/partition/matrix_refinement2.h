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


#ifndef MATRIXREFINEMENT2_H_
#define MATRIXREFINEMENT2_H_

#include <permlib/predicate/pointwise_stabilizer_predicate.h>
#include <permlib/search/partition/refinement.h>

#include <map>

namespace permlib {
namespace partition {

/// concrete \f$\mathcal P\f$-refinement for symmetric matrix automorphisms
/**
 * implements [Leon1991] Section 9 (g)
 */
template<class PERM,class MATRIX>
class MatrixRefinement2 : public Refinement<PERM> {
public:
	/// constructor
	explicit MatrixRefinement2(unsigned long n, const MATRIX& matrix);
	
	virtual unsigned int apply(Partition& pi) const;
	
	virtual bool init(Partition& pi);
	
private:
	const MATRIX& m_matrix;
	
	/// distribution of element frequency across a matrix row or column
	class Fingerprint {
		public:
			Fingerprint(unsigned long k) : m_fingerprint(k) {}
			
			/// lex-min ordering
			bool operator<(const Fingerprint& f) const {
				BOOST_ASSERT(f.m_fingerprint.size() == m_fingerprint.size());
				for (unsigned int i=0; i<m_fingerprint.size(); ++i) {
					if (m_fingerprint[i] < f.m_fingerprint[i])
						return true;
					if (m_fingerprint[i] > f.m_fingerprint[i])
						return false;
				}
				return false;
			}
			bool operator==(const Fingerprint& f) const {
				BOOST_ASSERT(f.m_fingerprint.size() == m_fingerprint.size());
				for (unsigned int i=0; i<m_fingerprint.size(); ++i) {
					if (m_fingerprint[i] != f.m_fingerprint[i])
						return false;
				}
				return true;
			}
			unsigned long& operator[](unsigned long i) { 
				BOOST_ASSERT(i < m_fingerprint.size());
				return m_fingerprint[i]; 
			}
			const unsigned long& operator[](unsigned long i) const { 
				BOOST_ASSERT(i < m_fingerprint.size());
				return m_fingerprint[i]; 
			}
		private:
			std::vector<unsigned long> m_fingerprint;
	};
	
	unsigned int splitCell(Partition& pi, unsigned long i) const;
	void computeFingerprint(const Partition& pi, unsigned long i, unsigned long j, std::map<Fingerprint,std::list<unsigned long> >& map) const;
};

template<class PERM,class MATRIX>
MatrixRefinement2<PERM,MATRIX>::MatrixRefinement2(unsigned long n, const MATRIX& matrix) 
	: Refinement<PERM>(n, Default), m_matrix(matrix)
{
}

template<class PERM,class MATRIX>
unsigned int MatrixRefinement2<PERM,MATRIX>::apply(Partition& pi) const {
	BOOST_ASSERT( this->initialized() );
	
	unsigned int ret = 0;
	std::list<int>::const_iterator cellPairIt = Refinement<PERM>::m_cellPairs.begin();
	while (cellPairIt != Refinement<PERM>::m_cellPairs.end()) {
		unsigned long i = *cellPairIt++;
		ret += splitCell(pi, static_cast<unsigned long>(i));
	}
	return ret;
}


template<class PERM,class MATRIX>
bool MatrixRefinement2<PERM,MATRIX>::init(Partition& pi) {
	for (unsigned long i = 0; i < pi.cells(); ++i) {
		if (splitCell(pi, i))
			Refinement<PERM>::m_cellPairs.push_back(i);
	}
	
	if (!Refinement<PERM>::m_cellPairs.empty()) {
		typename Refinement<PERM>::RefinementPtr ref(new MatrixRefinement2<PERM,MATRIX>(*this));
		Refinement<PERM>::m_backtrackRefinements.push_back(ref);
		return true;
	}
	return false;
}

template<class PERM,class MATRIX>
unsigned int MatrixRefinement2<PERM,MATRIX>::splitCell(Partition& pi, unsigned long i) const {
	unsigned long ret = 0;
	if (pi.cellSize(i) < 2)
		return ret;
	for (unsigned long j = 0; j < pi.cells(); ++j) {
		std::map<Fingerprint,std::list<unsigned long> > map;
		computeFingerprint(pi, i, j, map);
		if (map.size() > 1) {
			PERMLIB_DEBUG(std::cout << "split " << i << " because of " << j << " in " << pi << std::endl;)
			typename std::map<Fingerprint,std::list<unsigned long> >::const_iterator fit;
			for (fit = map.begin(); fit != map.end(); ++fit) {
				std::pair<Fingerprint, std::list<unsigned long> > splitCellPair = *fit;
				/*std::cout << "FOO ";
				BOOST_FOREACH(unsigned long a, splitCellPair.second) {
					std::cout << (a+1) << " ";
				}
				std::cout << std::endl;
				std::cout << "GOO ";
				BOOST_FOREACH(unsigned long a, splitCellPair.first.m_fingerprint) {
					std::cout << (a) << " ";
				}
				std::cout << std::endl;*/
				if (pi.intersect(splitCellPair.second.begin(), splitCellPair.second.end(), i)) {
					++ret;
				}
			}
			break;
		}
	}
	return ret;
}

template<class PERM,class MATRIX>
void MatrixRefinement2<PERM,MATRIX>::computeFingerprint(const Partition& pi, unsigned long i, unsigned long j, std::map<Fingerprint,std::list<unsigned long> >& map) const {
	for (Partition::CellIt cellI = pi.cellBegin(i); cellI != pi.cellEnd(i); ++cellI) {
		Fingerprint f(m_matrix.k());
		for (Partition::CellIt cellJ = pi.cellBegin(j); cellJ != pi.cellEnd(j); ++cellJ) {
			++f[m_matrix.at(*cellI, *cellJ)];
		}
		std::pair<typename std::map<Fingerprint,std::list<unsigned long> >::iterator, bool> p = 
			map.insert(std::pair<Fingerprint, std::list<unsigned long> >(f, std::list<unsigned long>()));
		std::list<unsigned long>& l = p.first->second;
		l.push_back(*cellI);
	}
}

}
}

#endif // -- MATRIXREFINEMENT2_H_
