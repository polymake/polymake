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

  Computes the data for BB_VISUAL
*/


#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/convex_hull_tools.h"
#include "polymake/polytope/convex_hull.h"

namespace polymake { namespace tropical {

/**
   This takes a list of vertices (as row vectors, without leading ones)
   and computes the corresponding bounding box,
   i.e. it computes the minima /maxima over all coordinates, subtracts/adds a given distance
   and returns the resulting 2x(no of coordinates)-matrix.
   The first row contains the min-coords, the second the max-coords
   @param Matrix<Rational> rays The vertices
   @param Rational distance The minimal distance the box should have from all vertices
   @param Bool make_it_a_cube If this is true (it is false by default), the bounding box is made
   a cube, by setting all coordinates to be the respective maximum / minimum
*/
Matrix<Rational> boundingBox(const Matrix<Rational> &rays, const Rational &distance, bool make_it_a_cube = 0)
{
  if(rays.rows() == 0) return Matrix<Rational>(2,rays.cols());
  Vector<Rational> min_values = rays.row(0);
  Vector<Rational> max_values = rays.row(0);
  for (auto c = entire<indexed>(cols(rays)); !c.at_end(); ++c) {
    min_values[c.index()] = accumulate(*c, operations::min());
    max_values[c.index()] = accumulate(*c, operations::max());
  }
  if (make_it_a_cube) {
    Rational total_max = accumulate( max_values, operations::max());
    Rational total_min = accumulate( min_values, operations::min());
    max_values = ones_vector<Rational>(rays.cols()) * total_max;
    min_values = ones_vector<Rational>(rays.cols()) * total_min;
  }
  // Add distance
  min_values -= distance* ones_vector<Rational>(rays.cols());
  max_values += distance* ones_vector<Rational>(rays.cols());
  return vector2row(min_values) / max_values;
} //END boundingBox

/**
   Computes the polyhedral data necessary for visualization with a bounding box.
   @param fan::PolyhedralComplex fan The polyhedral complex to be visualized, in
          non-tropically-homogeneous coordinates
   @param Matrix<Rational> bBox The bounding box.
          Given by two row vectors indicating the extreme points of the box
   @param Array<String> clabels If this array has positive length,
          these strings will be used to label the maximal cones (missing labels are replaced by the emtpy string)
   @return A perl::ListReturn containing
           1) the list of polytopes to be rendered
           2) A polytope::PointConfiguration that will contain the center of each cell as vertex,
              labelled with the corresponding weight.
              This is only computed if showWeights is true, but is contained in the ListReturn in any case.
*/
perl::ListReturn computeBoundedVisual(perl::Object fan, const Matrix<Rational> &bBox, const Array<std::string> &clabels)
{
  // Extract values
  const int ambient_dim = fan.call_method("AMBIENT_DIM");
  const Matrix<Rational> &facetNormals = fan.give("FACET_NORMALS");
  const Matrix<Rational> &facetNormalsInCones = fan.give("MAXIMAL_POLYTOPES_FACETS");
  const Matrix<Rational> &linearSpan = fan.give("LINEAR_SPAN_NORMALS");
  const IncidenceMatrix<> &linearSpanInCones = fan.give("MAXIMAL_POLYTOPES_AFFINE_HULL_NORMALS");
  const int fan_dim = fan.call_method("DIM");

  bool use_labels = clabels.size() > 0;

  // Compute facets of the bounding box
  Matrix<Rational> bbFacets(0,ambient_dim+1);

  // For each coordinate, contain minimum and maximum
  Vector<Rational> minCoord(ambient_dim);
  Vector<Rational> maxCoord(ambient_dim);
  for (int i = 0; i < ambient_dim; i++) {
    maxCoord[i] = std::max(bBox(0,i), bBox(1,i));
    minCoord[i] = std::min(bBox(0,i), bBox(1,i));
  }
  // Now make these coordinates into facets
  for (int i = 0; i < ambient_dim; i++) {
    Vector<Rational> facetVector = unit_vector<Rational>(ambient_dim,i);
    bbFacets /= (maxCoord[i] | -facetVector);
    bbFacets /= (-minCoord[i] | facetVector);
  }

  perl::ListReturn result;

  // This will contain the cell centers with the weight labels
  perl::Object weightCenters("polytope::PointConfiguration");
  Matrix<Rational> centermatrix(0,ambient_dim+1);
  std::vector<std::string> centerlabels;

  // Now compute all polyhedra to be rendered
  for (int mc = 0; mc < linearSpanInCones.rows(); mc++) {
    // Compute the facets ans equalities of the current cone and add the bbox facets
    ListMatrix<Vector<Rational> > facets(0,ambient_dim+1);
    Matrix<Rational> linspan = linearSpan.minor(linearSpanInCones.row(mc),All);
    Set<int> positive_facets = indices(attach_selector( facetNormalsInCones.row(mc), operations::positive()));
    auto negative_facets = support( facetNormalsInCones.row(mc)) - positive_facets;
    facets /= facetNormals.minor( positive_facets,All);
    facets /= - facetNormals.minor( negative_facets,All);
    facets /= bbFacets;

    // Compute the polytope vertices from that
    Matrix<Rational> polyRays = polytope::try_enumerate_vertices(facets, linspan, false).first;
    // Normalize
    normalize_rays(polyRays);

    // We have to make sure that the polytope has
    // at least dim +1 vertices after cutting, otherwise its a point set or graph to the
    // visualization and all the Facet options don't work

    if (polyRays.rows() >= fan_dim+1) {
      perl::Object polytope("polytope::Polytope<Rational>");
      polytope.take("VERTICES") << polyRays; //The polytope shouldn't have a lineality space
      result << polytope;

      // If weight labels should be displayed, compute the vertex barycenter of the polytope and
      // label it
      if (use_labels) {
        Vector<Rational> barycenter = average(rows(polyRays));
        centermatrix = centermatrix / barycenter;
        std::ostringstream wlabel;
        if (mc < clabels.size()) wlabel << clabels[mc];
        centerlabels.push_back(wlabel.str());
      }
    }

  } //END iterate rendered polyhedra

  if (use_labels) {
    weightCenters.take("POINTS") << centermatrix;
    weightCenters.take("LABELS") << centerlabels;
  }
  result << weightCenters;

  return result;
}

Function4perl(&boundingBox, "boundingBox(Matrix;$=1,$=0)");
Function4perl(&computeBoundedVisual, "computeBoundedVisual(fan::PolyhedralComplex, Matrix<Rational>, Array<String>)");

} }
