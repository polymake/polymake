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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/RandomGenerators.h"
#include <algorithm>

/** @file gale_vertices
 *
 *  Calculate the coordinates of points for an affine Gale diagram.
 *  First the projection vector $(1, 1, ... )$ is tried, then random vectors.
 */

namespace polymake { namespace polytope {

template <typename Scalar>
Matrix<double> gale_vertices(const Matrix<Scalar>& G)
{
   const Int n = G.rows();
   UniformlyRandom<Rational> random(log2_ceil(n)+1);
   Vector<Scalar> y(G.cols(), Scalar(1)), G_y(G.rows());

   bool feasible;
   do {
      G_y = G*y;
      feasible = true;
      for (auto g_y = find_in_range_if(entire(G_y), operations::is_zero());
           !g_y.at_end();  g_y = find_in_range_if(++g_y, operations::is_zero())) {
         if (!is_zero(G[g_y - G_y.begin()])) {
            copy_range(translate(random, Scalar(Rational(-1,2))).begin(), entire(y));
            feasible = false;
            break;
         }
      }
   } while (!feasible);

   Matrix<Scalar> P=null_space(y);
   orthogonalize(entire(rows(P)));
   y /= sqr(y);

   Matrix<double> GV(G.rows(), G.cols());

   for (Int i = 0; i < n; ++i)
      if ((GV(i,0) = double(sign(G_y[i]))) != 0) {
         GV[i].slice(range_from(1)) = convert_to<double>(P*(G[i]/G_y[i] - y));
      }
   return GV;
}

FunctionTemplate4perl("gale_vertices<Scalar> (Matrix<Scalar>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
