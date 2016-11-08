/* Copyright (c) 1997-2016
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
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/Graph.h"
#include "polymake/PowerSet.h"
#include "polymake/graph/connected.h"

namespace polymake { namespace polytope {

typedef Graph<Undirected> graph_type;

namespace {

Vector<int> cut_vector(const graph_type& G, const Set<int>& cut)
{
   Vector<int> cv(G.edges()+1);
   cv[0]=1; // homogenizing coordinate
   int i=1;
   for (Entire< Edges< graph_type > >::const_iterator e=entire(edges(G));  !e.at_end();  ++e) {
      const int u(e.from_node()), v(e.to_node());
      if ((cut.contains(u) && !cut.contains(v)) || (cut.contains(v) && !cut.contains(u)))
         cv[i] = 1;
      ++i;
   }
   return cv;
}

}

perl::Object fractional_cut_polytope(const graph_type& G)
{
   if (!graph::is_connected(G) )
      throw std::runtime_error("cut_polytope: input graph must be connected");

   const int n_nodes=G.nodes();
   const int n_edges=G.edges();
   const int n_cuts=(1<<(n_nodes-1)); // one cut/vertex per split of [0..n_nodes-1], including empty set; see Schrijver, Combinatorial Optimization, §75.7.
   Matrix<int> V(n_cuts,n_edges+1);

   Rows< Matrix<int> >::iterator r=rows(V).begin();
   bool n_nodes_is_even=!bool(n_nodes%2);
   const int max_k=n_nodes_is_even? n_nodes/2-1 : n_nodes/2;

   for (int k=0; k<=max_k; ++k) {
      // each cut considered here has a complement which contains more than half of the nodes
      for (Entire< Subsets_of_k<const sequence&> >::const_iterator ei = entire(all_subsets_of_k(sequence(0,n_nodes),k)); 
           !ei.at_end(); ++ei) {
         *r=cut_vector(G,*ei);
         ++r;
      }
   }
   if (n_nodes_is_even) {
      // special treatment for cuts whose size is half of the total size
      PowerSet<int> half_size_cuts(entire(all_subsets_of_k(sequence(0,n_nodes),n_nodes/2)));
      while (!half_size_cuts.empty()) {
         Set<int> this_cut=half_size_cuts.front();
         half_size_cuts-=sequence(0,n_nodes)-this_cut;
         half_size_cuts-=this_cut;
         *r=cut_vector(G,this_cut);
         ++r;
      }
   }

   perl::Object p("Polytope<Rational>");
   p.take("VERTICES") << V;
   p.take("N_VERTICES") << n_cuts;
   p.take("CONE_AMBIENT_DIM") << n_edges+1;
   p.take("BOUNDED") << true;
   return p;
}
      
UserFunction4perl("#@category Producing a polytope from graphs"
                  "# Cut polytope of an undirected graph."
                  "# @param Graph G"
                  "# @return Polytope",
                  &fractional_cut_polytope,"fractional_cut_polytope(props::Graph)");
      
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
