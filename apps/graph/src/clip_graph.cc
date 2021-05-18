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

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Graph.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Bitset.h"

namespace polymake { namespace graph {
namespace {

Rational mu_intersect(const Vector<Rational>& u, const Vector<Rational>& v, const Vector<Rational>& i)
{
   // compute the intersection of the line segment [u,v] with the hyperplane defined by i
   // condition: <u,i> < 0 && <v,i> >= 0
   // return value depends on whether or not u is finite
   if (u[0]!=0)
      return -(v*i)/((u-v)*i);
   else
      return -(v*i)/(u*i);
}

Vector<Rational> intersect(const Vector<Rational>& u, const Vector<Rational>& v, const Rational& mu)
{
   // compute the intersection of the line segment [u,v] with the hyperplane defined by i [not provided]
   // condition: <u,i> < 0 && <v,i> >= 0
   if (u[0]!=0)
      return (1-mu)*v+mu*u;
   else
      return v+mu*u;
}
}

BigObject clip_graph(const Graph<>& G, const Matrix<Rational>& V, const Matrix<Rational>& BB)
{
   const Int n = V.rows();
   Graph<> truncG(G);  // copy the graph information in order to change it later
   NodeMap<Undirected, Vector<Rational>> coords(truncG);

   // check which vertices satisfy all the inequalities of the bounding box; note that the rays must be sorted out, too
   Bitset valid(n);
   for (Int i = 0; i < n; ++i) {
      coords[i] = V[i];
      if (V(i, 0) != 0 && find_in_range_if(entire(BB*V[i]), polymake::operations::negative()).at_end())
         valid+=i;
   }

   for (Int i = 0; i < n; ++i)
      if (!valid.contains(i)) {
         for (auto e=G.out_edges(i).begin(); !e.at_end(); ++e) {
            const Int j = e.to_node();
            if (valid.contains(j)) {
               auto ineq_it=entire(rows(BB));
               while (!ineq_it.at_end() && (*ineq_it)*V[i]>=0) ++ineq_it;
               // there must be at least one invalid inequality for V[i]
               Rational mu=mu_intersect(V[i],V[j],*ineq_it);
               for (++ineq_it; !ineq_it.at_end(); ++ineq_it)
                  if ((*ineq_it)*V[i]<0)
                     assign_min(mu, mu_intersect(V[i],V[j],*ineq_it));
               const Int new_node = truncG.add_node();
               coords[new_node] = intersect(V[i],V[j],mu);
               truncG.edge(j,new_node);
            }
         }
         truncG.delete_node(i);
      }

   truncG.squeeze();
   
   return BigObject("GeometricGraph", "ADJACENCY", truncG, "COORDINATES", coords);
}

UserFunction4perl("# @category Visualization"
                  "# Clip a graph with respect to a given bounding box."
                  "# Used for the visualization of Voronoi diagrams."
                  "# @param Graph G"
                  "# @param Matrix V"
                  "# @param Matrix BB"
                  "# @return GeometricGraph",
                  &clip_graph, "clip_graph");
} } 

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
