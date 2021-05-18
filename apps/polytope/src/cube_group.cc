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

#include "polymake/polytope/cube_group.h"

namespace polymake { namespace polytope {

namespace {

Array<Array<Int>> cube_facet_gens(const Int d)
{
   Array<Array<Int>> gens(d);
   if (d == 1) {
      gens[0] = Array<Int>{1, 0};
   } else {
      Array<Int> gen{sequence(0, 2*d)};
      gen[0] = 1;
      gen[1] = 0;

      gens[0] = gen;

      // restore gen (=> gen=[0..2d-1])
      gen[0] = 0;
      gen[1] = 1;

      for (Int i = 1; i < d; ++i) { 
         gen[2*i-2] = 2*i;
         gen[2*i] = 2*i-2;
         gen[2*i-1] = 2*i+1;
         gen[2*i+1] = 2*i-1;
         gens[i]=gen;

         // restore gen (=> gen=[0..2d-1])
         gen[2*i-2] = 2*i-2;
         gen[2*i] = 2*i;
         gen[2*i-1] = 2*i-1;
         gen[2*i+1] = 2*i+1;
      }
   }
   return gens;
}

}

void add_cube_group(BigObject& p, const Int d,
                    const std::string& primal_action,
                    const std::string& dual_action,
                    const bool character_table_flag)
{
   BigObject g("group::Group", "fullCombinatorialGroup");
   g.set_description() << "full combinatorial group" << endl;
   p.take("GROUP") << g;

   if (character_table_flag && d >= 1 && d <= 6) {
      // generate the primal combinatorial symmetry group (for the cube: on the vertices; for the cross-polytope: on the facets),
      // along with the character table and conjugacy classes.
      // In this case, don't explicitly generate the dual action, because the conjugacy classes will be missing,
      // and it's easier to get those from an explicit conversion VERTICES_ACTION -> FACETS_ACTION (cube) or FACETS_ACTION => VERTICES_ACTION (cross)
      BigObject cg = call_function("group::cube_group", d);
      g.take("CHARACTER_TABLE") << cg.give("CHARACTER_TABLE");
      g.take(primal_action) << cg.give("PERMUTATION_ACTION");
   } else {
      // generate the combinatorial symmetry group on the facets, without character table information
      BigObject a("group::PermutationAction",
                  "GENERATORS", cube_facet_gens(d));
      g.take(dual_action) << a;
   }
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
