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
	Copyright (c) 2016-2022
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Implements the composition of two morphisms.
	*/

#pragma once

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/morphism_thomog.h"
#include "polymake/tropical/refine.h"
#include "polymake/tropical/morphism_values.h"
#include "polymake/polytope/convex_hull.h"

namespace polymake { namespace tropical {

/**
   @brief Computes the composition g(f) of two morphisms f and g (as in f:X->Y, g:Y->Z). 
   Actually, f and g can also be piecewise linear maps on their domains, the method will work equally well. 
   The function does not require that g's [[DOMAIN]] contain the image of f, the composition will always
   be defined on the preimage of g's [[DOMAIN]] under f.
   @param BigObject f A Morphism
   @param BigObject g A Morphism 
   @return BigObject A Morphism object, the composition "g after f" The weights of f's domain are
   copied onto the cones of the new domain, where they have full dimension.
*/
template <typename Addition>
BigObject morphism_composition(BigObject f, BigObject g)
{
  // -------------------------- PREPARATIONS ----------------------------------- //

  // Extract values of f
  BigObject f_domain = f.give("DOMAIN");
  Matrix<Rational> f_rays = f_domain.give("SEPARATED_VERTICES");
  Matrix<Rational> f_lin = f_domain.give("LINEALITY_SPACE");
  bool f_has_weights = f_domain.exists("WEIGHTS");
  Vector<Integer> f_domain_weights;
  if (f_has_weights)
    f_domain.give("WEIGHTS") >> f_domain_weights;
  const IncidenceMatrix<> f_cones = f_domain.give("SEPARATED_MAXIMAL_POLYTOPES");
  bool f_has_matrix = f.exists("MATRIX") || f.exists("TRANSLATE");
  Matrix<Rational> f_on_rays = f.give("VERTEX_VALUES");
  Matrix<Rational> f_on_lin = f.give("LINEALITY_VALUES");
  Matrix<Rational> f_prop_matrix; Vector<Rational> f_prop_translate;
  Matrix<Rational> f_dehomog_matrix; Vector<Rational> f_dehomog_translate;
  if (f_has_matrix) {
    f.give("MATRIX") >> f_prop_matrix;
    f.give("TRANSLATE") >> f_prop_translate;
    std::pair<Matrix<Rational>, Vector<Rational> > dehom_f = tdehomog_morphism(f_prop_matrix, f_prop_translate);
    f_dehomog_matrix = dehom_f.first;
    f_dehomog_translate = dehom_f.second;
  }
  std::vector< Matrix<Rational> > f_hreps_ineq;
  std::vector< Matrix<Rational> > f_hreps_eq;

  // Now tropically dehomogenize everything and create version of f-values with leading coordinate.
  f_rays = tdehomog(f_rays,0);
  f_lin = tdehomog(f_lin,0);
  f_on_rays = tdehomog(f_on_rays, 0, false);
  f_on_lin = tdehomog(remove_zero_rows(f_on_lin), 0, false);
  Matrix<Rational> f_on_rays_homog = f_rays.col(0) | f_on_rays;
  Matrix<Rational> f_on_lin_homog = zero_vector<Rational>() | f_on_lin;

  Int f_ambient_dim = std::max(f_rays.cols(), f_lin.cols()); 

  // Extract values of g
  BigObject g_domain = g.give("DOMAIN");
  bool g_has_matrix = g.exists("MATRIX") || g.exists("TRANSLATE");
  Matrix<Rational> g_prop_matrix; Vector<Rational> g_prop_translate;
  Matrix<Rational> g_dehomog_matrix; Vector<Rational> g_dehomog_translate;
  if (g_has_matrix) {
    g.give("MATRIX") >> g_prop_matrix;
    g.give("TRANSLATE") >> g_prop_translate;
    std::pair<Matrix<Rational>, Vector<Rational>> dehom_g = tdehomog_morphism(g_prop_matrix, g_prop_translate);
    g_dehomog_matrix = dehom_g.first;
    g_dehomog_translate = dehom_g.second;
  }
  Matrix<Rational> g_rays = g_domain.give("SEPARATED_VERTICES");
  Matrix<Rational> g_lin = g_domain.give("LINEALITY_SPACE");
  Int g_domain_dim = g_domain.give("PROJECTIVE_DIM");
  IncidenceMatrix<> g_cones = g_domain.give("SEPARATED_MAXIMAL_POLYTOPES");
  Matrix<Rational> g_on_rays = g.give("VERTEX_VALUES");
  Matrix<Rational> g_on_lin = g.give("LINEALITY_VALUES");
  std::vector< Matrix<Rational> > g_hreps_ineq;
  std::vector< Matrix<Rational> > g_hreps_eq;

  // Tropically dehomogenize everything - g's values don't need a leading coordinate
  g_rays = tdehomog(g_rays,0);
  g_lin = tdehomog(g_lin,0);
  g_on_rays = tdehomog(g_on_rays,0,false);
  g_on_lin = tdehomog(g_on_lin,0,false);	
  
  // Prepare result variables
  ListMatrix<Vector<Rational>> pullback_rays(0, f_ambient_dim);
  ListMatrix<Vector<Rational>> pullback_lineality(0, f_ambient_dim);
  bool lineality_computed = false;
  std::vector<Set<Int>> pullback_cones;
  Set<Set<Int>> pullback_cones_set; //Used to check for doubles
  ListMatrix<Vector<Rational>> pullback_ray_values(0,g_on_rays.cols());
  ListMatrix<Vector<Rational>> pullback_lin_values(0,g_on_lin.cols());
  std::vector<Integer> pullback_weights;
  // The following two variables contain for each cone of the pullback domain the representation 
  // as an affine linear function on this cone
  std::vector<Matrix<Rational> > pullback_matrices; 
  std::vector<Vector<Rational> > pullback_translates;

  // Compute H-representations of all cones in f_domain and g_domain

  for (Int fcone = 0; fcone < f_cones.rows(); ++fcone) {
    const auto p = polytope::enumerate_facets(f_rays.minor(f_cones.row(fcone),All), f_lin, false);
    f_hreps_ineq.push_back(p.first);
    f_hreps_eq.push_back(p.second);
  } //END compute fcone-H-rep

  for (Int gcone = 0; gcone < g_cones.rows(); ++gcone) {
    const auto p = polytope::enumerate_facets(g_rays.minor(g_cones.row(gcone),All), g_lin, false);
    g_hreps_ineq.push_back(p.first);
    g_hreps_eq.push_back(p.second);
  } //END compute gcone-H-rep

  bool have_full_dimensional_pullback_cone = false;

  // --------------------------- COMPUTE GEOMETRY ---------------------------------- //

  // Now iterate all cones of f's domain
  for (Int fcone = 0; fcone < f_cones.rows(); ++fcone) {
    // Compute H-representation of the image of the cone
    const auto image_rep = polytope::enumerate_facets(f_on_rays_homog.minor(f_cones.row(fcone),All), f_on_lin_homog, false); 

    Int image_dim = image_rep.first.cols() - image_rep.second.rows() - 1;

    // Compute representation of morphism on current cone
    Matrix<Rational> fmatrix;
    Vector<Rational> ftranslate;
    if (f_has_matrix) {
      fmatrix = f_dehomog_matrix;
      ftranslate = f_dehomog_translate;
    } else {
      computeConeFunction(f_rays.minor(f_cones.row(fcone),All), f_lin,
                          f_on_rays.minor(f_cones.row(fcone),All),
                          f_on_lin, ftranslate, fmatrix);
    }

    // Iterate all cones of the function
    for (Int gcone = 0; gcone < g_cones.rows(); ++gcone) {
      // Compute intersection 
      Matrix<Rational> intersection_ineq;
      Matrix<Rational> intersection_eq;

      // Compute an irredundant H-rep of the intersection

      intersection_ineq = image_rep.first / g_hreps_ineq[gcone];
      intersection_eq = image_rep.second / g_hreps_eq[gcone];
      const auto isMatrix = intersection_ineq / intersection_eq;

      const auto isection = polytope::get_non_redundant_inequalities(intersection_ineq, intersection_eq, true);
      const Int interdim = isMatrix.cols()  - isection.second.size() - 1;
      std::tie(intersection_ineq, intersection_eq) = std::make_pair(isMatrix.minor(isection.first, All), isMatrix.minor(isection.second, All));

      // Check dimension of intersection - if its not the correct one, take the next g cone
      if (interdim != std::min(image_dim, g_domain_dim)) continue;

      // Compute g's representation on the current cone
      Vector<Rational> gtranslate;
      Matrix<Rational> gmatrix;
      if (g_has_matrix) {
        gmatrix = g_dehomog_matrix;
        gtranslate = g_dehomog_translate;
      } else {
        computeConeFunction(g_rays.minor(g_cones.row(gcone),All), g_lin, 
                            g_on_rays.minor(g_cones.row(gcone),All), g_on_lin, gtranslate, 
                            gmatrix);
      }

      // Compute preimage of the intersection cone
      // If (b,-A) is the representation of (in)equalities of the cone
      // and x |-> v + Mx is the representation of the morphism, then
      // (b - Av, -AM) is the representation of the preimage

      Matrix<Rational> preimage_ineq = intersection_ineq.minor(All, range_from(1)) * fmatrix;
      preimage_ineq = (intersection_ineq.col(0) + intersection_ineq.minor(All, range_from(1)) * ftranslate)
        | preimage_ineq;

      Matrix<Rational> preimage_eq(0,preimage_ineq.cols());
      if (intersection_eq.rows() > 0) { //For the equalities consider the special case that there are none
        preimage_eq = intersection_eq.minor(All, range_from(1)) * fmatrix;
        preimage_eq = (intersection_eq.col(0) + intersection_eq.minor(All, range_from(1)) * ftranslate)
                      | preimage_eq;
      }

      // Intersect with the fcone
      preimage_ineq /= f_hreps_ineq[fcone];
      preimage_eq /= f_hreps_eq[fcone];

      auto preimage_cone = polytope::try_enumerate_vertices(preimage_ineq, preimage_eq, false);
      Matrix<Rational> preimage_rays = preimage_cone.first;
      Matrix<Rational> preimage_lin = preimage_cone.second;

      // Canonicalize rays and create cone
      if (!lineality_computed) {
        pullback_lineality = preimage_lin;
        lineality_computed = true;
      }
      Set<Int> pcone; 
      for (Int r = 0; r < preimage_rays.rows(); ++r) {
        // Canonicalize ray
        if (preimage_rays(r,0) != 0) {
          preimage_rays.row(r) *= (1/preimage_rays(r,0));
        } else {
          for (Int c = 1; c < preimage_rays.cols(); ++c) {
            if (preimage_rays(r,c) != 0) {
              preimage_rays.row(r) /= abs(preimage_rays(r,c));
              break;
            }
          }
        }
        // Find correct ray index
        Int ray_index = -1;
        Int oray = 0;
        for (auto pb_ray_it = entire(rows(pullback_rays)); !pb_ray_it.at_end(); ++oray, ++pb_ray_it) {
          if (*pb_ray_it == preimage_rays.row(r)) {
            ray_index = oray; 
            break;
          }
        }
        // Insert ray if necessary and add index to set
        if (ray_index == -1) {
          pullback_rays /= preimage_rays.row(r);
          ray_index = pullback_rays.rows() -1;
        }
        pcone += ray_index;
      }
      // Add cone if it doesn't exist yet
      if (!pullback_cones_set.contains(pcone)) {
        pullback_cones.push_back(pcone);
        pullback_cones_set += pcone;

        if (interdim == image_dim)
          have_full_dimensional_pullback_cone = true;
        // If the pullback cone has full dimension (<=> the dimension of
        // the intersection is the dimension of the image cone), we copy f's domain's weight.
        if (f_has_weights) { 
          pullback_weights.push_back( (interdim == image_dim? f_domain_weights[fcone] :0) );
        }

        // Now we compute the representation of h = g after f 
        Matrix<Rational> hmatrix = gmatrix * fmatrix;
        Vector<Rational> htranslate = gmatrix * ftranslate + gtranslate;

        pullback_matrices.push_back(hmatrix);
        pullback_translates.push_back(htranslate);
      }
    } //END iterate all cones of morphism g 
  } //END iterate cones of morphism f

  // ------------------------- COMPUTE VALUES --------------------------------- //

  // Compute SEPARATED_VERTICES / SEPARATED_MAXIMAL_POLYTOPES

  BigObject pullback_domain("Cycle", mlist<Addition>());
  pullback_domain.take("VERTICES") << thomog(pullback_rays);
  pullback_domain.take("MAXIMAL_POLYTOPES") << pullback_cones;
  pullback_domain.take("LINEALITY_SPACE") << thomog(pullback_lineality);
  if ((f_has_weights && have_full_dimensional_pullback_cone) || pullback_rays.rows() == 0) { 
    pullback_domain.take("WEIGHTS") << pullback_weights;
  }
  Matrix<Rational> pb_cmplx_rays = pullback_domain.give("SEPARATED_VERTICES");
  Matrix<Rational> pb_crays_dehomog = (tdehomog(pb_cmplx_rays)).minor(All, range_from(1));
  IncidenceMatrix<> pb_cmplx_cones = pullback_domain.give("SEPARATED_MAXIMAL_POLYTOPES");
  IncidenceMatrix<> pb_cones_by_rays = T(pb_cmplx_cones);

  // Go trough all rays
  Int basepoint = -1; // Save the first vertex
  Vector<Rational> basepoint_value;
  for (Int cr = 0; cr < pb_cmplx_rays.rows(); ++cr) {
    // Take any cone containing this ray
    Int cone_index = *(pb_cones_by_rays.row(cr).begin());
    const Matrix<Rational>& cone_matrix = pullback_matrices[cone_index];
    const Vector<Rational>& cone_translate = pullback_translates[cone_index];
    // If its a vertex, just compute the value
    if (pb_cmplx_rays(cr, 0) == 1) {
      auto pbr_value = cone_matrix* pb_crays_dehomog.row(cr) + cone_translate;
      pullback_ray_values /= pbr_value;
      if (basepoint == -1) {
        basepoint_value = pbr_value; basepoint = cr;
      }
    }
    // Otherwise find an associated vertex
    else {
      const auto& rays_in_cone = pb_cmplx_cones.row(cone_index);
      for (auto aRay = entire(rays_in_cone); !aRay.at_end(); ++aRay) {
        if (pb_cmplx_rays(*aRay,0) == 1) {
          Vector<Rational> sum = pb_crays_dehomog.row(*aRay) + pb_crays_dehomog.row(cr);
          pullback_ray_values /= 
             (cone_matrix* sum + cone_translate) - 
             (cone_matrix* pb_crays_dehomog.row(*aRay) + cone_translate);
          break;
        }
      }
    }
  }

  // Now compute lineality values
  Matrix<Rational> pb_lin_dehomog = pullback_lineality.minor(All, range_from(1));
  for (Int l = 0; l < pullback_lineality.rows(); ++l) {
    Vector<Rational> sum = pb_crays_dehomog.row(basepoint) + pb_lin_dehomog.row(l);
    pullback_lin_values /= pullback_matrices[0] * sum + pullback_translates[0] - basepoint_value ;
  }

  // ------------------------------- RETURN RESULT ------------------------------- //

  BigObject result("Morphism", mlist<Addition>());
  result.take("DOMAIN") << pullback_domain;
  result.take("VERTEX_VALUES") << thomog(pullback_ray_values,0,false);
  result.take("LINEALITY_VALUES") << thomog(pullback_lin_values,0,false);

  if (f_has_matrix && g_has_matrix) {
    result.take("MATRIX") << (g_prop_matrix * f_prop_matrix);
    result.take("TRANSLATE") << (g_prop_matrix * f_prop_translate + g_prop_translate);
  }

  return result;
}

} }

