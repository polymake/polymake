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

#pragma once

#include "polymake/GenericMatrix.h"
#include "polymake/permutations.h"

namespace polymake { namespace common {

template <typename E>
struct matrix_elem_comparator {
   typedef operations::cmp type;
};

template <>
struct matrix_elem_comparator<double> {
   typedef operations::cmp_with_leeway type;
};

template <typename Matrix1, typename Matrix2, typename E>
optional<Array<Int>>
find_matrix_row_permutation(const GenericMatrix<Matrix1, E>& M1, const GenericMatrix<Matrix2, E>& M2,
                            bool expect_duplicate_rows = false)
{
   if (M1.rows() != M2.rows() || M1.cols() != M2.cols())
      return nullopt;
   return expect_duplicate_rows
      ? find_permutation_with_duplicates(rows(M1), rows(M2), typename matrix_elem_comparator<E>::type())
      : find_permutation(rows(M1), rows(M2), typename matrix_elem_comparator<E>::type());
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
