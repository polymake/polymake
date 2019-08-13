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
   Computes the polyhedral data necessary for visualization with a bounding box.
   @param fan::PolyhedralComplex fan The polyhedral complex to be visualized, in
          non-tropically-homogeneous coordinates
   @param Matrix<Rational> bbFacets the bounding facets
   @param Array<String> clabels If this array has positive length,
          these strings will be used to label the maximal cones (missing labels are replaced by the emtpy string)
   @return A perl::ListReturn containing
           1) the list of polytopes to be rendered
           2) A polytope::PointConfiguration that will contain the center of each cell as vertex,
              labelled with the corresponding weight.
              This is only computed if showWeights is true, but is contained in the ListReturn in any case.
*/
perl::ListReturn computeBoundedVisual(perl::Object fan, const Matrix<Rational> &bbFacets, const Array<std::string> &clabels)
{
  // Extract values
  const int ambient_dim = fan.call_method("AMBIENT_DIM");
  const Matrix<Rational> &facetNormals = fan.give("FACET_NORMALS");
  const Matrix<Rational> &facetNormalsInCones = fan.give("MAXIMAL_POLYTOPES_FACETS");
  const Matrix<Rational> &linearSpan = fan.give("LINEAR_SPAN_NORMALS");
  const IncidenceMatrix<> &linearSpanInCones = fan.give("MAXIMAL_POLYTOPES_AFFINE_HULL_NORMALS");
  const int fan_dim = fan.call_method("DIM");

  bool use_labels = clabels.size() > 0;

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

Function4perl(&computeBoundedVisual, "computeBoundedVisual(fan::PolyhedralComplex, Matrix<Rational>, Array<String>)");

} }
