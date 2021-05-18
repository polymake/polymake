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
Vector<TropicalNumber<Addition> > tpluecker(const Matrix<TropicalNumber<Addition> > &M)
{
   const Int n(M.rows()), d(M.cols()); // points are row vectors
   if (n<d)
      throw std::runtime_error("tpluecker: n (#rows) >= d (#cols) required");
   const Int n_d(Integer::binom(n,d));

   Vector<TropicalNumber<Addition> > pi(n_d);
   const sequence all_rows = sequence(0,n);

   Int i = 0;
   for (auto sigma = entire(all_subsets_of_k(all_rows, d)); !sigma.at_end(); ++sigma) {
      pi[i] = tdet(Matrix<TropicalNumber<Addition>>(M.minor(*sigma, All)));
      ++i;
   }
   return pi;
}
      
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
                          "# Compute the tropical Plücker vector of the matrix //M// by taking all maximal tropical minors."
                          "# See also [[lifted_pluecker]] for a variation"
                          "# and [[common::pluecker]] for ordinary Plücker vectors."
                          "# @param Matrix<TropicalNumber<Addition> > M"
                          "# @return Vector<TropicalNumber<Addition> >"
                          "# @example with parameters (2,4)"
                          "# > $M = new Matrix<TropicalNumber<Min>>([[0,'inf'],['inf',0],[0,0],[0,1]]);"
                          "# > print tpluecker($M);"
                          "# | 0 0 1 0 0 0"
                          ,
                          "tpluecker<Addition>(Matrix<TropicalNumber<Addition> >)");

UserFunctionTemplate4perl("# @category Other"
                          "# Compute a tropical Pluecker vector from the matrix //V// whose rows represent points"
                          "# in the tropical projective space.  This is the same as [[tpluecker]] with a dxd tropical"
                          "# identity matrix prepended.  This can be used to lift regular subdivisions"
                          "# of a product of simplices to a matroid decomposition of hypersimplices."
                          "# Also known as the //tropical Stiefel map//."
                          "# @param Matrix<TropicalNumber<Addition> > V"
                          "# @return Vector<TropicalNumber<Addition> >"
                          "# @example with parameters (2,4)"
                          "# > $V = new Matrix<TropicalNumber<Min>>([[0,0],[0,1]]);"
                          "# > print lifted_pluecker($V);"
                          "# | 0 0 1 0 0 0"
                          ,
                          "lifted_pluecker<Addition>(Matrix<TropicalNumber<Addition> >)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
