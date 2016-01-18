/* Copyright (c) 1997-2015
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
#include "polymake/linalg.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

template <typename Matrix, typename Coord>
Vector<Coord> inner_point(const GenericMatrix<Matrix, Coord>& V)
{
   const Set<int> b=basis_rows(V);

   // center of gravity of the basis vectors
   const Vector<Coord> rel_int_pt=average(rows(V.minor(b, All)));

   // we require rel_int_pt to be affine
   // otherwise we might be in trouble with the orientation
   if (is_zero(rel_int_pt[0]))
      throw std::runtime_error("computed point not affine");

   return rel_int_pt;
}

UserFunctionTemplate4perl("# @category Optimization"
                          "# Compute a true inner point of a convex hull of the given set of //points//."
                          "# @param Matrix points"
                          "# @return Vector"
                          "# @example To print an inner point of the square, do this:"
                          "# > print inner_point(cube(2)->VERTICES);"
                          "# |1 -1/3 -1/3",
                          "inner_point(Matrix)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
