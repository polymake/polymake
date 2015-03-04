/* Copyright (c) 1997-2015
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
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Graph.h"

namespace polymake { namespace tropical {

void extract_pseudovertices(perl::Object t, perl::Object p)
{
   const Matrix<Rational> V=p.give("VERTICES"); // col(1) expected to be zero due to construction in trop2poly
   const Set<int> far_face=p.give("FAR_FACE");
   const Graph<> G=p.give("BOUNDED_COMPLEX.GRAPH.ADJACENCY");
   const int trop_adim=t.give("AMBIENT_DIM");

   const Matrix<Rational> VV= V.minor(sequence(0,V.rows())-far_face,sequence(1,trop_adim+1)); // we use the 0 in col(1)

   t.take("PSEUDOVERTICES") << VV;
   t.take("PSEUDOVERTEX_GRAPH.ADJACENCY") << renumber_nodes(G);
}

UserFunction4perl("# @category Other"
                  "# Get the pseudovertices of a tropical polytope //T// from the bounded subcomplex of the corresponding unbounded polyhedron //P//.\n"
                  "# @param TropicalPolytope T"
                  "# @param polytope::Polytope P",
                  &extract_pseudovertices, "extract_pseudovertices(TropicalPolytope<Rational> polytope::Polytope<Rational>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
