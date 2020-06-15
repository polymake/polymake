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
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/Rational.h"

namespace polymake { namespace polytope {

template <typename Scalar>
BigObject crosscut_complex(BigObject p_in, OptionSet options)
{
   const IncidenceMatrix<> C = p_in.give("VERTICES_IN_FACETS");
   const bool 
      realize = options["geometric_realization"],
      bounded = p_in.give("BOUNDED");
   BigObjectType result_type =
      (realize && bounded) ? BigObjectType("topaz::GeometricSimplicialComplex", mlist<Scalar>())
                           : BigObjectType("topaz::SimplicialComplex");
   BigObject p_out(result_type);
   p_out.set_description() << "Crosscut complex of " << p_in.name() << endl;
   p_out.take("FACETS") << rows(C);

   if (realize && bounded) {
      const Matrix<Scalar> V = p_in.give("VERTICES");
      p_out.take("COORDINATES") << dehomogenize(V);
   }

   return p_out;
}

UserFunctionTemplate4perl("# @category Producing other objects"
                          "# Produce the __crosscut complex__ of the boundary of a polytope.\n"
                          "# @param Polytope p"
                          "# @option Bool geometric_realization create a [[topaz::GeometricSimplicialComplex]]; default is true"
                          "# @return topaz::SimplicialComplex",
                          "crosscut_complex<Scalar>(Polytope<Scalar> { geometric_realization => 1 } )");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
