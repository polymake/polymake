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

	Computes all lines (and families thereof) in a tropical cubic surface in R^3.
	*/

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Polynomial.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/divisor.h"
#include "polymake/tropical/convex_hull_tools.h"
#include "polymake/tropical/lines_in_cubic_data.h"
#include "polymake/tropical/lines_in_cubic_helper.h"
#include "polymake/tropical/lines_in_cubic_reachable.h"
#include "polymake/tropical/specialcycles.h"
#include "polymake/tropical/thomog.h"

namespace polymake { namespace tropical {

namespace {

/**
   @brief This takes a result of computeFacets and finds all the facets visible from "direction", i.e. all facets whose outer normal has strict positive scalar product with direction (it doesn't matter which normal we take: The direction must lie in the span of the two-dimensional cone, so the equation g of the cone is zero on direction. Any two representatives of a facet normal only differ by a multiple of g).
*/
void appendVisibleFaces(RestrictedIncidenceMatrix<>& faces, const FacetData& fd, const Vector<Rational>& direction)
{
  for (Int f = 0; f < fd.ineqs.rows(); ++f) {
    if (fd.ineqs.row(f) * direction < 0) { // <0, since ineqs has the inner normals
      faces /= fd.facets.row(f);
    }
  }
}
}

// Takes a max(!)-tropical polynomial of degree 3 in affine coordinates and computes all
// lines in the corresponding cubic.
BigObject linesInCubic(const Polynomial<TropicalNumber<Max>>& f)
{
  // First, we compute the divisor of f
  BigObject r3 = projective_torus<Max>(3,1);
  BigObject ratfct = call_function("rational_fct_from_affine_numerator", f);	
  BigObject X = call_function("divisor", r3, ratfct);
  BigObject lindom = ratfct.give("DOMAIN");
  Matrix<Rational> lindom_rays = lindom.give("VERTICES");
  lindom_rays = tdehomog(lindom_rays);
  IncidenceMatrix<> lindom_cones = lindom.give("MAXIMAL_POLYTOPES");
  Matrix<Rational> funmat(f.monomials_as_matrix());
  Vector<Rational> funcoeffs(f.coefficients_as_vector());
  // Rearrange so that values of terms on points can be computed as product with matrix
  funmat = funcoeffs | funmat;

  Matrix<Rational> degree = -unit_matrix<Rational>(3);
  degree = ones_vector<Rational>(3) / degree;
  degree = zero_vector<Rational>(4) | degree;

  // Then we compute all reachable points for each direction
  std::vector<ReachableResult> reachable_points;
  reachable_points.reserve(4);
  for (Int i = 0; i < 4; ++i) {
    reachable_points.push_back(reachablePoints(f,X,i));
  }
		
  // This maps pairs of leafs to increasing numbers and vice versa
  Map<Int, Map<Int, Int>> edge_pair_map;
  Map<Int, Set<Int>> index_to_pair_map;
  Int count = 0;
  for (Int a = 0; a <= 3; ++a) {
    edge_pair_map[a] = Map<Int, Int>();
    for (Int b = a+1; b <= 3; ++b) {
      edge_pair_map[a][b] = count;
      index_to_pair_map[count] = scalar2set(a) + scalar2set(b);
      ++count;
    }
  }

  // Result variables

  // These variables contain the lines in the cubic
  Vector<VertexLine> vertex_line;
  Vector<VertexFamily> vertex_family;
  Vector<EdgeLine> edge_line;
  Vector<EdgeFamily> edge_family;

  // Then we compute the six intersection sets of the reachable loci, paired in complements
  // In each step we go through all pairs of maximal cones of these two complexes. We add to each
  // cone the direction of the potential bounded edge of the line and then intersect the two
  // resulting polyhedra. Afterwards we check if the result lies in X.

  Matrix<Rational> dummy_lineality(0,4);
  for (Int i = 1; i <= 3; ++i) {
    Vector<Int> remaining(sequence(1,3) - i);

    // All points reachable from direction 0 and i
    fan_intersection_result z_inter =
      fan_intersection(reachable_points[0].rays, dummy_lineality, reachable_points[0].cells / reachable_points[0].edges,
                       reachable_points[i].rays, dummy_lineality, reachable_points[i].cells / reachable_points[i].edges);
    // All points reachable from the other two directions
    fan_intersection_result c_inter =
      fan_intersection(reachable_points[remaining[0]].rays, dummy_lineality, reachable_points[remaining[0]].cells / reachable_points[remaining[0]].edges,
                       reachable_points[remaining[1]].rays, dummy_lineality, reachable_points[remaining[1]].cells / reachable_points[remaining[1]].edges);
    // Clean up the intersection
    DirectionIntersection z_inter_clean = cleanUpIntersection(z_inter);
    DirectionIntersection c_inter_clean = cleanUpIntersection(c_inter);

    IncidenceMatrix<> all_z_cones = z_inter_clean.cells / z_inter_clean.edges / z_inter_clean.points;
    IncidenceMatrix<> all_c_cones = c_inter_clean.cells / c_inter_clean.edges / c_inter_clean.points;

    // Go through all pairs of cells, add the appropriate direction vector and intersect the two polyhedra
    Vector<Rational> dir_z = - degree.row(0) - degree.row(i);
    Vector<Rational> dir_c = - dir_z;
    for (Int zc = 0; zc < all_z_cones.rows(); ++zc) {
      // We compute the number of relevant cells in zc. If the dimension of zc is <= 1, this is just zc
      // If the dimension is 2, we take all the codimension one faces of zc that are visible
      // "from the direction of dir_z", i.e. the scalar product of the outer facet normal with dir_z is
      // strictly positive. We will then do computations separately for each such codim one face
      RestrictedIncidenceMatrix<> z_cones;
      Int z_dim = (all_z_cones.row(zc).size() <= 2? all_z_cones.row(zc).size()-1 : 2);
      if (z_dim < 2) {
        z_cones /= all_z_cones.row(zc);
      } else {
        const FacetData z_facets = computeFacets(z_inter_clean.rays, all_z_cones.row(zc));
        appendVisibleFaces(z_cones, z_facets, dir_z);
      }

      // Now we go through all cells of c and apply the same procedure
      for (Int cc = 0; cc < all_c_cones.rows(); ++cc) {
        RestrictedIncidenceMatrix<> c_cones;
        Int c_dim = (all_c_cones.row(cc).size() <= 2? all_c_cones.row(cc).size()-1 : 2);
        if (c_dim < 2) {
          c_cones /= all_c_cones.row(cc);
        } else {
          const FacetData c_facets = computeFacets(c_inter_clean.rays, all_c_cones.row(cc));
          appendVisibleFaces(c_cones, c_facets, dir_c);
        }

        // Now we go trough all pairs of elements of z_cones and c_cones and find families in between them
        for (Int zlist = 0; zlist < z_cones.rows(); ++zlist) {
          for (Int clist = 0; clist < c_cones.rows(); ++clist) {
            // First we compute the intersection of the two polyhedra we obtain if we add to the
            // cells in zlist/clist a ray in edge direction. The result cannot have lineality space,
            // as neither of the two factors has one (not even implicit).
            Matrix<Rational> center =
              cone_intersection(z_inter_clean.rays.minor(z_cones.row(zlist), All) / dir_z, dummy_lineality,
                                c_inter_clean.rays.minor(c_cones.row(clist), All) / dir_c, dummy_lineality).first;
            if (center.rows() == 0) continue;
            if (is_zero(center.col(0))) continue;

            // If the intersection is 0-dimensional, we just check if the maximum is attained twice,
            // then add it as solution
            if (center.rows() == 1) {
              if (maximumAttainedTwice(funmat * center.row(0))) {
                VertexLine vl;
                vl.vertex = center.row(0);
                Set<Int> spans;
                if (z_dim == 2) spans += (edge_pair_map[0])[i];
                if (c_dim == 2) spans += (edge_pair_map[remaining[0]])[remaining[1]];
                vl.cells = spans;
                vertex_line |= vl;
              }
              continue;
            } //END if 0-dimensional

            // First we compute the "border" of center, i.e. the intersection with the two cones
            Matrix<Rational> z_border
              = cone_intersection(z_inter_clean.rays.minor(z_cones.row(zlist), All), dummy_lineality, center, dummy_lineality).first;
            Matrix<Rational> c_border
              = cone_intersection(c_inter_clean.rays.minor(c_cones.row(clist), All), dummy_lineality, center, dummy_lineality).first;

            // Then we refine this polyhedron along the domains of f
            const IncidenceMatrix<> center_cone(rowwise(), sequence(0,center.rows()));
            DirectionIntersection center_ref = cleanUpIntersection(fan_intersection(
                center, dummy_lineality, center_cone, lindom_rays, dummy_lineality, lindom_cones));

            // If the intersection is 1-dimensional , we have two possibilities:
            // (1) The center cone is equal to the intersection of the two border cones. Then the
            //     whole cone is a one-dimensional family of lines without bounded edge.
            // (2) The center cone intersects the border cones each in a vertex. Then we check if all
            //     cells of center_ref are contained in X. If so, we have an isolated line.
            if (center.rows() == 2 ) {
              if (z_border.rows() == 2) {
                VertexFamily vf;
                vf.edge = z_border;
                vertex_family |= vf;
              } //END case (1)
              else {
                bool found_bad_cell = false;
                for (Int cencone = 0; cencone < center_ref.edges.rows(); ++cencone) {
                  Matrix<Rational> cencone_rays = center_ref.rays.minor(center_ref.edges.row(cencone),All);
                  Vector<Rational> interior_point =
                    accumulate(rows(cencone_rays), operations::add()) / accumulate(cencone_rays.col(0),operations::add());
                  if (!maximumAttainedTwice(funmat*interior_point)) {
                    found_bad_cell = true; break;
                  }
                } // END iterate edges of refined center cell
                // If all edges lie in X, we add the line as isolated line
                if (!found_bad_cell) {
                  EdgeLine el;
                  el.vertexAtZero = z_border.row(0);
                  el.vertexAwayZero = c_border.row(0);
                  el.leafAtZero = i;
                  el.spanAtZero = (z_dim == 2);
                  el.spanAwayZero = (c_dim == 2);
                  el.maxDistAtZero = maximalDistanceVector(el.vertexAtZero,
                                                           degree.row(0) + degree.row(el.leafAtZero),
                                                           lindom_rays, lindom_cones, funmat);
                  el.maxDistAwayZero = maximalDistanceVector(el.vertexAwayZero,
                                                             degree.row(remaining[0]) + degree.row(remaining[1]),
                                                             lindom_rays, lindom_cones, funmat);
                  edge_line |= el;
                }
              } // END case (2)
              continue;
            } // END if 1-dimensional

            // If the cone is 2-dimensional, we have to refine, project and refine again,
            // similarly to the procedure used when computing the reachable points
            if (center.rows() > 2) {
              LinesInCellResult lines_in_cell = computeEdgeFamilies(center_ref, z_border, c_border,i, funmat);
              edge_family |= lines_in_cell.edge_families;
              // Add spans and max-dist-vectors before adding edge_line and vertex_line
              for (Int el = 0; el < lines_in_cell.edge_lines.dim(); ++el) {
                lines_in_cell.edge_lines[el].spanAtZero = (z_dim == 2);
                lines_in_cell.edge_lines[el].spanAwayZero = (c_dim == 2);
                lines_in_cell.edge_lines[el].maxDistAtZero = maximalDistanceVector(lines_in_cell.edge_lines[el].vertexAtZero,
                                                                                   degree.row(0) + degree.row(i),
                                                                                   lindom_rays, lindom_cones, funmat);
                lines_in_cell.edge_lines[el].maxDistAwayZero = maximalDistanceVector(lines_in_cell.edge_lines[el].vertexAwayZero,
                                                                                     degree.row(remaining[0]) + degree.row(remaining[1]),
                                                                                     lindom_rays, lindom_cones, funmat);
              } // END fine-tune edge_lines
              for (Int vl = 0; vl < lines_in_cell.vertex_lines.dim(); ++vl) {
                Set<Int> spans;
                if (z_dim == 2) spans += (edge_pair_map[0])[i];
                if (c_dim == 2) spans += (edge_pair_map[remaining[0]])[remaining[1]];
                lines_in_cell.vertex_lines[vl].cells = spans;
              } // END fine-tune vertex_lines

              edge_line |= lines_in_cell.edge_lines;
              vertex_line |= lines_in_cell.vertex_lines;
            } // END if 2-dimensional
          } // END iterate relevant cones from c
        } // END iterate relevant cones from z
      } // END iterate j-k-reachable cones
    } // END iterate 0-i-reachable cones
  } // END compute intersections of reachable loci

  // .............................................................................................
  // Step I: Clean up each type individually by checking if one contains the other or has to be glued
  // together somehow

  // Vertex line: Only check if one is contained in the other
  Set<Int> double_vertices;
  for (Int vl = 0; vl < vertex_line.dim(); ++vl) {
    if (!double_vertices.contains(vl)) {
      for (Int ovl = 0; ovl < vertex_line.dim(); ++ovl) {
        if (vl != ovl) {
          if (vertex_line[vl].vertex == vertex_line[ovl].vertex &&
              (vertex_line[vl].cells * vertex_line[ovl].cells).size() == vertex_line[ovl].cells.size() ) {
            double_vertices += ovl;
          }
        }
      }
    }
  } // END clean up vertex_line
  vertex_line = vertex_line.slice(~double_vertices);

  // For vertex families we have to check if two families can be glued together
  std::list<VertexFamily> vf_queue;
  for (Int vf = 0; vf < vertex_family.dim(); ++vf) {
    vf_queue.push_back(vertex_family[vf]);
  }
  Vector<VertexFamily> approved_vfamilies;
  while (!vf_queue.empty()) {
    VertexFamily fam = vf_queue.front(); vf_queue.pop_front();
    bool is_approved = true;
    for (Int vf = 0; vf < approved_vfamilies.dim(); ++vf) {
      // Case I: Both are identical: Just ignore the new one and add nothing new
      if ((approved_vfamilies[vf].edge.row(0) == fam.edge.row(0) &&
           approved_vfamilies[vf].edge.row(1) == fam.edge.row(1)) ||
          (approved_vfamilies[vf].edge.row(0) == fam.edge.row(1) &&
           approved_vfamilies[vf].edge.row(1) == fam.edge.row(0))) {
        is_approved = false;
        break;
      }

      // Case II: At least one consists of two vertices, one of which is equal to
      // a vertex of the other. Then we glue both together, add the new element to the vf_queue and
      // go to the next vf_queue element
      if ((approved_vfamilies[vf].edge(0,0) + approved_vfamilies[vf].edge(1,0) == 2) ||
          (fam.edge(0,0) + fam.edge(1,0) == 2) ) {
        Int avf_equal = -1;
        Int fam_equal = -1;
        if (approved_vfamilies[vf].edge.row(0) == fam.edge.row(0)) { avf_equal = 0; fam_equal = 0; }
        if (approved_vfamilies[vf].edge.row(0) == fam.edge.row(1)) { avf_equal = 0; fam_equal = 1; }
        if (approved_vfamilies[vf].edge.row(1) == fam.edge.row(0)) { avf_equal = 1; fam_equal = 0; }
        if (approved_vfamilies[vf].edge.row(1) == fam.edge.row(1)) { avf_equal = 1; fam_equal = 1; }

        if (avf_equal != -1) {
          Int other_avf = avf_equal == 0 ? 1 : 0;
          Int other_fam = fam_equal == 0 ? 1 : 0;
          VertexFamily replacement;
          replacement.edge = vector2row(approved_vfamilies[vf].edge.row(other_avf)) / fam.edge.row(other_fam);
          vf_queue.push_back(replacement);
          approved_vfamilies = approved_vfamilies.slice(~scalar2set(vf));
          is_approved = false;
          break;
        } // END create new vertex family
      } // END Case II	
    } // END iterate approved families

    if (is_approved) approved_vfamilies |= fam;
  } // END clean up vertex families

  vertex_family = approved_vfamilies;

  // For edge lines, we check if two are equal or can be glued together
  std::list<EdgeLine> el_queue;
  for (Int el = 0; el < edge_line.dim(); ++el) {
    el_queue.push_back(edge_line[el]);
  }
  Vector<EdgeLine> approved_elfamilies;
  while (!el_queue.empty()) {
    EdgeLine fam = el_queue.front(); el_queue.pop_front();
    bool is_approved = true;
    for (Int el = 0; el < approved_elfamilies.dim(); ++el) {
      // Interesting things only happen if both have the same direction
      if (approved_elfamilies[el].leafAtZero == fam.leafAtZero) {
        // Compatibility = Both families can be glued, i.e. in each direction either:
        // - the vertices at that end agree -> keep vertex with largest span
        // - one vertex has unbounded span and the other vertex differs by a positive multiple of
        //   the edge direction -> keep vertex with unbounded span
        // - one vertex has bounded span, which contains the other vertex. -> keep vertex with bounded span
        bool isCompatibleAtZero = false;
        Vector<Rational> vertexAtZero;
        Vector<Rational> mdistAtZero;
        bool spanAtZero = false;
        if (approved_elfamilies[el].vertexAtZero == fam.vertexAtZero) {
          isCompatibleAtZero = true;
          vertexAtZero = fam.vertexAtZero;
          spanAtZero = approved_elfamilies[el].spanAtZero || fam.spanAtZero;
        } else {
          // Check if difference of vertices is multiple of edge direction
          Vector<Rational> edir = degree.row(0) + degree.row(fam.leafAtZero);
          Rational vdiff = vertexDistance(approved_elfamilies[el].vertexAtZero,
                                          fam.vertexAtZero, edir);
          // If so, check if span is compatible
          if (vdiff != 0) {
            if (vdiff > 0 && approved_elfamilies[el].spanAtZero) {
              Rational sdiff = vertexDistance(approved_elfamilies[el].vertexAtZero,
                                              approved_elfamilies[el].maxDistAtZero, edir);
              isCompatibleAtZero = sdiff == 0 || sdiff >= vdiff;
              vertexAtZero = approved_elfamilies[el].vertexAtZero;
              mdistAtZero = approved_elfamilies[el].maxDistAtZero;
              spanAtZero = true;
            }		
            if (vdiff < 0 && fam.spanAtZero) {
              Rational sdiff = vertexDistance(fam.vertexAtZero, fam.maxDistAtZero,edir);
              isCompatibleAtZero = sdiff == 0 || sdiff >= -vdiff;
              vertexAtZero = fam.vertexAtZero;
              mdistAtZero = fam.maxDistAtZero;
              spanAtZero = true;
            }
          } // END if is multiple of edge direction
        } // END check compat at zero
        if (!isCompatibleAtZero) continue;

        bool isCompatibleAwayZero = false;
        Vector<Rational> vertexAwayZero;
        Vector<Rational> mdistAwayZero;
        bool spanAwayZero = false;
        if (approved_elfamilies[el].vertexAwayZero == fam.vertexAwayZero) {
          isCompatibleAwayZero = true;
          vertexAwayZero = fam.vertexAwayZero;
          spanAwayZero = approved_elfamilies[el].spanAwayZero || fam.spanAwayZero;
        } else {
          // Check if difference of vertices is multiple of edge direction
          Vector<Int> rem(sequence(1,3) - fam.leafAtZero);
          Vector<Rational> edir = degree.row(rem[0]) + degree.row(rem[1]);
          Rational vdiff = vertexDistance(approved_elfamilies[el].vertexAwayZero,
                                          fam.vertexAwayZero, edir);
          // If so, check if span is compatible
          if (vdiff != 0) {
            if (vdiff > 0 && approved_elfamilies[el].spanAwayZero) {
              Rational sdiff = vertexDistance(approved_elfamilies[el].vertexAwayZero,
                                              approved_elfamilies[el].maxDistAwayZero, edir);
              isCompatibleAwayZero = sdiff == 0 || sdiff >= vdiff;
              vertexAwayZero = approved_elfamilies[el].vertexAwayZero;
              mdistAwayZero = approved_elfamilies[el].maxDistAwayZero;
              spanAwayZero = true;
            }		
            if (vdiff < 0 && fam.spanAwayZero) {
              Rational sdiff = vertexDistance(fam.vertexAwayZero, fam.maxDistAwayZero,edir);
              isCompatibleAwayZero = sdiff == 0 || sdiff >= -vdiff;
              vertexAwayZero = fam.vertexAwayZero;
              mdistAwayZero = fam.maxDistAwayZero;
              spanAwayZero = true;
            }
          } // END if is multiple of edge direction
        } // END check compat away zero
        if (!isCompatibleAwayZero) continue;

        // If we arrive here, we have to glue
        EdgeLine newel;
        newel.vertexAtZero = vertexAtZero;
        newel.vertexAwayZero = vertexAwayZero;
        newel.leafAtZero = fam.leafAtZero;
        newel.spanAtZero = spanAtZero;
        newel.spanAwayZero = spanAwayZero;
        newel.maxDistAtZero = mdistAtZero;
        newel.maxDistAwayZero = mdistAwayZero;
        approved_elfamilies = approved_elfamilies.slice(~scalar2set(el));
        el_queue.push_back(newel);
        is_approved = false;
        break;
      } // END if same direction
    } // END iterate approved families
    if (is_approved) approved_elfamilies |= fam;
  } // END clean up edge lines
  edge_line = approved_elfamilies;

  // Clean up edge_families: Check if two can be glued together
  std::list<EdgeFamily> ef_queue;
  for (Int ef = 0; ef < edge_family.dim(); ++ef) {
    ef_queue.push_back(edge_family[ef]);
  }
  Vector<EdgeFamily> approvedFam;
  while (!ef_queue.empty()) {
    EdgeFamily fam = ef_queue.front(); ef_queue.pop_front();
    bool is_approved = true;
    for (Int ef = 0; ef < approvedFam.dim(); ++ef) {
      EdgeFamily compFam = approvedFam[ef];
      // We can only glue, if both have the same direction
      if (fam.leafAtZero == compFam.leafAtZero) {
        // We can glue if at least one family has 2-vertex-borders that intersect the other edges
        // in a vertex
        if (fam.borderAtZero(0,0) + fam.borderAtZero(1,0) == 2 || compFam.borderAtZero(0,0) + compFam.borderAtZero(1,0) == 2) {
          Int fam_equal = -1;
          Int comp_equal = -1;
          // Check if two vertices agree
          if (fam.borderAtZero.row(0) == compFam.borderAtZero.row(0)) { fam_equal = 0; comp_equal = 0;}
          if (fam.borderAtZero.row(0) == compFam.borderAtZero.row(1)) { fam_equal = 0; comp_equal = 1;}
          if (fam.borderAtZero.row(1) == compFam.borderAtZero.row(0)) { fam_equal = 1; comp_equal = 0;}
          if (fam.borderAtZero.row(1) == compFam.borderAtZero.row(1)) { fam_equal = 1; comp_equal = 1;}

          if (fam_equal != -1) {
            Int fam_other = 1 - fam_equal;
            Int comp_other = 1 - comp_equal;
            // Check if on the other side also two vertices agree AND that this is not the same
            // vertex as on this side
            Int side_fam_equal = -1;
            Int side_comp_equal = -1;
            if (fam.borderAwayZero.row(0) == compFam.borderAwayZero.row(0) &&
                fam.borderAwayZero.row(0) != fam.borderAtZero.row(fam_equal)) {
              side_fam_equal = 0; side_comp_equal = 0;
            }
            if (fam.borderAwayZero.row(0) == compFam.borderAwayZero.row(1) &&
                fam.borderAwayZero.row(0) != fam.borderAtZero.row(fam_equal)) {
              side_fam_equal = 0; side_comp_equal = 1;
            }
            if (fam.borderAwayZero.row(1) == compFam.borderAwayZero.row(0) &&
                fam.borderAwayZero.row(1) != fam.borderAtZero.row(fam_equal)) {
              side_fam_equal = 1; side_comp_equal = 0;
            }
            if (fam.borderAwayZero.row(1) == compFam.borderAwayZero.row(1) &&
                fam.borderAwayZero.row(1) != fam.borderAtZero.row(fam_equal)) {
              side_fam_equal = 1; side_comp_equal = 1;
            }
            if (side_fam_equal != -1) {
              Int side_fam_other = 1 - side_fam_equal;
              Int side_comp_other = 1 - side_comp_equal;

              Matrix<Rational> nzero_border = vector2row(fam.borderAtZero.row(fam_other)) / compFam.borderAtZero.row(comp_other);
              Matrix<Rational> naway_border = vector2row(fam.borderAwayZero.row(side_fam_other)) / compFam.borderAwayZero.row(side_comp_other);
              EdgeFamily replacement;
              replacement.leafAtZero = fam.leafAtZero;
              replacement.edgesAtZero = fam.edgesAtZero | compFam.edgesAtZero;
              replacement.edgesAwayZero = fam.edgesAwayZero | compFam.edgesAwayZero;
              replacement.borderAtZero = nzero_border;
              replacement.borderAwayZero = naway_border;
              ef_queue.push_back(replacement);
              approvedFam = approvedFam.slice(~scalar2set(ef));
              is_approved = false;
              break;
            } // END if two vertices agree on the other side
          } // END if two vertices agree
        } // END if can be glued
      } // END if direction agrees	
    } // END iterate approved families
    if (is_approved) approvedFam |= fam;
  } // END clean up edge families
  edge_family = approvedFam;

  // STEP II: A family of one type may be contained in a family of a different type

  // Check if any edge line is contained in a spanning vertex line
  Set<Int> redundant_el;
  for (Int el = 0; el < edge_line.dim(); ++el) {
    for (Int vl = 0; vl < vertex_line.dim(); ++vl) {
      // One vertex has to be equal and the other vertex has to be in the span of the vertex line
      if (edge_line[el].vertexAtZero == vertex_line[vl].vertex ||
          edge_line[el].vertexAwayZero == vertex_line[vl].vertex) {
        Vector<Rational> other_vertex;
        Set<Int> dir_indices;
        if (edge_line[el].vertexAtZero == vertex_line[vl].vertex) {
          other_vertex = edge_line[el].vertexAwayZero;
          dir_indices = sequence(1,3) - scalar2set(edge_line[el].leafAtZero);
        } else {
          other_vertex = edge_line[el].vertexAtZero;
          dir_indices = scalar2set(0) + scalar2set(edge_line[el].leafAtZero);
        }
        bool span_has_dir = false;
        for (auto c = entire(vertex_line[vl].cells); !c.at_end(); ++c) {
          if ((dir_indices * index_to_pair_map[*c]).size() == 2) {
            span_has_dir = true; break;
          }
        }
        bool is_compatible = false;
        if (span_has_dir) {
          Vector<Rational> edir = accumulate(rows(degree.minor(dir_indices,All)), operations::add());
          Rational vdiff = vertexDistance(vertex_line[vl].vertex, other_vertex, edir);
          if (vdiff > 0) {
            Vector<Rational> mdist = maximalDistanceVector(vertex_line[vl].vertex, edir, lindom_rays, lindom_cones, funmat);
            Rational mdiff = vertexDistance(vertex_line[vl].vertex, mdist, edir);
            is_compatible = mdiff == 0 || mdiff >= vdiff;
          }
        }
        if (is_compatible) redundant_el += el;
      }
    }
  }
  edge_line = edge_line.slice(~redundant_el);

  // Check if any vertex_line is contained in a vertex_family
  Set<Int> redundant_vl;
  for (Int vl = 0; vl < vertex_line.dim(); ++vl) {
    for (Int vf = 0; vf < vertex_family.dim(); ++vf) {
      // Check if the vertex is a vertex of the family and the spans contain the direction
      Int fam_dir = vertexFamilyDirection(vertex_family[vf]);
      if (vertex_line[vl].vertex == vertex_family[vf].edge.row(0) ||
          vertex_line[vl].vertex == vertex_family[vf].edge.row(1)) {
        Set<Int> cells = vertex_line[vl].cells;
        bool is_compatible = true;
        for (const Int c : cells) {
          if (!index_to_pair_map[c].contains(fam_dir)) {
            is_compatible = false; break;
          }
        }
        if (is_compatible) redundant_vl += vl;
      }
    } // END iterate vertex_family
  } // END compare vertex_line to vertex_family
  vertex_line = vertex_line.slice(~redundant_vl);

  // Create corresponding line objects ...............................................................
  BigObject result("LinesInCubic", mlist<Max>(),
                   "CUBIC", X,
                   "POLYNOMIAL", ratfct.give("NUMERATOR"));

  // Create vertex_line objects:
  // If two rays in such an object span a 2-dim-cell, this is computed as follows:
  // We check, how far the line in direction of the sum of the two corr. rays lies in X
  // If all of it lies in X, the 2-dim cell is just the vertex + the two rays. If not, let
  // w be the end vertex of the line. Then we have two 2-dim. cells: conv(vertex,w) + each of the rays
  for (Int ivert = 0; ivert < vertex_line.dim(); ++ivert) {
    Matrix<Rational> var_rays = degree / vertex_line[ivert].vertex ;
    RestrictedIncidenceMatrix<> var_cones;
    // Find all rays that are NOT involved in a 2-dim cell
    Set<Int> rays_in_cells;
    for (const Int vcell : vertex_line[ivert].cells) {
      rays_in_cells += index_to_pair_map[vcell];
    }
    Set<Int> rays_not_in_cells = sequence(0,4) - rays_in_cells;
    // Add those rays not in 2-dim. cells
    for (const Int i : rays_not_in_cells) {
      var_cones /= scalar2set(4) + scalar2set(i);
    }
    // Now we compute the 2-dimensional cells as described above
    for (const auto& span : vertex_line[ivert].cells) {
      Vector<Rational> md_vector = maximalDistanceVector(vertex_line[ivert].vertex,
                                                         accumulate(rows(degree.minor(index_to_pair_map[span],All)),operations::add()),
                                                         lindom_rays, lindom_cones, funmat);
      if (md_vector.dim() == 0) {
        var_cones /= scalar2set(4) + index_to_pair_map[span];
      } else {
        var_rays /= md_vector;
        Vector<Int> dirs(index_to_pair_map[span]);
        var_cones /= scalar2set(4) + scalar2set(var_rays.rows()-1) + dirs[0];
        var_cones /= scalar2set(4) + scalar2set(var_rays.rows()-1) + dirs[1];
      }
    }
    BigObject var("Cycle<Max>");
    var.take("VERTICES") << thomog(var_rays);
    const IncidenceMatrix<> var_cones_final(std::move(var_cones));
    var.take("MAXIMAL_POLYTOPES") << var_cones_final;
    if (vertex_line[ivert].cells.size() == 0) {
      var.take("WEIGHTS") << ones_vector<Integer>(var_cones_final.rows());
      result.add("LIST_ISOLATED_NO_EDGE", var);
    } else {
      var.take("PURE") << false;
      result.add("LIST_FAMILY_FIXED_VERTEX", var);
    }
  }

  // Create vertex_family objects: Find the direction spanned by the family and only add the remaining three
  // as rays
  for (Int fvert = 0; fvert < vertex_family.dim(); ++fvert) {
    Int missing_dir = vertexFamilyDirection(vertex_family[fvert]);
    Matrix<Rational> var_rays = vertex_family[fvert].edge / degree.minor(~scalar2set(missing_dir),All);
    const IncidenceMatrix<> var_cones{ {0,1,2}, {0,1,3}, {0,1,4} };
    BigObject var("Cycle<Max>",
                  "VERTICES", thomog(var_rays),
                  "MAXIMAL_POLYTOPES", var_cones,
                  "PURE", false);
    result.add("LIST_FAMILY_MOVING_VERTEX", var);
  }

  // Create edge_lines
  // Two-dimensional cells at each end are computed as for vertex_line
  for (Int el = 0; el < edge_line.dim(); ++el) {
    Matrix<Rational> var_rays = edge_line[el].vertexAtZero / (edge_line[el].vertexAwayZero / degree);
    RestrictedIncidenceMatrix<> var_cones;
    var_cones /= sequence(0,2);
    if (edge_line[el].spanAtZero) {
      Vector<Rational> z_mdvector = edge_line[el].maxDistAtZero;
      if (z_mdvector.dim() == 0) {
        var_cones /= (scalar2set(0) + scalar2set(2) + scalar2set(edge_line[el].leafAtZero+2));
      } else {
        var_rays /= z_mdvector;
        var_cones /= (scalar2set(0) + scalar2set(var_rays.rows()-1) + scalar2set(2));
        var_cones /= (scalar2set(0) + scalar2set(var_rays.rows()-1) + scalar2set(edge_line[el].leafAtZero+2));
      }
    } else {
      var_cones /= (scalar2set(0) + scalar2set(2));
      var_cones /= (scalar2set(0) + scalar2set(edge_line[el].leafAtZero+2));
    }

    Vector<Int> rem(sequence(1,3) - edge_line[el].leafAtZero);
    if (edge_line[el].spanAwayZero) {
      Vector<Rational> c_mdvector = edge_line[el].maxDistAwayZero;
      if (c_mdvector.dim() == 0) {
        var_cones /= (scalar2set(1) + scalar2set(rem[0]+2) + scalar2set(rem[1]+2));
      } else {
        var_rays /= c_mdvector;
        var_cones /= (scalar2set(1) + scalar2set(var_rays.rows()-1) + scalar2set(rem[0]+2));
        var_cones /= (scalar2set(1) + scalar2set(var_rays.rows()-1) + scalar2set(rem[1]+2));
      }
    } else {
      var_cones /= (scalar2set(1) + (rem[0]+2));
      var_cones /= (scalar2set(1) + (rem[1]+2));
    }

    BigObject var("Cycle<Max>");
    var.take("VERTICES") << thomog(var_rays);
    const IncidenceMatrix<> var_cones_final(std::move(var_cones));
    var.take("MAXIMAL_POLYTOPES") << var_cones_final;
    if (edge_line[el].spanAtZero || edge_line[el].spanAwayZero) {
      var.take("PURE") << false;
      result.add("LIST_FAMILY_FIXED_EDGE",var);
    } else {
      var.take("WEIGHTS") << ones_vector<Integer>(var_cones_final.rows());
      result.add("LIST_ISOLATED_EDGE",var);
    }
  }

  // Create edge families
  for (Int ef = 0; ef < edge_family.dim(); ++ef) {
    Matrix<Rational> var_rays = degree;
    RestrictedIncidenceMatrix<> var_cones;
    for (Int eg = 0; eg < edge_family[ef].edgesAtZero.dim(); ++eg) {
      Int start_index = var_rays.rows();
      var_rays /= edge_family[ef].edgesAtZero[eg];
      var_rays /= edge_family[ef].edgesAwayZero[eg];
      var_cones /= sequence(start_index, 4);

      // We have to canonicalize each of the direction cones, since they might be one-dimensional
      Vector<Int> z_cone(sequence(start_index,2));
      z_cone |= 0;
      Vector<Int> i_cone(sequence(start_index,2));
      i_cone |= (edge_family[ef].leafAtZero);

      Vector<Int> rem(sequence(1,3) - edge_family[ef].leafAtZero);

      Vector<Int> j_cone(sequence(start_index+2,2));
      j_cone |= rem[0];
      Vector<Int> k_cone(sequence(start_index+2,2));
      k_cone |= rem[1];

      var_cones /= Set<Int>(z_cone.slice(polytope::get_non_redundant_points(var_rays.minor(Set<Int>(z_cone), All), dummy_lineality, true).first));
      var_cones /= Set<Int>(i_cone.slice(polytope::get_non_redundant_points(var_rays.minor(Set<Int>(i_cone), All), dummy_lineality, true).first));
      var_cones /= Set<Int>(j_cone.slice(polytope::get_non_redundant_points(var_rays.minor(Set<Int>(j_cone), All), dummy_lineality, true).first));
      var_cones /= Set<Int>(k_cone.slice(polytope::get_non_redundant_points(var_rays.minor(Set<Int>(k_cone), All), dummy_lineality, true).first));
    }

    const IncidenceMatrix<> var_cones_final(std::move(var_cones));
    BigObject var("Cycle<Max>",
                  "VERTICES", thomog(var_rays),
                  "MAXIMAL_POLYTOPES", var_cones_final,
                  "PURE", false);
    result.add("LIST_FAMILY_MOVING_EDGE", var);
  }

  return result;
}

Function4perl(&linesInCubic,"linesInCubic(Polynomial<TropicalNumber<Max>>)");
} }
