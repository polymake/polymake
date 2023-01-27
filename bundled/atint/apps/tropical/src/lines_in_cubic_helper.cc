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

	Implements lines_in_cubic_helper.h
	*/

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/codim_one_with_locality.h"
#include "polymake/tropical/divisor.h"
#include "polymake/tropical/linear_algebra_tools.h"
#include "polymake/tropical/lines_in_cubic_reachable.h"
#include "polymake/tropical/lines_in_cubic_helper.h"
#include "polymake/tropical/lines_in_cubic_data.h"
#include "polymake/polytope/convex_hull.h"


namespace polymake { namespace tropical {

/**
   @brief Takes a fan_intersection_result and cleans up the result so that no cone is contained in another and that cones are sorted according to their dimension.
   @param fan_intersection_result fir
   @return DirectionIntersection
*/
DirectionIntersection cleanUpIntersection(const fan_intersection_result& fir)
{
  DirectionIntersection result;
  result.rays = fir.rays;
  IncidenceMatrix<> fir_cones = fir.cones;
  // First we sort all cells according to their dimension
  Set<Int> cell_set;
  Set<Int> edge_set;
  Set<Int> point_set;
  for (Int fc = 0; fc < fir_cones.rows(); ++fc) {
    if (fir_cones.row(fc).size() > 2) cell_set += fc;
    if (fir_cones.row(fc).size() == 2) edge_set += fc;
    if (fir_cones.row(fc).size() == 1) point_set += fc;
  }
  // Go through all edges, compare to cells, remove redundant ones
  Set<Int> redundant_edges;
  for (auto e = entire(edge_set); !e.at_end(); ++e) {
    bool found_container = false;
    for (auto c = entire(cell_set); !c.at_end(); ++c) {
      if ( (fir_cones.row(*c) * fir_cones.row(*e)).size() == fir_cones.row(*e).size()) {
        found_container = true;
        break;
      }
    }
    if (found_container) redundant_edges += (*e);
  }
  edge_set -= redundant_edges;

  // Same for points
  Set<Int> redundant_points;
  for (auto p = entire(point_set); !p.at_end(); ++p) {
    bool found_container = false;
    Set<Int> containers = cell_set + edge_set;
    for (auto c = entire(containers); !c.at_end(); ++c) {
      if ( (fir_cones.row(*c) * fir_cones.row(*p)).size() == fir_cones.row(*p).size()) {
        found_container = true;
        break;
      }
    }
    if (found_container) redundant_points += (*p);
  }
  point_set -= redundant_points;

  result.cells = fir_cones.minor(cell_set,All);
  result.edges = fir_cones.minor(edge_set,All);
  result.points = fir_cones.minor(point_set,All);

  return result;
} // END cleanUpIntersection

/**
   @brief This takes a (two-dimensional) cone in R^3 in terms of a subset of rays and computes all codimension one faces.
   @return A FacetData object (see above)
*/
FacetData computeFacets(const Matrix<Rational>& rays, const Set<Int>& cone)
{
  // Compute facet equations and store them
  const auto ceq = polytope::enumerate_facets(rays.minor(cone, All), false);
  FacetData result;
  result.eq = ceq.second.row(0);
  result.ineqs = ceq.first;
  // Now go through all inequalities and find the corresponding vertices
  Set<Int> exclude_ineqs;
  RestrictedIncidenceMatrix<> facets;
  for (auto ineq=entire<indexed>(rows(result.ineqs)); !ineq.at_end(); ++ineq) {
    Set<Int> facet;
    bool vertex_seen=false;
    for (auto c=entire(cone); !c.at_end(); ++c) {
      if (is_zero((*ineq) * rays.row(*c))) {
        facet.push_back(*c);
        if (!vertex_seen && !is_zero(rays(*c, 0))) {
          vertex_seen=true;
        }
      }
    }
    // Each facet has to have at least one vertex
    if (vertex_seen) {
      facets /= facet;
    } else {
      exclude_ineqs += ineq.index();
    }
  }
  result.facets = std::move(facets);
  if (!exclude_ineqs.empty()) {
    result.ineqs = result.ineqs.minor(~exclude_ineqs, All);
  }
  return result;
}

// ------------------------------------------------------------------------------------------------

/**
   @brief This takes a vertex in the cubic (whose function's domain is describes by frays and fcones) and a direction and computes the vertex w farthest away from vertex in this direction, such that the convex hull of vertex and w still lies in X. It returns the empty vertex, if the complete half-line lies in X.
*/
Vector<Rational> maximalDistanceVector(const Vector<Rational>& vertex,
                                       const Vector<Rational>& direction,
                                       const Matrix<Rational>& frays,
                                       const IncidenceMatrix<>& fcones,
                                       const Matrix<Rational>& funmat)
{
  // Create the one-dimensional half-line from vertex
  Matrix<Rational> hl_rays = vector2row(vertex) / direction;
  IncidenceMatrix<> hl_cones({ { 0, 1 } });
  Matrix<Rational> lin(0, hl_rays.cols());
  //Intersect with f-domain
  DirectionIntersection ref_line =
    cleanUpIntersection(fan_intersection(hl_rays, lin, hl_cones, frays,lin,fcones));
  // Find vertex
  Int v_index = -1;
  for (Int r = 0; r < ref_line.rays.rows(); ++r) {
    if (ref_line.rays.row(r) == vertex) {
      v_index = r;
      break;
    }
  }
  IncidenceMatrix<> rays_in_cones = T(ref_line.edges);
  // Go through edges, starting at vertex and check if it is contained in X
  Int current_edge = *(rays_in_cones.row(v_index).begin());
  Int current_vertex = v_index;
  // When the current edge has only one vertex, we're done
  do {
    // Check if the edge lies in X
    Vector<Rational> interior_point =
      accumulate(rows(ref_line.rays.minor(ref_line.edges.row(current_edge),All)), operations::add()) /
      accumulate(ref_line.rays.minor(ref_line.edges.row(current_edge),All).col(0),operations::add());
    if (!maximumAttainedTwice(funmat * interior_point)) {
      return ref_line.rays.row(current_vertex);
    } else {
      current_vertex = *( (ref_line.edges.row(current_edge) - current_vertex).begin());
      if (ref_line.rays.row(current_vertex)[0] == 0) {
        current_vertex = -1;
      } else {
        current_edge = *( (rays_in_cones.row(current_vertex) - current_edge).begin());
      }
    }
  } while(current_vertex >= 0);

  return Vector<Rational>();
}

// ------------------------------------------------------------------------------------------------

/**
   @brief Takes two vectors v1 and v2 and a standard direction e_i + e_j for i,j in {0,..,3} and computes the rational number r, such that v1 + r*direction = v2. Returns also zero, if no such number exists. It also accepts a 0-dimensional vertex for v2 and will return 0 in that case.
*/
Rational vertexDistance(const Vector<Rational>& v1, const Vector<Rational>& v2, const Vector<Rational>& direction)
{
  if (v2.dim() == 0) return 0;
  Vector<Rational> diff = v2 - v1;
  Rational div = 0;
  for (Int i = 1; i <= 3; ++i) {
    if ((diff[i] == 0 && direction[i] != 0) || (diff[i] != 0 && direction[i] == 0))
      return 0;
    if (diff[i] != 0) {
      Rational d = diff[i] / direction[i];
      if (div == 0) {
        div = d;
      } else {
        if (d != div) return 0;
      }
    }
  }
  return div;
}

// ------------------------------------------------------------------------------------------------

/**
   @brief Takes a vertex family and computes the index of the standard direction in 0,..,3 corresponding to its edge
*/
Int vertexFamilyDirection(const VertexFamily& f)
{
  Vector<Rational> dir;
  if (f.edge(0,0) == 0) dir = f.edge.row(0);
  if (f.edge(1,0) == 0) dir = f.edge.row(1);
  if (dir.dim() == 0) dir = f.edge.row(0) - f.edge.row(1);
  if (dir[1] == 0 && dir[2] == 0) return 3;
  if (dir[1] == 0 && dir[3] == 0) return 2;
  if (dir[2] == 0 && dir[3] == 0) return 1;
  return 0;
}

// ------------------------------------------------------------------------------------------------

/**
   @brief Computes all edge families lying in a 2-dimensional cone for a given direction
   @param DirectionIntersection cone A cone, refined along f
   @param Matrix<Rational> z_border The intersection of the cone with a cone in the 0-i-rechable locus
   @param Matrix<Rational> c_border The intersection of the cone with a cone in the j-k-reachable locus
   @param Int leafAtZero The index of the leaf together with 0
   @param Matrix<Rational> funmat The function matrix of f, made compatible for vector multiplication
   @return LinesInCellResult A list of all edge families and edge lines lying in the cone
*/
LinesInCellResult computeEdgeFamilies(const DirectionIntersection& cone,
                                      const Matrix<Rational>& z_border,
                                      const Matrix<Rational>& c_border,
                                      Int leafAtZero,
                                      const Matrix<Rational>& funmat)
{
  Matrix<Rational> degree = -unit_matrix<Rational>(3);
  degree = ones_vector<Rational>(3) / degree;
  degree = zero_vector<Rational>(4) | degree;

  // First we project all vertices of the cone onto z_border
  Matrix<Rational> z_edge_rays = z_border;
  RestrictedIncidenceMatrix<> z_edges_grow={ {0, 1} };
  Vector<Rational> direction = degree.row(0) + degree.row(leafAtZero);
  Vector<Int> rem(sequence(1,3) - leafAtZero);

  for (Int dr = 0; dr < cone.rays.rows(); ++dr) {
    if (cone.rays(dr,0) == 1) {
      // We go through all edges of z_edges until we find one that intersects (vertex + R_>=0 *direction)
      // This is computed as follows: Assume p is a vertex of an edge of z_edges and that w is the direction
      // from p into that edge (either a ray or p2-p1, if the edge is bounded). Then we compute a linear representation of (vertex - p_1) in terms of w and
      // direction. If a representation exists and the second edge generator is also a vertex, we have to check
      // that the coefficient of w = p2-p1 is in between 0 and 1, otherwise it has to be > 0
      //
      // If we find an intersecting edge, we refine it (in z_edges) such that it contains the intersection point.
      for (Int ze = 0; ze < z_edges_grow.rows(); ++ze) {
        Matrix<Rational> edge_generators = z_border.minor(z_edges_grow.row(ze), All);
        Vector<Rational> p1 = edge_generators(0,0) == 0? edge_generators.row(1) : edge_generators.row(0);
        bool bounded = edge_generators(0,0) == edge_generators(1,0);
        Vector<Rational> w;
        if (bounded) {
          w = edge_generators.row(1) - edge_generators.row(0);
        } else {
          w = edge_generators(0,0) == 0 ? edge_generators.row(0) : edge_generators.row(1);
        }
        Vector<Rational> lin_rep = linearRepresentation(cone.rays.row(dr) - p1, vector2row(w) / direction);
        // Check that:
        //  - There is a representation
        //  - The coefficient of w is > 0 (and < 1 if bounded)
        if (lin_rep.dim() > 0) {
          if (lin_rep[0] > 0 && (!bounded || lin_rep[0] < 1)) {
            // Then we add a vertex, stop searching for an edge and go to the next vertex
            Vector<Rational> new_vertex = p1 + lin_rep[0]*w;
            z_edge_rays /= new_vertex;
            auto edge_index_list=z_edges_grow.row(ze).begin();
            const Set<Int> one_cone={ *edge_index_list, z_edge_rays.rows()-1 };
            const Set<Int> other_cone={ *++edge_index_list, z_edge_rays.rows()-1 };
            z_edges_grow.row(ze)=one_cone;
            z_edges_grow /= other_cone;
            break;
          }
        } // END if linear rep exists
      } // END iterate z_edges
    } // END if vertex
  } // END project vertices

  const IncidenceMatrix<> z_edges(std::move(z_edges_grow));
  // Then refine the cone along the new z_edges - direction
  // and compute codim one data
  Matrix<Rational> dummy_lineality(0, z_border.cols());
  DirectionIntersection refined_cone =
    cleanUpIntersection(fan_intersection(cone.rays, dummy_lineality, cone.cells,
                                         z_edge_rays, dummy_lineality / direction, z_edges));
  CodimensionOneResult codimData =
    calculateCodimOneData(refined_cone.rays, refined_cone.cells,  dummy_lineality, IncidenceMatrix<>());
  IncidenceMatrix<> codim =codimData.codimOneCones;
  IncidenceMatrix<> rayInCo = T(codim);
  IncidenceMatrix<> coInMax = codimData.codimOneInMaximal;
  IncidenceMatrix<> maxInCo = T(coInMax);

  // Find all edges that span direction
  Set<Int> direction_edges;
  for (Int cc = 0; cc < codim.rows(); ++cc) {
    Matrix<Rational> cc_rays = refined_cone.rays.minor(codim.row(cc),All);
    Vector<Rational> cc_span;
    if (cc_rays(0,0) == cc_rays(1,0)) {
      cc_span = cc_rays.row(0) - cc_rays.row(1);
    } else {
      cc_span = cc_rays(0,0) == 0 ? cc_rays.row(0) : cc_rays.row(1);
    }
    if (cc_span[leafAtZero] == 0 && cc_span[rem[0]] == cc_span[rem[1]])
      direction_edges += cc;
  }
  Set<Int> remaining_codim = sequence(0,codim.rows()) - direction_edges;

  // Now we go through all z_edges, find the corresponding codim cone in the refined cone
  // and check if the complete "path" to the other side lies in X. In addition we keep track of all
  // vertices of z_edges that do not lie in a 2-dim path to the other side and have to be checked
  // for isolated solutions
  Vector<EdgeFamily> family_list(0);
  Vector<EdgeLine> line_list(0);
  Vector<VertexLine> vertex_list(0);

  Set<Int> all_vertices_this_side;
  Set<Int> covered_vertices;

  for (Int ze = 0; ze < z_edges.rows(); ++ze) {
    Matrix<Rational> ze_rays = z_edge_rays.minor(z_edges.row(ze), All);
    // Find rays in refined cone
    Int index_1 = -1;
    Int index_2 = -1;
    for (Int rc = 0; rc < refined_cone.rays.rows(); ++rc) {
      if (refined_cone.rays.row(rc) == ze_rays.row(0)) index_1 = rc;
      if (refined_cone.rays.row(rc) == ze_rays.row(1)) index_2 = rc;
      if (index_1 >= 0 && index_2 >= 0) break;
    }
    if (refined_cone.rays(index_1,0) == 1) all_vertices_this_side += index_1;
    if (refined_cone.rays(index_2,0) == 1) all_vertices_this_side += index_2;

    // Now find the codimension one cone containing these two rays
    Int co_index = -1;
    for (auto cc = entire(remaining_codim); !cc.at_end(); ++cc) {
      if (codim.row(*cc).contains(index_1) && codim.row(*cc).contains(index_2)) {
        co_index = *cc;
        break;
      }
    }
    remaining_codim -= co_index;

    // Find all maximal cones on the "path" to the other side and check if they are in X
    Int current_cone = *(coInMax.row(co_index).begin());
    bool found_bad = false;
    Int cone_index_other_side = -1;
    while (current_cone >= 0) {
      // Compute interior point and check if it is in X
      Vector<Rational> interior_point =
        accumulate( rows(refined_cone.rays.minor(refined_cone.cells.row(current_cone),All)), operations::add()) /
        accumulate(refined_cone.rays.minor(refined_cone.cells.row(current_cone),All).col(0),operations::add());
      if (!maximumAttainedTwice(funmat * interior_point)) {
        found_bad = true;
        break;
      }

      // If all is fine, find the next maximal cone, i.e. find an unused codimension one face
      // in that cone and take the other maximal cone adjacent to it.
      Int next_codim = *( (maxInCo.row(current_cone) * remaining_codim).begin() );
      remaining_codim -= next_codim;

      Set<Int> next_max = coInMax.row(next_codim) - current_cone;
      // If there is no other maximal cone, we have arrived at the other end
      if (next_max.size() == 0) {
        cone_index_other_side = next_codim;
        current_cone = -1;
      } else {
        current_cone = *(next_max.begin());
      }
    } //END iterate maximal cones to the other side

    // If everything lies in X, add the family (and register the vertices as "covered")
    if (!found_bad) {
      EdgeFamily ef;
      ef.edgesAtZero = Vector< Matrix<Rational> >(0);
      ef.edgesAwayZero = Vector< Matrix<Rational> >(0);
      ef.leafAtZero = leafAtZero;
      ef.edgesAtZero |= ze_rays;
      ef.edgesAwayZero |= refined_cone.rays.minor(codim.row(cone_index_other_side),All);
      ef.borderAtZero = ef.edgesAtZero[0];
      ef.borderAwayZero = ef.edgesAwayZero[0];
      family_list |= ef;

      covered_vertices += codim.row(co_index);
    }

  } //END iterate z_edges

  // Finally we consider all vertices not in a 2-dim family and check if the line to the other
  // side is contained in X
  Set<Int> vertices_to_check = all_vertices_this_side - covered_vertices;
  Set<Int> used_up_edges;
  Set<Int> used_up_vertices;
  for (auto vtc = entire(vertices_to_check); !vtc.at_end(); ++vtc) {
    used_up_vertices += *vtc;

    // Find first edge containing it
    Set<Int> current_edge_set = rayInCo.row(*vtc) * direction_edges;

    // If there is no edge, we have a vertex line
    if (current_edge_set.empty()) {
      VertexLine vl;
      vl.vertex = refined_cone.rays.row(*vtc);
      vl.cells = Set<Int>();
      vertex_list |= vl;
      continue;
    }

    Int current_edge = *(current_edge_set.begin());

    used_up_edges += current_edge;
    Int  vertex_index_other_side;
    bool found_bad = true;
    while (current_edge >= 0) {
      // Compute interior point and check if it is in X
      Vector<Rational> interior_point =
        accumulate( rows(refined_cone.rays.minor(codim.row(current_edge),All)), operations::add()) /
        accumulate(refined_cone.rays.minor(codim.row(current_edge),All).col(0),operations::add());
      if (!maximumAttainedTwice(funmat * interior_point)) {
        found_bad = true;
        break;
      }

      // Find next edge. If there is none, we have arrived at the other side
      Int next_vertex = *( (codim.row(current_edge) - used_up_vertices).begin());
      used_up_vertices += next_vertex;
      Set<Int> next_edge_set = (rayInCo.row(next_vertex)*direction_edges) - used_up_edges;
      if (next_edge_set.empty()) {
        vertex_index_other_side = next_vertex;
        current_edge = -1;
      } else {
        current_edge = *(next_edge_set.begin());
        used_up_edges += current_edge;
      }
    } // END iterate edges to other side
    if (!found_bad) {
      EdgeLine el;
      el.leafAtZero = leafAtZero;
      el.vertexAtZero = refined_cone.rays.row(*vtc);
      el.vertexAwayZero = refined_cone.rays.row(vertex_index_other_side);
      line_list |= el;
    }
  } //END iterate non-contained vertices

  // Return result
  LinesInCellResult result;
  result.edge_families = family_list;
  result.edge_lines = line_list;
  result.vertex_lines = vertex_list;

  return result;

} // END computeEdgeFamilies

} }
