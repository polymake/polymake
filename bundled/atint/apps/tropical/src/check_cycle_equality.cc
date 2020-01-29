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
	Copyright (c) 2016-2020
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Contains a function to check whether two cycles are identical.
	*/


#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/tropical/thomog.h"


namespace polymake { namespace tropical {

// Documentation see perl wrapper
template <typename Addition>
bool check_cycle_equality(BigObject X, BigObject Y, bool check_weights = true)
{
  // Extract values
  Matrix<Rational> xrays = X.give("VERTICES");
  xrays = tdehomog(xrays);
  IncidenceMatrix<> xcones = X.give("MAXIMAL_POLYTOPES");
  Matrix<Rational> xlin = X.give("LINEALITY_SPACE");
  xlin = tdehomog(xlin);
  Int xambi = X.give("PROJECTIVE_AMBIENT_DIM");
  Vector<Integer> xweights;
  if (!(X.lookup("WEIGHTS") >> xweights))
    check_weights = false;

  Matrix<Rational> yrays = Y.give("VERTICES");
  yrays = tdehomog(yrays);
  IncidenceMatrix<> ycones = Y.give("MAXIMAL_POLYTOPES");
  Matrix<Rational> ylin = Y.give("LINEALITY_SPACE");
  ylin = tdehomog(ylin);
  Int yambi = Y.give("PROJECTIVE_AMBIENT_DIM");
  Vector<Integer> yweights;
  if (!(Y.lookup("WEIGHTS") >> yweights))
    check_weights = false;

  // Check dimensional equality
  if (xambi != yambi) return false;

  // Check equality of lineality spaces
  if (rank(xlin) != rank(ylin) || rank(xlin / ylin) > rank(xlin))
    return false;

  // Find ray permutation
  if (xrays.rows() != yrays.rows())
    return false;

  Map<Int, Int> permutation;
  for (Int x = 0; x < xrays.rows(); ++x) {
    for (Int y = 0; y < yrays.rows(); ++y) {
      // Check if yray = xray modulo lineality
      Matrix<Rational> diff_rays = xrays.minor(scalar2set(x),All) - yrays.minor(scalar2set(y),All);
      if (rank(xlin) == rank(xlin / diff_rays)) {
        permutation[x] = y;
        break;
      }
      // If we arrive here, there is no ray matching x
      if (y == yrays.rows()-1)
        return false;
    }
  } //END compute ray permutation

  // Now check if all cones are equal
  Set<Int> matched_cones;
  for (Int xc = 0; xc < xcones.rows(); ++xc) {
    // Compute permuted cone
    Set<Int> perm_cone{ permutation.map(xcones.row(xc)) };
    // Find this cone in Y
    for (Int yc = 0; yc < ycones.rows(); ++yc) {
      if (!matched_cones.contains(yc)) {
        if (ycones.row(yc).size() == perm_cone.size()) {
          if ((ycones.row(yc) * perm_cone).size() == perm_cone.size()) {
            matched_cones += yc;
            // Check equality of weights, if necessary
            if (check_weights) {
              if (xweights[xc] != yweights[yc])
                return false;
            }
            break;
          }
        } //END check cone equality
      }
      // If we arrive here, there is no match:
      if (yc == ycones.rows()-1)
        return false;
    } //END iterate Y cones
  } //END iterate X cones

  // Check if we actually matched ALL Y cones
  return matched_cones.size() == ycones.rows();
}

UserFunctionTemplate4perl("# @category Basic polyhedral operations"
                          "# This takes two pure-dimensional polyhedral complexes and checks if they are equal"
                          "# i.e. if they have the same lineality space, the same rays (modulo lineality space)"
                          "# and the same cones. Optionally, it can also check if the weights are equal"
                          "# @param Cycle<Addition> X A weighted complex"
                          "# @param Cycle<Addition> Y A weighted complex"
                          "# @param Bool check_weights Whether the algorithm should check for equality of weights. "
                          "# This parameter is optional and true by default"
                          "# @return Bool Whether the cycles are equal",
                          "check_cycle_equality<Addition>(Cycle<Addition>,Cycle<Addition>;$=1)");
} }
