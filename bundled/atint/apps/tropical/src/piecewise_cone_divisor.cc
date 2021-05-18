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

	Contains functions to compute the affine transform of a cycle 
	*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/tropical/divisor.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/misc_tools.h"
#include "polymake/tropical/skeleton.h"

namespace polymake { namespace tropical {

// Documentation see perl wrapper
template <typename Addition>	
BigObject piecewise_divisor(BigObject fan, const IncidenceMatrix<>& cones, const Vector<Integer>& coefficients)
{
  // Basic security checks
  if (cones.rows() != coefficients.dim()) 
    throw std::runtime_error("Cannot compute divisor of piecewise polynomial: Number of cones does not match number of coefficients");

  // Compute fan dimension
  Int fan_dim = fan.give("PROJECTIVE_DIM");

  Matrix<Rational> fan_rays = fan.give("SEPARATED_VERTICES");
  Set<Int> nonfar = far_and_nonfar_vertices(fan_rays).second;

  // First we compute the appropriate skeleton of fan
  if (cones.rows() == 0) return fan;
  Int result_dim = fan_dim - cones.row(0).size() + 1; // Cones have a vertex!
  BigObject skeleton = skeleton_complex<Addition>(fan,result_dim,true);

  // Extract values of skeleton
  Matrix<Rational> sk_rays = skeleton.give("VERTICES");
  sk_rays = tdehomog(sk_rays);
  IncidenceMatrix<> sk_cones = skeleton.give("MAXIMAL_POLYTOPES");

  // This will contain the weights of the cones in the linear combination
  Vector<Integer> result_weights = zero_vector<Integer>(sk_cones.rows());

  // Now go through the divisors psi_tau for all cones tau in cones
  for (Int tau = 0; tau < cones.rows(); ++tau) {
    if (coefficients[tau] != 0) {
      // Create function matrix
      Matrix<Rational> psi_tau(0,fan_rays.rows());
      // Remove vertex
      const Set<Int> tau_set = cones.row(tau) - nonfar;
      if (tau_set.size() != fan_dim - result_dim) 
        throw std::runtime_error("Cannot compute divisor of piecewise polynomials: Cones have different dimension.");
      for (const Int ts : tau_set) {
        psi_tau /= unit_vector<Rational>(fan_rays.rows(), ts);
      }

      // Compute divisor
      BigObject divisor = divisorByValueMatrix<Addition>(fan,psi_tau);

      // Extract cones, rays and weights
      Matrix<Rational> div_rays = divisor.give("VERTICES");
      div_rays = tdehomog(div_rays);
      IncidenceMatrix<> div_cones = divisor.give("MAXIMAL_POLYTOPES");
      Vector<Integer> div_weights = divisor.give("WEIGHTS");

      // Associate to each ray its original index 
      Map<Int, Int> div_ray_to_old;
      for (Int dr = 0; dr < div_rays.rows(); ++dr) {
        for (Int oray = 0; oray < sk_rays.rows(); ++oray) {
          if (sk_rays.row(oray) == div_rays.row(dr)) {
            div_ray_to_old[dr] = oray; break;
          }
        }
      } // END translate ray indices

      // Now go through all d-dimensional cones in the divisor and insert their weight at the appropriate point
      for (Int rho = 0; rho < div_cones.rows(); ++rho) {
        // Map rho rays to old rays
        Set<Int> rho_old{ div_ray_to_old.map(div_cones.row(rho)) };
        // Find the original cone equal to that
        for (Int oc = 0; oc < sk_cones.rows(); ++oc) {
          if ((sk_cones.row(oc) * rho_old).size() == sk_cones.row(oc).size()) {
            result_weights[oc] += (coefficients[tau] * div_weights[rho]); 
            break;
          }
        }
      } // END iterate divisor cones
    } // END if coeff !=0
  } // END iterate cones 

  // Clean up by removing weight zero cones
  Set<Int> used_cones;
  for (Int c = 0; c < result_weights.dim(); ++c) {
    if (result_weights[c] != 0) used_cones += c;
  }

  Set<Int> used_rays = accumulate(rows(sk_cones.minor(used_cones,All)),operations::add());
  sk_rays = sk_rays.minor(used_rays,All);
  sk_cones = sk_cones.minor(used_cones,used_rays);
  result_weights = result_weights.slice(used_cones);

  return BigObject("Cycle", mlist<Addition>(),
                   "VERTICES", thomog(sk_rays),
                   "MAXIMAL_POLYTOPES", sk_cones,
                   "WEIGHTS", result_weights);
}

UserFunctionTemplate4perl("# @category Divisor computation"
                          "# Computes a divisor of a linear sum of certain piecewise polynomials on a simplicial fan."
                          "# @param Cycle<Addition> F A simplicial fan without lineality space in non-homogeneous coordinates"
                          "# @param IncidenceMatrix cones A list of cones of F (not maximal, but all of the same "
                          "# dimension). Each cone t corresponds to a piecewise polynomial psi_t, defined by "
                          "# subsequently applying the rational functions that are 1 one exactly one ray of t and "
                          "# 0 elsewhere. "
                          "# Note that cones should refer to indices in [[SEPARATED_VERTICES]], which may have"
                          "# a different order"
                          "# @param Vector<Integer> coefficients A list of coefficients a_t corresponding to the "
                          "# cones. "
                          "# @return Cycle<Addition> The divisor sum_t a_t psi_t * F",
                          "piecewise_divisor<Addition>(Cycle<Addition>, $, $)");
} }
