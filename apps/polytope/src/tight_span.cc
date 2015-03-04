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
#include "polymake/Matrix.h"
#include "polymake/Vector.h"

namespace polymake { namespace polytope {

template<typename Scalar>
perl::Object tight_span(const Matrix<Scalar>& vertices, const Vector<Scalar>& weight, bool dim)
{
   Matrix<Scalar> ineq;

   if (vertices.rows()!=weight.size())
      throw std::runtime_error("Weight vector has the wrong dimension.");

   if (!dim) ineq = weight|vertices.minor(All,range(1,vertices.cols()-1));
   else ineq =  weight|vertices;

   perl::Object p(perl::ObjectType::construct<Scalar>("Polytope"));
   p.take("INEQUALITIES") << ineq;
   p.take("BOUNDED") << false;
   return p;
}

template<typename Scalar>
perl::Object tight_span(perl::Object p_in)
{
   const Matrix<Scalar> verts=p_in.give("VERTICES");
   const Vector<Scalar> w = p_in.give("POLYTOPAL_SUBDIVISION.WEIGHTS");
   const int dim = p_in.CallPolymakeMethod("DIM");
   return tight_span<Scalar>(verts, w, dim==verts.cols()-1);
}

UserFunctionTemplate4perl("#@category Triangulations, subdivisions and volume"
                          "# Compute the tight span dual to the regular subdivision"
                          "# obtained by lifting //points// to //weight//"
                          "# and taking the lower complex of the resulting polytope."
                          "# @param Matrix points"
                          "# @param Vector weight"
                          "# @param Bool full true if the polytope is full-dimensional."
                          "#        Default value is 1."
                          "# @return Polytope (The polymake object [[TightSpan]] is only used for tight spans of finite metric spaces, not for tight spans of subdivisions in general.)"
                          "# @author Sven Herrmann",
                          "tight_span<Scalar>(Matrix<type_upgrade<Scalar>> Vector<type_upgrade<Scalar>>; $=1)");

UserFunctionTemplate4perl("#@category Triangulations, subdivisions and volume"
                          "# Compute the tight span dual to the regular subdivision of a polytope //P//"
                          "# obtained by the [[Polytope::WEIGHTS|WEIGHTS]] and taking the lower complex of the resulting polytope."
                          "# @param Polytope P"
                          "# @return Polytope(The polymake object [[TightSpan]] is only used for tight spans of finite metric spaces, not for tight spans of subdivisions in general.)"
                          "# @author Sven Herrmann",
                          "tight_span<Scalar>(Polytope<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
