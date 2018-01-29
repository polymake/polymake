/* Copyright (c) 1997-2018
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

namespace polymake { namespace polytope {

namespace {

Array<Array<int>> cube_facet_gens(int d)
{
   Array<Array<int>> gens(d);
   if ( d==1 ) {
      gens[0]=Array<int>{1, 0};
   } else {
      Array<int> gen{sequence(0, 2*d)};
      gen[0]=1;
      gen[1]=0;

      gens[0]=gen;

      // restore gen (=> gen=[0..2d-1])
      gen[0]=0;
      gen[1]=1;

      for ( int i=1; i<d; ++i ) { 
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

void add_group(perl::Object& p,
               int d,
               const std::string& primal_action,
               const std::string& dual_action,
               bool character_table_flag)
{
   perl::Object g("group::Group");
   if ( character_table_flag && d>=1 && d<=6 ) {
      // generate the primal combinatorial symmetry group (for the cube: on the vertices; for the cross-polytope: on the facets),
      // along with the character table and conjugacy classes.
      // In this case, don't explicitly generate the dual action, because the conjugacy classes will be missing,
      // and it's easier to get those from an explicit conversion VERTICES_ACTION -> FACETS_ACTION (cube) or FACETS_ACTION => VERTICES_ACTION (cross)
      perl::Object cg = call_function("group::cube_group", d);
      g.take("CHARACTER_TABLE") << cg.give("CHARACTER_TABLE");
      g.set_description() << "full combinatorial group" << endl;
      g.set_name("fullCombinatorialGroup");
      p.take("GROUP") << g;
      p.take("GROUP." + primal_action) << cg.give("PERMUTATION_ACTION");
   } else {
      // generate the combinatorial symmetry group on the facets, without character table information
      perl::Object a("group::PermutationAction");
      a.take("GENERATORS") << cube_facet_gens(d);
      g.set_description() << "full combinatorial group" << endl;
      g.set_name("fullCombinatorialGroup");
      p.take("GROUP") << g;
      p.take("GROUP." + dual_action) << a;
   }
   
}
   
} // end anonymous namespace

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
