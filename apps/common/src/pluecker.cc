/* Copyright (c) 1997-2023
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

#include "polymake/client.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/PowerSet.h"
#include "polymake/linalg.h"
#include "polymake/list"
#include <string>
#include <algorithm>

namespace polymake { namespace common {

template <typename Scalar>
Vector<Scalar> pluecker(const Matrix<Scalar>& M)
{
   const Int n(M.rows()),
             d(M.cols()),
             r(rank(M)),
             sz(Integer::binom(n,r) * Integer::binom(d,r));

   const sequence all_rows = sequence(0, n);
   const sequence all_cols = sequence(0, d);

   Vector<Scalar> pi(sz);

   Int i = 0;
   for (auto rho = entire(all_subsets_of_k(all_rows, r)); !rho.at_end(); ++rho)
      for (auto sigma = entire(all_subsets_of_k(all_cols, r)); !sigma.at_end(); ++sigma) {
         pi[i] = det(Matrix<Scalar>(M.minor(*rho,*sigma)));
         ++i;
      }

   return pi;
}

UserFunctionTemplate4perl("# @category Linear Algebra"
                          "# Compute the vector of maximal minors of the matrix //M//."
                          "# See also [[tropical::tpluecker]] which is related."
                          "# @param Matrix M"
                          "# @return Vector"
                          "# @example with parameters (2,4)"
                          "# > $M = new Matrix<Rational>([[1,0],[0,1],[1,1],[1,3]]);"
                          "# > print pluecker($M);"
                          "# | 1 1 3 -1 -1 2"
                          ,
                          "pluecker(Matrix)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
