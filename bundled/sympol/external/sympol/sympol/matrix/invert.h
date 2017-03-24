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

#ifndef SYMPOL_MATRIX_INVERT_H_
#define SYMPOL_MATRIX_INVERT_H_

#include "algorithm.h"

namespace sympol {
namespace matrix {

/// inverts a matrix by LUP-decomposition
template<class Matrix>
class Invert : public Algorithm<Matrix> {
public:
	Invert(Matrix* matrix) : Algorithm<Matrix>(matrix) {
		BOOST_ASSERT(matrix->rows() == matrix->cols());
	}

	using Algorithm<Matrix>::at;
	using Algorithm<Matrix>::m_matrix;

	bool invert(Matrix* inv);
protected:
	typedef typename Matrix::Type T;

	bool LUPdecompose(std::vector<ulong>& pi);
	void LUPsolve(const std::vector<ulong>& pi, const std::vector<T>& b, std::vector<T>& x) const;
};

template<class Matrix>
inline bool Invert<Matrix>::LUPdecompose(std::vector<ulong>& pi) {
	const ulong n = m_matrix->dimension();
	
	for (uint i = 0; i < n; ++i)
		pi[i] = i;
	
	for (uint k = 0; k < n; ++k) {
		T p;
		uint k_prime = 0;
		for (uint i = k; i < n; ++i) {
			if (cmp(abs(at(i,k)), p) > 0) {
				p = abs(at(i,k));
				k_prime = i;
			}
		}
		if (sgn(p) == 0) {
			return false;
		}
		std::swap(pi[k], pi[k_prime]);
		for (uint i = 0; i < n; ++i) {
			std::swap(at(k,i), at(k_prime, i));
		}
		for (uint i = k+1; i < n; ++i) {
			at(i,k) /= at(k,k);
			for (uint j = k+1; j < n; ++j) {
				at(i,j) -= at(i,k) * at(k,j);
			}
		}
	}
	return true;
}

template<class Matrix>
inline void Invert<Matrix>::LUPsolve(const std::vector<ulong>& pi, const std::vector<T>& b, std::vector<T>& x) const {
	const ulong n = m_matrix->dimension();
	std::vector<T> y(n);
	
	for (uint i = 0; i < n; ++i) {
		y[i] = b[pi[i]];
		for (uint j = 0; j < i; ++j) {
			y[i] -= at(i,j) * y[j];
		}
	}
	
	for (int i = n-1; i >= 0; --i) {
		x[i] = y[i];
		for (uint j = i+1; j < n; ++j) {
			x[i] -= at(i,j) * x[j];
		}
		x[i] /= at(i,i);
	}
}

template<class Matrix>
inline bool Invert<Matrix>::invert(Matrix* inv) {
	BOOST_ASSERT(inv->dimension() == m_matrix->dimension());
	const T zero(0);
	const T one(1);
	
	const ulong n = m_matrix->dimension();
	std::vector<ulong> pi(n);
	if (!LUPdecompose(pi))
		return false;
	
	std::vector<mpq_class> b(n), x(n);
	for (uint i = 0; i < n; ++i) {
		b[i] = one;
		if (i > 0)
			b[i-1] = zero;
		
		for (uint j = 0; j < n; ++j)
			x[j] = zero;
		
		LUPsolve(pi, b, x);
		for (uint j = 0; j < n; ++j)
			inv->at(j,i) = x[j];
	}
	return true;
}

} // ::matrix
} // ::sympol

#endif //SYMPOL_MATRIX_INVERT_H_
