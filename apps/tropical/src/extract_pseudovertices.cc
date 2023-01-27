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
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Graph.h"

namespace polymake { namespace tropical {

template <typename Addition, typename Scalar>
void extract_pseudovertices(BigObject cone)
{
   BigObject dome = cone.give("DOME");
   Matrix<Scalar> vertices = dome.give("VERTICES");
   IncidenceMatrix<> VIF = dome.give("VERTICES_IN_FACETS");
   Set<Int> far_face = dome.give("FAR_FACE");

   Set<Set<Int>> facets_as_set (rows(VIF));
   facets_as_set -= far_face;

   cone.take("PSEUDOVERTICES") << vertices;
   cone.take("FAR_PSEUDOVERTICES") << far_face;
   cone.take("MAXIMAL_COVECTOR_CELLS") << IncidenceMatrix<>(facets_as_set);
}

FunctionTemplate4perl("extract_pseudovertices<Addition,Scalar>(Polytope<Addition,Scalar>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
