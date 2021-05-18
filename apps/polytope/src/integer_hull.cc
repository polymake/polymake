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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"


namespace polymake { namespace polytope {

/*
 *  computes the integer hull of a polyhedron
 */
BigObject integer_hull(BigObject p_in)
{
  // compute the lattice points contained in the polyhedron
  // FIXME: use LATTICE_POINTS_GENERATORS instead
  Matrix<Rational> Lattice_Points = p_in.call_method("LATTICE_POINTS");
  const Int dim = p_in.give("CONE_AMBIENT_DIM");

  // define a new polyhedron as a convex hull of all the lattice points
  return BigObject("Polytope<Rational>",
                   "POINTS", Lattice_Points,
                   "FEASIBLE", Lattice_Points.rows() > 0,
                   "BOUNDED", true,
                   "POINTED", true,
                   "CONE_AMBIENT_DIM", dim);	
}

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Produces the integer hull of a polyhedron"
                  "# @param Polytope P"
                  "# @return Polytope"
                  "# @example [prefer cdd]" 
                  "# > $p = new Polytope(VERTICES=>[[1,13/10,1/2],[1,1/5,6/5],[1,1/10,-3/2],[1,-7/5,1/5]]);"
                  "# > $ih = integer_hull($p);"
                  "# > print $ih->VERTICES;"
                  "# | 1 -1 0"
                  "# | 1 0 -1"
                  "# | 1 0 1"
                  "# | 1 1 0",
                  &integer_hull, "integer_hull(Polytope)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
