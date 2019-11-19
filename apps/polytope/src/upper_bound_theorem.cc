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
#include "polymake/Integer.h"
#include "polymake/Array.h"

namespace polymake { namespace polytope {

perl::Object upper_bound_theorem(const int d, const int n)
{
   if ((d < 0) || (d >= n)) {
      throw std::runtime_error("upper_bound_theorem: d >= 0 and n > d required\n");
   }

   perl::Object p("Polytope<Rational>");

   Array<Integer> h(d+1);
   for (int k=0; k<=d/2; ++k) {
      h[d-k] = h[k] = Integer::binom(n-d-1+k,k);
   }
   p.take("COMBINATORIAL_DIM") << d;
   p.take("N_VERTICES") << n;
   p.take("H_VECTOR") << h;
   p.take("SIMPLICIAL") << true;
   return p;
}
   
UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce combinatorial data common to all simplicial d-polytopes with n vertices"
                  "# with the maximal number of facets as given by McMullen's Upper-Bound-Theorem."
                  "# Essentially this lets you read off all possible entries of the [[H_VECTOR]] and the [[F_VECTOR]]."
                  "# @param Int d the dimension"
                  "# @param Int n the number of points"
                  "# @return Polytope"
                  "# @example This produces the combinatorial data as mentioned above for 5 points in dimension 3 and prints the h-vector:" 
                  "# > $p = upper_bound_theorem(3,5);"
                  "# > print $p->H_VECTOR;"
                  "# | 1 2 2 1",
                  &upper_bound_theorem, "upper_bound_theorem($$)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
