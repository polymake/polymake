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


#ifndef MATRIXREFINEMENT1_H_
#define MATRIXREFINEMENT1_H_

#include <permlib/predicate/pointwise_stabilizer_predicate.h>
#include <permlib/search/partition/refinement.h>

namespace permlib {
namespace partition {

/// concrete \f$\mathcal P\f$-refinement for symmetric matrix automorphisms
/**
 * exploits that if \f$\alpha^g=\beta\f$ then the rows \f$\alpha,\beta\f$ must have the same
 * diagonal elements for an automorphism \f$g\f$
 */
template<class PERM,class MATRIX>
class MatrixRefinement1 : public Refinement<PERM> {
public:
	/// constructor
	explicit MatrixRefinement1(unsigned long n, const MATRIX& matrix);
	
	virtual unsigned int apply(Partition& pi) const;
	
	virtual bool init(Partition& pi);
	
private:
	const MATRIX& m_matrix;
	std::vector<std::list<unsigned long> > m_diagonalPartition;
};

template<class PERM,class MATRIX>
MatrixRefinement1<PERM,MATRIX>::MatrixRefinement1(unsigned long n, const MATRIX& matrix) 
	: Refinement<PERM>(n, Default), m_matrix(matrix)
{
}

template<class PERM,class MATRIX>
unsigned int MatrixRefinement1<PERM,MATRIX>::apply(Partition& pi) const {
	BOOST_ASSERT( this->initialized() );
	
	unsigned int ret = 0;
	std::list<int>::const_iterator cellPairIt = Refinement<PERM>::m_cellPairs.begin();
	while (cellPairIt != Refinement<PERM>::m_cellPairs.end()) {
		unsigned long cell = *cellPairIt;
		++cellPairIt;
		while (cellPairIt != Refinement<PERM>::m_cellPairs.end() && *cellPairIt != -1) {
			unsigned long diagIndex = *cellPairIt;
			if (pi.intersect(m_diagonalPartition[diagIndex].begin(), m_diagonalPartition[diagIndex].end(), cell))
				++ret;
			++cellPairIt;
		}
		++cellPairIt;
	}
	return ret;
}


template<class PERM,class MATRIX>
bool MatrixRefinement1<PERM,MATRIX>::init(Partition& pi) {
	m_diagonalPartition.resize(m_matrix.k());
	for (unsigned long i = 0; i < m_matrix.dimension(); ++i) {
		m_diagonalPartition[m_matrix.at(i,i)].push_back(i);
	}
	
	bool foundIntersection = false;
	for (unsigned int c = 0; c < pi.cells(); ++c) {
		Refinement<PERM>::m_cellPairs.push_back(c);
		for (unsigned long i = 0; i < m_diagonalPartition.size(); ++i) {
			if (pi.intersect(m_diagonalPartition[i].begin(), m_diagonalPartition[i].end(), c)) {
				Refinement<PERM>::m_cellPairs.push_back(i);
				foundIntersection = true;
			}
		}
		Refinement<PERM>::m_cellPairs.push_back(-1);
	}
	if (foundIntersection) {
		typename Refinement<PERM>::RefinementPtr ref(new MatrixRefinement1<PERM,MATRIX>(*this));
		Refinement<PERM>::m_backtrackRefinements.push_back(ref);
		return true;
	}
	return false;
}


}
}

#endif // -- MATRIXREFINEMENT1_H_
