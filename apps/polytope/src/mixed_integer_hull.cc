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
#include "polymake/list"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/linalg.h"


namespace polymake { namespace polytope {


/*
 *  computes the mixed integer hull of a polyhedron
 */
BigObject mixed_integer_hull(BigObject p_in, const Array<Int>& int_coords)
{
  // declaration of variables
  const Matrix<Rational> facets = p_in.give("FACETS");
  const Int d = facets.cols();
  if (d == 0)
    throw std::runtime_error("mixed_integer_hull: non-empty facet matrix required");
  if (d==1 || int_coords.empty())
    return p_in;

  // project to the integral coordinates and  take the convex hull
  BigObject p_project = call_function("projection", p_in, int_coords);
  Matrix<Rational> proj_lattice_points = p_project.call_method("LATTICE_POINTS");

  ListMatrix<Vector<Rational>> out_points, temp_points;
  Matrix<Rational> temp_ineq(unit_matrix<Rational>(d).minor(scalar2set(0), All));

  // for every lattice point in the projected polyhedron compute the intersection
  // between P and a proper affine linear space which goes through the lattice point
  for (Int i = 0; i < proj_lattice_points.rows(); ++i)
  {
    // computing the equation-set of the affine space
    Vector<Rational> right_side;
    right_side = proj_lattice_points.row(i).slice(range_from(1));
    Matrix<Rational> temp_eq(-right_side | unit_matrix<Rational>(d).minor(int_coords, range_from(1)));

    BigObject p_fiber("Polytope<Rational>", "INEQUALITIES", temp_ineq, "EQUATIONS", temp_eq);

    // intersecting it with P
    BigObject p_intersection = call_function("intersection", p_in, p_fiber);

    // remembering the vertices
    p_intersection.give("VERTICES") >> temp_points;
    out_points /= temp_points;
  }

  // convex hull of all vertices computed before
  return BigObject("Polytope<Rational>",
                   "POINTS", out_points);	
}


UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Produces the mixed integer hull of a polyhedron"
                          "# @param Polytope P"
                          "# @param Array<Int> int_coords the coordinates to be integral;"
                          "# @return Polytope",
                          "mixed_integer_hull(Polytope, $)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
