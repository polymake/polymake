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
#include "polymake/PowerSet.h"
#include "polymake/IndexedSubgraph.h"
#include "polymake/graph/all_spanningtrees.h"
#include "polymake/matroid/util.h"

namespace polymake { namespace matroid {

BigObject matroid_from_graph(BigObject g)
{
   const Graph<>& graph = g.give("ADJACENCY");
   const Int n_elements = g.give("N_EDGES");
   const Int n_nodes = g.give("N_NODES");
   const Int n_components = g.give("N_CONNECTED_COMPONENTS");
   const Int r = n_nodes-n_components;
   Set<Int> empty_set;
   Array<Set<Int>> bases(1, empty_set);
   
   Array<Int> start_nodes(n_elements); //the starting nodes of the edges
   Array<Int> end_nodes(n_elements); //the ending nodes of the edges
   Array<std::string> labels(n_elements);
   Int l = 0;
   for (auto i = entire(edges(graph)); !i.at_end(); ++i) {
      start_nodes[l]=i.from_node();
      end_nodes[l]=i.to_node();
      std::ostringstream label;
      label<<"{"<<i.from_node()<<" "<<i.to_node()<<"}";
      labels[l++]=label.str();
   }
   
   //compute for each relevant component of the graph the bases via all_spanningtrees() and combine them
   const IncidenceMatrix<> components=g.give("CONNECTED_COMPONENTS");
   Int n_edges = 0;
   for (auto comp_it=entire(rows(components)); !comp_it.at_end(); ++comp_it) {
      if (comp_it->size() > 1) {
         Graph<> indg = renumber_nodes(induced_subgraph(graph,*comp_it));
         bases = product(bases, shift_elements(graph::all_spanningtrees(indg).first, n_edges), operations::add());
         n_edges += indg.edges();
      }
   }

   BigObject m("Matroid",
               "BASES", bases,
               "N_BASES", bases.size(),
               "RANK", r,
               "N_ELEMENTS", n_elements,
               "LABELS", labels);
   m.set_description()<<"Matroid of graph "<<g.name()<<endl;
   return m;
}

UserFunction4perl("# @category Producing a matroid from other objects"
                  "# Creates a graphical matroid from a graph //g//."
                  "# @param  graph::Graph g"
                  "# @return Matroid",
                  &matroid_from_graph, "matroid_from_graph(graph::Graph)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
