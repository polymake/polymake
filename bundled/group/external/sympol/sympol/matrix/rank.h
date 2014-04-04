// ---------------------------------------------------------------------------
//
// This file is part of SymPol
//
// Copyright (C) 2006-2010  Thomas Rehn <thomas@carmen76.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// ---------------------------------------------------------------------------

#ifndef SYMPOL_MATRIX_RANK_H_
#define SYMPOL_MATRIX_RANK_H_

#include "algorithm.h"

#include <algorithm>		// for std::min
#include <list>
#include <set>
#include <boost/foreach.hpp>
#include <boost/assert.hpp>

namespace sympol {
namespace matrix {

/// computes the rank, the column defect or kernel of a matrix by Gaussian eliminiation
template<class Matrix>
class Rank : public Algorithm<Matrix> {
public:
	Rank(Matrix* matrix) : Algorithm<Matrix>(matrix) {}

	using Algorithm<Matrix>::at;
	using Algorithm<Matrix>::m_matrix;

	/**
	 * @return rank of the matrix
	 */
	ulong rank();
	/**
	 * @param freeColumnsIt iterator to store the indices of redundant columns
	 */
	template<class InsertIterator>
	void columnDefect(InsertIterator freeColumnsIt);
	/**
	 * @return matrix whose columns span the kernel of the matrix
	 */
	Matrix* kernel();
private:
	template<class InsertIterator>
	void rowReducedEchelonForm(bool allowTransposition, InsertIterator freeVariablesIt);
};

template<class Matrix>
inline ulong Rank<Matrix>::rank() {
	std::list<uint> freeVariables;
	rowReducedEchelonForm(true, std::inserter(freeVariables, freeVariables.end()));
	return std::min(m_matrix->cols(), m_matrix->rows()) - freeVariables.size();
}

template<class Matrix>
template<class InsertIterator>
inline void Rank<Matrix>::columnDefect(InsertIterator freeColumns) {
	rowReducedEchelonForm(false, freeColumns);
}

template<class Matrix>
inline Matrix* Rank<Matrix>::kernel() {
	std::set<uint> freeVariables;
	rowReducedEchelonForm(false, std::inserter(freeVariables, freeVariables.end()));
	if (freeVariables.size() == 0)
		return NULL;
	
	Matrix* kern = new Matrix(m_matrix->cols(), freeVariables.size());
	uint kernIndex = 0;
	// back-substitution to compute kernel base,
	// one vector for each free variable
	BOOST_FOREACH(const uint& f, freeVariables) {
		kern->at(f,kernIndex) = -1;
		int i = m_matrix->rows() - 1;
		for (int k = m_matrix->cols() - 1; k >= 0; --k) {
			if (freeVariables.count(k) == 0) {
				while (sgn(m_matrix->at(i, k)) == 0) {
					--i;
				}
				BOOST_ASSERT( i >= 0 );
				for (int l = k+1; l < m_matrix->cols(); ++l) {
					kern->at(k,kernIndex) += kern->at(l,kernIndex) * m_matrix->at(i, l);
				}
				kern->at(k,kernIndex) = -kern->at(k,kernIndex);
			}
		}
		++kernIndex;
	}
	return kern;
}


template<class Matrix>
template<class InsertIterator>
inline void Rank<Matrix>::rowReducedEchelonForm(bool rankOnly, InsertIterator freeVariablesIt) {
	if (rankOnly && m_matrix->cols() > m_matrix->rows())
		m_matrix->transpose();

	const ulong maxRank = std::min(m_matrix->cols(), m_matrix->rows());
	ulong rank = 0;
	uint k = 0;
	const ulong m = m_matrix->rows();
	const ulong n = m_matrix->cols();
	std::vector<ulong> pi(m);
	for (uint i = 0; i < m; ++i)
		pi[i] = i;

	for (uint r = 0; r < n; ++r) {
		typename Matrix::Type p;
		uint k_prime = 0;
		for (uint i = k; i < m; ++i) {
			if (cmp(abs(at(i,r)), p) > 0) {
				p = abs(at(i,r));
				k_prime = i;
			}
		}
		if (sgn(p) == 0) {
			*freeVariablesIt++ = r;
			continue;
		}
		++rank;
		if (rankOnly && rank == maxRank)
			return;

		std::swap(pi[k], pi[k_prime]);
		for (uint j = 0; j < n; ++j) {
			std::swap(at(k,j), at(k_prime, j));
		}
		for (uint i = k+1; i < m; ++i) {
			at(i,r) /= at(k,r);
			for (uint j = r+1; j < n; ++j) {
				at(i,j) -= at(i,r) * at(k,j);
			}
			at(i,r) = 0;
		}
		for (uint j = r+1; j < n; ++j) {
			at(k,j) /= at(k,r);
		}
		at(k,r) = 1;
		++k;
	}
}

} // ::matrix
} // ::sympol

#endif // SYMPOL_MATRIX_RANK_H_
