/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA  02110-1301, USA.

	---
	Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

	---
	Copyright (c) 2016-2019
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Implementations of miscellaneous tools
	*/

#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/morphism_values.h"
#include "polymake/tropical/homogeneous_convex_hull.h"
#include "polymake/tropical/misc_tools.h"

namespace polymake { namespace tropical {

using matrix_pair = std::pair<Matrix<Rational>, Matrix<Rational>>;

IncidenceMatrix<> all_cones_as_incidence(perl::Object complex)
{
  Array<IncidenceMatrix<>> all_cones = complex.give("CONES");
  if (all_cones.size() == 0) return IncidenceMatrix<>();
  return IncidenceMatrix<>(rowwise(), all_cones);
}

Matrix<Rational> binaryMatrix(int n)
{
  ListMatrix<Vector<Rational>> result(0,n);
  Vector<Rational> prev_row = -ones_vector<Rational>(n);
  result /= prev_row;
  // Now increase the last row of result by "one" in each iteration and append the new row to result
  Integer iterations = Integer::pow(2,n)-1;
  for (int i = 1; i <= iterations; ++i) {
    // Find the first -1-entry
    Vector<Rational> newrow(prev_row);
    auto neg_it = find_in_range_if(entire(newrow), operations::negative());
    // Now toggle this and all previous entries
    auto nr_it = entire(newrow);
    while (nr_it != neg_it) {
      *nr_it = -1;
      ++nr_it;
    }
    *nr_it = 1;
    result /= newrow;
    prev_row = newrow;
  }
  return result;
}

/**
   @brief Computes the labels for each cell of a function domain, given its ray and lineality values
   @param perl::Object domain A Cycle object, representing the domain of the function
   @param Matrix<Rationl> ray_values Values of the function on the rays, given as row vectors in non-homog. coordinates
   @param Matrix<Rational> lin_values Values of the function on the lineality space, given as row vectors in non-homog. coordinates.
   @param bool values_are_homogeneous Whether vertex and lineality values are given in tropical projective
   coordinates.
   @return A list of std::strings
*/
perl::ListReturn computeFunctionLabels(perl::Object domain, Matrix<Rational> ray_values, Matrix<Rational> lin_values, const bool values_are_homogeneous)
{
  // Extract values
  const Matrix<Rational>& rays_ref = domain.give("SEPARATED_VERTICES");
  const Matrix<Rational> rays = tdehomog(rays_ref,0);
  const IncidenceMatrix<>& cones = domain.give("SEPARATED_MAXIMAL_POLYTOPES");
  const Matrix<Rational>& lineality_ref = domain.give("LINEALITY_SPACE");
  const Matrix<Rational> lineality = tdehomog(lineality_ref,0);

  if (values_are_homogeneous) {
    ray_values = tdehomog(ray_values,0);
    lin_values = tdehomog(lin_values,0);
  }

  perl::ListReturn result;

  for (auto mc = entire(rows(cones)); !mc.at_end(); ++mc) {
    Matrix<Rational> matrix;
    Vector<Rational> translate;
    computeConeFunction(rays.minor(*mc,All), lineality, ray_values.minor(*mc,All), lin_values, translate, matrix);

    matrix = thomog(matrix,0,false);
    translate = thomog_vec(translate,0,false);

    std::ostringstream rep;
    if (matrix.rows() > 1) {
      wrap(rep) << "(" << translate << ")" << " + ";
      for (auto i = entire(rows(matrix)); !i.at_end(); ++i)
        wrap(rep) << "[" << (*i) << "]";
    }
    // We have a special representation format for functions to R
    else {
      bool hadnonzeroterm = false;
      for (int i = 0; i < matrix.cols(); ++i) {
        if (matrix(0,i) != 0) {
          if (hadnonzeroterm) rep << " + ";
          hadnonzeroterm = true;
          if (matrix(0,i) < 0)
            rep << "(" << matrix(0,i) << ")";
          else
            rep << matrix(0,i);
          rep << "*x_" << (i+1);
        }
      }
      if (translate[0] < 0 && hadnonzeroterm) rep << " - " << -translate[0];
      if (translate[0] > 0 && hadnonzeroterm) rep << " + " << translate[0];
      if (!hadnonzeroterm) rep << translate[0];
    }
    result << rep.str();
  }

  return result;
}

// Documentation see perl wrapper
bool contains_point(perl::Object complex, const Vector<Rational>& point)
{
  // Special case: Empty cycle
  if (call_function("is_empty", complex))
    return false;

  // Extract values
  Matrix<Rational> rays = complex.give("VERTICES");
  Matrix<Rational> linspace = complex.give("LINEALITY_SPACE");
  IncidenceMatrix<> cones = complex.give("MAXIMAL_POLYTOPES");

  if (point.dim() != rays.cols() && point.dim() != linspace.cols()) {
    throw std::runtime_error("Point does not have the same ambient dimension as the complex.");
  }

  for (int mc = 0; mc < cones.rows(); ++mc) {
    if (is_ray_in_cone(rays.minor(cones.row(mc),All), linspace, point, true))
      return true;
  }

  return false;
}

UserFunction4perl("# @category Lattices"
                  "# Returns n random integers in the range 0.. (max_arg-1),inclusive"
                  "# Note that this algorithm is not optimal for real randomness:"
                  "# If you change the range parameter and then change it back, you will"
                  "# usually get the exact same sequence as the first time"
                  "# @param Int max_arg The upper bound for the random integers"
                  "# @param Int n The number of integers to be created"
                  "# @return Vector<Integer>",
                  &randomInteger,"randomInteger($, $)");

UserFunction4perl("# @category Basic polyhedral operations"
                  "# Takes a weighted complex and a point and computed whether that point lies in "
                  "# the complex"
                  "# @param Cycle A weighted complex"
                  "# @param Vector<Rational> point An arbitrary vector in the same ambient"
                  "# dimension as complex. Given in tropical projective coordinates with leading coordinate."
                  "# @return Bool Whether the point lies in the support of complex",
                  &contains_point,"contains_point(Cycle,$)");

Function4perl(&computeFunctionLabels, "computeFunctionLabels(Cycle, Matrix,Matrix,$)");

} }
