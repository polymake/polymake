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
#include "polymake/permutations.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"

namespace polymake { namespace polytope {

BigObject birkhoff(Int n, bool even, OptionSet options)
{
   BigObject p("Polytope<Rational>");
   p.set_name("B" + std::to_string(n));
   if (even)
      p.set_description() << "Even Birkhoff polytope for n=" << n << endl;
   else
      p.set_description() << "Birkhoff polytope for n=" << n << endl;

   Matrix<Int> V(Int(even ? Integer::fac(n)/2 : Integer::fac(n)), n*n+1);
   auto v_i=rows(V).begin();
   // automorphism group of the symmetric group.
   AllPermutations<> perms(n);
   for (AllPermutations<>::const_iterator perm=entire(perms);  !perm.at_end();  ++perm, ++v_i) {
      *v_i = 1 | concat_rows( permutation_matrix<Int>(*perm) );

      // only every second permutation is even
      if (even)
         ++perm;
   }
   p.take("VERTICES") << V;

   // generate the combinatorial symmetry group on the coordinates
   const bool group = options["group"];
   if (group) {
      Array<Array<Int>> gens(2);
      Array<Int> gen{sequence(0,n*n)};
      //swap rows 0 and 1
      for (Int i=0; i<n; ++i) {
         gen[i] = i+n;
         gen[i+n] = i;
      }
      gens[0]=gen;

      // front-shift all rows by one (cyclic)
      for (Int j = 0; j < n; ++j) {
         gen[j] = n*n-n+j;
      }
      for (Int j = n; j < n*n; ++j) {
         gen[j] = j-n;
      }
      gens[1]=gen;

      BigObject a("group::PermutationAction", "GENERATORS", gens);
      BigObject g("group::Group", "combinatorialGroupOnCoords");
      g.set_description() << "action of the symmetric group as combinatorial symmetry group on coordinates of " << n
                          << (n==1 ? "st"
                              : (n==2 ? "nd"
                                 : (n==3 ? "rd"
                                    : "th" ))) << " Birkhoff polytope" << endl;
      p.take("GROUP") << g;
      p.take("GROUP.COORDINATE_ACTION") << a;
   }
   return p;
}


UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Constructs the Birkhoff polytope of dimension //n//<sup>2</sup>. It is the polytope of"
                  "# //n//x//n// stochastic matrices (encoded as //n//<sup>2</sup> row vectors), thus matrices"
                  "# with non-negative entries whose row and column entries sum up to one."
                  "# Its vertices are the permutation matrices."
                  "# @param Int n"
                  "# @param Bool even Defaults to '0'. Set this to '1' to get vertices only for even permutation matrices."
                  "# @option Bool group add the symmetry group induced by the symmetric group to the resulting polytope"
                  "# @return Polytope",
                  &birkhoff, "birkhoff($;$=0,{group=>undef})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
