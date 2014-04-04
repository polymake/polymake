/* Copyright (c) 1997-2014
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
#include "polymake/graph/HasseDiagram.h"

namespace polymake { namespace polytope {
namespace {

template <typename Iterator> inline
void fill_graph(Graph<>& G, Iterator vertex_set_i, int first_vertex)
{
   for (; !vertex_set_i.at_end(); ++vertex_set_i)
      G.edge(vertex_set_i->front() - first_vertex,
             vertex_set_i->back() - first_vertex);
}
}

Graph<> vertex_graph_from_face_lattice(perl::Object HD_obj)
{
   const graph::HasseDiagram HD(HD_obj);
   if (HD.dim()<0) return Graph<>(0);
   const graph::HasseDiagram::nodes_of_dim_set vertex_nodes=HD.nodes_of_dim(0);
   Graph<> G(vertex_nodes.size());

   // vertex sets stored by the polytope edge faces (dim==1)
   // are exactly the vertex pairs we need for the graph
   fill_graph(G, entire(select(HD.faces(), HD.nodes_of_dim(1))), 0);
   return G;
}

Graph<> facet_graph_from_face_lattice(perl::Object HD_obj)
{
   const graph::HasseDiagram HD(HD_obj);
   if (HD.dim()<0) return Graph<>(0);
   const graph::HasseDiagram::nodes_of_dim_set facet_nodes=HD.nodes_of_dim(-1);
   Graph<> G(facet_nodes.size());

   // the node numbers of the polytope facets (which are neighbors of the ridge faces, dim==-2)
   // relate to the whole Hasse diagram graph!
   fill_graph(G, entire(select(rows(adjacency_matrix(HD.graph())), HD.nodes_of_dim(-2))), facet_nodes.front());
   return G;
}

Function4perl(&vertex_graph_from_face_lattice, "vertex_graph(FaceLattice)");
Function4perl(&facet_graph_from_face_lattice, "facet_graph(FaceLattice)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
