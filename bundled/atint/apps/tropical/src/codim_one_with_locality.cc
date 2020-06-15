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

	Implements codim_one_with_locality.h
	*/

#include "polymake/tropical/codim_one_with_locality.h"
#include "polymake/tropical/separated_data.h"
#include "polymake/polytope/convex_hull.h"
#include "polymake/FacetList.h"

namespace polymake { namespace tropical {
		
CodimensionOneResult calculateCodimOneData(const Matrix<Rational>& rays, const IncidenceMatrix<>& maximalCones,
                                           const Matrix<Rational>& linspace, const IncidenceMatrix<>& local_restriction)
{
  // First we construct the set of all facets 
  // Array<IncidenceMatrix<> > maximal_cone_incidence = fan.give("MAXIMAL_CONES_INCIDENCES");
  // Compute the rays-in-facets for each cone directly
  std::vector<RestrictedIncidenceMatrix<>> maximal_cone_incidence(maximalCones.rows());
  for (Int mc = 0; mc < maximalCones.rows(); ++mc) {
    // Extract inequalities
    const Matrix<Rational> facets = polytope::enumerate_facets(rays.minor(maximalCones.row(mc), All),
                                                               linspace, false).first;
    // For each inequality, check which rays lie in it
    rows(maximal_cone_incidence[mc]).resize(facets.rows());
    auto rays_in_facet_it=rows(maximal_cone_incidence[mc]).begin();
    for (auto facet_it=entire(rows(facets)); !facet_it.at_end(); ++facet_it, ++rays_in_facet_it) {
      for (auto mc_it=entire(maximalCones.row(mc)); !mc_it.at_end(); ++mc_it) {
        if (is_zero((*facet_it) * rays.row(*mc_it))) {
          rays_in_facet_it->push_back(*mc_it);
        }
      }
    }
  }

  // This will contain the set of indices defining the codim one faces
  FacetList allFacets(rays.rows());

  // This will define the codim-1-maximal-cone incidence matrix
  RestrictedIncidenceMatrix<> fIncones(0);

  for (auto maxcone_it=entire<indexed>(maximal_cone_incidence); !maxcone_it.at_end();  ++maxcone_it) {
    for (auto facet=entire(rows(*maxcone_it));  !facet.at_end();  ++facet) {
      // If there is a local restriction, check if the facet is compatible
      if (local_restriction.rows() > 0) {
        if (!is_coneset_compatible(*facet, local_restriction)) continue;
      }
      // Check if this facet intersects x0 = 1, otherwise go to the next one 
      // More precisely: Check if at least one of its rays has x0-coord != 0
      if (is_zero(rays.col(0).slice(*facet))) {
        continue;
      }
      // Add the facet if necessary and add its maximal-cone indices
      auto existing=allFacets.find(*facet);
      if (existing == allFacets.end()) {
        allFacets.insert(*facet);
        fIncones /= scalar2set(maxcone_it.index());
      } else {
        fIncones(existing->get_id(), maxcone_it.index())=true;
      }
    }
  }

  return { IncidenceMatrix<>(allFacets), IncidenceMatrix<>(std::move(fIncones)) };
}

/*
 * @brief Computes [[CODIMENSION_ONE_POLYTOPES]], [[MAXIMAL_AT_CODIM_ONE]] 
 * and [[FACET_NORMALS_BY_PAIRS]] for cycles with [[LOCAL_RESTRICTION]]
 */
template <typename Addition>
void codim_one_with_locality(BigObject cycle)
{
  Matrix<Rational> rays = cycle.give("VERTICES");
  IncidenceMatrix<> cones = cycle.give("MAXIMAL_POLYTOPES");
  IncidenceMatrix<> local_restriction = cycle.give("LOCAL_RESTRICTION");
  Matrix<Rational> lineality = cycle.give("LINEALITY_SPACE");

  // Create a proxy object without the local restriction
  BigObject proxy("Cycle", mlist<Addition>(),
                  "PROJECTIVE_VERTICES", rays,
                  "MAXIMAL_POLYTOPES", cones,
                  "LINEALITY_SPACE", lineality);

  // Extract non-local data
  IncidenceMatrix<> codim_one = proxy.give("CODIMENSION_ONE_POLYTOPES");
  IncidenceMatrix<> maximal_at_codim = proxy.give("MAXIMAL_AT_CODIM_ONE");
  Map<std::pair<Int, Int>, Int> facets_by_pairs = proxy.give("FACET_NORMALS_BY_PAIRS");

  // Find non-local codim one cones
  Set<Int> nonlocal;
  Map<Int, Int> new_codim_index; //Maps old codim indices to new ones
  Int next_index = 0;
  for (Int cc = 0; cc < codim_one.rows(); ++cc) {
    if (!is_coneset_compatible(codim_one.row(cc),local_restriction)) {
      nonlocal += cc;
    } else {
      new_codim_index[cc] = next_index;
      ++next_index;
    }
  }

  // Clean map
  Map<std::pair<Int, Int>, Int> local_map;
  for (const auto& fct : facets_by_pairs) {
    if (!nonlocal.contains(fct.first.first))
      local_map[std::make_pair(new_codim_index[fct.first.first], fct.first.second)] = fct.second;
  }

  cycle.take("CODIMENSION_ONE_POLYTOPES") << codim_one.minor(~nonlocal,All);
  cycle.take("MAXIMAL_AT_CODIM_ONE") << maximal_at_codim.minor(~nonlocal,All);
  cycle.take("FACET_NORMALS_BY_PAIRS") << local_map;
}

FunctionTemplate4perl("codim_one_with_locality<Addition>(Cycle<Addition>)");

} }
