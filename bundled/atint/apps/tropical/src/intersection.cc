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
	Copyright (c) 2016-2021
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Implements the description of the intersection product in the projective
	torus given by Jensen and Yu.
	*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/Smith_normal_form.h"
#include "polymake/RandomGenerators.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/lattice.h"
#include "polymake/tropical/specialcycles.h"
#include "polymake/tropical/codim_one_with_locality.h"
#include "polymake/tropical/convex_hull_tools.h"
#include "polymake/tropical/star.h"

namespace polymake { namespace tropical { 

using StarResult = std::pair<Matrix<Rational>, std::vector<Set<Int>>>;

// Documentation see perl wrapper
Integer lattice_index(const Matrix<Integer>& lattice_rays)
{
  // Compute the Smith Normal form 
  SmithNormalForm<Integer> solution = smith_normal_form(lattice_rays);
  return abs(accumulate( solution.form.diagonal().slice(sequence(0,solution.rank)), operations::mul()));
}

///////////////////////////////////////////////////////////////////////////////////////

/**
   @brief Computes the Minkowski multiplicity of two fans.

   Find two cones that add up to full dimension.
   Then it chooses an interior vector in the difference and finds all cone differences containing it
   as an interior vector. For all these differences it adds the product of the weights
   times the lattice index of the sum of the lattices.
*/
Integer computeFanMultiplicity(const Matrix<Rational>& xrays, const Matrix<Rational>& xlin,
                               const std::vector<Set<Int>>& xcones, const Vector<Integer>& xweights,
                               const Int xdim,
                               const Matrix<Rational>& yrays, const Matrix<Rational>& ylin,
                               const std::vector<Set<Int>>& ycones, const Vector<Integer>& yweights,
                               const Int ydim)
{
  Integer weight(0);

  // First, we compute all H-representations of xcone - ycone, keeping
  // only full-dimensional ones
  std::vector<Matrix<Rational>> full_dimensional_cones;
  std::vector<Int> full_dimensional_xindex; //Keep track of associated x- and ycones
  std::vector<Int> full_dimensional_yindex;
  for (auto xc = entire<indexed>(xcones); !xc.at_end(); ++xc) {
    for (auto yc = entire<indexed>(ycones); !yc.at_end(); ++yc) {
      const auto x_sub_rays = remove_zero_rows(xrays.minor(*xc, All));
      const auto y_sub_rays = remove_zero_rows(-yrays.minor(*yc,All));
      const auto eqs = polytope::enumerate_facets((x_sub_rays / y_sub_rays), (xlin / ylin), true);
      if (eqs.second.rows() == 0) {
        full_dimensional_cones.push_back(eqs.first);
        full_dimensional_xindex.push_back(xc.index());
        full_dimensional_yindex.push_back(yc.index());
      }
    }
  }

  // If there are no full-dimensional cones, the result is 0
  if (full_dimensional_cones.size() == 0) return weight;

  // Otherwise, we need to compute a generic vector. We compute a 
  // random vector and go through all cones. We add up appropriate
  // weights for those cones containing the point in their interior.
  // If we find one that contains the point in its boundary, we 
  // create another interior point and try again.
  bool point_found;
  UniformlyRandom<Rational> random_gen;
  Vector<Rational> interior_point(xrays.cols());
  do {
    weight = 0;
    copy_range(random_gen.begin(), entire(interior_point));
    interior_point[0] = 1;
    point_found = true;
    // Now go through all full-dimensional cones
    for (auto fullcone = entire<indexed>(full_dimensional_cones); !fullcone.at_end(); ++fullcone) {
      // If the cone is the full space, i.e. has no facets, we don't need to check containment
      bool is_interior = true;
      bool is_in_boundary = false;
      if (fullcone->rows() > 0) {
        const Vector<Rational> eq_check = (*fullcone) * interior_point;
        for (const auto& c : eq_check) {
          if (c == 0) {
            is_in_boundary = true; break;
          }
          if (c < 0) {
            is_interior = false; break;
          }
        } //END check for interiorness
        // If its in the boundary of something, try another point.
        if (is_in_boundary) {
          point_found = false; break;
        }
      }
      // If it's in interior, add the appropriate weight.
      if (is_interior) {
        Integer latticeIndex = lattice_index(
           lattice_basis_of_cone(xrays.minor(xcones[full_dimensional_xindex[fullcone.index()]],All),xlin,xdim,false) / 
           lattice_basis_of_cone(yrays.minor(ycones[full_dimensional_yindex[fullcone.index()]],All),ylin,ydim,false));
        weight += (xweights[full_dimensional_xindex[fullcone.index()]] * yweights[full_dimensional_yindex[fullcone.index()]] * latticeIndex);
      }
    } //END iterate full-dimensional cones
  } while (!point_found);

  return weight;
}

///////////////////////////////////////////////////////////////////////////////////

// Documentation see perl wrapper
template <typename Addition>
ListReturn intersect_check_transversality(BigObject X, BigObject Y, bool ensure_transversality = false)
{
  // Extract values
  const Int Xcodim = X.give("PROJECTIVE_CODIMENSION");
  const Int Xdim   = X.give("PROJECTIVE_DIM");
  const Int Ycodim = Y.give("PROJECTIVE_CODIMENSION");
  const Int Ydim   = Y.give("PROJECTIVE_DIM");
  const Int Xambi  = X.give("PROJECTIVE_AMBIENT_DIM");

  // If the codimensions of the varieties add up to something larger then CMPLX_AMBIENT_DIM, return the 0-cycle 
  if (Xcodim + Ycodim > Xambi) {
    ListReturn zeroResult;
    zeroResult << empty_cycle<Addition>(Xambi);
    zeroResult << false;
    return zeroResult;
  }

  // Extract values
  const Matrix<Rational> &xrays_ref = X.give("VERTICES");
  const Matrix<Rational> xrays = tdehomog(xrays_ref);
  const Matrix<Rational> &xlin_ref = X.give("LINEALITY_SPACE");
  const Matrix<Rational> xlin = tdehomog(xlin_ref);
  const IncidenceMatrix<> xcones = X.give("MAXIMAL_POLYTOPES");
  const Vector<Integer> xweights = X.give("WEIGHTS");
  const Int xambdim = X.give("PROJECTIVE_AMBIENT_DIM");

  const Matrix<Rational> &yrays_ref = Y.give("VERTICES");
  const Matrix<Rational> yrays = tdehomog(yrays_ref);
  const Matrix<Rational> &ylin_ref = Y.give("LINEALITY_SPACE");
  const	Matrix<Rational> ylin = tdehomog(ylin_ref);
  const IncidenceMatrix<> ycones = Y.give("MAXIMAL_POLYTOPES");
  const Vector<Integer> yweights = Y.give("WEIGHTS");
  const Int yambdim = Y.give("PROJECTIVE_AMBIENT_DIM");

  if (xambdim != yambdim) {
    throw std::runtime_error("Cannot compute intersection product: Cycles live in different spaces.");
  }

  // Compute the expected dimension of the intersection product 
  Int k = Xambi - (Xcodim + Ycodim);

  // Compute the intersection complex
  fan_intersection_result f = fan_intersection(xrays,xlin,xcones,yrays,ylin,ycones);

  // Now we compute the k-skeleton of the intersection complex together with the data of original maximal
  // cones containing these cones
  const Matrix<Rational> &interrays = f.rays;
  const Matrix<Rational> &interlin = f.lineality_space;
  Int i_lineality_dim = rank(f.lineality_space);
  std::vector<Set<Int>> intercones;
  std::vector<Set<Int>> xcontainers;
  std::vector<Set<Int>> ycontainers;

  bool is_transversal = true;

  for (auto ic = entire<indexed>(rows(f.cones)); !ic.at_end(); ++ic) {
    // Check that the cone dimension is at least the expected dimension
    Int cone_dim = rank(interrays.minor(*ic,All)) + i_lineality_dim -1;
    if (cone_dim >= k) {
      if (cone_dim > k) {
        is_transversal = false;
        if (ensure_transversality) { 
          ListReturn zeroResult;
          zeroResult << empty_cycle<Addition>(Xambi);
          zeroResult << false;
          return zeroResult;
        }
      }

      // Now we compute the k-skeleton of the intersection cone
      Vector<Set<Int>> singlecone(1); singlecone[0] = *ic;
      IncidenceMatrix<> k_skeleton_matrix(singlecone);
      for (Int i = cone_dim; i > k; --i) {
        k_skeleton_matrix = 
          calculateCodimOneData(interrays, k_skeleton_matrix, interlin, IncidenceMatrix<>()).codimOneCones;
      }

      // Go through all cones and add them (if they haven't already been added)
      for (auto kc = entire(rows(k_skeleton_matrix)); !kc.at_end(); ++kc) {
        Int cone_index = -1;
        for (auto oc = entire<indexed>(intercones); !oc.at_end(); ++oc) {
          // Since both cones have the same dimension, it suffices to check, whether the old cone
          // is contained in the new cone
          if (incl( *oc, *kc) <= 0) {
            cone_index = oc.index();
            break;
          }
        }
        // If it doesn't exist yet, add it
        if (cone_index == -1) {
          intercones.push_back(*kc);
          xcontainers.push_back(Set<Int>());
          ycontainers.push_back(Set<Int>());
          cone_index = intercones.size()-1;
        }

        // Now add containers
        xcontainers[cone_index] += f.xcontainers.row(ic.index());
        ycontainers[cone_index] += f.ycontainers.row(ic.index());

      } //END iterate all k-skeleton cones

    } //END if cone_dim >= k
  } //END iterate intersection cones

  // If no cones remain, return the zero cycle
  if (intercones.empty()) {
    ListReturn zeroResult;
    zeroResult << empty_cycle<Addition>(Xambi);
    zeroResult << false;
    return zeroResult;
  }

  // Now we compute weights
  Vector<Integer> weights(intercones.size());
  Set<Int> weight_zero_cones;

  Matrix<Rational> xlin_dehom = xlin.minor(All, range_from(1));
  Matrix<Rational> ylin_dehom = ylin.minor(All, range_from(1));

  for (auto c = entire<indexed>(intercones); !c.at_end(); ++c) {
    // Find interior point
    Vector<Rational> interior_point = accumulate(rows(interrays.minor(*c,All)),operations::add());
    Rational count_vertices = accumulate(interrays.col(0).slice(*c),operations::add());
    if (count_vertices != 0) interior_point /= count_vertices;

    // Compute stars
    Matrix<Rational> xstar_rays, ystar_rays;
    std::vector<Set<Int>> xstar_cones, ystar_cones;

    StarResult starx = computeStar(interior_point, xrays, xcones.minor(xcontainers[c.index()],All));
    StarResult stary = computeStar(interior_point, yrays, ycones.minor(ycontainers[c.index()],All));
    xstar_rays = starx.first; xstar_cones = starx.second;
    ystar_rays = stary.first; ystar_cones = stary.second;

    Integer w = computeFanMultiplicity(xstar_rays, xlin_dehom, xstar_cones, xweights.slice(xcontainers[c.index()]), Xdim,
                                       ystar_rays, ylin_dehom, ystar_cones, yweights.slice(ycontainers[c.index()]), Ydim);

    weights[c.index()] = w;
    if (w == 0) weight_zero_cones += c.index();
  }

  // Check if any cones remain
  if (weight_zero_cones.size() == Int(intercones.size())) {
    ListReturn zeroResult;
    zeroResult << empty_cycle<Addition>(Xambi);
    zeroResult << false;
    return zeroResult;
  }

  // Clean up rays and cones

  IncidenceMatrix<> intercones_matrix(intercones);
  intercones_matrix = intercones_matrix.minor(~weight_zero_cones,All);
  Set<Int> used_rays = accumulate(rows(intercones_matrix), operations::add());
  intercones_matrix = intercones_matrix.minor(All,used_rays);
  weights = weights.slice(~weight_zero_cones);

  // Finally create the result  
  BigObject result("Cycle", mlist<Addition>(),
                   "VERTICES", thomog(interrays.minor(used_rays, All)),
                   "MAXIMAL_POLYTOPES", intercones_matrix,
                   "LINEALITY_SPACE", thomog(interlin),
                   "WEIGHTS", weights);

  ListReturn positiveResult;
  positiveResult << result;
  positiveResult << is_transversal;
  return positiveResult;
}

// ------------------------- PERL WRAPPERS ---------------------------------------------------

UserFunction4perl("# @category Lattices"
                  "# This computes the index of a lattice in its saturation."
                  "# @param Matrix<Integer> m A list of (row) generators of the lattice."
                  "# @return Integer The index of the lattice in its saturation.",
                  &lattice_index,"lattice_index(Matrix<Integer>)");

UserFunctionTemplate4perl("# @category Intersection theory"
                          "# Computes the intersection product of two tropical cycles in R^n and tests whether the intersection is transversal (in the sense that the cycles intersect set-theoretically in the right dimension)."
                          "# @param Cycle X A tropical cycle"
                          "# @param Cycle Y A tropical cycle, living in the same space as X"
                          "# @param Bool ensure_transversality Whether non-transversal intersections should not be computed. Optional and false by default. If true,"
                          "# returns the zero cycle if it detects a non-transversal intersection"
                          "# @return List( Cycle intersection product, Bool is_transversal)."
                          "#  Intersection product is a zero cycle if ensure_transversality is true and the intersection is not transversal."
                          "#  //is_transversal// is false if the codimensions of the varieties add up to more than the ambient dimension.",
                          "intersect_check_transversality<Addition>(Cycle<Addition>,Cycle<Addition>; $=0)");

InsertEmbeddedRule("# @category Intersection theory"
                   "# Computes the intersection product of two tropical cycles in the projective torus"
                   "# Use [[intersect_check_transversality]] to check for transversal intersections"
                   "# @param Cycle X A tropical cycle"
                   "# @param Cycle Y A tropical cycle, living in the same ambient space as X"
                   "# @return Cycle The intersection product\n"
                   "user_function intersect<Addition>(Cycle<Addition>,Cycle<Addition>) {\n"
                   "	my ($X,$Y) = @_;\n"
                   "	my @r = intersect_check_transversality($X,$Y);\n"
                   "	return $r[0];\n"
                   "}\n");

FunctionTemplate4perl("computeStar(Vector,Matrix,IncidenceMatrix)");

} }
