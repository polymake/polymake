/* Copyright (c) 1997-2014
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

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

template <typename Dir>
Vector<Rational> lifted_pluecker(Matrix<Rational> V)
{
   const int n(V.rows()), d(V.cols());
   const int nd_d=Integer::binom(d+n,d).to_int();
   Vector<Rational> pi(nd_d);

   const sequence all_rows = sequence(0,d+n); // d extra rows plus n given ones
   int i=0;

   // first merging given and extra rows and then taking them apart again
   // yields the desired ordering of the coefficients of the tropical Pluecker vector
   for (Entire< Subsets_of_k<const sequence&> >::const_iterator rho = entire(all_subsets_of_k(all_rows,d));
        !rho.at_end(); ++rho) {
      const Set<int> ri(*rho);
      Set<int> sigma, tau;
      for (Entire< Set<int> >::const_iterator rii=entire(ri); !rii.at_end(); ++rii)
         // take given and extra rows apart
         if (*rii < d)
            tau.push_back(*rii);
         else
            sigma.push_back(*rii-d);
      pi[i]=tdet<Dir>(Matrix<Rational>(V.minor(sigma,~tau)));
      ++i;
   }
   return pi;
}

UserFunctionTemplate4perl("# @category Other"
                  "# Compute tropical Pluecker vector from matrix representing points in tropical torus."
                  "# Can be used to lift regular subdivision of ordinary product of simplices to"
                  "# matroid decomposition of hypersimplices."
                  "# @param Matrix V",
                  "lifted_pluecker<Dir=Min>($)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
