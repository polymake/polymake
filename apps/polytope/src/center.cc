/* Copyright (c) 1997-2018
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
#include "polymake/SparseMatrix.h"
#include "polymake/polytope/transform.h"

namespace polymake { namespace polytope {

template <typename Scalar>
perl::Object center(perl::Object p_in)
{
   // read some point in the relative interior
   const Vector<Scalar> point=p_in.give("REL_INT_POINT");
   const int d = point.dim();

   // check if point is affine
   if (is_zero(point[0]))
      throw std::runtime_error("relative interior point not affine");

   SparseMatrix<Scalar> tau=unit_matrix<Scalar>(d);
   tau[0].slice(1)=-point.slice(1);

   perl::Object p_out=transform<Scalar>(p_in, tau);
   p_out.set_description() << "Centered polytope transformed from " << p_in.name() << endl;

   p_out.take("CENTERED") << true;
   return p_out;
}

UserFunctionTemplate4perl("# @category Transformations"
                          "# Make a polyhedron centered."
                          "# Apply a linear transformation to a polyhedron //P// such that a relatively interior point"
                          "# (preferably the vertex barycenter) is moved to the origin (1,0,...,0)."
                          "# @param Polytope P"
                          "# @return Polytope"
                          "# @example Consider this triangle not containing the origin:"
                          "# > $P = new Polytope(VERTICES => [[1,1,1],[1,2,1],[1,1,2]]);"
                          "# > $origin = new Vector([1,0,0]);"
                          "# > print $P->contains_in_interior($origin);"
                          "# | "
                          "# To create a translate that contains the origin, do this:"
                          "# > $PC = center($P);"
                          "# > print $PC->contains_in_interior($origin);"
                          "# | 1"
                          "# This is what happened to the vertices:"
                          "# > print $PC->VERTICES;"
                          "# | 1 -1/3 -1/3"
                          "# | 1 2/3 -1/3"
                          "# | 1 -1/3 2/3"
                          "# There also exists a property to check whether a polytope is centered:"
                          "# > print $PC->CENTERED;"
                          "# | 1",
                          "center<Scalar> (Polytope<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
