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
BigObject revert(BigObject p_in)
{
   const Matrix<Scalar> RT_v=p_in.get_attachment("REVERSE_TRANSFORMATION");
   BigObject p_out=transform<Scalar>(p_in, RT_v, false);
   p_out.set_description() << "Polytope reversely transformed from " << p_in.name() << endl;
   return p_out;
}

UserFunctionTemplate4perl("# @category Transformations"
                          "# Apply a reverse transformation to a given polyhedron //P//."
                          "# All transformation clients keep track of the polytope's history."
                          "# They write or update the attachment REVERSE_TRANSFORMATION."
                          "# "
                          "# Applying revert to the transformed polytope reconstructs the original polytope."
                          "# @param Polytope P a (transformed) polytope"
                          "# @return Polytope the original polytope"
                          "# @example The following translates the square and then reverts the transformation:"
                          "# > $v = new Vector(1,2);"
                          "# > $p = translate(cube(2),$v);"
                          "# > print $p->VERTICES;"
                          "# | 1 0 1"
                          "# | 1 2 1"
                          "# | 1 0 3"
                          "# | 1 2 3"
                          "# > $q = revert($p);"
                          "# > print $q->VERTICES;"
                          "# | 1 -1 -1"
                          "# | 1 1 -1"
                          "# | 1 -1 1"
                          "# | 1 1 1",
                          "revert<Scalar> (Polytope<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
