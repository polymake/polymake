/* Copyright (c) 1997-2018
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

#ifndef POLYMAKE_POLYTOPE_SEPARATING_HYPERPLANE_H
#define POLYMAKE_POLYTOPE_SEPARATING_HYPERPLANE_H

#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/polytope/solve_LP.h"

namespace polymake { namespace polytope {

template <typename Scalar, typename VectorType, typename MatrixType>
Vector<Scalar> separating_hyperplane(const GenericVector<VectorType, Scalar>& q, 
                                     const GenericMatrix<MatrixType, Scalar>& points)
{
   /*
     construction of LP according to cdd redundancy check for points, see
     http://www.ifor.math.ethz.ch/~fukuda/polyfaq/node22.html#polytope:Vredundancy
    */
   const Matrix<Scalar> 
      ineqs(  (zero_vector<Scalar>() | (ones_vector<Scalar>() | -points.minor(All, range(1,points.cols()-1))))
              // z^t p_i - z_0 <= 0; CAUTION: p_i is affine part of i-th point! 
              / (ones_vector<Scalar>(2) | -q.slice(range_from(1))) ),
              // z^t q - z_0 <= 1, prevents unboundedness
      affine_hull(null_space(points/q)),
      extension2(affine_hull.rows(), 2);
   Matrix<Scalar>
      affine_hull_minor(affine_hull.rows(), affine_hull.cols()-1);
   if (affine_hull.cols() > 1) {
      affine_hull_minor = affine_hull.minor(All, range(1, affine_hull.cols()-1));
   }
   const Matrix<Scalar> eqs(extension2 | -affine_hull_minor);
   const Vector<Scalar> obj(zero_vector<Scalar>(1) | -ones_vector<Scalar>(1) | q.slice(range_from(1))); // z^t q - z_0

   const auto S = solve_LP(ineqs, eqs, obj, true);
   if (S.status != LP_status::valid || S.objective_value <= 0) //q non-red. <=> obj_val > 0
      throw infeasible();

   // H: z^t x = z_0, i.e., z_0 - z^t x = 0
   return S.solution[1] | -S.solution.slice(range_from(2));
}

}}

#endif

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
