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

	Implements lines_in_cubic_reachable.h
	*/


#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Polynomial.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/divisor.h"
#include "polymake/tropical/convex_hull_tools.h"
#include "polymake/tropical/lines_in_cubic_reachable.h"
#include "polymake/tropical/linear_algebra_tools.h"
#include "polymake/tropical/rational_function.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/misc_tools.h"
#include "polymake/tropical/lines_in_cubic_reachable.h"

namespace polymake { namespace tropical {

bool maximumAttainedTwice(const Vector<Rational>& values)
{
  if (values.dim() <= 1) return false;
  Rational max = values[0];
  Int count = 1;
  for (Int j = 1; j < values.dim(); j++) {
    if (values[j] > max) {
      max = values[j]; count = 1; continue;
    }
    if (values[j] == max) count++;
  }
  return count >= 2;
}	

ReachableResult reachablePoints(const Polynomial<TropicalNumber<Max>>& f, BigObject X, Int direction)
{
  // Extract values
  BigObject ratfct = call_function("rational_fct_from_affine_numerator", f);
  BigObject lindom = ratfct.give("DOMAIN");
  // Create matrix: Put coefficients in front of monomials

  Matrix<Rational> funmat(f.monomials_as_matrix());
  Vector<Rational> funcoeffs(f.coefficients_as_vector());
  // Rearrange so that values of terms on points can be computed as product with matrix
  funmat = funcoeffs | funmat;

  IncidenceMatrix<> cones = X.give("MAXIMAL_POLYTOPES");
  IncidenceMatrix<> codim = X.give("CODIMENSION_ONE_POLYTOPES");
  IncidenceMatrix<> codim_by_max = X.give("MAXIMAL_AT_CODIM_ONE");
  codim_by_max = T(codim_by_max);
  Matrix<Rational> rays = X.give("VERTICES");
  rays = tdehomog(rays);
  Set<Int> vertices = far_and_nonfar_vertices(rays).second;

  const Matrix<Rational> degree = zero_vector<Rational>(4) | (ones_vector<Rational>(3) / -unit_matrix<Rational>(3));
  if (direction < 0 || direction > 3) {
    throw std::runtime_error("Wrong direction index. Must lie in 0,1,2,3");
  }

  // Find the ray in X corresponding to the chosen directions
  Int dir_index = -1;
  for (Int r = 0; r < rays.rows(); ++r) {
    if (rays.row(r) == degree.row(direction)) {
      dir_index = r; break;
    }
  }
  if (dir_index == -1) {
    throw std::runtime_error("Cannot find direction ray in surface. Maybe not a cubic?");
  }

  // Find all cones that use the chosen direction and
  // keep only the codimension one locus of theses
  Set<Int> d_edges;
  for (const Int c : cones.col(dir_index)) {
    d_edges += codim_by_max.row(c);
  }

  // For each of these edges, add +/-direction as lineality
  // Then refine the resulting complex along the linearity domains of f
  Set<Int> used_rays_in_edges = accumulate(rows(codim.minor(d_edges,All)),operations::add());
  Matrix<Rational> d_rays = rays.minor(used_rays_in_edges,All);
  IncidenceMatrix<> d_cone_matrix = codim.minor(d_edges,used_rays_in_edges);
  Vector<Set<Int>> d_cone_list;
  for (Int dc = 0; dc < d_cone_matrix.rows(); ++dc) {
    d_cone_list |= d_cone_matrix.row(dc);
  }
  Matrix<Rational> d_lineality(0,degree.cols());
  d_lineality /= degree.row(direction);
  BigObject d_complex("Cycle", mlist<Max>(),
                      "VERTICES", thomog(d_rays),
                      "MAXIMAL_POLYTOPES", d_cone_list,
                      "LINEALITY_SPACE", thomog(d_lineality),
                      "PURE", false);
  d_complex = call_function("intersect_container", d_complex, lindom);

  // We now go through all (new) vertices of this complex and "project" them onto the one-dimensional
  // complex of the edges defined by d_edges(i.e. we find an edge intersecting the ray (vertex + R*direction))
  Matrix<Rational> d_ref_rays = d_complex.give("VERTICES");
  d_ref_rays = tdehomog(d_ref_rays);
  const Set<Int> d_ref_vertices = far_and_nonfar_vertices(d_ref_rays).second;
  const Set<Int> old_vertices = used_rays_in_edges * vertices;
  for (const Int vert : d_ref_vertices) {
    // First we check if this vertex is an "old" vertex, i.e. already exists in the d_edges complex
    Int old_index = -1;
    for (const Int oldvert : old_vertices) {
      if (rays.row(oldvert) == d_ref_rays.row(vert)) {
        old_index = oldvert; break;
      }
    }
    if (old_index >= 0) continue;

    // Otherwise we go through all edges of d_edges until we find one that intersects (vertex + R*direction)
    // This is computed as follows: Assume p is a vertex of an edge of d_edges and that w is the direction
    // from p into that edge (either a ray or p2-p1, if the edge is bounded). Then we compute a linear representation of (vertex - p_1) in terms of w and
    // direction. If a representation exists and the second edge generator is also a vertex, we have to check
    // that the coefficient of w = p2-p1 is in between 0 and 1, otherwise it has to be > 0
    //
    // If we find an intersecting edge, we refine it (in d_cone_list) such that it contains the intersection point.
    for (Int ed = 0; ed < d_cone_list.dim(); ++ed) {
      Matrix<Rational> edge_generators = d_rays.minor(d_cone_list[ed],All);
      Vector<Rational> p1 = edge_generators(0,0) == 0 ? edge_generators.row(1) : edge_generators.row(0);
      bool bounded = edge_generators(0,0) == edge_generators(1,0);
      Vector<Rational> w;
      if (bounded)
        w = edge_generators.row(1) - edge_generators.row(0);
      else
        w = edge_generators(0,0) == 0 ? edge_generators.row(0) : edge_generators.row(1);
      Vector<Rational> lin_rep = linearRepresentation(d_ref_rays.row(vert) - p1, vector2row(w) / degree.row(direction));
      // Check that:
      // - There is a representation
      // - The coefficient of w is > 0 (and < 1 if bounded)
      if (lin_rep.dim() > 0) {
        if (lin_rep[0] > 0 && (!bounded || lin_rep[0] < 1)) {
          // Then we add a vertex, stop searching for an edge and go to the next vertex
          Vector<Rational> new_vertex = p1 + lin_rep[0]*w;
          d_rays /= new_vertex;
          Vector<Int> edge_index_list(d_cone_list[ed]);
          d_cone_list = d_cone_list.slice(~scalar2set(ed));
          const Set<Int> one_cone{ edge_index_list[0], d_rays.rows()-1 };
          const Set<Int> other_cone{ edge_index_list[1], d_rays.rows()-1 };
          d_cone_list |= one_cone;
          d_cone_list |= other_cone;
          break;
        }
      }
    } // END intersect with edges.
  } // END project vertices

  // Now take the refined edge complex, add lineality again and intersect it with d_complex to get
  // the final refined complex
  BigObject d_vert_complex("Cycle", mlist<Max>(),
                           "VERTICES", thomog(d_rays),
                           "MAXIMAL_POLYTOPES", d_cone_list,
                           "LINEALITY_SPACE", thomog(d_lineality),
                           "PURE", false);
  BigObject final_refined = call_function("intersect_container", d_vert_complex, d_complex);
  Matrix<Rational> final_rays = final_refined.give("VERTICES");
  Set<Int> final_vertices = far_and_nonfar_vertices(final_rays).second;
  IncidenceMatrix<> final_cones = final_refined.give("MAXIMAL_POLYTOPES");
  IncidenceMatrix<> final_codim = final_refined.give("CODIMENSION_ONE_POLYTOPES");
  IncidenceMatrix<> final_co_in_max = final_refined.give("MAXIMAL_AT_CODIM_ONE");
  IncidenceMatrix<> final_max_to_co = T(final_co_in_max);
  final_rays = tdehomog(final_rays);
  Int final_direction_index = -1;
  for (Int fr = 0; fr < final_rays.rows(); ++fr) {
    if (final_rays.row(fr) == degree.row(direction)) {
      final_direction_index = fr; break;
    }
  }
	
  // First find all codimension one cells (i.e. edges) whose span is generated by direction
  Set<Int> direction_edges;
  for (Int cc = 0; cc < final_codim.rows(); ++cc) {
    Matrix<Rational> cc_rays = final_rays.minor(final_codim.row(cc),All);
    Vector<Rational> cc_span;
    if (cc_rays(0,0) == cc_rays(1,0))
      cc_span = cc_rays.row(0) - cc_rays.row(1);
    else
      cc_span = cc_rays(0,0) == 0 ? cc_rays.row(0) : cc_rays.row(1);
    if (direction == 0) {
      if (cc_span[1] == cc_span[2] && cc_span[2] == cc_span[3])
        direction_edges += cc;
    } else {
      Vector<Int> entries_to_check(sequence(1,3) - direction);
      if (cc_span[entries_to_check[0]] == 0 && cc_span[entries_to_check[1]] == 0)
        direction_edges += cc;
    }
  }
  Set<Int> codimension_blacklist; // This will also later include edges we already used for iteration
  codimension_blacklist += direction_edges;

  // Find all maximal cells which have direction as ray and use them to iterate the refined complex:
  // In each step we find all maximal cells, that share an edge "in the right direction" with one
  // of the currently selected cells and whose interior point is in the support of f. These cells
  // then serve as reference in the next iteration step. An edge in the right direction is an edge, whose
  // span is not generated by direction and which has not been chosen before as edge
  //
  // Also, in each iteration we keep track of vertices lying in "edges in right direction", whose adjacent
  // cell is not in the support of f. Afterwards we apply a similar procedure to all such vertices
  // that are not in the reachable complex yet to find the one-dimensional part of this complex.
  Set<Int> reachable_2_cells; //Indices of maximal cells that are "reachable"
  Set<Int> problematic_vertices; //Vertices we might have to check afterwards to compute 1-dim. part
  Set<Int> reference_cells = T(final_cones).row(final_direction_index);
  while (!reference_cells.empty()) {
    reachable_2_cells += reference_cells;
    // First compute all edges in right direction
    Set<Int> edges_in_direction = accumulate(rows(final_max_to_co.minor(reference_cells,All)),operations::add()) -
      codimension_blacklist;
    // ... and add them to edges we don't need in the future
    codimension_blacklist += edges_in_direction;
    // For each of these edges, compute the adjacent cell in the correct direction and check if its
    // in the support of X
    Set<Int> next_reference_cells;
    for (auto eid = entire(edges_in_direction); !eid.at_end(); ++eid) {
      Int adjacent_cell =  *( (final_co_in_max.row(*eid) - reference_cells).begin());
      // Compute interior vector of adjacent cell
      Vector<Rational> ip = accumulate(rows(final_rays.minor(final_cones.row(adjacent_cell),All)), operations::add()) /
                            (final_cones.row(adjacent_cell) * final_vertices).size();
      //Compute values of f on ip
      Vector<Rational> ipv = funmat * ip;
      // If the cell is in the support of X, add it to the set of reference cells for the next iteration
      // Otherwise keep the vertices as potentially problematic
      if (maximumAttainedTwice(ipv)) {
        next_reference_cells += adjacent_cell;
      } else {
        problematic_vertices += final_codim.row(*eid) * final_vertices;
      }
    } // END iterate edges in direction
    reference_cells = next_reference_cells;
  } // END iterate 2-cells

  // Now we have already computed the 2-dimensional part of the reachable set.

  // Remove from the "problematic" vertices all those that were not problematic after all:
  // A vertex is only problematic, if it is only contained in one-valent codimension one faces of reach_two
  // (counting only those not in the span of the direction)
  Set<Int> not_so_problematic_vertices;
  Set<Int> reach_two_codim_indices = accumulate(rows(final_max_to_co.minor(reachable_2_cells,All)),operations::add());
  Set<Int> reach_two_codim_nospan_indices = reach_two_codim_indices - direction_edges;
  IncidenceMatrix<> reach_two_codim = final_codim.minor(reach_two_codim_nospan_indices,All);
  IncidenceMatrix<> reach_two_co_in_max = final_co_in_max.minor(reach_two_codim_nospan_indices,reachable_2_cells);
  for (auto pv = entire(problematic_vertices); !pv.at_end(); ++pv) {
    for (const Int cocodim : reach_two_codim.col(*pv)) {
      if (reach_two_co_in_max.row(cocodim).size() > 1) {
        not_so_problematic_vertices += *pv; continue;
      }
    }
  } // END sort out non-problematic vertices
  problematic_vertices -= not_so_problematic_vertices;

  // Now we apply the same procedure as before. This time we have edges as reference cells
  // We start with the edges in direction span containing our problematic vertices that "point away"
  // from the reach-2-locus (i.e. are not a codimension one face of it)
  Set<Int> interesting_dir_edges = direction_edges - reach_two_codim_indices; // Set of all dir edges not in reach_two
  Set<Int> reference_edges = accumulate(rows(T(final_codim).minor(problematic_vertices,All)),operations::add());
  reference_edges *= interesting_dir_edges;

  Set<Int> blacklisted_edges = reach_two_codim_indices;
  Set<Int> reachable_1_cells;

  // Go through all reference edges. Test on each one, if it lies in the support. If so, we take
  // the "next" edge as a new reference edge.
  while (!reference_edges.empty()) {
    blacklisted_edges += reference_edges;
    Set<Int> next_reference_edges;
    for (auto re = entire(reference_edges); !re.at_end(); ++re) {
      // Compute an interior point and compute values on it
      Vector<Rational> ip = accumulate(rows(final_rays.minor(final_codim.row(*re),All)), operations::add()) /
        (final_codim.row(*re) * final_vertices).size();
      Vector<Rational> ipv = funmat * ip;
      if (maximumAttainedTwice(ipv)) {
        reachable_1_cells += *re;
        // Find the "next" edge, i.e. a direction edge, sharing a vertex with this edge
        // that is NOT a codimension one edge of reach_two and that is NOT blacklisted
        next_reference_edges += (accumulate(rows(T(final_codim).minor(final_codim.row(*re)*final_vertices,All)),operations::add())*direction_edges) - blacklisted_edges;
      }
    } // END iterate reference edges
    reference_edges = next_reference_edges;
  } // END iterate 1-cells

  //Now we also have the one-dim. part
  Set<Int> all_rays_used =
    accumulate(rows(final_cones.minor(reachable_2_cells,All)),operations::add()) +
    accumulate(rows(final_codim.minor(reachable_1_cells,All)),operations::add());
  Matrix<Rational> reachable_rays = final_rays.minor(all_rays_used,All);
  IncidenceMatrix<> reach_cells = final_cones.minor(reachable_2_cells, all_rays_used);
  IncidenceMatrix<> reach_edges = final_codim.minor(reachable_1_cells, all_rays_used);

  ReachableResult result;
  result.rays = reachable_rays;
  result.cells = reach_cells;
  result.edges = reach_edges;
  return result;
}

} }
