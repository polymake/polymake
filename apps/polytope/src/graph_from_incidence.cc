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
#include "polymake/Graph.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope {

template <typename IMatrix>
Graph<> graph_from_incidence(const GenericIncidenceMatrix<IMatrix>& IM)
{
   const Int n_vertices = IM.cols();
   Graph<> G(n_vertices);
   if (n_vertices < 3) {
      if (n_vertices == 2) G.edge(0, 1);
      return G;
   }

   EdgeMap<Undirected, Set<Int>> intersects(G);

   for (auto n1 = entire(nodes(G)); !n1.at_end(); ++n1) {
      auto n2 = n1;
      while (!(++n2).at_end()) {
         Set<Int> common = IM.col(*n1) * IM.col(*n2);
         if (common.empty()) continue;

         Graph<>::out_edge_list::iterator edge=n1.out_edges().begin();
         bool add=true;
         while (!edge.at_end()) {
            const Int inc = incl(intersects[*edge],common);
            if (inc == 2) {
               ++edge;
            } else {
               if (inc<=0) n1.out_edges().erase(edge++);
               if (inc>=0) { add=false; break; }
            }
         }
         if (add) intersects[n1.edge(*n2)]=common;
      }
   }

   return G;
}

Graph<> dual_graph_from_incidence(const IncidenceMatrix<>& VIF)
{
   return graph_from_incidence(T(VIF));
}

FunctionTemplate4perl("graph_from_incidence(IncidenceMatrix)");
Function4perl(&dual_graph_from_incidence, "dual_graph_from_incidence");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
