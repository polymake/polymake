/* Copyright (c) 1997-2022
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
#include "polymake/Matrix.h"
#include "polymake/PowerSet.h"
#include "polymake/TropicalNumber.h"

namespace polymake { namespace tropical {

template<typename Addition>
BigObject hypersimplex(Int d, Int k)
{
   if (d < 1)
      throw std::runtime_error("hypersimplex: dimension >= 2 required");
   if (k < 1 || k > d)
      throw std::runtime_error("hypersimplex: 1 <= k <= d required");

   // number of vertices
   const Int n(Integer::binom(d+1, k));

   Matrix<TropicalNumber<Addition>> Vertices(n, d+1, same_value(TropicalNumber<Addition>::one()).begin());
   auto v = rows(Vertices).begin();
   for (auto s=entire(all_subsets_of_k(range(0,d), k));  !s.at_end();  ++s, ++v) {
      v->slice(*s).fill(TropicalNumber<Addition>(-Addition::orientation()));
   }

   BigObject p("Polytope", mlist<Addition>(), "POINTS", Vertices);
   p.set_description() << "tropical (" << k << "," << d << ")-hypersimplex" << endl;
   return p;
}

UserFunctionTemplate4perl("# @category Producing a tropical polytope"
                          "# Produce the tropical hypersimplex Δ(//k//,//d//)."
                          "# Cf." 
                          "# \t M. Joswig math/0312068v3, Ex. 2.10."
                          "# The value of //k// defaults to 1, yielding a tropical standard simplex."
                          "# @param Int d the dimension"
                          "# @param Int k the number of +/-1 entries"
                          "# @tparam Addition Max or Min"
                          "# @return Polytope<Addition>"
                          "# @example"
                          "# > $h = hypersimplex<Min>(2,1);"
                          "# > print $h->VERTICES;"
                          "# | 0 1 1"
                          "# | 0 -1 0"
                          "# | 0 0 -1",
                          "hypersimplex<Addition>($;$=1)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
