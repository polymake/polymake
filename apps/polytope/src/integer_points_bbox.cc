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
#include <cstdlib>
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Integer.h"
#include "polymake/polytope/to_interface.h"
#include <sstream>

namespace polymake {
namespace polytope {


template <typename Scalar>
Matrix<Integer> integer_points_bbox(perl::Object p_in)
{
   typedef polytope::to_interface::solver<Scalar> Solver;
   //typedef Solver::coord_type coord_type;

   // get specification of polytope
   const Matrix<Scalar> H = p_in.give("FACETS | INEQUALITIES");
   const Matrix<Scalar> E = p_in.lookup("AFFINE_HULL | EQUATIONS");
   const int d = H.cols() - 1;

   // First find lower and upper bounds on each component by solving LPs in each +/- unit
   // direction. The order does not matter, since the LP solver (currently) does not allow for a
   // warm start.

   // initialize LP solver
   Solver LPsolver;

   Vector<Scalar> obj(d+1);    // objective
   Vector<Integer> L(d+1);        // lower bounds
   Vector<Integer> U(d+1);        // upper bounds

   // initialize 0 component
   L[0] = 1;
   U[0] = 1;
   obj[0] = 0;

   // pass through dimensions
   for (int i = 1; i <= d; ++i)
   {
      // set up unit vector
      for (int j = 1; j <= d; ++j)
      {
         if ( i != j )
            obj[j] = 0;
         else
            obj[j] = 1;
      }

      // maximize along unit vector
      typename Solver::lp_solution solution;
      try
      {
         solution = LPsolver.solve_lp(H, E, obj, true);
      }
      catch (const linalg_error& error)
      {
         std::stringstream s;
         s << "Cannot determine bounds for generating integer points: " << error.what() << std::ends;
         throw std::runtime_error(s.str());
      }

      // set upper bound to rounded objective function value
      U[i] = floor(solution.first);

      // minimize along unit vector (do not catch exceptions)
      try
      {
         solution = LPsolver.solve_lp(H, E, obj, false);
      }
      catch (const linalg_error& error)
      {
         std::stringstream s;
         s << "Cannot determine bounds for generating integer points: " << error.what() << std::ends;
         throw std::runtime_error(s.str());
      }

      // set lower bound to rounded objective function value
      L[i] = ceil(solution.first);
   }

   // Second, enumerate all integer points within the bounds

   ListMatrix< Vector<Integer> > P(0,d+1);        // to collect the integer points
   Vector<Integer> cur(L);   // stores current point
   bool finished = false;
   do
   {
      // test whether point is valid
      bool valid = true;

      // check whether current point satisfies all equations
      for (int i = 0; i < E.rows(); ++i)
      {
         if ( convert_to<Scalar>(cur) * E.row(i) != 0 )
         {
            valid = false;
            break;
         }
      }

      // check whether current point satisfies all inequalities
      if ( valid )
      {
         for (int i = 0; i < H.rows(); ++i)
         {
            if ( convert_to<Scalar>(cur) * H.row(i) < 0 )
            {
               valid = false;
               break;
            }
         }
      }

      // append point if it is valid
      if ( valid )
         P /= cur;

      // propagate carry if necessary
      int curd = 1;
      while ( curd <= d && cur[curd] >= U[curd] )
      {
         cur[curd] = L[curd];
         ++curd;
      }

      if ( curd <= d)
         cur[curd] += 1;
      else
         finished = true;
   }
   while (! finished );

   // return Matrix of generated points
   return P;
}


UserFunctionTemplate4perl("# @category Geometry\n"
   "# Enumerate all integer points in the given polytope by searching a bounding box.\n"
   "# @author Marc Pfetsch\n"
   "# @param  Polytope<Scalar> P\n"
   "# @return Matrix<Integer>\n",
   "integer_points_bbox<Scalar>(Polytope<Scalar>)");

}
}
