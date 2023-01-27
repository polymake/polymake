/* Copyright (c) 1997-2023
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
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/TropicalNumber.h"
#include "polymake/tropical/arithmetic.h"

namespace polymake { namespace tropical {
  
	//FIXME Adapt to tropical numbers. -- only finite coordinates
      
      template <typename Addition, typename Scalar>
      Vector<TropicalNumber<Addition, Scalar> > nearest_point(BigObject t_in, const Vector<TropicalNumber<Addition, Scalar> >& point)
   {
      typedef TropicalNumber<Addition, Scalar> TNumber;
      Matrix<TNumber> V = t_in.give("POINTS");
      //Matrix<TNumber> V = t_in.give("VERTICES");
      
      Vector<TNumber> lambda = principal_solution(T(V), point);

      return T(V) * lambda;

      //should this output be homogenized?

   } 
      UserFunctionTemplate4perl("# @category Tropical operations"
                                "# Compute the projection of a point //x// in tropical projective space onto a tropical cone //C//."
                                "# Cf."
                                "# \t Develin & Sturmfels math.MG/0308254v2, Proposition 9."
                                "# @param Polytope<Addition,Scalar> C"
                                "# @param Vector<TropicalNumber<Addition,Scalar>> x"
                                "# @return Vector<TropicalNumber<Addition,Scalar>>"
                                "# @author Katja Kulas"
                                "# @example Note that the output is not homogenized, e.g. here (1,2,1) represents the point (0,1,0)."
                                "# > $C = new Polytope<Min>(POINTS=>[[0,0,0],[0,2,0],[0,1,2]]);"
                                "# > $x = new Vector<TropicalNumber<Min>>([0,2,1]);"
                                "# > print nearest_point($C, $x);"
                                "# | 1 2 1",
                                "nearest_point<Addition,Scalar>(Polytope<Addition,Scalar>,Vector<TropicalNumber<Addition,Scalar>>)"); 

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
