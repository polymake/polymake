/* Copyright (c) 1997-2022
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/topaz/sum_triangulation_tools.h"
#include "polymake/PowerSet.h"
#include "polymake/hash_set"

namespace polymake { namespace topaz {

// this function takes a facet F and glues it around the boundary of
// the ball defined by the WEB. It creates a new simplicial complex
// which contains all the facets indicated by WEB.
void glue_facet(const Set<Int>& F_in,
                const Array<Int>& F_vertex_indices,
                const Array<Set<Int>>& facets,
                const Array<Int>& facets_vertex_indices,
                const Set<Int>& web,
                const Int shift,
                bool shift_facet,
                std::vector<Set<Int>>& result)
{
   // compute the boundary ridges
   hash_set<Set<Int>> boundary;

   for (const auto& sf : web) {
      for (auto rit = entire(all_subsets_less_1(facets[sf])); !rit.at_end(); ++rit) {
         if (boundary.exists(*rit)) {
            boundary -= *rit;
         } else {
            boundary += *rit;
         }
      }
   }

   // take care of index shifting for unused points
   Array<Int> vertex_indices = facets_vertex_indices;

   // take into account the vertex indices of F
   Set<Int> F(permuted_inv(F_in, F_vertex_indices));

   // shift the indices of F or boundary facet
   if (shift_facet) {
      // TODO: introduce Set::transpose instead of this madness
      Set<Int> F_shifted(entire(attach_operation(F, same_value(shift), operations::add())));
      F = F_shifted;
   } else {
      // we shift the vertex indices of the boundary facet via the
      // vertex permutation so that we don't have to do it later
      for (Int& v : vertex_indices) v+=shift;
   }

   // glue everything together
   for (const auto& bf : boundary) {
      result.push_back(F + permuted_inv(bf, vertex_indices));
   }
}


template <typename Scalar>
BigObject sum_triangulation(BigObject p_in,
                            BigObject q_in,
                            const IncidenceMatrix<> webOfStars_in,
                            OptionSet options)
{
   return sum_triangulation_impl<Scalar>(p_in, q_in, webOfStars_in, options);
}

UserFunctionTemplate4perl("# @category Producing a new simplicial complex from others\n"
                          "# Produce a specific sum-triangulation of two given triangulations.\n"
                          "# and a WebOfStars.  There are P-sum-triangulations and Q-sum-triangulations."
                          "# If the image of the star of the origin of P is empty then we have a"
                          "# Q-sum-triangulation; otherwise it is a P-sum-triangulation."
                          "# For details see Assarf, Joswig & Pfeifle:"
                          "# Webs of stars or how to triangulate sums of polytopes, to appear"
                          "# @param GeometricSimplicialComplex P first complex"
                          "# @param GeometricSimplicialComplex Q second complex"
                          "# @param IncidenceMatrix WebOfStars Every row corresponds to a full dimensional simplex in P and every column to a full dimensional simplex in Q."
                          "# @option Bool origin_first decides if the origin should be the first point in the resulting complex. Default=0"
                          "# @return GeometricSimplicialComplex",
                          "sum_triangulation<Scalar>(GeometricSimplicialComplex<type_upgrade<Scalar>> GeometricSimplicialComplex<type_upgrade<Scalar>>; IncidenceMatrix=new IncidenceMatrix() { origin_first => 0 })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
