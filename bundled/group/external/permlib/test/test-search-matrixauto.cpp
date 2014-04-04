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


#define BOOST_TEST_DYN_LINK 
#define BOOST_TEST_MODULE Backtrack search test - matrix automorphism
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>

#include <permlib/permutation.h>
#include <permlib/bsgs.h>
#include <permlib/symmetric_group.h>
#include <permlib/construct/schreier_sims_construction.h>

#include <permlib/transversal/schreier_tree_transversal.h>
#include <permlib/change/conjugating_base_change.h>
#include <permlib/search/partition/matrix_automorphism_search.h>

#include "test-common.h"

#include <boost/foreach.hpp>

using namespace permlib;
using namespace permlib::test;

template<class T>
class Matrix {
	public:
		Matrix(unsigned long dimension) : m_dimension(dimension), m_matrix(dimension*dimension) {}
		
		unsigned long dimension() const { return m_dimension; }
		inline const T& at(unsigned long i, unsigned long j) const { return m_matrix[j * m_dimension + i]; }
		inline T& at(unsigned long i, unsigned long j) { return m_matrix[j * m_dimension + i]; }
		
	private:
		const unsigned long m_dimension;
		std::vector<T> m_matrix;
};

class ZMatrix : public Matrix<unsigned long> {
	public:
		ZMatrix(unsigned long dimension, unsigned long k = 0) : Matrix<unsigned long>(dimension), m_k(k) {}
		
		unsigned long k() const { return m_k; }
		unsigned long& k() { return m_k; }
	private:
		unsigned long m_k;
};


BOOST_AUTO_TEST_CASE( matrix_auto )
{
	// we use elementary permutations
	typedef Permutation PERM;
	// and Schreier tree transversals
	typedef SchreierTreeTransversal<PERM> TRANSVERSAL;
	
	// our group will have degree 4
	const unsigned long n = 4;
	
	ZMatrix matrix(4,3);
	matrix.at(0,0) = 1;
	matrix.at(0,1) = 0;
	matrix.at(0,2) = 0;
	matrix.at(0,3) = 2;
	
	matrix.at(1,1) = 1;
	matrix.at(1,2) = 0;
	matrix.at(1,3) = 2;
	
	matrix.at(2,2) = 2;
	matrix.at(2,3) = 2;
	
	matrix.at(3,3) = 1;
	
	for(unsigned int i=0; i<n; ++i) {
		for(unsigned int j=i+1; j<n; ++j) {
			matrix.at(j,i) = matrix.at(i,j);
		}
	}
	
	SymmetricGroup<PERM> s_n(n);
	partition::MatrixAutomorphismSearch<SymmetricGroup<PERM>, TRANSVERSAL> mas(s_n, false);
	mas.construct(matrix);
	
	BSGS<PERM,TRANSVERSAL> K(n);
	mas.search(K);
	
	BOOST_CHECK(K.order() == 2);
}
