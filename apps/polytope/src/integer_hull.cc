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
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"


namespace polymake { namespace polytope {

/*
 *  computes the integer hull of a polyhedron
 */
perl::Object integer_hull(perl::Object p_in)
{
  // compute the lattice points contained in the polyhedron
  // FIXME: use LATTICE_POINTS_GENERATORS instead
  Matrix<Rational> Lattice_Points = p_in.CallPolymakeMethod("LATTICE_POINTS");

  perl::Object p_out(perl::ObjectType::construct<Rational>("Polytope"));
  
  // define a new polyhedron as a convex hull of all the lattice points
  p_out.take("POINTS") << Lattice_Points;
  p_out.take("FEASIBLE") << (Lattice_Points.rows() > 0);
  p_out.take("BOUNDED") << true;
  p_out.take("POINTED") << true;

  return p_out;	
}

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Produces the integer hull of a polyhedron"
                  "# @param Polytope P"
                  "# @return Polytope"
                  "# @example > $p = new Polytope(VERTICES=>[[1,1.3,0.5],[1,0.2,1.2],[1,0.1,-1.5],[1,-1.4,0.2]]);"
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
