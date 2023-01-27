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
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/TropicalNumber.h"
#include "polymake/internal/type_union.h"

namespace polymake { namespace tropical {

template <typename Addition, typename Scalar>
BigObject matroid_polytope(BigObject m, const Scalar& value)
{
   const Array<Set<Int>> bases = m.give("BASES");
   const Int n_bases = bases.size();
   const Int n_elements = m.give("N_ELEMENTS");
   const TropicalNumber<Addition, Scalar> tvalue(value);

   Matrix<TropicalNumber<Addition, Scalar>> V(n_bases,n_elements);
   V.fill(TropicalNumber<Addition, Scalar>::one());

   for (Int b = 0; b < n_bases; ++b) {
      for (auto i = entire(bases[b]); !i.at_end(); ++i)
         V(b,(*i)) = tvalue;
   }

   return BigObject("Polytope", mlist<Addition, Scalar>(),
                    "POINTS", V);
}


UserFunctionTemplate4perl("# @category Producing a tropical polytope"
                          "# Produce the tropical matroid polytope from a matroid //m//."
                          "# Each vertex corresponds to a basis of the matroid,"
                          "# the non-bases coordinates get value 0, the bases coordinates"
                          "# get value //v//, default is -orientation."
                          "# @param matroid::Matroid m"
                          "# @param Scalar v value for the bases"
                          "# @tparam Addition Min or Max"
                          "# @tparam Scalar coordinate type"
                          "# @return Polytope<Addition,Scalar>"
                          "# @example"
                          "# > $m = new matroid::Matroid(VECTORS=>[[1,0,0],[1,0,1],[1,1,0],[1,0,2]]);"
                          "# > $P = matroid_polytope<Min>($m);"
                          "# > print $P->VERTICES;"
                          "# | 0 0 0 1"
                          "# | 0 1 0 0"
                          "# | 0 -1 -1 -1",
                          "matroid_polytope<Addition,Scalar> [ is_ordered_field_with_unlimited_precision(type_upgrade<Scalar, Rational>) ](matroid::Matroid; type_upgrade<Scalar> = -Addition->orientation())");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
