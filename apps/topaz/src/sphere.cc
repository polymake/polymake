/* Copyright (c) 1997-2020
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
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace topaz {
  
BigObject sphere(const Int d)
{
   const Array<Set<Int>> F(d+2, all_subsets_less_1(sequence(0,d+2)).begin());   
   Matrix<Int> Geom(d+2, d+1);
   for (Int i = 0; i < d+1; ++i)
      Geom(i+1, i) = 1;
   
   BigObject p("GeometricSimplicialComplex<Rational>",
               "FACETS", F,
               "DIM", d,
               "PURE", true,
               "MANIFOLD", true,
               "CLOSED_PSEUDO_MANIFOLD", true,
               "ORIENTED_PSEUDO_MANIFOLD", true,
               "SPHERE", true,
               "COORDINATES", Geom);
   p.set_description() << "The " << d << "-dimensional sphere.\nRealized as the boundary of a "
                       << d+1 << "-simplex.\n";
   return p;
}

UserFunction4perl("# @category Producing from scratch\n"
                  "# The //d//-dimensional __sphere__, realized as the boundary of the (//d//+1)-simplex.\n"
                  "# @param Int d dimension"
                  "# @return GeometricSimplicialComplex",
                  &sphere, "sphere($)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
