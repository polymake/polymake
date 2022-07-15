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
#include "polymake/polytope/matroid_polytopes.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/internal/PlainParser.h"
#include "polymake/SparseMatrix.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace fan {

BigObject hypersimplex_vertex_splits(Int k, Int d, OptionSet options)
{
   if (d < 2)
      throw std::runtime_error("hypersimplex_vertex_splits: dimension >= 2 required");
   if (k <= 0 || k >= d)
      throw std::runtime_error("hypersimplex_vertex_splits: 0 < k < d required");

   BigObject HA("HyperplaneArrangement<Rational>");
   HA.set_description() << "Arrangement of vertex split hyperplanes of (" << k << "," << d << ")-hypersimplex" << endl;

   HA.take("HYPERPLANE_AMBIENT_DIM") << d;

   // we already know the number of vertices
   const Int n(Integer::binom(d,k));
   HA.take("N_HYPERPLANES") << n;

   // const bool group_flag = options["group"];
   
   std::ostringstream label;
   Array<std::string> labels(n);
   auto label_it = labels.begin();
   Matrix<Rational> Hyperplanes(ones_matrix<Rational>(n,d));
   auto v = rows(Hyperplanes).begin();
   for (auto s=entire(all_subsets_of_k(range(0,d-1),k)); !s.at_end(); ++s, ++v, ++label_it) {
      v->slice(range_from(0)).slice(*s).fill(-1);
      label.str("");
      wrap(label) << *s;
      *label_it = label.str();
   }
   HA.take("HYPERPLANE_LABELS") << labels;
   HA.take("HYPERPLANES") << Hyperplanes;

   /*
   // generate the combinatorial symmetry group on the vertices
   if ( group_flag ) {
      Array<Array<int>> gens(2);
      Array<int> gen{sequence(0,d)};
      gen[0]=1;
      gen[1]=0;
      gens[0]=gen;

      gen[0]=d-1;
      for (int j=1; j<=d-1; ++j) {
         gen[j]=j-1;
      }
      gens[1]=gen;

      perl::Object a("group::PermutationAction");
      a.take("GENERATORS") << gens;

      perl::Object g("group::Group");
      g.set_description() << "full combinatorial group on coordinates of " <<  "(" << k << "," << d << ")-hypersimplex" << endl;
      g.set_name("fullCombinatorialGroupOnCoords");

      HA.take("GROUP") << g;
      HA.take("GROUP.COORDINATE_ACTION") << a;
   }
   */

   return HA;
}


UserFunction4perl("# @category Producing a hyperplane arrangement"
                  "# Produce the arrangement of vertex splits of the hypersimplex $ Δ(k,d) $"
                  "# @param Int k number of 1s"
                  "# @param Int d ambient dimension"
                  "# @option Bool group"
                  "# @option Bool no_vertices do not compute vertices"
                  "# @option Bool no_facets do not compute facets"
                  "# @option Bool no_vif do not compute vertices in facets"
                  "# @return Polytope"
                  "# @example This corresponds to the hypersimplex in dimension 4 with vertices with exactly two 1-entries"
                  "# and computes its symmetry group:"
                  "# > $H = hypersimplex_vertex_splits(2,4,group=>1);",
                  &hypersimplex_vertex_splits, "hypersimplex_vertex_splits($,$;{group=>undef})");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
