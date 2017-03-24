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

#ifndef SYMPOL_MATRIX_ALGORITHM_H_
#define SYMPOL_MATRIX_ALGORITHM_H_

namespace sympol {
namespace matrix {

template<class Matrix>
class Algorithm {
public:
	Algorithm(Matrix* matrix) : m_matrix(matrix) {}
protected:
	Matrix* m_matrix;

	typedef typename Matrix::Type T;
	T& at(uint i, uint j) { return m_matrix->at(i,j); }
	const T& at(uint i, uint j) const { return m_matrix->at(i,j); }
};

} // ::matrix
} // ::sympol

#endif // SYMPOL_MATRIX_ALGORITHM_H_
