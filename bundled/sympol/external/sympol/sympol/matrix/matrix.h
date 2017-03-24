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

#ifndef SYMPOL_MATRIX_H_
#define SYMPOL_MATRIX_H_

#include <iostream>
#include <vector>

namespace sympol {
namespace matrix {

template<class T>
class Matrix;

template <class T>
std::ostream &operator<< (std::ostream &out, const Matrix<T>& matrix) {
	for (uint i=0; i<matrix.m_rows; ++i) {
		for (uint j=0; j<matrix.m_cols; ++j) {
			out << matrix.at(i,j) << " ";
		}
		out << std::endl;
	}
	return out;
}

template<class T>
class Matrix {
	public:
		Matrix(ulong dimension) : m_rows(dimension), m_cols(dimension), m_matrix(dimension*dimension), m_transposed(false) {}
		Matrix(ulong rows, ulong cols) : m_rows(rows), m_cols(cols), m_matrix(rows*cols), m_transposed(false) {}
		Matrix(const Matrix<T>& mat) : m_rows(mat.m_rows), m_cols(mat.m_cols), m_matrix(mat.m_matrix), m_transposed(mat.m_transposed) {}
		
		inline ulong dimension() const { return m_rows; }
		inline ulong rows() const { return m_rows; }
		inline ulong cols() const { return m_cols; }
		inline const T& at(ulong i, ulong j) const {
			if (m_transposed)
				return m_matrix[i * m_cols + j];
			return m_matrix[j * m_rows + i];
		}
		inline T& at(ulong i, ulong j) {
			if (m_transposed)
				return m_matrix[i * m_cols + j];
			return m_matrix[j * m_rows + i];
		}
		void transpose() {
			m_transposed ^= true;
			std::swap(m_rows, m_cols);
		}
		
		friend std::ostream &operator<< <> (std::ostream &out, const Matrix<T>& matrix);
		typedef T Type;
	private:
		ulong m_rows;
		ulong m_cols;
		std::vector<T> m_matrix;
		bool m_transposed;
};

} // ::matrix
} // ::sympol

#endif // SYMPOL_MATRIX_H_
