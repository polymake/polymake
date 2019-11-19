/* Copyright (c) 1997-2019
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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Graph.h"

namespace polymake { namespace polytope {

template <typename Scalar>
perl::Object edge_middle(perl::Object p_in)
{
   const bool bounded=p_in.give("BOUNDED");
   if (!bounded)
      throw std::runtime_error("edge_middle: unbounded polyhedron");

   perl::Object p_out("Polytope", mlist<Scalar>());
   p_out.set_description() << "Convex hull of all edge middle points of " << p_in.name() << endl;

   const Matrix<Scalar> V=p_in.give("VERTICES");
   const Graph<> G=p_in.give("GRAPH.ADJACENCY");

   Matrix<Scalar> V_out(G.edges(), V.cols());
   typename Rows< Matrix<Scalar> >::iterator v_out=rows(V_out).begin();
   for (auto e=entire(edges(G));  !e.at_end();  ++e, ++v_out)
      *v_out = (V[e.from_node()] + V[e.to_node()]) / 2;
            
   p_out.take("VERTICES") << V_out;
   p_out.take("BOUNDED") << true;
   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Produce the convex hull of all edge middle points of some polytope //P//."
                          "# The polytope must be [[BOUNDED]]."
                          "# @param Polytope P"
                          "# @return Polytope",
                          "edge_middle<Scalar>(Polytope<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
