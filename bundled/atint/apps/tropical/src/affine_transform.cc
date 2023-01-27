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
	Copyright (c) 2016-2023
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Contains functions to compute the affine transform of a cycle
	*/

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/misc_tools.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/specialcycles.h"

namespace polymake { namespace tropical {

template <typename Addition>
BigObject affine_transform(BigObject cycle, Matrix<Rational> matrix, Vector<Rational> translate)
{
  // Extract values
  Matrix<Rational> vertices = cycle.give("VERTICES");
  IncidenceMatrix<> polytopes = cycle.give("MAXIMAL_POLYTOPES");
  Matrix<Rational> lineality = cycle.give("LINEALITY_SPACE");
  bool weights_exist = cycle.exists("WEIGHTS");
  bool local_exists = cycle.exists("LOCAL_RESTRICTION");

  if (translate.dim() == 0)
    translate = zero_vector<Rational>(matrix.rows());

  // Sanity checks
  if (call_function("is_empty", cycle)) {
    return empty_cycle<Addition>(matrix.rows()-1);
  }
  if (matrix.rows() != translate.dim() || matrix.cols() != vertices.cols()-1)
    throw std::runtime_error("affine_transform: Dimension mismatch.");

  // Change matrix and translate so we can just apply it to rays
  matrix = zero_vector<Rational>() / matrix;
  matrix = unit_vector<Rational>(matrix.rows(),0) | matrix;
  translate = Rational(0) | translate;

  // Transform
  Set<Int> nonfar = far_and_nonfar_vertices(vertices).second;
  vertices = vertices * T(matrix);
  for (auto nf = entire(nonfar); !nf.at_end(); ++nf) {
    vertices.row(*nf) += translate;
  }
  if (lineality.rows() > 0)
    lineality = lineality * T(matrix);
		
  // The seemingly unnecessary thomog(tdehomog - calls take care of the fact
  // that the normalization polymake applies to vertices is not compatible with
  // tropical projective equivalence relation. By dehomogenizing, the homogenizing,
  // we pick a unique representative on an affine chart.
  BigObject result("Cycle", mlist<Addition>());
  result.take("VERTICES") << thomog(tdehomog(vertices));
  result.take("MAXIMAL_POLYTOPES") << cycle.give("MAXIMAL_POLYTOPES");
  result.take("LINEALITY_SPACE") << thomog(tdehomog(lineality));
  if (weights_exist)
    result.take("WEIGHTS") << cycle.give("WEIGHTS");
  if (local_exists)
    result.take("LOCAL_RESTRICTION") << cycle.give("LOCAL_RESTRICTION");
			
  return result;
}

template <typename Addition>
BigObject shift_cycle(BigObject cycle, Vector<Rational> translate)
{
  return affine_transform<Addition>(cycle, unit_matrix<Rational>(translate.dim()), translate);
}

template <typename Addition>
BigObject affine_transform(BigObject cycle, BigObject morphism)
{
  if (!morphism.exists("MATRIX") && !morphism.exists("TRANSLATE"))
    throw std::runtime_error("affine_transform: Morphism has no matrix or translate");
  Matrix<Rational> matrix = morphism.give("MATRIX");
  Vector<Rational> translate = morphism.give("TRANSLATE");
  return affine_transform<Addition>(cycle, matrix,translate);
}

UserFunctionTemplate4perl("# @category Basic polyhedral operations"
                          "# Computes the affine transform of a cycle under an affine linear map."
                          "# This function assumes that the map is a lattice isomorphism on the cycle, i.e."
                          "# no push-forward computations are performed, in particular the weights remain unchanged"
                          "# @param Cycle<Addition> C a tropical cycle"
                          "# @param Matrix<Rational> M The transformation matrix. Should be given in tropical projective"
                          "# coordinates and be homogeneous, i.e. the sum over all rows should be the same."
                          "# @param Vector<Rational> T The translate. Optional and zero vector by default. Should be given in"
                          "# tropical projective coordinates (but without leading coordinate for vertices or rays)."
                          "# If you only want to shift a cycle, use [[shift_cycle]]."
                          "# @return Cycle<Addition> The transform M*C + T",
                          "affine_transform<Addition>(Cycle<Addition>, $; $ = new Vector())");

UserFunctionTemplate4perl("# @category Basic polyhedral operations"
                          "# Computes the affine transform of a cycle under an affine linear map."
                          "# This function assumes that the map is a lattice isomorphism on the cycle, i.e."
                          "# no push-forward computations are performed, in particular the weights remain unchanged"
                          "# @param Cycle<Addition> C a tropical cycle"
                          "# @param Morphism<Addition> M A morphism. Should be defined via [[MATRIX]] and [[TRANSLATE]],"
                          "# though its [[DOMAIN]] will be ignored."
                          "# @return Cycle<Addition> The transform M(C)",
                          "affine_transform<Addition>(Cycle<Addition>, Morphism<Addition>)");

UserFunctionTemplate4perl("# @category Basic polyhedral operations"
                          "# Computes the shift of a tropical cycle by a given vector"
                          "# @param Cycle<Addition> C a tropical cycle"
                          "# @param Vector<Rational> T The translate. Optional and zero vector by default. Should be given in"
                          "# tropical projective coordinates (but without leading coordinate for vertices or rays)."
                          "# @return Cycle<Addition> The shifted cycle",
                          "shift_cycle<Addition>(Cycle<Addition>, $)");
} }
