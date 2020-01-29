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
        Copyright (c) 2016-2020
        Ewgenij Gawrilow, Michael Joswig, and the polymake team
        Technische Universität Berlin, Germany
        https://polymake.org

        The functions here deal with all the [[SEPARATED..]] properties 
        */

#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/separated_data.h"


namespace polymake { namespace tropical {

void computeSeparatedData(BigObject X)
{
  // Extract properties of X
  Matrix<Rational> rays = X.give("VERTICES");

  IncidenceMatrix<> codimOneCones = X.give("CODIMENSION_ONE_POLYTOPES");
  IncidenceMatrix<> maximalCones = X.give("MAXIMAL_POLYTOPES");
  IncidenceMatrix<> facet_incidences = X.give("MAXIMAL_AT_CODIM_ONE");
  facet_incidences = T(facet_incidences);

  // Result variables
  Matrix<Rational> cmplxrays(0,rays.cols());
  Vector<Set<Int>> maxcones(maximalCones.rows());
  Vector<Set<Int>> codimone(codimOneCones.rows());
  Vector<Int> conversion;

  // Divide the set of rays into those with x0 != 0 and those with x0 = 0
  Set<Int> affineRays;
  Set<Int> directionalRays;
  Map<Int, Int> newAffineIndices; //This maps the old ray indices to the new ones in cmplxrays
  for (Int r = 0; r < rays.rows(); ++r) {
    if (is_zero(rays.row(r)[0])) {
      directionalRays = directionalRays + r;
    } else {
      affineRays = affineRays + r;
      cmplxrays = cmplxrays / rays.row(r);
      conversion |= r;
      newAffineIndices[r] = cmplxrays.rows()-1;
    }
  }
                
  // Insert the indices of the new affine rays for each cone
  for (Int co = 0; co < codimOneCones.rows(); ++co) {
    Set<Int> corays = codimOneCones.row(co) * affineRays;
    codimone[co] = Set<Int>();
    for (auto e = entire( corays); !e.at_end(); ++e) {
      codimone[co] = codimone[co] + newAffineIndices[*e];
    }
  }
  for (Int mc = 0; mc < maximalCones.rows(); ++mc) {
    Set<Int> mcrays = maximalCones.row(mc) * affineRays;
    maxcones[mc] = Set<Int>();
    for (auto e = entire( mcrays); !e.at_end(); ++e) {
      maxcones[mc] = maxcones[mc] + newAffineIndices[*e];
    }
  }

  // Now we go through the directional rays and compute the connected component for each one
  for (auto r = entire(directionalRays); !r.at_end(); ++r) {

    // List of connected components of this ray, each element is a component
    // containing the indices of the maximal cones
    Vector<Set<Int>> connectedComponents;
    // The inverse of the component matrix, i.e. maps cone indices to row indices of connectedComponents
    Map<Int, Int> inverseMap;

    // Compute the set of maximal cones containing r
    Set<Int> rcones;
    for (Int mc = 0; mc < maximalCones.rows(); ++mc) {
      if (maximalCones.row(mc).contains(*r)) {
        rcones += mc;
      }
    }

    // For each such maximal cone, compute its component (if it hasnt been computed yet).
    for (auto mc = entire(rcones); !mc.at_end(); ++mc) {
      if (!inverseMap.exists(*mc)) {
        // Create new component
        Set<Int> newset{ *mc };
        connectedComponents |= newset;
        inverseMap[*mc] = connectedComponents.dim()-1;

        // Do a breadth-first search for all other cones in the component
        std::list<Int> queue;
        queue.push_back(*mc);
        // Semantics: Elements in that queue have been added but their neighbours might not
        while (!queue.empty()) {
          Int node = queue.front(); //Take the first element and find its neighbours
          queue.pop_front();
          for (auto othercone = entire(rcones); !othercone.at_end(); ++othercone) {
            // We only want 'homeless' cones
            if (!inverseMap.exists(*othercone)) {
              // This checks whether both cones share a ray with x0=1
              if (!(maximalCones.row(node) * maximalCones.row(*othercone) * affineRays).empty()) {
                // Add this cone to the component
                connectedComponents[connectedComponents.dim()-1] += *othercone;
                inverseMap[*othercone] = connectedComponents.dim()-1;
                queue.push_back(*othercone);
              }
            }
          }
        }
        
      }
    } //END computation of connected components

    // Now add r once for each connected component to the appropriate cones
    for (Int cc = 0; cc < connectedComponents.dim(); ++cc) {
      cmplxrays = cmplxrays / rays.row(*r);
      conversion |= (*r);
      Int rowindex = cmplxrays.rows()-1;
      Set<Int> ccset = connectedComponents[cc];
      for (auto mc = entire(ccset); !mc.at_end(); ++mc) {
        maxcones[*mc] = maxcones[*mc] + rowindex;
        // For each facet of mc that contains r, add rowindex
        Set<Int> fcset;
        // If there are maximal cones not intersecting x0 = 1, they have no facets
        // in facet_incidences, hence the following check
        if (*mc < facet_incidences.rows()) {
          fcset = facet_incidences.row(*mc);
        }
        for (auto fct = entire(fcset); !fct.at_end(); ++fct) {
          if (codimOneCones.row(*fct).contains(*r)) {
            codimone[*fct] += rowindex;
          }
        }
      }
    }

  } // END iterate over all rays

  X.take("SEPARATED_VERTICES") << cmplxrays;
  X.take("SEPARATED_MAXIMAL_POLYTOPES") << maxcones;
  X.take("SEPARATED_CODIMENSION_ONE_POLYTOPES") << codimone;
  X.take("SEPARATED_CONVERSION_VECTOR") << conversion;

} // END computeSeparatedData

Function4perl(&computeSeparatedData,"computeSeparatedData(Cycle)");

} }
