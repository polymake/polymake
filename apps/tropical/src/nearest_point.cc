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
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"

namespace polymake { namespace tropical {
   
   template <typename Scalar>
   Vector<Scalar> nearest_point(perl::Object t_in, const Vector<Scalar>& point)
   {
      Matrix<Scalar> V = t_in.give("VERTICES");
      const int n(V.rows()), d(V.cols());
      
      Vector<Scalar> lambdas(n);
      Vector<Scalar> n_point(d);
      
      for (int i=0; i<n; ++i)
      {
         lambdas[i]=accumulate(point-V[i], operations::max());
         V[i]=V[i]+same_element_vector(lambdas[i],d);
      }
      
      Scalar first=accumulate(V.col(0), operations::min());
      n_point[0]=0;
      for (int j=1; j<d;++j)
         n_point[j]=accumulate(V.col(j), operations::min())-first;
      
      return n_point;   
   }
   
	UserFunctionTemplate4perl("# @category Tropical operations"
                             "# Compute the projection of a point //x// in  tropical projective space onto a tropical polytope //P//."
                             "# Cf."
                             "# \t Develin & Sturmfels math.MG/0308254v2, Proposition 9."
                             "# @param TropicalPolytope P"
                             "# @param Vector<Coord> x"
                             "# @tparam Coord"
                             "# @return Vector"
                             "# @author Katja Kulas",
                             "nearest_point<Scalar>(TropicalPolytope<Scalar> Vector<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
