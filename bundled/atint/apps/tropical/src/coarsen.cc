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

	This file contains a function to compute the coarsest polyhedral structure of a
	tropical variety
	*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/tropical/thomog.h"
#include "polymake/polytope/convex_hull.h"
#include "polymake/vector"

namespace polymake { namespace tropical {

/**
 * @brief Takes a polyhedron in its V-description and tests whether it fulfills a given inequality
 * @param Matrix<Rational> rays The rays/vertices of the polyhedron
 * @param Matrix<Rational> linspace The lineality space generators of the polyhedron
 * @param Vector<Rational> facet The inequality, in standard polymake format. Note that all three should be given in the same format, i.e. in homog. or non-homog. coordinates.
 * @return True, if the polyhedron fulfills the inequality; false otherwise.
 */
bool coneInHalfspace(const Matrix<Rational>& rays, const Matrix<Rational>& linspace,
                     const Vector<Rational>& facet)
{
  Matrix<Rational> allgenerators = rays / linspace;
  Vector<Rational> product = allgenerators * facet;
  for (Int i = 0; i < product.dim(); ++i) {
    if (product[i] < 0) return false;
  }
  return true;
}

// Documentation see perl wrapper
template <typename Addition>
BigObject coarsen(BigObject complex, bool testFan = false)
{
  // Extract values
  Matrix<Rational> rays = complex.give("VERTICES");
  rays = tdehomog(rays);
  Matrix<Rational> linspace = complex.give("LINEALITY_SPACE");
  linspace = tdehomog(linspace);
  Int cmplx_dim = complex.give("PROJECTIVE_DIM");
  Int cmplx_ambient_dim = complex.give("PROJECTIVE_AMBIENT_DIM");
  bool weights_exist = false;
  Vector<Integer> weights;
  if (complex.lookup("WEIGHTS") >> weights) {
    weights_exist = true;
  }
  IncidenceMatrix<> maximalCones = complex.give("MAXIMAL_POLYTOPES");
  IncidenceMatrix<> codimOneCones = complex.give("CODIMENSION_ONE_POLYTOPES");
  IncidenceMatrix<> codimInMaximal = complex.give("MAXIMAL_AT_CODIM_ONE");
  IncidenceMatrix<> maximalOverCodim = T(codimInMaximal);
  Int noOfCones = maximalCones.rows();

  // For fan testing, we need the equations of all non-twovalent codimension one faces
  Vector<Matrix<Rational>> codimOneEquations(codimOneCones.rows());
  Set<Int> highervalentCodim;
  for (Int cd = 0; cd < codimOneCones.rows(); ++cd) {
    if (codimInMaximal.row(cd).size() > 2) {
      codimOneEquations[cd] = null_space( rays.minor(codimOneCones.row(cd),All) / linspace);
      highervalentCodim += cd;
    } //END only for >2-valent
  } //END compute codim one cone equations.

  // If the fan has no rays, it has only lineality space and we return the complex itself
  if (rays.rows() == 0)  return complex;

  // If the fan has no codim 1 faces, it must be 0- dimensional or a lineality space and we return
  // the complex itself
  if (codimOneCones.rows() == 0) return complex;

  // Compute equivalence classes of maximal cones
  std::vector<Set<Int>> equivalenceClasses;
  std::vector<bool> hasBeenAdded(noOfCones); //contains whether a cone has been added to an equivalence class

  for (Int mc = 0; mc < noOfCones; ++mc) {
    if (!hasBeenAdded[mc]) {
      equivalenceClasses.push_back(Set<Int>{ mc });
      // Do a breadth-first search for all other cones in the component
      std::list<Int> queue;
      queue.push_back(mc);
      // Semantics: Elements in that queue have been added but their neighbours might not
      while (!queue.empty()) {
        // Take the first element and find its neighbours
        Int node = queue.front();
        queue.pop_front();
        Set<Int> cdset = maximalOverCodim.row(node);
        for (auto cd = entire(cdset); !cd.at_end(); ++cd) {
          Set<Int> otherMaximals = codimInMaximal.row(*cd) - node;
          // We are only interested in codim-one-faces that have exactly two adjacent maximal cones
          if (otherMaximals.size() == 1) {
            Int othermc = *(otherMaximals.begin());
            if (!hasBeenAdded[othermc]) {
              // Now add the cone to the equivalence class of mc
              equivalenceClasses.back() += othermc;
              queue.push_back(othermc);
              hasBeenAdded[othermc] = true;
            }
          }
        }
      } //End iterate queue
    }
  } //END iterate maximal cones

  /* Test equivalence classes for convexit:
     An equivalence class S = (s1,...,sr) is convex, iff it fulfills the following criterion: Let T be the set of all outer (i.e. not two-valent) codim one facets and write f_t >= a_t for the corresponding facet inequality. Then for each pair t,t' in T the polyhedron t must fulfill the inequality f_t' >= a_t'.
  */
  if (testFan) {
    // Check each equivalence class
    for (const auto& conesInClass : equivalenceClasses) {
      // First, we compute the equation of the subdivision class
      Matrix<Rational> classEquation =
        null_space(rays.minor(maximalCones.row(*(conesInClass.begin())), All) / linspace);
      // Compute all outer codim one faces and their facet inequality;
      Set<Int> outerFacets;
      Map<Int, Vector<Rational>> facetInequality;
      for (auto mc = entire(conesInClass); !mc.at_end(); ++mc) {
        Set<Int> outerCodimInMc = maximalOverCodim.row(*mc) * highervalentCodim;
        outerFacets += outerCodimInMc;
        for (auto fct = entire(outerCodimInMc); !fct.at_end(); ++fct) {
          // Find the one equation of fct that is not an equation of the class
          for (Int r = 0; r < codimOneEquations[*fct].rows(); ++r) {
            if (rank(classEquation / codimOneEquations[*fct].row(r)) > cmplx_ambient_dim - cmplx_dim) {
              // Find the right sign and save the inequality
              Int additionalRay = *( (maximalCones.row(*mc) - codimOneCones.row(*fct)).begin());
              facetInequality[*fct] = codimOneEquations[*fct].row(r);
              if (facetInequality[*fct] * rays.row(additionalRay) < 0) {
                facetInequality[*fct] = - facetInequality[*fct];
              }
              break;
            }
          } //END iterate equations of fct
        } //END iterate all outer facets of the cone
      } //END iterate all maximal cones in the class

      // Now go through all pairs of outer facets
      for (const Int out1 : outerFacets) {
        for (const Int out2 : outerFacets - out1) {
          // Check if out2 fulfills the facet inequality of out1
          if (!coneInHalfspace(rays.minor(codimOneCones.row(out2),All), linspace, facetInequality[out1])) {
            throw std::runtime_error("The equivalence classes are not convex. There is no coarsest structure.");
          }
        } //END iterate second facet.
      } //END iterate first facet
    } //END iterate classes
  } //END test convexity of equivalence classes

  // Now compute the new cones as unions of the cones in each equivalence class
  Matrix<Rational> newlin;
  Int newlindim = 0;
  bool newlin_computed = false;
  Vector<Set<Int>> newcones;
  Vector<Integer> newweights;
  Matrix<Rational> complete_matrix = rays / linspace;
  Set<Int> used_rays;

  for (const auto& conesInClass : equivalenceClasses) {
    Matrix<Rational> union_rays(0, rays.cols());
    Vector<Int> union_ray_list;
    for (auto mc = entire(conesInClass); !mc.at_end(); ++mc) {
      union_rays /= rays.minor(maximalCones.row(*mc),All);
      union_ray_list |= Vector<Int>(maximalCones.row(*mc));
    }

    const auto union_cone = polytope::get_non_redundant_points(union_rays, linspace, true);

    // If we need to test fan-ness, we need to check if different classes have different lin. spaces
    if (testFan && newlin_computed) {
      Matrix<Rational> otherlin = (union_rays / linspace).minor(union_cone.second, All);
      if (newlindim != rank(otherlin)) {
        throw std::runtime_error("Different classes have different lineality spaces. There is no coarsest structure.");
      } else if (rank(newlin / otherlin) > rank(newlin)) {
        throw std::runtime_error("Different classes have different lineality spaces. There is no coarsest structure.");
      }
    } //END check equality of lineality spaces.

    if (!newlin_computed) {
      newlin = (union_rays / linspace).minor(union_cone.second, All);
      if (testFan) newlindim = rank(newlin);
      newlin_computed = true;
    }

    // Convert indices of rays in union_rays to indices in rays
    Set<Int> ray_set(union_ray_list.slice(union_cone.first));

    newcones |= ray_set;
    used_rays += ray_set;
    if (weights_exist) newweights |= weights[*(conesInClass.begin())];
  }

  // Some rays might become equal (modulo lineality space) when coarsening, so we have to clean up
  Map<Int, Int> ray_index_conversion;
  Int next_index = 0;
  Matrix<Rational> final_rays(0, complete_matrix.cols());
  Int linrank = rank(newlin);

  for (const Int r1 : used_rays) {
    if (!keys(ray_index_conversion).contains(r1)) {
      ray_index_conversion[r1] = next_index;
      final_rays /= complete_matrix.row(r1);

      for (const Int r2 : used_rays) {
        if (!keys(ray_index_conversion).contains(r2)) {
          // Check if both rays are equal mod lineality space
          Vector<Rational> diff_vector = complete_matrix.row(r1) - complete_matrix.row(r2);
          if (rank(newlin / diff_vector) == linrank) {
            ray_index_conversion[r2] = next_index;
          }
        } //END if second not mapped yet
      } //END iterate second ray index
      ++next_index;
    } //END if first not mapped yet
  } //END iterate first ray index

  // Now convert the cones
  for (Int nc = 0; nc < newcones.dim(); ++nc) {
    newcones[nc] = Set<Int>{ ray_index_conversion.map(newcones[nc]) };
  }

  // Produce final result
  BigObject result("Cycle", mlist<Addition>());
  result.take("VERTICES") << thomog(final_rays);
  result.take("MAXIMAL_POLYTOPES") << newcones;
  result.take("LINEALITY_SPACE") << thomog(newlin);
  if (weights_exist) {
    result.take("WEIGHTS") << newweights;
  }

  return result;
}

UserFunctionTemplate4perl("# @category Basic polyhedral operations"
                          "# Takes a tropical variety on which a coarsest polyhedral structure exists"
                          "# and computes this structure."
                          "# @param Cycle<Addition> complex A tropical variety which has a unique "
                          "# coarsest polyhedral structre "
                          "# @param Bool testFan (Optional, FALSE by default). Whether the algorithm should perform some consistency "
                          "# checks on the result. If true, it will check the following: "
                          "# - That equivalence classes of cones have convex support"
                          "# - That all equivalence classes have the same lineality space"
                          "# If any condition is violated, the algorithm throws an exception"
                          "# Note that it does not check whether equivalence classes form a fan"
                          "# This can be done via [[fan::check_fan]] afterwards, but it is potentially slow."
                          "# @return Cycle<Addition> The corresponding coarse complex. Throws an "
                          "# exception if testFan = True and consistency checks fail.",
                          "coarsen<Addition>(Cycle<Addition>; $=0)");
} }
