/* Copyright (c) 1997-2019
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
                  "# the product of a //k-1//- and an //l-1//-dimensional simplex."
                  "# @param Int k the number of vertices of the first simplex"
                  "# @param Int l the number of vertices of the second simplex"
                  "# @return Vector<Rational>"
                  "# @example [application fan][prefer cdd]"
                  "# The following creates the staircase triangulation of the product"
                  "# of the 2- and the 1-simplex."
                  "# > $w = staircase_weight(3,2);"
                  "# > $p = product(simplex(2),simplex(1));"
                  "# > $p->POLYTOPAL_SUBDIVISION(WEIGHTS=>$w);"
                  "# > print $p->POLYTOPAL_SUBDIVISION->MAXIMAL_CELLS;"
                  "# | {0 2 4 5}"
                  "# | {0 2 3 5}"
                  "# | {0 1 3 5}",
                  &staircase_weight,"staircase_weight"); 
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
