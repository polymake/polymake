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
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/linalg.h"

namespace polymake { namespace tropical {

/*
 * @brief Checks balancing for a Cycle
 * @param Cycle c The cycle
 * @param bool find_all_non_balanced_faces Do not stop at the first non-balanced face. Instead, it will check all faces and return a full set of indices 
 * referring to [[CODIMENSION_ONE_POLYTOPES]], indicating all non-balanced faces. False by default.
 * @return Set<Int> empty if the cycle is balanced, or the index of the first non-balanced face if @a find_all_non_balanced_faces == false,
 * or the full set of indices of non-balanced faces referring to [[CODIMENSION_ONE_POLYTOPES]]
 */
Set<Int> check_balancing(BigObject C, bool find_non_balanced_faces = false)
{
  // Extract values
  Matrix<Rational> vertices = C.give("VERTICES");
  Matrix<Rational> linealitySpace = C.give("LINEALITY_SPACE");
  IncidenceMatrix<> codimensionOnePolytopes = C.give("CODIMENSION_ONE_POLYTOPES");
  Map<std::pair<Int, Int>, Vector<Integer>> latticeNormals = C.give("LATTICE_NORMALS");
  Vector<Integer> weights = C.give("WEIGHTS");
  Set<Int> farVertices = C.give("FAR_VERTICES");
  Int fanAmbientDim = C.give("FAN_AMBIENT_DIM");

  Matrix<Integer> lattice_normal_sums(codimensionOnePolytopes.rows(), fanAmbientDim);
  Set<Int> non_balanced;
  // Iterate through all lattice normals and compute weighted sums
  for (auto lnormal = entire(latticeNormals); !lnormal.at_end(); ++lnormal) {
    lattice_normal_sums.row(lnormal->first.first) += 
      weights[lnormal->first.second] * lnormal->second;
  } //END iterate lattice normals

  // For each codimension one face check if the lattice normal sum is in the span
  // modulo the all-ones-vector
  Vector<Rational> proj_vector = fanAmbientDim > 0
    ? ones_vector<Rational>(fanAmbientDim) - unit_vector<Rational>(fanAmbientDim,0)
    : Vector<Rational>();
  for (Int codim = 0; codim < codimensionOnePolytopes.rows(); ++codim) {
    Matrix<Rational> span_matrix = linealitySpace;
    // To create the span, add all far vertices and take differences of vertices with a basepoint.
    span_matrix /= vertices.minor( codimensionOnePolytopes.row(codim) * farVertices,All);
    Set<Int> codimOneRays = codimensionOnePolytopes.row(codim) - farVertices;
    Int basepoint = *(codimOneRays.begin());
    codimOneRays -= basepoint;
    for (auto nf = entire(codimOneRays); !nf.at_end(); ++nf) {
      span_matrix /= (vertices.row(*nf) - vertices.row(basepoint));
    }
    span_matrix /= proj_vector;
    span_matrix /= (Vector<Rational>(lattice_normal_sums.row(codim)));
                
    // The lattice normal is the last column of the matrix for which we compute the kernel.
    // If it's in the span of the other columns, there must be a kernel element with
    // nontrivial last entry.
    Matrix<Rational> kernel = null_space(T(span_matrix));
    if (kernel.cols() == 0 || is_zero(kernel.col(kernel.cols()-1))) {
      non_balanced += codim;  
      if (!find_non_balanced_faces) break;
    }
  } //END iterate codim one polytopes

  return non_balanced;
} //END check_balancing

bool is_balanced(BigObject C)
{
  return check_balancing(C, false).empty();
}

/*
 * @brief Wrapper for check_balancing that returns the set of indices of unbalanced
 * codimensionOnePolytopes.
 */
Set<Int> unbalanced_faces(BigObject C)
{
  return check_balancing(C, true);
}

UserFunction4perl("# @category Weights and lattices"
                  "# This computes whether a given cycle is balanced."
                  "# Note that, while cycles are per definition balanced polyhedral complexes,"
                  "# polymake allows the creation of Cycle objects which are not balanced."
                  "# @param Cycle C The cycle for which to check balancing."
                  "# @return Bool Whether the cycle is balanced."
                  "# @example"
                  "# > $x = new Cycle<Max>(PROJECTIVE_VERTICES=>[[1,0,0,0],[0,-1,0,0],[0,0,-1,0],[0,0,0,-1]],MAXIMAL_POLYTOPES=>[[0,1],[0,2],[0,3]],WEIGHTS=>[1,1,1]);"
                  "# > print is_balanced($x);"
                  "# | true",
                  &is_balanced, "is_balanced(Cycle)");

Function4perl(&unbalanced_faces, "unbalanced_faces(Cycle)");
Function4perl(&check_balancing, "check_balancing(Cycle; $=false)");

} }
