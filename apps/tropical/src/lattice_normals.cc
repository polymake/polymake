/* Copyright (c) 1997-2021
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
#include "polymake/IncidenceMatrix.h"
#include "polymake/Map.h"
#include "polymake/linalg.h"
#include "polymake/integer_linalg.h"
#include "polymake/common/lattice_tools.h"
#include "polymake/tropical/thomog.h"

namespace polymake { namespace tropical {

/**
 * @brief Computes the property [[LATTICE_NORMALS]]
 */
void compute_lattice_normals(BigObject C)
{
  // Extract necessary values
  IncidenceMatrix<> maximalPolytopes = C.give("MAXIMAL_POLYTOPES");
  IncidenceMatrix<> codimensionOnePolytopes = C.give("CODIMENSION_ONE_POLYTOPES");
  IncidenceMatrix<> maximalAtCodim = C.give("MAXIMAL_AT_CODIM_ONE");
  IncidenceMatrix<> maximalConeNormals = C.give("MAXIMAL_POLYTOPES_AFFINE_HULL_NORMALS");
  Map<std::pair<Int, Int>, Int> facetNormalsByPairs = C.give("FACET_NORMALS_BY_PAIRS");
  Matrix<Rational> facetNormals = C.give("FACET_NORMALS");
  Matrix<Int> maximalPolytopesFacets = C.give("MAXIMAL_POLYTOPES_FACETS");
  Matrix<Rational> affineHull = C.give("AFFINE_HULL");
  Int fanDim = C.give("FAN_DIM");
		
  Map<std::pair<Int, Int>, Vector<Integer>> lattice_normals;

  // We forget the constant coefficient of all normals, since we only care about
  // the (non-affine) space spanned by a cone
  if (affineHull.cols() > 0)
    affineHull.col(0) = zero_vector<Rational>(affineHull.rows());
  if (facetNormals.cols() > 0)
    facetNormals.col(0) = zero_vector<Rational>(facetNormals.rows());

  // The equation x_0 = 0 has to be added manually to all matrices, since it is
  // assumed implicitly for complexes. This ensure that every lattice normal
  // has a leading 0.
  Vector<Integer> add_eq = affineHull.cols() > 0
    ? unit_vector<Integer>(affineHull.cols(),0)
    : Vector<Integer>();

  // Iterate through all the key-value pairs of facetNormalsByPairs
  for (auto key = entire(facetNormalsByPairs); !key.at_end(); ++key) {
    // Construct the affine hull normals matrix of the codimension one facet
    // Make sure the additional equation of the facet wrt to the maximal cone is at the bottom
    const Matrix<Rational> normal_matrix = affineHull.minor(maximalConeNormals.row( (*key).first.second),All)
                                           / facetNormals.row(key->second);

    // Compute the transformation to a weak Hermite normal form.
    const SparseMatrix<Integer> transformation = 
      hermite_normal_form(add_eq / common::eliminate_denominators_in_rows(normal_matrix),false).companion;
			
    // The last fanDim-1 columns of the transformation matrix are a Z-basis for
    // the space spanned by the maximal cone. The last fanDim-2 columns do the same
    // for the codimension one cone. Hence the column with index cols()-fanDim+1 is
    // a representative of the lattice normal (up to sign)
    Vector<Integer> lnormal(transformation.col(transformation.cols() - fanDim +1));

    // The facet normal must be positive on the lattice normal
    if (maximalPolytopesFacets(key->first.second, key->second) * facetNormals.row(key->second) * lnormal < 0) {
      lnormal.negate();
    }

    lattice_normals[ (*key).first] = lnormal;
  }

  C.take("LATTICE_NORMALS") << lattice_normals;

} //END compute lattice normals


/**
 * @brief Checks whether two sets of lattice normals are equivalent for a given cycle
 * (i.e. lattice normals are the same modulo the respective codimension one faces)
 * @param Matrix<Rational> vertices The VERTICES of the cycle
 * @param Matrix<Rational> lineality The LINEALITY_SPACE of the cycle
 * @param IncidenceMatrix<> codim The CODIMENSION_ONE_POLYTOPES of the cycle
 * @param Map<..> lnormals1 The first set of lattice normals
 * @param Map<..> lnormals2 The second set of lattice normals.
 * @return bool
 */
bool compare_lattice_normals(const Matrix<Rational>& vertices_in,
                             const Matrix<Rational>& lineality_in,
                             const IncidenceMatrix<>& codim,
                             const Map<std::pair<Int, Int>, Vector<Integer>>& lnormals1,
                             const Map<std::pair<Int, Int>, Vector<Integer>>& lnormals2)
{
  // First we check if the maps are defined on the same key set
  if (lnormals1.size() != lnormals2.size()) return false;

  if (vertices_in.cols() != lineality_in.cols())
    throw std::runtime_error("dimension mismatch between VERTICES and LINEALITY_SPACE");
  if (vertices_in.cols() == 0)
    return lnormals1.empty();

  // We only need far rays and affine coordinates without leading coordinate for span computation
  Set<Int> far_rays = far_points(vertices_in);
  Matrix<Rational> vertices = tdehomog(vertices_in).minor(All, range_from(1));
  Matrix<Rational> lineality = tdehomog(lineality_in).minor(All, range_from(1));

  for (auto& l1pair : lnormals1) {
    if (!lnormals2.contains(l1pair.first)) return false;

    // If the key exists, check equality of normals
    Vector<Rational> v1( lnormals1[ l1pair.first ] );
    v1 = tdehomog_vec(v1).slice(range_from(1));
    Vector<Rational> v2( lnormals2[ l1pair.first ] );
    v2 = tdehomog_vec(v2).slice(range_from(1));

    const Int cone_index = l1pair.first.first;

    const Matrix<Rational> span_matrix = vertices.minor(codim.row(cone_index) * far_rays,All) / lineality / v1;
    if (rank(span_matrix) != rank(span_matrix / v2)) return false;
  }

  return true;
}

Function4perl(&compute_lattice_normals, "compute_lattice_normals(Cycle)");
Function4perl(&compare_lattice_normals, "compare_lattice_normals");

} }
