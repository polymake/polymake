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
#include "polymake/Graph.h"

namespace polymake { namespace topaz {

template <typename Complex>
perl::Object mixed_graph(const Complex& C, const Graph<>& PG, const Graph<>& DG, const double weight)
{
   perl::Object MG("EdgeWeightedGraph");
   Graph<Undirected> G(PG.nodes() + DG.nodes());
   EdgeMap<Undirected, double> WT(G);

   // add primal edges
   for (Entire< Edges< Graph<> > >::const_iterator e=entire(edges(PG));
        !e.at_end(); ++e)
      WT(e.from_node(), e.to_node())=1.0;

   // add dual edges
   const int diff = PG.nodes();
   for (Entire< Edges< Graph<> > >::const_iterator e=entire(edges(DG));
        !e.at_end(); ++e)
      WT(e.from_node()+diff, e.to_node()+diff)=1.0;

   // add mixed edges
   typedef typename Complex::value_type Facet;
   int c=diff;

   for (typename Entire<Complex>::const_iterator c_it=entire(C);
        !c_it.at_end(); ++c_it, ++c) {
      Facet f=*c_it; 
      for (typename Entire<Facet>::iterator v=entire(f);
           !v.at_end(); ++v)
         WT(c,*v)=weight;
   }

   MG.take("ADJACENCY") << G;
   MG.take("EDGE_WEIGHTS") << WT;
   return MG;
}




void mixed_graph_complex(perl::Object p, perl::OptionSet options)
{
   const Array< Set<int> > C = p.give("FACETS");
   const Graph<> PG = p.give("GRAPH.ADJACENCY");
   const Graph<> DG = p.give("DUAL_GRAPH.ADJACENCY");
   const int dim = p.give("DIM");
   
   // default for weight = sqrt((dim+1)/(12*Pi))
   // approximation for the radius of the embedding-sphere of a springembedded d-simplex
   double weight;
   if((options["edge_weight"]>>weight)) weight*= sqrt(dim+1) / 6.14;
   else weight= sqrt(dim+1) / 6.14;
   
   //   Graph<nothing,double> MG=t_mixed_graph(C,PG,DG,weight);
   //   cerr << "MIXED_GRAPH\n" << MG << endl;
   p.take("MIXED_GRAPH",perl::temporary)<< mixed_graph(C,PG,DG,weight);
}

UserFunction4perl("# Produces the mixed graph of a simplicial @a complex.\n"
                  "#args: complex [ edge_weight => VALUE ]",
                  &mixed_graph_complex,"mixed_graph(SimplicialComplex { edge_weight=>undef })");
   
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
