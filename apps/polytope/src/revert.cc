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
perl::Object revert(perl::Object p_in)
{
   const Matrix<Scalar> RT_v=p_in.get_attachment("REVERSE_TRANSFORMATION");
   perl::Object p_out=transform<Scalar>(p_in, RT_v, false);
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
                          "# @return Polytope the original polytope",
                          "revert<Scalar> (Polytope<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
