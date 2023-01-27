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
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include <cmath>

namespace polymake { namespace polytope {

Matrix<double> points2metric_Euclidean(const Matrix<double> &V)
{ 
  const Int n = V.rows();
  Matrix<double> dist(n, n);

  for (Int i = 0; i < n; ++i)
      for (Int j = i; j < n; ++j)
        dist(i,j) = dist(j,i) = sqrt(sqr(V.row(i)-V.row(j))); // Euclidean distance
  return dist;
}

template <typename Scalar>
Matrix<Scalar> points2metric_max(const Matrix<Scalar> &V)
{
  const Int n = V.rows();
  Matrix<Scalar> dist(n,n);
  for (Int i = 0; i < n; ++i)
      for (Int j = i; j < n; ++j)
        dist(i,j) = dist(j,i) = accumulate(attach_operation(V.row(i)-V.row(j), operations::abs_value()), operations::max());
  return dist;
}

template <typename Scalar>
Matrix<Scalar> points2metric_l1(const Matrix<Scalar> &V)
{
  const Int n = V.rows();
  Matrix<Scalar> dist(n, n);
  for (Int i = 0; i < n; ++i)
      for (Int j = i; j < n; ++j)
        dist(i,j) = dist(j,i) = accumulate(attach_operation(V.row(i)-V.row(j), operations::abs_value()), operations::add());
  return dist;
}

Function4perl(&points2metric_Euclidean,"points2metric_Euclidean($)");

FunctionTemplate4perl("points2metric_max(Matrix)");

FunctionTemplate4perl("points2metric_l1(Matrix)");

InsertEmbeddedRule("# @category Triangulations, subdivisions and volume"
                   "# Define a metric by restricting the Euclidean distance function to a given set of //points//."
                   "# Due to floating point computations (sqrt is used) the metric defined may not be exact."
                   "# If the option //max// or //l1// is set to true the max-norm or l1-norm is used instead (with exact computation)."
                   "# @param Matrix points"
                   "# @option Bool max triggers the usage of the max-norm (exact computation)"
                   "# @option Bool l1 triggers the usage of the l1-norm (exact computation)"
                   "# @return Matrix"
                   "# @example"
                   "# > print points2metric(cube(2)->VERTICES, max=>1);"
                   "# | 0 2 2 2"
                   "# | 2 0 2 2"
                   "# | 2 2 0 2"
                   "# | 2 2 2 0\n"
                   "user_function points2metric(Matrix { max => 0, l1 => 0 }) {\n"
                   "if ($_[1]->{'max'}) { return points2metric_max($_[0]); }\n"
                   "if ($_[1]->{'l1'}) { return points2metric_l1($_[0]); }\n"
                   "points2metric_Euclidean($_[0]); }\n");

InsertEmbeddedRule("# @category Triangulations, subdivisions and volume"
                   "# Define a metric by restricting the Euclidean distance function to the vertex set of a given polytope //P//."
                   "# Due to floating point computations (sqrt is used) the metric defined may not be exact."
                   "# If the option //max// or //l1// is set to true the max-norm or l1-norm is used instead (with exact computation)."
                   "# @param Polytope P"
                   "# @option Bool max triggers the usage of the max-norm (exact computation)"
                   "# @return Matrix"
                   "# @example"
                   "# > print poly2metric(cube(2), max=>1);"
                   "# | 0 2 2 2"
                   "# | 2 0 2 2"
                   "# | 2 2 0 2"
                   "# | 2 2 2 0\n"
                   "user_function poly2metric(Polytope { max => 0, l1 => 0 }) {\n"
                   "points2metric($_[0]->VERTICES,$_[1]); }\n"); 

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
