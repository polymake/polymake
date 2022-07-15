/* Copyright (c) 1997-2022
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
#include "polymake/Matrix.h"

namespace polymake { namespace tropical {
        
// Note: This is not in dual_addition_version.h, as the compiler gets confused 
// with all the overloaded functions when dealing with big objects.
        
template <typename Addition>
BigObject dual_addition_version(BigObject c, bool strong = true)
{
  // Extract defining values
  Matrix<Rational> vertices = c.give("VERTICES");
  vertices.minor(All, range_from(1)) *= (strong? -1 : 1);
  BigObject result("Cycle", mlist<typename Addition::dual>());
  result.take("VERTICES") << vertices;
  result.take("MAXIMAL_POLYTOPES") << c.give("MAXIMAL_POLYTOPES");
  result.take("LINEALITY_SPACE") << c.give("LINEALITY_SPACE");
  if (c.exists("WEIGHTS"))
    result.take("WEIGHTS") << c.give("WEIGHTS");
  return result;
}

UserFunctionTemplate4perl("# @category Conversion of tropical addition"
                          "# This function takes a tropical cycle and returns a tropical cycle that "
                          "# uses the opposite tropical addition. By default, the signs of the vertices are inverted."
                          "# @param Cycle<Addition> cycle"
                          "# @param Bool strong_conversion This is optional and TRUE by default."
                          "# It indicates, whether the signs of the vertices should be inverted."
                          "# @return Cycle"
                          "# @example This first creates the standard tropical min-line with apex (0:1:1) in the 3-torus,"
                          "# and then computes from it the corresponding max-cycle - in this case the standard max-line"
                          "# with apex (0:-1:-1), and assigns it to the variable $new_cycle."
                          "# > $cycle = new Hypersurface<Min>(POLYNOMIAL=>toTropicalPolynomial(\"min(a,b-1,c-1)\"));"
                          "# > $new_cycle = dual_addition_version($cycle);",
                          "dual_addition_version<Addition>(Cycle<Addition>;$=1)");

} }
