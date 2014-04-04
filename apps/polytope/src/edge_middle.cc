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
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Graph.h"

namespace polymake { namespace polytope {

perl::Object edge_middle(perl::Object p_in)
{
   const bool bounded=p_in.give("BOUNDED");
   if (!bounded)
      throw std::runtime_error("edge_middle: unbounded polyhedron");

   perl::Object p_out("Polytope<Rational>");
   p_out.set_description() << "Convex hull of all edge middle points of " << p_in.name() << endl;

   const Matrix<Rational> V=p_in.give("VERTICES");
   const Graph<> G=p_in.give("GRAPH.ADJACENCY");

   Matrix<Rational> V_out(G.edges(), V.cols());
   Rows< Matrix<Rational> >::iterator v_out=rows(V_out).begin();
   for (Entire< Edges< Graph<> > >::const_iterator e=entire(edges(G));  !e.at_end();  ++e, ++v_out)
      *v_out = (V[e.from_node()] + V[e.to_node()]) / 2;
            
   p_out.take("VERTICES") << V_out;
   p_out.take("BOUNDED") << true;
   return p_out;
}

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Produce the convex hull of all edge middle points of some polytope //P//."
                  "# The polytope must be [[BOUNDED]]."
                  "# @param Polytope P"
                  "# @return Polytope",
                  &edge_middle, "edge_middle(Polytope)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
