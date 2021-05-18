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

namespace polymake { namespace polytope {

template <typename MatrixTop>
EdgeMap<Undirected, Vector<typename MatrixTop::element_type> >
edge_directions(BigObject g, const GenericMatrix<MatrixTop>& vertices)
{
   const Graph<> G = g.give("ADJACENCY");
   EdgeMap<Undirected, Vector<typename MatrixTop::element_type>> directions(G);

   for (auto e = entire(edges(G));  !e.at_end();  ++e) {
      const Int n_from = e.from_node(), n_to=e.to_node();
      directions[*e]=vertices[n_to]-vertices[n_from];
   }
   return directions;
}

template <typename MatrixTop>
EdgeMap<Undirected, Vector<typename MatrixTop::element_type> >
edge_directions(BigObject g, const GenericMatrix<MatrixTop>& vertices, const Set<Int>& rays)
{
   const Graph<> G = g.give("ADJACENCY");
   EdgeMap<Undirected, Vector<typename MatrixTop::element_type> > directions(G);

   for (auto e = entire(edges(G));  !e.at_end();  ++e) {
      const Int n_from = e.from_node(), n_to = e.to_node();
      if (rays.contains(n_from)) {
         if (rays.contains(n_to))
            directions[*e]=zero_vector<typename MatrixTop::element_type>(vertices.cols());
         else
            directions[*e]=vertices[n_from];
      } else {
         if (rays.contains(n_to))
            directions[*e]=vertices[n_to];
         else
            directions[*e]=vertices[n_to]-vertices[n_from];
      }
   }
   return directions;
}

FunctionTemplate4perl("edge_directions(Graph Matrix Set)");
FunctionTemplate4perl("edge_directions(Graph Matrix)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
