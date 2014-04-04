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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"

namespace polymake { namespace tropical {

template <typename Scalar>
perl::Object trop2poly(perl::Object t_in)
{
   Matrix<Scalar> V=t_in.give("POINTS");
   const int n(V.rows()), d(V.cols()-1);
  
   // The coordinates are in the (1+d+n)-dimensional space W = Z x Y:
   // 0: homogenizing coordinate; 1..d+1: Z; d+2..d+1+n: Y
   Matrix<Scalar> I(n*(d+1),n+d+2); // initialized as zero matrix
   for (int i=0; i<n; ++i)
      for (int j=0; j<=d; ++j) {
         I(i*(d+1)+j,0)=V(i,j);
         I(i*(d+1)+j,1+j)=I(i*(d+1)+j,2+d+i)=-1; // V(i,j) - z(j) - y(i) >= 0
      }
  
   Vector<Scalar> normalizing_equation(unit_vector<Scalar>(n+d+2,1));

   perl::Object p_out(perl::ObjectType::construct<Scalar>("polytope::Polytope"));
   p_out.set_description() << "Envelope for "<< t_in.name() <<endl;

   p_out.take("INEQUALITIES") << I;
   p_out.take("EQUATIONS") << normalizing_equation;
   return p_out;
}

UserFunctionTemplate4perl("# @category Other"
                          "# Given points in the tropical projective space, compute an ordinary unbounded polyhedron such that"
                          "# the tropical convex hull of the input is the bounded subcomplex of the latter."
                          "# Cf."
                          "#    Develin & Sturmfels math.MG/0308254v2, Lemma 22."
                          "# "
                          "# Warning: This client does not implement the reverse transformation to [[poly2trop]]."
                          "# @param TropicalPolytope T"
                          "# @return Polytope",
                          "trop2poly<Scalar>(TropicalPolytope<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
