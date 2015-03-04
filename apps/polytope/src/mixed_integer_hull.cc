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
#include "polymake/list"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Series.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/linalg.h"


namespace polymake { namespace polytope {


/*
 *  computes the mixed integer hull of a polyhedron
 */
perl::Object mixed_integer_hull(perl::Object p_in, const Array<int>& int_coords)
{
  // declaration of variables
  const Matrix<Rational> facets = p_in.give("FACETS");
  const int d = facets.cols();
  if (d==0)
    throw std::runtime_error("mixed_integer_hull: non-empty facet matrix required");
  if ( (d==1) || (int_coords.empty()))
    return p_in;
  
  perl::Object p_project;
  perl::Object p_out(perl::ObjectType::construct<Rational>("Polytope"));
  ListMatrix<Vector<Rational> > out_points, temp_points;
  Matrix<Rational> temp_ineq(unit_matrix<Rational>(d).minor(scalar2set(0), All));

  // project to the integral coordinates and  take the convex hull   
  p_project = CallPolymakeFunction("projection", p_in, int_coords);
  Matrix<Rational> proj_lattice_points = p_project.CallPolymakeMethod("LATTICE_POINTS");
  
  // for every lattice point in the projected polyhedron compute the intersection
  // between P and a proper affine linear space which goes through the lattice point
  for (int i=0; i<proj_lattice_points.rows(); ++i)
  {
    // computing the equation-set of the affine space
    Vector<Rational> right_side;
    right_side = proj_lattice_points.row(i).slice(~scalar2set(0));    
    Matrix<Rational> temp_eq(-right_side | unit_matrix<Rational>(d).minor(int_coords, ~scalar2set(0)));

    perl::Object p_fiber(perl::ObjectType::construct<Rational>("Polytope"));
    p_fiber.take("INEQUALITIES") << temp_ineq;
    p_fiber.take("EQUATIONS") << temp_eq;

    // intersecting it with P
    perl::Object p_intersection;
    p_intersection = CallPolymakeFunction("intersection", p_in,p_fiber);

    // remembering the vertices
    p_intersection.give("VERTICES") >> temp_points;
    out_points /= temp_points;
  }  
  
  // convex hull of all vertices computed before
  p_out.take("POINTS") << out_points;

  return p_out;	
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
