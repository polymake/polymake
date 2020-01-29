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

#ifndef POLYMAKE_POLYTOPE_INNER_POINT_H
#define POLYMAKE_POLYTOPE_INNER_POINT_H

#include "polymake/client.h"
#include "polymake/linalg.h"
#include "polymake/Matrix.h"
#include "polymake/polytope/solve_LP.h"

namespace polymake { namespace polytope {

// find a point in the true interior of the polyhedron spanned by the facets f
// works with inequalities too
template <typename MatrixType, typename Coord>
Vector<Coord> inner_point_from_facets(const GenericMatrix<MatrixType, Coord>& F)
{
   /* The LP should maximize eps in F * x >= eps, 0 <= eps <= 1

      The LP looks like this:
      |0 -1 |        |
      |.  . |        |   | z |    |0|
      |.  . |   F    |   |eps|    |.|
      |0 -1 |        | * |x_0| >= |.|
      |--------------|   |...|    |.|
      |0  1 | 0 . . 0|   |x_n|    |0|
      |1 -1 | 0 . . 0|

      The solver sets z=0 or z=1 always
      second-to-last row guarantees eps >= 0
      last row guarantees z-eps>=0
      we catch the eps=0 case below.
   */

   Int c = F.cols();
   Int r = F.rows();

   const Matrix<Coord> ineqs = (zero_vector<Coord>(r) | (-ones_vector<Coord>(r) | F))
                             / unit_vector<Coord>(c+2,1)                                // eps >= 0
                             / (unit_vector<Coord>(c+2,0) - unit_vector<Coord>(c+2,1)); // eps <= 1

   const auto S = solve_LP(ineqs, unit_vector<Coord>(c+2, 1), true);  // maximize eps
   if (S.status != LP_status::valid || S.solution[1] == 0)
      throw std::runtime_error("Polyhedron has empty interior.");

   Vector<Coord> ip = S.solution.slice(sequence(2, F.cols()));
   if (!is_zero(ip[0])) ip.dehomogenize();
   return ip;
}

} }

#endif // POLYMAKE_POLYTOPE_INNER_POINT_H


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
