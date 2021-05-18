/* Copyright (c) 1997-2021
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

#include "polymake/common/bounding_box.h"

namespace polymake { namespace common {

UserFunctionTemplate4perl("# @category Utilities"
                           "# Compute the column-wise bounds for the given Matrix //m//."
                           "# @param Matrix m"
                           "# @return Matrix a Matrix with two rows and //m//->[[Matrix::cols|cols]] columns; [[Matrix::row|row]](0) contains the lower bounds, [[Matrix::row|row]](1) contains the upper bounds.",
                           "bounding_box<Scalar>( Matrix<type_upgrade<Scalar>> )");

FunctionTemplate4perl("extend_bounding_box(Matrix& Matrix)");

}}
