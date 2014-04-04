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
#include "polymake/polytope/transform.h"

namespace polymake { namespace polytope {

template <typename Scalar>
perl::Object scale(perl::Object p_in, const Scalar& factor,
                   bool store_reverse_transformation)
{
   int amb_dim=p_in.CallPolymakeMethod("AMBIENT_DIM");
   Matrix<Scalar> T(diag(1,(factor*unit_matrix<Scalar>(amb_dim))));
   return transform<Scalar>(p_in,T,store_reverse_transformation);
}


template <typename Scalar>
perl::Object translate(perl::Object p_in, const Vector<Scalar>& trans,
                       bool store_reverse_transformation)
{
   int amb_dim=p_in.CallPolymakeMethod("AMBIENT_DIM");
   Matrix<Scalar> T((unit_vector<Scalar>(amb_dim+1,0)|(trans/unit_matrix<Scalar>(amb_dim))));
   return transform<Scalar>(p_in,T,store_reverse_transformation);
}

UserFunctionTemplate4perl("# @category Transformations"
                          "# Transform a polyhedron //P// according to the linear"
                          "# transformation //trans//."
                          "# @param Polytope P the polyhedron to be transformed"
                          "# @param Matrix trans the transformation matrix"
                          "# @param Bool store stores the reverse transformation as an attachment (REVERSE_TRANSFORMATION);"
                          "#   default value: 1."
                          "# @return Polytope",
                          "transform<Scalar>(Polytope<Scalar> *;$=1)");

UserFunctionTemplate4perl("# @category Transformations"
                          "# Scale a polyhedron //P// by a given scaling parameter //factor//."
                          "# @param Polytope P the polyhedron to be scaled"
                          "# @param Scalar factor the scaling factor"
                          "# @param Bool store stores the reverse transformation as an attachment (REVERSE_TRANSFORMATION);"
                          "#   default value: 1."
                          "# @return Polytope",
                          "scale<Scalar>(Polytope<Scalar> $;$=1)");

UserFunctionTemplate4perl("# @category Transformations"
                          "# Translate a polyhedron //P// by a given translation vector //trans//."
                          "# @param Polytope P the polyhedron to be translated"
                          "# @param Vector trans the translation vector"
                          "# @param Bool store stores the reverse transformation as an attachment (REVERSE_TRANSFORMATION);"
                          "#   default value: 1."
                          "# @return Polytope",
                          "translate<Scalar>(Polytope<Scalar> Vector<Scalar>;$=1)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
