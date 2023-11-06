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

#include "polymake/polytope/projection.h"

namespace polymake {
namespace fan {

template <typename Scalar>
BigObject project_full_fan_impl(BigObject p_in, OptionSet options)
{
   if (!p_in.exists("RAYS | INPUT_RAYS"))
      throw std::runtime_error("projection is not defined for combinatorially given objects");

   const Matrix<Scalar> rays = p_in.give("RAYS | INPUT_RAYS");
   const Matrix<Scalar> lineality = p_in.give("LINEALITY_SPACE | INPUT_LINEALITY");
   const Matrix<Scalar> linear_span(null_space(rays/lineality));

   const Int codim = rank(linear_span);
   if (codim == 0) return p_in; // nothing to do

   Array<Int> indices;
   const Set<Int> coords_to_eliminate = polytope::coordinates_to_eliminate<Scalar>(indices, linear_span.cols(), codim, p_in, options["revert"]);   // set of columns to project to

   BigObject p_out(p_in.type());

   polytope::process_rays<Scalar>(p_in, indices, options, coords_to_eliminate, p_out);

   IncidenceMatrix<> MC;
   if (p_in.lookup("MAXIMAL_CONES") >> MC)
      p_out.take("MAXIMAL_CONES") << MC;
   else if (p_in.lookup("INPUT_CONES") >> MC)
      p_out.take("INPUT_CONES") << MC;
   return p_out;
}

FunctionTemplate4perl("project_full_fan_impl<Scalar=Rational>(PolyhedralFan {revert => 0, nofm => 0})");


} // end namespace fan
} // end namespace polymake
