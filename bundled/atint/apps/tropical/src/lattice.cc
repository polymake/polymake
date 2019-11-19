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

	Computes all [[LATTICE...]- related properties
	*/

#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/Map.h"
#include "polymake/linalg.h"
#include "polymake/common/lattice_tools.h"
#include "polymake/integer_linalg.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/lattice.h"
#include "polymake/tropical/linear_algebra_tools.h"
#include "polymake/tropical/misc_tools.h"


namespace polymake { namespace tropical {

using LatticeMap = Map< std::pair<int,int>, Vector<Integer> >;

/*
 * @brief Computes [[LATTICE_NORMAL_SUM]]
 */
void computeLatticeNormalSum(perl::Object cycle)
{
  const LatticeMap& latticeNormals = cycle.give("LATTICE_NORMALS");
  const int ambient_dim = cycle.give("FAN_AMBIENT_DIM");
  const Vector<Integer> &weights = cycle.give("WEIGHTS");
  const IncidenceMatrix<> &codimInc = cycle.give("MAXIMAL_AT_CODIM_ONE");

  // This will contain the result
  ListMatrix< Vector<Integer> > summatrix(0, ambient_dim);

  // Iterate over all codim one faces
  for (auto facet = entire<indexed>(rows(codimInc)); !facet.at_end(); ++facet) {
    // This will contain the weighted sum of the lattice normals
    Vector<Integer> result = zero_vector<Integer>(ambient_dim);
    // Go through all adjacent cones
    for (auto e = entire(facet->top()); !e.at_end(); ++e) {
      result = result +latticeNormals[std::make_pair(facet.index(),*e)] * weights[*e];
    }
    summatrix /= result;
  }

  cycle.take("LATTICE_NORMAL_SUM") << summatrix;

} // END computeLatticeNormalSum

/*
 * @brief Computes properties [[LATTICE_NORMAL_FCT_VECTOR]], [[LATTICE_NORMAL_SUM_FCT_VECTOR]]
 */
void computeLatticeFunctionData(perl::Object cycle)
{
  // Extract properties from the cycle
  const Matrix<Rational> &linealitySpace_ref = cycle.give("LINEALITY_SPACE");
  const Matrix<Rational> linealitySpace = tdehomog(linealitySpace_ref);	
  const int lineality_dim = linealitySpace.rows();

  const Matrix<Rational> &rays_ref = cycle.give("SEPARATED_VERTICES");
  const Matrix<Rational> rays = tdehomog(rays_ref);
  const LatticeMap &latticeNormals = cycle.give("LATTICE_NORMALS");
  const Matrix<Rational> &normalsums_ref = cycle.give("LATTICE_NORMAL_SUM");
  const Matrix<Rational> normalsums = tdehomog(normalsums_ref);
  const IncidenceMatrix<> &codimOneCones = cycle.give("SEPARATED_CODIMENSION_ONE_POLYTOPES");
  const IncidenceMatrix<> &maximalCones = cycle.give("SEPARATED_MAXIMAL_POLYTOPES");
  const IncidenceMatrix<> &coneIncidences = cycle.give("MAXIMAL_AT_CODIM_ONE");

  // Result variables
  Map<std::pair<int,int>, Vector<Rational> > summap;
  ListMatrix<Vector<Rational> > summatrix;
  Vector<bool> balancedFaces(codimOneCones.rows());

  // Iterate over all codim 1 faces
  auto coInc = entire(rows(coneIncidences));
  for (auto fct = entire<indexed>(rows(codimOneCones)); !fct.at_end(); ++fct, ++coInc) {
    for (const auto &mc : coInc->top()) {
      Vector<Rational> normalvector(latticeNormals[std::make_pair(fct.index(),mc)]);
      normalvector = tdehomog_vec(normalvector);
      // Compute the representation of the normal vector
      summap[std::make_pair(fct.index(),mc)] =
        functionRepresentationVector(maximalCones.row(mc),
                                     normalvector,
                                     rays, linealitySpace);
    }

    // Now compute the representation of the sum of the normals
    try {
      summatrix /= functionRepresentationVector(*fct,
						normalsums.row(fct.index()),
						rays, linealitySpace);
    }
    catch (std::runtime_error &e) {
      // This goes wrong, if X is not balanced at a given codim 1 face
      summatrix /= zero_vector<Rational>(rays.rows() + lineality_dim);
    }
  }

  // Set fan properties
  cycle.take("LATTICE_NORMAL_FCT_VECTOR") << summap;
  cycle.take("LATTICE_NORMAL_SUM_FCT_VECTOR") << summatrix; 
} // END computeLatticeFunctionData


Matrix<Integer> lattice_basis_of_cone(const Matrix<Rational>& rays,
                                      const Matrix<Rational> &lineality, 
                                      int dim, bool has_leading_coordinate)
{
  // Special case: If the cone is full-dimensional, return the standard basis
  int ambient_dim = std::max(rays.cols(), lineality.cols()) - (has_leading_coordinate ? 1 : 0);
  if (dim == ambient_dim)
    return unit_matrix<Integer>(ambient_dim);

  // Compute span matrix if there is a leading coordinate 
  Matrix<Rational> span_matrix(0, ambient_dim);
  if (has_leading_coordinate) {
    std::pair<Set<int>, Set<int>> sortedVertices = far_and_nonfar_vertices(rays);
    span_matrix = rays.minor(sortedVertices.first, range_from(1)); 
    if(lineality.rows() > 0) span_matrix /= lineality.minor(All, range_from(1));
    int first = *(sortedVertices.second.begin());
    sortedVertices.second -= first;
    for (auto vtx = entire(rows(rays.minor(sortedVertices.second,All))); !vtx.at_end(); ++vtx) {
      span_matrix /= (*vtx - rays.row(first)).slice(range_from(1));
    }
  } else {
    span_matrix = rays / lineality;
  }

  // Compute span of cone
  Matrix<Rational> linspan = null_space(span_matrix);
  SparseMatrix<Integer> transformation =
    pm::hermite_normal_form( common::eliminate_denominators_in_rows(linspan),false).companion;

  // The last dim columns are a Z-basis for the cone
  return Matrix<Integer>(T(transformation.minor(All,sequence(transformation.cols()-dim,dim))));
} // END lattice_basis_of_cone

/*
 * @brief Computes properties [[LATTICE_BASES]] and [[LATTICE_GENERATORS]]
 */
void computeLatticeBases(perl::Object cycle)
{
  // Extract properties
  const Matrix<Rational> &rays_ref = cycle.give("VERTICES");
  const Matrix<Rational> rays = tdehomog(rays_ref).minor(All, range_from(1));
  const Matrix<Rational> &linspace_ref = cycle.give("LINEALITY_SPACE");
  const Matrix<Rational> linspace = tdehomog(linspace_ref).minor(All, range_from(1));
  const IncidenceMatrix<> &cones = cycle.give("MAXIMAL_POLYTOPES");
  // FIXME Use unimodularity?
  const Set<int> &directional = cycle.give("FAR_VERTICES");
  const Set<int> vertices = sequence(0,rays.rows()) - directional; 
  const int dim = cycle.give("PROJECTIVE_DIM");

  Matrix<Integer> generators;
  std::vector<Set<int>> bases;

  // Iterate over all cones
  for (auto mc = entire(rows(cones)); !mc.at_end(); ++mc) {
    // Compute a lattice basis for the cone:
    // Construct ray matrix of cone:
    Matrix<Rational> mc_rays = rays.minor((*mc) * directional, All);      
    Matrix<Rational> mc_vert = rays.minor((*mc) * vertices,All);
    for (int v = 1; v < mc_vert.rows(); v++) {
      mc_rays /= (mc_vert.row(v) - mc_vert.row(0));
    }

    Matrix<Integer> basis;
    // FIXME Use unimodularity?
    basis = lattice_basis_of_cone(mc_rays, linspace,dim,false);
    basis = zero_vector<Integer>(basis.rows()) | basis;

    Set<int> basis_set;
    // Add rays, looking for doubles
    for (auto b = entire(rows(basis)); !b.at_end(); ++b) {
      // We normalize s.t. the first non-zero entry is > 0
      auto first_non_zero = find_in_range_if(entire(b->top()), operations::non_zero());
      if (*first_non_zero < 0) b->negate();
      int ray_index = -1;
      for (auto gr = entire<indexed>(rows(generators)); !gr.at_end(); ++gr) {
        if (*gr == *b) {
          ray_index = gr.index(); break;
        }
      }
      if (ray_index == -1) {
        generators /= *b;
        ray_index = generators.rows()-1;
      }
      basis_set += ray_index;
    } // END go through basis elements
    bases.push_back(basis_set);
  } // END iterate over all maximal cones

  // Set properties
  cycle.take("LATTICE_GENERATORS") << thomog(generators);
  cycle.take("LATTICE_BASES") << bases;
}

Function4perl(&computeLatticeNormalSum,"computeLatticeNormalSum(Cycle)");
Function4perl(&computeLatticeFunctionData,"computeLatticeFunctionData(Cycle)");
Function4perl(&computeLatticeBases, "computeLatticeBases(Cycle)");
Function4perl(&lattice_basis_of_cone,"lattice_basis_of_cone(Matrix,Matrix,$,$)");

} }
