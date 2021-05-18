
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

	Implements convex_hull_tools.h
	*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/FacetList.h"
#include "polymake/polytope/convex_hull.h"
#include "polymake/tropical/convex_hull_tools.h"
#include "polymake/tropical/thomog.h"

namespace polymake { namespace tropical {


///////////////////////////////////////////////////////////////////////////////////////

//Documentation see header
Vector<Int> insert_rays(Matrix<Rational>& rays, Matrix<Rational> nrays, bool is_normalized)
{
  // Normalize new rays, if necessary
  if (!is_normalized)
    normalize_rays(nrays);

  // Insert rays
  std::vector<Int> new_ray_indices;
  for (auto nr = entire(rows(nrays)); !nr.at_end(); ++nr) {
    Int new_rayindex = -1;
    for (auto oray = entire<indexed>(rows(rays)); !oray.at_end(); ++oray) {
      if (*oray == *nr) {
        new_rayindex = oray.index(); break;
      }
    }
    if (new_rayindex == -1) {
      rays /= *nr;
      new_rayindex = rays.rows()-1;
    }
    new_ray_indices.push_back(new_rayindex);
  }

  return Vector<Int>(new_ray_indices);
} //END insert_rays

///////////////////////////////////////////////////////////////////////////////////////

//Documentation see header
std::pair<Matrix<Rational>, Matrix<Rational> >
cone_intersection(const Matrix<Rational>& xrays, const Matrix<Rational>& xlin,
                  const Matrix<Rational>& yrays, const Matrix<Rational>& ylin)
{
  // Compute facets
  const auto x_eq = polytope::enumerate_facets(xrays, xlin, false);
  const auto y_eq = polytope::enumerate_facets(yrays, ylin, false);

  // Compute intersection rays
  auto inter = polytope::try_enumerate_vertices(x_eq.first / y_eq.first, x_eq.second / y_eq.second, false);

  // normalize
  normalize_rays(inter.first);

  return inter;
}

///////////////////////////////////////////////////////////////////////////////////////

//Documentation see header
fan_intersection_result
fan_intersection(const Matrix<Rational>& xrays, const Matrix<Rational>& xlin, const IncidenceMatrix<>& xcones,
                 const Matrix<Rational>& yrays, const Matrix<Rational>& ylin, const IncidenceMatrix<>& ycones)
{
  // Precompute h-representations of the x-cones and y-cones
  std::vector<polytope::convex_hull_result<Rational>> xequations;
  xequations.reserve(xcones.rows());
  for (auto xc = entire(rows(xcones)); !xc.at_end(); ++xc) {
    xequations.push_back(polytope::enumerate_facets(xrays.minor(*xc,All), xlin, false));
  }

  std::vector<polytope::convex_hull_result<Rational>> yequations;
  yequations.reserve(ycones.rows());
  for (auto yc = entire(rows(ycones)); !yc.at_end(); ++yc) {
    yequations.push_back(polytope::enumerate_facets(yrays.minor(*yc,All), ylin, false));
  }

  // Now compute intersections
  Matrix<Rational> interrays;
  Matrix<Rational> interlineality;
  bool lineality_computed = false;
  std::vector<Set<Int>> intercones;

  std::vector<Set<Int>> xcontainers;
  std::vector<Set<Int>> ycontainers;

  for (auto xc = entire<indexed>(xequations); !xc.at_end(); ++xc) {
    for (auto yc = entire<indexed>(yequations); !yc.at_end(); ++yc) {
      // Compute intersection
      auto inter = polytope::try_enumerate_vertices( xc->first / yc->first,
                                                     xc->second / yc->second,
                                                     false );
      if (!lineality_computed) {
        interlineality = inter.second.rows() > 0 ? inter.second : Matrix<Rational>();
        lineality_computed = true;
      }

      // The empty cone will not be included
      if (inter.first.rows() == 0) continue;

      // If cone contains no vertices (i.e. the intersection is actually empty),
      // we leave it out
      if (is_zero(inter.first.col(0))) continue;

      normalize_rays(inter.first);

      // Insert rays into ray list and create cone
      Set<Int> new_cone_set{ insert_rays(interrays, inter.first, true) };

      // Make sure we don't add a cone twice
      // Also: Remember intersections that are contained in this one or contain this one
      Set<Int> containedCones;
      Set<Int> containerCones;
      Int new_cone_index = -1;
      for (auto ic = entire<indexed>(intercones); !ic.at_end(); ++ic) {
        const Int cmp_set = incl(*ic, new_cone_set);
        if (cmp_set == 0)
          new_cone_index = ic.index();
        else if (cmp_set == -1)
          containedCones += ic.index();
        else if (cmp_set == 1)
          containerCones += ic.index();
      }
      if (new_cone_index == -1) {
        intercones.push_back(new_cone_set);
        new_cone_index = intercones.size()-1;
        xcontainers.push_back(Set<Int>());
        ycontainers.push_back(Set<Int>());
      }

      // First add all containers from the containing cones
      for (auto sup = entire(containerCones); !sup.at_end(); ++sup) {
        xcontainers[new_cone_index] += xcontainers[*sup];
        ycontainers[new_cone_index] += ycontainers[*sup];
      }
      // Add xc and yc as containers
      xcontainers[new_cone_index] += xc.index();
      ycontainers[new_cone_index] += yc.index();
      // Add all current containers to the contained cones
      for (auto sub = entire(containedCones); !sub.at_end(); ++sub) {
        xcontainers[*sub] += xcontainers[new_cone_index];
        ycontainers[*sub] += ycontainers[new_cone_index];
      }

    } //END iterate ycones
  } //END iterate xcones

  // Create result:
  fan_intersection_result f;
  f.rays = interrays;
  if (interlineality.rows() == 0) interlineality = Matrix<Rational>(0,interrays.cols());
  f.lineality_space = interlineality;
  f.cones = IncidenceMatrix<>(intercones);
  f.xcontainers = IncidenceMatrix<>(xcontainers);
  f.ycontainers = IncidenceMatrix<>(ycontainers);
  return f;
}

/*
 * @brief Computes the set-theoretic intersection of two Cycles and returns it as a
 * PolyhedralComplex
 * @param Cycle A
 * @param Cycle B
 * @return PolyhedralComplex in non-tropical-homogeneous coordinates
 */
BigObject set_theoretic_intersection(BigObject A, BigObject B)
{
  // Extract results
  const Matrix<Rational> &arays = A.give("VERTICES");
  const IncidenceMatrix<> &acones = A.give("MAXIMAL_POLYTOPES");
  const Matrix<Rational> &alineality = A.give("LINEALITY_SPACE");

  const Matrix<Rational> &brays = B.give("VERTICES");
  const IncidenceMatrix<> &bcones = B.give("MAXIMAL_POLYTOPES");
  const Matrix<Rational> &blineality = B.give("LINEALITY_SPACE");

  fan_intersection_result result = fan_intersection(arays,alineality,acones, brays,blineality,bcones);

  // Check for contained cones
  FacetList flist;
  for (auto c = entire(rows(result.cones)); !c.at_end(); ++c) flist.insertMax(*c);

  return BigObject("fan::PolyhedralComplex",
                   "VERTICES", tdehomog(result.rays),
                   "MAXIMAL_POLYTOPES", flist,
                   "LINEALITY_SPACE", tdehomog(result.lineality_space));
}


// ------------------------- PERL WRAPPERS ---------------------------------------------------

Function4perl(&cone_intersection, "cone_intersection(Matrix<Rational>,Matrix<Rational>,Matrix<Rational>,Matrix<Rational>,$)");

FunctionTemplate4perl("normalize_rays(Matrix<Rational>)");

UserFunction4perl("# @category Basic polyhedral operations"
                  "# Computes the set-theoretic intersection of two cycles and returns it as a polyhedral complex."
                  "# The cycles need not use the same tropical addition"
                  "# @param Cycle A"
                  "# @param Cycle B"
                  "# @return fan::PolyhedralComplex The set-theoretic intersection of the supports of A and B",
                  &set_theoretic_intersection, "set_theoretic_intersection(Cycle,Cycle)");
} }
