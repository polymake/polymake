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

   Contains functionality for computing divisors
   */

#pragma once

#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/Map.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/localize.h"
#include "polymake/tropical/minimal_interior.h"
#include "polymake/tropical/specialcycles.h"


namespace polymake { namespace tropical {

using LatticeMap = Map<std::pair<Int, Int>, Vector<Integer>>;
using LatticeFunctionMap = Map<std::pair<Int, Int>, Vector<Rational>>;


/**
   @brief Takes as input a tropical cycle and a matrix of rational values.
   Each row of the matrix is interpreted as a value vector on the [[SEPARATED_VERTICES]] and
   [[LINEALITY_SPACE]] generators. The row count of the matrix is arbitrary in principle,
   but should be smaller than or equal to the dimension of the cycle. The function will then compute
   the Weil divisor obtained by intersecting with all the functions described by the rows (starting from top).
   Note that this still produces a meaningful result, if the Cycle is not balanced:
   The "divisor" of a given function is computed by taking all codim-1-faces, at which f is balanced and
   computing weights there.
   @param Cycle fan A tropical variety
   @param Matrix<Rational> values A matrix of rational values
   @tparam Addition Whether divisor values are computed using min or max.
   @return The divisor r_k * ... * r_1 * fan, where r_i is the function described by the i-th row.
*/
template <typename Addition>
BigObject divisorByValueMatrix(BigObject complex, const Matrix<Rational>& values)
{
  // This value carries all the intermediate results.
  BigObject result = complex;

  // Now we extract the values that we will later recompute by hand or that don't change at all

  Matrix<Rational> rays = complex.give("VERTICES");
  Matrix<Rational> crays = complex.give("SEPARATED_VERTICES");
  Vector<Integer> weights = complex.give("WEIGHTS");
  Matrix<Rational> lineality_space = complex.give("LINEALITY_SPACE");
  const Int lineality_dim = complex.give("LINEALITY_DIM");
  IncidenceMatrix<> local_restriction;
  if (complex.exists("LOCAL_RESTRICTION")) {
    complex.give("LOCAL_RESTRICTION") >> local_restriction;
  }

  Matrix<Integer> lattice_generators = complex.give("LATTICE_GENERATORS");
  IncidenceMatrix<> lattice_bases = complex.give("LATTICE_BASES");

  // Do a compatibility check on the value matrix to avoid segfaults in the case of faulty input
  if (values.cols() != crays.rows() + lineality_space.rows()) {
    throw std::runtime_error("Value matrix is not compatible with variety. Aborting computation");
  }

  // WARNING if this cset is put into the next line gcc 5 and 6 misoptimize the code
  // and everything gets messy, i.e. *const* lineality_values changes during the loops below
  const auto& cset = sequence(0,values.cols() - lineality_dim);
  const auto& lineality_values = values.minor(All,~(cset));

  // Prepare the additional variables that will be used in all but the first iteration to recompute
  // the values vector

  // Contains at position i the row index of ray i in the ray matrix of the iteration before
  Vector<Int> newRaysToOldRays;

  // Contains the SEPARATED_MAXIMAL_POLYTOPES of the iteration before
  IncidenceMatrix<> cmplx_oldcones;

  // Tells which new maximal cone is contained in which old maximal cone (this is essentially the
  // MAXIMAL_AT_CODIM_ONE without the rows for cones of weight 0)
  IncidenceMatrix<> newConesInOld;

  // For each cmplx_ray in the LAST iteration, this tells which should be the appropriate
  // column index in values for function value computation
  Vector<Int> cmplx_origins(sequence(0,values.cols() - lineality_dim));

  // Contains the conversion vector for the last iteration (this one we recompute during
  // value recomputation)
  Vector<Int> old_conversion;

  // Only uses in the fan case: For all iterations but the first it contains the set of rays of
  // the last iteration that remained after computing the divisor
  Set<Int> remainingFanRays;

  // When computing the codim-one-weights, this contains the correct function value vector for the current iteration
  // When computing the new function vector for the current iteration, this means it contains the function
  // values of the old iteration
  Vector<Rational> currentValues;

  // Now we iterate through the matrix rows
  for (Int r = 0; r < values.rows(); ++r) {
    // First we recompute values that we can't/won't compute by hand

    const IncidenceMatrix<>& codimOneCones = result.give("CODIMENSION_ONE_POLYTOPES");
    if (codimOneCones.rows() == 0)
      return empty_cycle<Addition>(std::max(rays.cols(), lineality_space.cols())-2);
    const IncidenceMatrix<>& coneIncidences = result.give("MAXIMAL_AT_CODIM_ONE");

    const LatticeFunctionMap& lnFunctionVector = result.give("LATTICE_NORMAL_FCT_VECTOR");
    const Matrix<Rational>& lsumFunctionVector = result.give("LATTICE_NORMAL_SUM_FCT_VECTOR");
    const Set<Int>& unbalancedFaces = result.give("UNBALANCED_FACES");

    // Recompute the lattice bases
    RestrictedIncidenceMatrix<> new_lattice_bases(codimOneCones.rows());
    for (Int co = 0; co < codimOneCones.rows(); ++co) {
      new_lattice_bases.row(co) = lattice_bases.row(coneIncidences.row(co).front());
    }
    lattice_bases = std::move(new_lattice_bases);

    // Now we compute the correct value vector:

    if (r == 0) {
      currentValues = values.row(r);
    } else {
      const Matrix<Rational>& cmplx_rays = result.give("SEPARATED_VERTICES");
      const Vector<Int>& conversion_vector = result.give("SEPARATED_CONVERSION_VECTOR");
      // Compute the maximal cones containing each cmplx_ray
      IncidenceMatrix<> cmplx_cones_t = result.give("SEPARATED_MAXIMAL_POLYTOPES");
      cmplx_cones_t = T(cmplx_cones_t);

      std::vector<Rational> cval_list;
      std::vector<Int> newcmplx_origins;
      for (Int cr = 0; cr < cmplx_rays.rows(); ++cr) {
        // Find the corresponding cmplx_ray in the last iteration
        const Int mc = *(cmplx_cones_t.row(cr).begin()); // A cone containing the ray
        const Int oc = *(newConesInOld.row(mc).begin()); // An old cone containing mc
        // Now find the cmplx_ray of the old cone, such that
        // its corresponding ray is equal to the corresponding ray of the new ray
        for (const Int ocr : cmplx_oldcones.row(oc)) {
          // If the old ray (in non-complex counting in the old iteration) is the same as
          // the new ray (in non-complex counting) in the new iteration, we can
          // copy its function column index
          if (old_conversion[ocr] == newRaysToOldRays[conversion_vector[cr]]) {
            cval_list.push_back(values(r,cmplx_origins[ocr]));
            newcmplx_origins.push_back(cmplx_origins[ocr]);
            break;
          }
        }
      }
      currentValues = Vector<Rational>(cval_list);
      cmplx_origins = Vector<Int>(newcmplx_origins);
      // Finally append lineality values
      if (lineality_dim > 0)
        currentValues |= lineality_values.row(r);
    }

    // Then we compute the divisor
    std::vector<Integer> newweights; // Contains the new weights
    Set<Int> usedCones; // Contains the codim 1 cones with weight != 0
    Set<Int> usedRays; //Contains the rays in used cones
    // Go through each facet and compute its weight.
    for (Int co = 0; co < codimOneCones.rows(); ++co) {
      if (!unbalancedFaces.contains(co)) { // Only compute values at balanced codim-1-cones
        Rational coweight(0); // Have to take rational since intermediate values may be rational
        for (const auto mc : coneIncidences.row(co)) {
          coweight += weights[mc] * lnFunctionVector[std::make_pair(co,mc)] * currentValues;
        }
        // Now substract the value of the lattice normal sum
        coweight -= lsumFunctionVector.row(co) * currentValues;
        if (coweight != 0) {
          // Invert weight sign for min people, the computation is rigged for max
          coweight *= -Addition::orientation();
          newweights.push_back( Integer(coweight) );
          usedCones += co;
          usedRays += codimOneCones.row(co);
        }
      }
    } //END iterate co-1-cones

    // Compute the new-to-old maps used for recomputing the value vector in the next iteration
    if (r != values.rows()-1) {
      remainingFanRays = usedRays;
      newConesInOld = coneIncidences.minor(usedCones, All);
      result.give("SEPARATED_MAXIMAL_POLYTOPES") >> cmplx_oldcones;
      result.give("SEPARATED_CONVERSION_VECTOR") >> old_conversion;
      std::vector<Int> nrtor;
      for (const Int orays : usedRays) {
        nrtor.push_back(orays);
      }
      newRaysToOldRays = Vector<Int>(nrtor);
    }

    // Now recompute the rays and maximal cones for re-initialization of the result
    rays = rays.minor(usedRays,All);
    weights = Vector<Integer>(newweights);
    const auto& newMaximal = codimOneCones.minor(usedCones,usedRays);
    // Recompute local restriction cones
    if (local_restriction.rows() > 0) {
      // We need to adapt rays indices and remove old maximal local cones
      // and codimension one cones that have weight 0
      // Also we remove all local cones that lose rays
      const IncidenceMatrix<>& maxCones = result.give("MAXIMAL_CONES");
      Set<Int> removableCones;
      Set<Int> weightzerocones = sequence(0,codimOneCones.rows()) - usedCones;
      Set<Int> codimToReplace; // Indices of used codim one cones that are local
      for (Int lc = 0; lc < local_restriction.rows(); ++lc) {
        const auto& lrrow = local_restriction.row(lc);
        // If the local cone loses any rays, remove it
        if ((lrrow * usedRays).size() < lrrow.size()) {
          removableCones += lc;
          continue;
        }
        bool found_cone = false;
        for (Int mc = 0; mc < maxCones.rows(); ++mc) {
          if (incl(maxCones.row(mc), lrrow) <= 0) {
            removableCones += lc;
            found_cone = true;
            break;
          }
        }
        for (auto cz = entire(weightzerocones); !cz.at_end() && !found_cone; cz++) {
          if (incl(codimOneCones.row(*cz), lrrow) <= 0) {
            removableCones += lc;
            break;
          }
        }
      }

      // Remove cones
      local_restriction = local_restriction.minor(~removableCones, usedRays);

    } //END adapt local restriction	

    result = BigObject("Cycle", mlist<Addition>());
    result.take("PROJECTIVE_VERTICES") << rays;
    result.take("MAXIMAL_CONES") << newMaximal;
    result.take("WEIGHTS") << weights;
    result.take("LINEALITY_SPACE") << lineality_space;
    if (local_restriction.rows() > 0)
      result.take("LOCAL_RESTRICTION") << local_restriction;
    result.take("LATTICE_GENERATORS") << lattice_generators;
    lattice_bases = lattice_bases.minor(usedCones,All);
    result.take("LATTICE_BASES") << lattice_bases;//(lattice_bases.minor(usedCones,All));
  } //END iterate function rows

  return result;
}

/**
 *	@brief Computes the (k-fold) divisor of a RationalFunction on a given cycle
 *	@param Cycle complex A tropical cycle
 *	@param TropicalRationalFunction function A rational function, the cycle should be contained in
 *	its domain (as a set, not as a polyhedral complex)
 *	@tparam Addition Min or Max.
 *	@return Cycle The divisor.
 */
template <typename Addition>
BigObject divisor_with_refinement(BigObject cycle, BigObject function)
{
  // Restrict the function to the cycle
  const Int power = function.give("POWER");
  BigObject restricted_function = function.call_method("restrict", cycle);

  Vector<Rational> vertex_values = restricted_function.give("VERTEX_VALUES");
  const Vector<Rational>& lineality_values = restricted_function.give("LINEALITY_VALUES");

  BigObject domain = restricted_function.give("DOMAIN");
  // If the cycle had local restriction, we have to refine it as well
  if (cycle.exists("LOCAL_RESTRICTION")) {
    IncidenceMatrix<> ref_local = refined_local_cones(cycle, domain);
    const Matrix<Rational> &nonloc_separated_vertices = domain.give("SEPARATED_VERTICES"); 
    domain = local_restrict<Addition>(domain, ref_local);
    const Matrix<Rational> &loc_vertices = domain.give("VERTICES");
    const Set<Vector<Rational> > loc_vertices_as_set (rows(loc_vertices));
    // We have to check which of the original SEPARATED_VERTICES still remain
    Set<Int> usedRays;
    for (auto sp = entire<indexed>(rows(nonloc_separated_vertices)); !sp.at_end(); ++sp) {
      if (loc_vertices_as_set.contains(*sp)) usedRays += sp.index();
    }
    vertex_values = vertex_values.slice(usedRays);
  }

  const Vector<Rational>& full_values = vertex_values | lineality_values;

  Matrix<Rational> value_matrix(power, full_values.dim());
  for (Int it = 0; it < power; ++it) {
    value_matrix.row(it) = full_values;
  }

  return divisorByValueMatrix<Addition>(domain,value_matrix);
}

/**
 * @brief Computes the divisor of a RationalFunction on a cycle which is supposed to be
 * equal to the [[DOMAIN]] of the function (as a polyhedral complex!)
 * (Note that [[DOMAIN]] needn't have weights, so we can't just take this as cycle).
 */
template <typename Addition>
BigObject divisor_no_refinement(BigObject cycle, BigObject function)
{
  const Int power = function.give("POWER");
  const Vector<Rational>& vertex_values = function.give("VERTEX_VALUES");
  const Vector<Rational>& lineality_values = function.give("LINEALITY_VALUES");
  const Vector<Rational>& full_values = vertex_values | lineality_values;

  Matrix<Rational> value_matrix(power, full_values.dim());
  for (Int it = 0; it < power; ++it) {
    value_matrix.row(it) = full_values;
  }

  return divisorByValueMatrix<Addition>(cycle,value_matrix);
}

} }

