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
#include "polymake/polytope/transform.h"

namespace polymake { namespace polytope {

template <typename Scalar>
BigObject scale(BigObject p_in, const Scalar& factor,
                bool store_reverse_transformation)
{
   Int amb_dim = p_in.call_method("AMBIENT_DIM");
   Matrix<Scalar> T(diag(1,(factor*unit_matrix<Scalar>(amb_dim))));
   return transform<Scalar>(p_in, T, store_reverse_transformation);
}


template <typename Scalar>
BigObject translate(BigObject p_in, const Vector<Scalar>& trans,
                    bool store_reverse_transformation)
{
   Int amb_dim = p_in.call_method("AMBIENT_DIM");
   Matrix<Scalar> T((unit_vector<Scalar>(amb_dim+1, 0) | (trans/unit_matrix<Scalar>(amb_dim))));
   return transform<Scalar>(p_in, T, store_reverse_transformation);
}

UserFunctionTemplate4perl("# @category Transformations"
                          "# Transform a polyhedron //P// according to the linear"
                          "# transformation //trans//."
                          "# @param Polytope P the polyhedron to be transformed"
                          "# @param Matrix trans the transformation matrix"
                          "# @param Bool store stores the reverse transformation as an attachment (REVERSE_TRANSFORMATION);"
                          "#   default value: 1."
                          "# @return Polytope"
                          "# @example This translates the square by (23,23) and stores the transformation:"
                          "# > $M = new Matrix([1,23,23],[0,1,0],[0,0,1]);"
                          "# > $p = transform(cube(2),$M,1);"
                          "# > print $p->VERTICES;"
                          "# | 1 22 22"
                          "# | 1 24 22"
                          "# | 1 22 24"
                          "# | 1 24 24"
                          "# To retrieve the attached transformation, use this:"
                          "# > print $p->get_attachment('REVERSE_TRANSFORMATION');"
                          "# | 1 -23 -23"
                          "# | 0 1 0"
                          "# | 0 0 1"
                          "# Check out the __revert__ function to learn how to undo the transformation."
                          "# It might be more comfortable to use the __translate__ function to achieve the same result.",
                          "transform<Scalar>(Polytope<type_upgrade<Scalar>> Matrix<type_upgrade<Scalar>>; $=1)");

UserFunctionTemplate4perl("# @category Transformations"
                          "# Scale a polyhedron //P// by a given scaling parameter //factor//."
                          "# @param Polytope P the polyhedron to be scaled"
                          "# @param Scalar factor the scaling factor"
                          "# @param Bool store stores the reverse transformation as an attachment (REVERSE_TRANSFORMATION);"
                          "#   default value: 1."
                          "# @return Polytope"
                          "# @example To scale the square by 23, do this:"
                          "# > $p = scale(cube(2),23);"
                          "# > print $p->VERTICES;"
                          "# | 1 -23 -23"
                          "# | 1 23 -23"
                          "# | 1 -23 23"
                          "# | 1 23 23"
                          "# The transformation matrix is stored in an attachment:"
                          "# > print $p->get_attachment('REVERSE_TRANSFORMATION');"
                          "# | 1 0 0"
                          "# | 0 1/23 0"
                          "# | 0 0 1/23"
                          "# To reverse the transformation, you can use the __revert__ function."
                          "# > $q = revert($p);"
                          "# > print $q->VERTICES;"
                          "# | 1 -1 -1"
                          "# | 1 1 -1"
                          "# | 1 -1 1"
                          "# | 1 1 1",
                          "scale<Scalar>(Polytope<type_upgrade<Scalar>> type_upgrade<Scalar>; $=1)");

UserFunctionTemplate4perl("# @category Transformations"
                          "# Translate a polyhedron //P// by a given translation vector //trans//."
                          "# @param Polytope P the polyhedron to be translated"
                          "# @param Vector trans the translation vector"
                          "# @param Bool store stores the reverse transformation as an attachment (REVERSE_TRANSFORMATION);"
                          "#   default value: 1."
                          "# @return Polytope"
                          "# @example This translates the square by (23,23) and stores the transformation:"
                          "# > $t = new Vector(23,23);"
                          "# > $p = translate(cube(2),$t);"
                          "# > print $p->VERTICES;"
                          "# | 1 22 22"
                          "# | 1 24 22"
                          "# | 1 22 24"
                          "# | 1 24 24"
                          "# To retrieve the attached transformation, use this:"
                          "# > print $p->get_attachment('REVERSE_TRANSFORMATION');"
                          "# | 1 -23 -23"
                          "# | 0 1 0"
                          "# | 0 0 1"
                          "# Check out the __revert__ function to learn how to undo the transformation.",
                          "translate<Scalar>(Polytope<type_upgrade<Scalar>> Vector<type_upgrade<Scalar>>; $=1)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
