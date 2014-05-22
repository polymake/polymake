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
#include "polymake/topaz/complex_tools.h"

namespace polymake { namespace topaz {

// Computes the Graph of a simplicial complex.
template <typename Complex> inline
Graph<> vertex_graph(const Complex& C)
{
   const PowerSet<int> OSK=k_skeleton(C,1);
   const Set<int> V = accumulate(OSK, operations::add());

   Graph<> G(V.size());
   for (Entire< PowerSet<int> >::const_iterator c_it=entire(OSK);
        !c_it.at_end(); ++c_it)
      if (c_it->size() == 2)  // is edge
         G.edge(c_it->front(), c_it->back());

   return G;
}

template <typename Complex>
perl::Object mixed_graph(const Complex& C, const Graph<>& PG, const Graph<>& DG, const double weight)
{
   perl::Object MG("EdgeWeightedGraph");
   Graph<Undirected> G(PG.nodes() + DG.nodes());
   EdgeMap<Undirected, double> WT(G);

   // add primal edges
   for (Entire< Edges< Graph<> > >::const_iterator e=entire(edges(PG));
        !e.at_end(); ++e)
      WT[G.edge(e.from_node(), e.to_node())]=1.0;

   // add dual edges
   const int diff = PG.nodes();
   for (Entire< Edges< Graph<> > >::const_iterator e=entire(edges(DG));
        !e.at_end(); ++e)
      WT[G.edge(e.from_node()+diff, e.to_node()+diff)]=1.0;

   // add mixed edges
   typedef typename Complex::value_type Facet;
   int c=diff;

   for (typename Entire<Complex>::const_iterator c_it=entire(C);
        !c_it.at_end(); ++c_it, ++c) {
      Facet f=*c_it; 
      for (typename Entire<Facet>::iterator v=entire(f);
           !v.at_end(); ++v)
         WT[G.edge(c,*v)]=weight;
   }

   MG.take("ADJACENCY") << G;
   MG.take("EDGE_WEIGHTS") << WT;
   return MG;
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
