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
#include "polymake/Rational.h"
#include "polymake/Vector.h"

namespace polymake { namespace polytope {

Vector<Rational> staircase_weight(const int k, const int l)
{
   Vector<Rational> weight(k*l);
   int index=0;
   for (int i=1; i<=k; ++i)
      for (int j=k+1; j<=k+l; ++j) {
         weight[index++]=(k-i)*(j-k-1)+(i-1)*(k+l-j);
      }

   return weight;;
}

UserFunction4perl("# @category Triangulations, subdivisions and volume"
                  "# Gives a weight vector for the staircase triangulation of"
                  "# the product of a //k//- and an //l//-dimensional simplex."
                  "# @param Int k the dimension of the first simplex"
                  "# @param Int l the dimension of the second simplex"              
                  "# @return Vector<Rational>",
                  &staircase_weight,"staircase_weight"); 
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
