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

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/PowerSet.h"
#include "polymake/list"
#include "polymake/tropical/arithmetic.h"
#include <string>
#include <algorithm>

namespace polymake { namespace tropical {

template <typename Addition>
Vector<TropicalNumber<Addition> > lifted_pluecker(const Matrix<TropicalNumber<Addition> > &V)
{
   const Int n(V.rows()), d(V.cols());
   const Int nd_d(Integer::binom(d+n,d));
   Vector<TropicalNumber<Addition> > pi(nd_d);

   const sequence all_rows = sequence(0,d+n); // d extra rows plus n given ones
   Int i = 0;

   // first merging given and extra rows and then taking them apart again
   // yields the desired ordering of the coefficients of the tropical Pluecker vector
   for (auto rho = entire(all_subsets_of_k(all_rows, d)); !rho.at_end(); ++rho) {
      Set<Int> sigma, tau;
      for (auto rii=entire(*rho); !rii.at_end(); ++rii) {
         // take given and extra rows apart
         if (*rii < d)
            tau.push_back(*rii);
         else
            sigma.push_back(*rii-d);
      }
      pi[i] = tdet(Matrix<TropicalNumber<Addition>>(V.minor(sigma, ~tau)));
      ++i;
   }
   return pi;
}

UserFunctionTemplate4perl("# @category Other"
                  "# Compute the tropical Pluecker vector from a matrix representing points in the tropical torus."
                  "# This can be used to lift regular subdivisions of a product of simplices to a"
                  "# matroid decomposition of hypersimplices."
                  "# @param Matrix<TropicalNumber<Addition> > V"
                  "# @return Vector<TropicalNumber<Addition> >",
                  "lifted_pluecker<Addition>(Matrix<TropicalNumber<Addition> >)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
