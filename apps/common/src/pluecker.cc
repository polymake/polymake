/* Copyright (c) 1997-2019
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
Vector<Scalar> pluecker(const Matrix<Scalar>& V)
{
   const int n(V.rows()),
             d(V.cols()),
             r(rank(V)),
             sz(Integer::binom(n,r) * Integer::binom(d,r));

   const sequence all_rows = sequence(0, n);
   const sequence all_cols = sequence(0, d);

   Vector<Scalar> pi(sz);

   int i=0;
   for (auto rho = entire(all_subsets_of_k(all_rows, r)); !rho.at_end(); ++rho)
      for (auto sigma = entire(all_subsets_of_k(all_cols, r)); !sigma.at_end(); ++sigma) {
         pi[i]=det(Matrix<Scalar>(V.minor(*rho,*sigma)));
         ++i;
      }

   return pi;
}

UserFunctionTemplate4perl("# @category Linear Algebra"
                          "# Compute the vector of maximal minors of a matrix."
                          "# WARNING: interpretation different in [[tropical::lifted_pluecker]]"
                          "# @param Matrix V"
                          "# @return Vector",
                          "pluecker(Matrix)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
