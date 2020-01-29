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

	This file contains functions to compute triangulations (of fans, complexes,...)
	*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/polytope/beneath_beyond.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/misc_tools.h"

namespace polymake { namespace tropical {

// Documentation see perl wrapper
template <typename Addition>
BigObject triangulate_cycle(BigObject fan)
{
  Matrix<Rational> rays = fan.give("VERTICES");
  // Dehomogenize 
  rays = tdehomog(rays);
  IncidenceMatrix<> cones = fan.give("MAXIMAL_POLYTOPES");
  Matrix<Rational> lin_space = fan.give("LINEALITY_SPACE");

  const Int fan_dim = fan.give("PROJECTIVE_DIM");

  Vector<Integer> weights;
  const bool weights_exist = fan.lookup("WEIGHTS") >> weights;

  // Number of rays of a simplicial cone.
  const Int no_of_rays = fan_dim - lin_space.rows() + 1;

  // Will contain the triangulating cones and their weights
  Vector<Set<Int>> triangleCones;
  Vector<Integer> triangleWeights;

  const polytope::BeneathBeyondConvexHullSolver<Rational> bb_solver{};

  // Go through all cones
  for (Int sigma = 0; sigma < cones.rows(); ++sigma) {
    if (cones.row(sigma).size() > no_of_rays) {
      const Array<Set<Int>> triang_seq = bb_solver.placing_triangulation(rays.minor(cones.row(sigma), All));
      Vector<Int> rays_as_list(cones.row(sigma));
      for (const auto& tr : triang_seq) {
        triangleCones |= Set<Int>(rays_as_list.slice(tr));
        if (weights_exist) triangleWeights |= weights[sigma];
      }
    } else {
      triangleCones |= cones.row(sigma);
      if (weights_exist) {
        triangleWeights |= weights[sigma];
      }
    }
  } // END iterate all cones

  BigObject result("Cycle", mlist<Addition>());
  result.take("VERTICES") << thomog(rays);
  result.take("MAXIMAL_POLYTOPES") << triangleCones;
  result.take("LINEALITY_SPACE") << lin_space;
  if (weights_exist) result.take("WEIGHTS") << triangleWeights;

  return result;
}


// Documentation see perl wrapper
template <typename Addition>
BigObject insert_rays(BigObject fan, Matrix<Rational> add_rays)
{
  // Triangulate fan first
  fan = triangulate_cycle<Addition>(fan);

  // Extract values
  Matrix<Rational> rays = fan.give("VERTICES");
  Matrix<Rational> lineality = fan.give("LINEALITY_SPACE");
  // Dehomogenize
  rays = tdehomog(rays);
  lineality = tdehomog(lineality);
  IncidenceMatrix<> cones = fan.give("MAXIMAL_POLYTOPES");
  Vector<Integer> weights;
  const bool weights_exist = fan.lookup("WEIGHTS") >> weights;

  add_rays = tdehomog(add_rays);

  // First we check if any of the additional rays is already a ray of the fan

  Set<Int> rays_to_check;
  for (Int r = 0; r < add_rays.rows(); ++r) {
    bool is_contained = false;
    Matrix<Rational> linplusray = lineality / add_rays.row(r);
    const Int linrk = rank(linplusray);
    if (linrk == lineality.rows()) break; // Zero mod lineality 
    for (Int oray = 0; oray < rays.rows(); ++oray) {
      if (rank(linplusray / rays.row(r)) == linrk) {
        is_contained = true; break;
      }
    }
    if (!is_contained) rays_to_check += r;
  } //END check if rays are already in fan

  add_rays = add_rays.minor(rays_to_check,All);

  // Now iterate over the remaining rays
  for (Int nr = 0; nr < add_rays.rows(); ++nr) {
    Vector<Set<Int>> nr_cones;
    Vector<Integer> nr_weights;

    rays /= add_rays.row(nr);
    Int nr_ray_index = rays.rows()-1;

    // Go through all cones and check if they contain this ray
    for (Int mc = 0; mc < cones.rows(); ++mc) {
      bool contains_ray = false;
      Matrix<Rational> relations = null_space( T(rays.minor(cones.row(mc),All) / lineality / add_rays.row(nr)));
      // Since fan is simplicial, this matrix can have at most one relation
      if (relations.rows() > 0) {
        // Check if - assuming the last coefficient is < 0 - all ray coeffs are >= 0
        // Remember those ray coefficients that are > 0
        if (relations(0,relations.cols()-1) > 0) relations.negate();
        contains_ray = true;
        Set<Int> greater_zero;
        for (Int c = 0; c < cones.row(mc).size(); ++c) {
          if (relations(0,c) < 0) {
            contains_ray = false;
          }
          if (relations(0,c) > 0) {
            greater_zero += c;
          }
        } //END check coefficients

        // If it is contained, subdivide the cone accordingly:
        // For each non-zero-coefficient: take the codimension one face obtained
        // by removing the corresponding ray and add the new ray
        if (contains_ray) {
          Vector<Int> rays_as_list(cones.row(mc));
          for (auto gzrays = entire(greater_zero); !gzrays.at_end(); ++gzrays) {
            Set<Int> nr_cone(rays_as_list.slice(~scalar2set(*gzrays)));
            nr_cone += nr_ray_index;
            nr_cones |= nr_cone;
            if (weights_exist) nr_weights |= weights[mc];
          } // xsEND iterate rays with coeff > 0
        } // END subdivide cone
      } // END if there is a relation

      // If it does not contain the ray, just copy it
      if (!contains_ray) {
        nr_cones |= cones.row(mc);
        if (weights_exist) nr_weights |= weights[mc];
      }
    } // END iterate maximal cones

    // Copy new cones and weights
    cones = IncidenceMatrix<>(nr_cones);
    weights = nr_weights;

  } // END iterate new rays

  // Create result
  BigObject result("Cycle", mlist<Addition>());
  result.take("VERTICES") << thomog(rays);
  result.take("MAXIMAL_POLYTOPES") << cones;
  result.take("LINEALITY_SPACE") << thomog(lineality);
  if (weights_exist) result.take("WEIGHTS") << weights;

  return result;
}

UserFunctionTemplate4perl("# @category Basic polyhedral operations"
                          "# Takes a cycle and computes a triangulation"
                          "# @param Cycle<Addition> F A cycle (not necessarily weighted)"
                          "# @return Cycle<Addition> A simplicial refinement of F",
                          "triangulate_cycle<Addition>(Cycle<Addition>)");

UserFunctionTemplate4perl("# @category Basic polyhedral operations"
                          "# Takes a cycle and a list of rays/vertices in tropical projective coordinates with"
                          "# leading coordinate and triangulates the fan"
                          "# such that it contains these rays"
                          "# @param Cycle<Addition> F A cycle (not necessarily weighted)."
                          "# @param Matrix<Rational> R A list of normalized vertices or rays"
                          "# Note that the function will NOT subdivide the lineality space, i.e. rays that are "
                          "# equal to an existing ray modulo lineality space will be ignored."
                          "# @return Cycle<Addition> A triangulation of F that contains all the "
                          "# original rays of F plus the ones in R",
                          "insert_rays<Addition>(Cycle<Addition>,$)");
} }
