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

#ifndef SYMPOL_ZMATRIX_H_
#define SYMPOL_ZMATRIX_H_

#include "matrix.h"

namespace sympol {
namespace matrix {

class ZMatrix : public Matrix<ulong> {
	public:
		//TODO: symmetric matrix needs only dim*dim/2 space
		ZMatrix(ulong dimension, ulong k = 0) : Matrix<ulong>(dimension), m_k(k) {}
		
		ulong k() const { return m_k; }
		ulong& k() { return m_k; }
	private:
		ulong m_k;
};

} // ::matrix
} // ::sympol

#endif //SYMPOL_ZMATRIX_H_
