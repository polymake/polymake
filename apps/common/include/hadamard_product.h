/* Copyright (c) 1997-2020
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#include "polymake/GenericMatrix.h"

namespace polymake { namespace common {

template <typename Scalar, typename Matrix1, typename Matrix2>
pm::LazyMatrix2<const Matrix1&, const Matrix2&, operations::mul>
hadamard_product(const GenericMatrix<Matrix1, Scalar>& mat1, const GenericMatrix<Matrix2, Scalar>& mat2)
{
   if (mat1.rows() != mat2.rows() || mat1.cols() != mat2.cols())
      throw std::runtime_error("hadamard_product - dimension mismatch");

   return pm::LazyMatrix2<const Matrix1&, const Matrix2&, operations::mul>(mat1.top(), mat2.top());
}

}}
