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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/polytope/lrs_interface.h"

/** @file graph_from_vertices
 *
 *  Find the vertex graph of a polytope given by its vertices, without calculating the convex hull.
 *  Currently, can handle only bounded polytopes.
 *
 *  @author Thilo Rörig
 */

// FIXME fails for pointed cones
// corresponding rule currently not defined for cones

namespace polymake { namespace polytope {

Graph<> graph_from_vertices(const Matrix<Rational>& Vertices)
{
   const Int n_vertices = Vertices.rows();
   Graph<> graph(n_vertices);

   Matrix<Rational> Ineq(n_vertices,Vertices.cols());
   Matrix<Rational> Eq;
   const lrs_interface::LP_Solver LRS{};

   for (Int i = 0; i < n_vertices-1; i++) {
      for (Int j = i+1; j < n_vertices; j++) {
         for (Int k = 0; k < n_vertices; k++) {
            if (k == i) {
               Ineq[k] = Vertices[j] - Vertices[i];
            } else {
               Ineq[k] = Vertices[i] - Vertices[k];
               Ineq(k,0) = (k!=j) ? -1 : 0;
            }
         }

         if (LRS.check_feasibility(Ineq,Eq)) graph.edge(i,j);
      }
   }
   return graph;
}

Function4perl(&graph_from_vertices, "graph_from_vertices");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
