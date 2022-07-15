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
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/Graph.h"
#include "polymake/PowerSet.h"
#include "polymake/graph/connected.h"

namespace polymake { namespace polytope {

typedef Graph<Undirected> graph_type;

namespace {

Vector<Int> cut_vector(const graph_type& G, const Set<Int>& cut)
{
   Vector<Int> cv(G.edges()+1);
   cv[0] = 1; // homogenizing coordinate
   Int i = 1;
   for (auto e = entire(edges(G));  !e.at_end();  ++e) {
      const Int u = e.from_node(), v = e.to_node();
      if ((cut.contains(u) && !cut.contains(v)) || (cut.contains(v) && !cut.contains(u)))
         cv[i] = 1;
      ++i;
   }
   return cv;
}

}

BigObject fractional_cut_polytope(const graph_type& G)
{
   if (!graph::is_connected(G) )
      throw std::runtime_error("cut_polytope: input graph must be connected");

   const Int n_nodes = G.nodes();
   const Int n_edges = G.edges();
   const Int n_cuts = 1L << (n_nodes-1); // one cut/vertex per split of [0..n_nodes-1], including empty set; see Schrijver, Combinatorial Optimization, §75.7.
   Matrix<Int> V(n_cuts, n_edges+1);

   auto r = rows(V).begin();
   bool n_nodes_is_even = n_nodes%2 == 0;
   const Int max_k = n_nodes_is_even ? n_nodes/2-1 : n_nodes/2;

   for (Int k = 0; k <= max_k; ++k) {
      // each cut considered here has a complement which contains more than half of the nodes
      for (auto ei = entire(all_subsets_of_k(sequence(0,n_nodes),k)); !ei.at_end(); ++ei) {
         *r=cut_vector(G,*ei);
         ++r;
      }
   }
   if (n_nodes_is_even) {
      // special treatment for cuts whose size is half of the total size
      PowerSet<Int> half_size_cuts(entire(all_subsets_of_k(sequence(0, n_nodes), n_nodes/2)));
      while (!half_size_cuts.empty()) {
         Set<Int> this_cut = half_size_cuts.front();
         half_size_cuts -= sequence(0, n_nodes)-this_cut;
         half_size_cuts -= this_cut;
         *r = cut_vector(G, this_cut);
         ++r;
      }
   }

   return BigObject("Polytope<Rational>",
                    "VERTICES", V,
                    "N_VERTICES", n_cuts,
                    "CONE_AMBIENT_DIM", n_edges+1,
                    "BOUNDED", true);
}
      
UserFunction4perl("#@category Producing a polytope from graphs"
                  "# Cut polytope of an undirected graph."
                  "# @param Graph G"
                  "# @return Polytope",
                  &fractional_cut_polytope, "fractional_cut_polytope(GraphAdjacency)");
      
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
