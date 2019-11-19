/* Copyright (c) 1997-2019
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
#include <cstdlib>
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Integer.h"
#include "polymake/polytope/solve_LP.h"

namespace polymake {
namespace polytope {


template <typename Scalar>
Matrix<Integer> integer_points_bbox(perl::Object p_in)
{
   // get specification of polytope
   const Matrix<Scalar> H = p_in.give("FACETS | INEQUALITIES");
   const Matrix<Scalar> E = p_in.lookup("AFFINE_HULL | EQUATIONS");
   const int d = H.cols() - 1;

   // First find lower and upper bounds on each component by solving LPs in each +/- unit
   // direction. The order does not matter, since the LP solver (currently) does not allow for a
   // warm start.

   Vector<Scalar> obj(d+1);    // objective
   Vector<Integer> L(d+1);     // lower bounds
   Vector<Integer> U(d+1);     // upper bounds

   // initialize 0 component
   L[0] = 1;
   U[0] = 1;
   obj[0] = 0;

   // pass through dimensions
   for (int i = 1; i <= d; ++i) {
      // set up unit vector
      for (int j = 1; j <= d; ++j) {
         if (i != j)
            obj[j] = 0;
         else
            obj[j] = 1;
      }

      // maximize along unit vector
      auto S = solve_LP(H, E, obj, true);
      if (S.status != LP_status::valid)
         throw std::runtime_error("Cannot determine upper bounds for generating integer points");

      // set upper bound to rounded objective function value
      U[i] = floor(S.objective_value);

      // minimize along unit vector (do not catch exceptions)
      S = solve_LP(H, E, obj, false);
      if (S.status != LP_status::valid)
         throw std::runtime_error("Cannot determine lower bounds for generating integer points");

      // set lower bound to rounded objective function value
      L[i] = ceil(S.objective_value);
   }

   // Second, enumerate all integer points within the bounds

   ListMatrix< Vector<Integer> > P(0,d+1);        // to collect the integer points
   Vector<Integer> cur(L);   // stores current point

   for (;;) {
      // test whether point is valid
      bool valid = true;

      // check whether current point satisfies all equations
      for (int i = 0; i < E.rows(); ++i) {
         if (convert_to<Scalar>(cur) * E.row(i) != 0) {
            valid = false;
            break;
         }
      }

      // check whether current point satisfies all inequalities
      if (valid) {
         for (int i = 0; i < H.rows(); ++i) {
            if (convert_to<Scalar>(cur) * H.row(i) < 0) {
               valid = false;
               break;
            }
         }
      }

      // append point if it is valid
      if (valid)
         P /= cur;

      // propagate carry if necessary
      int curd = 1;
      while (curd <= d && cur[curd] >= U[curd]) {
         cur[curd] = L[curd];
         ++curd;
      }

      if (curd <= d)
         cur[curd] += 1;
      else
         break;
   }

   // return Matrix of generated points
   return P;
}


UserFunctionTemplate4perl("# @category Geometry"
                          "# Enumerate all integer points in the given polytope by searching a bounding box."
                          "# @author Marc Pfetsch"
                          "# @param  Polytope<Scalar> P"
                          "# @return Matrix<Integer>"
                          "# @example"
                          "# > $p = new Polytope(VERTICES=>[[1,13/10,1/2],[1,1/5,6/5],[1,1/10,-3/2],[1,-7/5,1/5]]);"
                          "# > print integer_points_bbox($p);"
                          "# | 1 0 -1"
                          "# | 1 -1 0"
                          "# | 1 0 0"
                          "# | 1 1 0"
                          "# | 1 0 1",
                          "integer_points_bbox<Scalar>(Polytope<Scalar>)");

}
}
