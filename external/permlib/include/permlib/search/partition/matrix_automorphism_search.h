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


#ifndef PARTITION_MATRIX_AUTOMORPHISM_SEARCH_H_
#define PARTITION_MATRIX_AUTOMORPHISM_SEARCH_H_

#include <permlib/search/partition/r_base.h>
#include <permlib/search/partition/matrix_refinement1.h>
#include <permlib/search/partition/refinement_family.h>
#include <permlib/predicate/matrix_automorphism_predicate.h>

namespace permlib {
namespace partition {

/// subgroup search for the automorphism group of a symmetric matrix based on partition backtracking
template<class BSGSIN,class TRANSRET>
class MatrixAutomorphismSearch : public RBase<BSGSIN,TRANSRET> {
public:
	typedef typename RBase<BSGSIN,TRANSRET>::PERM PERM;
	
	/// constructor
	/**
	 * @param bsgs BSGS of group
	 * @param pruningLevelDCM level up to which expensive double coset minimality pruning is performed; zero to disable
	 */
	MatrixAutomorphismSearch(const BSGSIN& bsgs, unsigned int pruningLevelDCM);
	
	/// initializes search with inital partition
	/**
	 * @param matrix symmetric matrix
	 * @param initialPartitionBegin begin-iterator to initial list of row/column indices that have to be mapped onto each other
	 * @param initialPartitionEnd   end-iterator to initial list of row/column indices that have to be mapped onto each other
	 */
	template<class MATRIX, class Iterator>
	void construct(const MATRIX& matrix, Iterator initialPartitionBegin, Iterator initialPartitionEnd);
	
	/// initializes search
	/**
	 * @param matrix symmetric matrix
	 */
	template<class MATRIX>
	void construct(const MATRIX& matrix) { construct<MATRIX, unsigned int*>(matrix, 0, 0); }
};

template<class BSGSIN,class TRANSRET>
MatrixAutomorphismSearch<BSGSIN,TRANSRET>::MatrixAutomorphismSearch(const BSGSIN& bsgs, unsigned int pruningLevelDCM) 
	: RBase<BSGSIN,TRANSRET>(bsgs, pruningLevelDCM)
{ }

template<class BSGSIN,class TRANSRET>
template<class MATRIX, class Iterator>
void MatrixAutomorphismSearch<BSGSIN,TRANSRET>::construct(const MATRIX& matrix, Iterator initialPartitionBegin, Iterator initialPartitionEnd) {
	MatrixAutomorphismPredicate<PERM,MATRIX>* matrixPred = new MatrixAutomorphismPredicate<PERM,MATRIX>(matrix);
	
	if (initialPartitionBegin != initialPartitionEnd) {
		// set up partitions such that the elements of initialPartition will be mapped onto itself
		RBase<BSGSIN,TRANSRET>::m_partition.intersect(initialPartitionBegin, initialPartitionEnd, 0);
		RBase<BSGSIN,TRANSRET>::m_partition2.intersect(initialPartitionBegin, initialPartitionEnd, 0);
	}
	
	MatrixRefinement1<PERM,MATRIX> matRef(RBase<BSGSIN,TRANSRET>::m_bsgs.n, matrix);
	matRef.initializeAndApply(RBase<BSGSIN,TRANSRET>::m_partition);
	PERM empty(RBase<BSGSIN,TRANSRET>::m_bsgs.n);
	matRef.apply2(RBase<BSGSIN,TRANSRET>::m_partition2, empty);
	
	RBase<BSGSIN,TRANSRET>::construct(matrixPred, 
		new MatrixAutomorphismRefinementFamily<PERM,MATRIX>(RBase<BSGSIN,TRANSRET>::m_bsgs.n, matrix));
}

}
}

#endif // -- PARTITION_MATRIX_AUTOMORPHISM_SEARCH_H_
