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
#include "polymake/Matrix.h"
#include "polymake/AccurateFloat.h"
#include "polymake/group/named_groups.h"
#include <math.h>

namespace polymake { namespace polytope {

BigObject cyclic_caratheodory(const Int d, const Int n, OptionSet options)
{
   if ((d < 2) || (d >= n)) {
      throw std::runtime_error("cyclic_caratheodory: (d >= 2) && (n > d)\n");
   }
   if (d%2) {
      throw std::runtime_error("cyclic_caratheodory: even dimension required.\n");
   }

   const bool group = options["group"];
   BigObject p(std::string(group ? "Polytope<Float>" : "Polytope<Rational>"));
   p.set_description() << "Cyclic " << d << "-polytope on " << n << " vertices on the trigonometric moment curve" << endl;

   Matrix<Rational> Vertices(n,d+1);
   auto v=concat_rows(Vertices).begin();

   AccurateFloat x(0), s, c;
   for (Int r = 0; r < n; ++r, ++x) {
      *v++ = 1;
      for (Int i = 1; i <= d/2; ++i) {
         sin_cos(s, c, x*i*2*M_PI/n);
         *v++ = c;
         *v++ = s;
      }
   }

   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("CONE_DIM") << d+1;
   p.take("N_VERTICES") << n;
   p.take("VERTICES") << Vertices;
   p.take("BOUNDED") << true;

   if (group) {
      BigObject g("group::Group");
      const BigObject D(group::dihedral_group_impl(2*n));
      g.take("CHARACTER_TABLE") << D.give("CHARACTER_TABLE");
      g.set_description() << "full combinatorial group" << endl;
      g.set_name("fullCombinatorialGroup");
      p.take("GROUP") << g;
      p.take("GROUP.VERTICES_ACTION") << D.give("PERMUTATION_ACTION");
   }
   
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a //d//-dimensional cyclic polytope with //n// points."
                  "# Prototypical example of a neighborly polytope.  Combinatorics completely known"
                  "# due to Gale's evenness criterion.  Coordinates are chosen on the trigonometric"
                  "# moment curve. For cyclic polytopes from other curves, see [[polytope::cyclic]]."
                  "# @param Int d the dimension. Required to be even."
                  "# @param Int n the number of points"
                  "# @option Bool group add a symmetry group description. If 0 (default), the return type is Polytope<Rational>, else Polytope<Float>"
                  "# so that the matrices corresponding to the symmetry action may be approximated"
                  "# @return Polytope",
                  &cyclic_caratheodory,"cyclic_caratheodory($$ { group=>0 })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
