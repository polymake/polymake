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

#include "polymake/Matrix.h"
#include "polymake/PowerSet.h"
#include "polymake/linalg.h"

namespace pm {

// A matrix is totally unimodular if the determinant of each square submatrix equals 0, 1, or -1.

// This is the naive test (exponential in the size of the matrix).
// For polynomial time algorithms see
// Schrijver: Theory of Linear and Integer Programming, section 20.3.

template <typename Matrix, typename E>
bool totally_unimodular(const GenericMatrix<Matrix, E>& M)
{
   const Int m = M.rows();
   const Int n = M.cols();
   const Int r = std::min(m, n);

   for (Int k = 1; k <= r; ++k)
      for (auto ri = entire(all_subsets_of_k(sequence(0,m),k)); !ri.at_end(); ++ri)
         for (auto ci = entire(all_subsets_of_k(sequence(0,n),k)); !ci.at_end(); ++ci) {
            const E d = det(M.minor(*ri, *ci));
            if (!is_zero(d) && !abs_equal(d, one_value<E>())) return false;
         }

   return true;
}

} // end namespace pm


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
