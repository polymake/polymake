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

	Contains a function to make an arbitrary list of polyhedra into a polyhedral complex.
	*/

#pragma once

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/PowerSet.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/thomog.h"
#include "polymake/polytope/convex_hull.h"

namespace polymake { namespace tropical {

/*
 * @brief Takes a collection of weighted polyhedra of the same dimension and refines them in such a way that
 * they form a weighted polyhedral complex. Weights of cells lying one over the other add up.
 * @param Matrix<Rational> rays A matrix of rays in tropical projective coordinates and with leading coordinate.
 * @param Vector<Set<Int>> max_cones A list of cells in terms of the rays.
 * @oaram Vector<Integer> weights The i-th element is the weight of the i-th cone.
 * @return Cycle A weighted complex, whose support is the union of the given cones.
 *
 */
template <typename Addition>
BigObject make_complex(Matrix<Rational> rays, Vector<Set<Int>> max_cones, Vector<Integer> weights)
{
  rays = tdehomog(rays);
  // First we compute all the H-representations of the cones
  Vector<Matrix<Rational>> inequalities(max_cones.dim());
  Vector<Matrix<Rational>> equalities(max_cones.dim());
  for (Int c = 0; c < max_cones.dim(); ++c) {
    // To make computations come out right, we always have to add a zero in front
    const std::pair<Matrix<Rational>, Matrix<Rational>> fanEqs =
      polytope::enumerate_facets(rays.minor(max_cones[c],All), true);
    inequalities[c] = fanEqs.first;
    equalities[c] = fanEqs.second;
  }
  // Compute the dimension
  const Int dimension = rays.cols() - equalities[0].rows() - 1;

  // The following matrix marks pairs of cones (i,j) that don't need to be made compatible since they already are
  // More precisely: a value of true at position (i,j) means that cone i and cone j are compatible.
  IncidenceMatrix<Symmetric> markedPairs(max_cones.dim(), max_cones.dim());

  // The following value indicates that new cones have been created through refinement 
  bool created;
  // The following values indicate that during an instance of the for(i,j)-loop below, 
  // the cone i and/or j has been removed and replaced by other cones. Note that here
  // assigning a different weight does count as "new"
  //     bool replacedI = false;
  //     bool replacedJ = false;

  // We go through all cone pairs i < j and check if we need to refine anything. As soon as we actually made some changes, we start all over
  do {
    created = false;
    for (Int i = 0; i < max_cones.dim() -1 && !created; ++i) {
      for (Int j = i+1; j < max_cones.dim() && !created; ++j) {
        // replacedI = false; replacedJ = false;
        // We only intersect non-marked pairs
        if (!markedPairs(i, j)) {
          // Compute an irredundant H-rep of the intersection of cones i and j
          Matrix<Rational> isIneq = inequalities[i]/inequalities[j];
          Matrix<Rational> isEq = equalities[i] / equalities[j];
          Matrix<Rational> isMatrix = isIneq / isEq;

          const auto isection = polytope::get_non_redundant_inequalities(isIneq, isEq, true);

          const Int isDimension = isMatrix.cols() - isection.second.size() - 1; 

          // If the dimension is < 0 (this can only occur in the homog. case), then the two
          // cells don't intersect at all and we don't need to refine
          if (isDimension < 0) continue;

          // This set contains the indices in isMatrix of equations coming from i
          Set<Int> IIndices = sequence(0, inequalities[i].rows()) + sequence(isIneq.rows(), equalities[i].rows());

          // We now put together the equations for the intersection, such that the equations from i
          // come first
          // Note: If the intersection is full-dimensional, we don't need to refine along the equalities
          Matrix<Rational> hsEquations = isMatrix.minor(isection.first * IIndices, All);
          if (isDimension < dimension)
            hsEquations /= isMatrix.minor(isection.second * IIndices, All);
          Int equationsFromI = hsEquations.rows();
          hsEquations /= isMatrix.minor(isection.first - IIndices, All);
          if (isDimension < dimension)
            hsEquations /= isMatrix.minor(isection.second - IIndices, All);

          // These variables remember the refinements of i and j (as indices in max_cones)
          Set<Int> newconesI, newconesJ;	

          for (Int coneindex = i; coneindex <= j; coneindex += (j-i)) {
            // First we have to determine the relevant sign choices. We only want to change
            // the signs of equations NOT coming from the cone we currently refine
            // A sign choice's semantics is that x is in signChoices[s], iff the equation at index
            // s in relevantEquations has sign +1, otherwise it has sign -1
            Matrix<Rational> relevantEquations;
            if (coneindex == j) { 
              relevantEquations = hsEquations.minor(sequence(0, equationsFromI), All);
            } else {
              relevantEquations = hsEquations.minor(range_from(equationsFromI), All);
            }
            Array<Set<Int>> signChoices{ all_subsets(sequence(0, relevantEquations.rows())) };

            for (Int s = 0; s < signChoices.size(); ++s) {
              // If the intersection is full-dimensional and the signs are all +1's, then we
              // only compute this intersection for coneindex == i
              if (isDimension == dimension && signChoices[s].size() == relevantEquations.rows() && coneindex == j) {
                continue;
              }

              // Compute intersection of coneindex with current sign choice

              Matrix<Rational> refIneqs = relevantEquations.minor(signChoices[s],All);
              refIneqs /= (-relevantEquations.minor(~signChoices[s],All));	

              Matrix<Rational> refineIneqs = inequalities[coneindex] / refIneqs;
              try {
                Matrix<Rational> ref = polytope::enumerate_vertices(refineIneqs,
                                                                    equalities[coneindex], false).first;
                // If the refinement is full-dimensional, we get a new cone 
                if (rank(ref) - 1 == dimension) {
                  // First we canonicalize the directional rays
                  for (Int rw = 0; rw < ref.rows(); ++rw) {
                    if (ref(rw,0) == 0) {
                      for (Int cl = 0; cl < ref.cols(); ++cl) {
                        if (ref(rw,cl) != 0) {
                          ref.row(rw) /= abs(ref(rw,cl));
                          break;
                        }
                      }
                    }
                  }

                  // Add as new cone
                  // Go through all rays and check if they already exist
                  // Assign appropriate indices
                  Set<Int> coneRays;
                  Int noOfRays = rays.rows();
                  for (Int nr = 0; nr < ref.rows(); ++nr) {
                    for (Int r = 0; r < noOfRays; ++r) {
                      if (rays.row(r) == ref.row(nr)) {
                        coneRays += r;
                        break;
                      }
                      if (r == noOfRays-1) {
                        rays /= ref.row(nr);
                        Int newrayindex = rays.rows()-1;
                        coneRays += newrayindex;
                      }
                    }
                  }

                  // We have to check for doubles: If i fulfills one of the relevant equations with
                  // equality, we can change its sign and still get the same refinement cone
                  bool isDouble = false;
                  for (auto existing = entire(coneindex == i? newconesI : newconesJ); !existing.at_end(); ++existing) {
                    if (max_cones[*existing] == coneRays) {
                      isDouble = true; break;
                    }
                  }
                  if (isDouble) continue;

                  // Add the cone
                  max_cones |= coneRays;
                  Int newconeindex = max_cones.dim()-1;
                  if (coneindex == i)
                    newconesI += newconeindex;
                  else
                    newconesJ += newconeindex;

                  // Weight is the weight of coneindex (or the sum of both, if s is everything)
                  weights |= (isDimension < dimension || 
                              signChoices[s].size() < relevantEquations.rows()? 
                              weights[coneindex] : weights[i] + weights[j]);

                  inequalities |= (inequalities[coneindex] / refIneqs);
                  equalities |= equalities[coneindex];
                  markedPairs.resize(markedPairs.rows() + 1, markedPairs.rows() + 1);
                } //END if refDimension = dimension
              }
              catch(...) {}
            } //END iterate sign choices
          } //END iterate i and j

          // Now if we did create anything new, we remove i and/or j
          // (depending on whether any proper refinement of each cone was created)
          // We only set 'created' to true, if at least one of i and j has 
          // been replaced by at least two cones.
          // If both have been replaced by a single cone (i.e. by themselves),
          // we throw out the new cones instead of the old ones
          // (Except, if the intersection is full-dimensional. In this case i and j agree
          // and they are replaced by the single new cone with weight the sum of their weights)
          if (newconesI.size() + newconesJ.size() > 0) {
            created = true; //newconesI.size() >= 2 || newconesJ.size() >= 2;
            // Will contain the indices of cones to be marked compatible
            Vector<Int> markIndices;
            // Will contain the indices of cones to be removed
            Set<Int> removedIndices;

            // Find out indices to be marked and removed:

            // If the cones intersect in full dimension, then we keep the new cones in any case
            // and discard the old ones
            if (isDimension == dimension) {
              removedIndices += i; removedIndices += j;
              markIndices |= Vector<Int>(newconesI);
              markIndices |= Vector<Int>(newconesJ);
              // replacedI = replacedJ = true;
            } else {
              // Otherwise we discard the old cones iff they have been refined by at least 2 cones
              if (newconesI.size() >= 2) {
                markIndices |= Vector<Int>(newconesI);
                removedIndices += i;
                //replacedI = true;
              } else {
                markIndices |= i;
                removedIndices += newconesI;
              }		  

              if (newconesJ.size() >= 2) {
                markIndices |= Vector<Int>(newconesJ);
                removedIndices += j;
                //replacedJ = true;
              } else {
                markIndices |= j;
                removedIndices += newconesJ;
              }
            }

            for (Int v = 0; v < markIndices.dim()-1; ++v) {
              for (Int w = v+1; w < markIndices.dim(); ++w) {
                markedPairs(markIndices[v], markIndices[w]) = true;
              }
            }

            // Now remove them
            max_cones = max_cones.slice(~removedIndices);
            weights = weights.slice(~removedIndices);
            inequalities = inequalities.slice(~removedIndices);
            equalities = equalities.slice(~removedIndices);
            markedPairs = markedPairs.minor(~removedIndices, ~removedIndices);
          }
        } //END refine i and j
      } //END iterate j
    } //END iterate i
  } while(created);

  return BigObject("Cycle", mlist<Addition>(),
                   "VERTICES", thomog(rays),
                   "MAXIMAL_POLYTOPES", max_cones,
                   "WEIGHTS", weights);
}

} }

