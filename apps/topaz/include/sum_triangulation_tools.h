/* Copyright (c) 1997-2020
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

#ifndef POLYMAKE_TOPAZ_SUM_TRIANGULATION_TOOLS_H
#define POLYMAKE_TOPAZ_SUM_TRIANGULATION_TOOLS_H

#include "polymake/Matrix.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/Map.h"
#include "polymake/linalg.h"
#include "polymake/PowerSet.h"
#include "polymake/hash_set"

namespace polymake { namespace topaz {

// is 0 a vertex of the configuration? Return its index if found, else -1
// the return value is shifted if necessary
template <typename Scalar>
Int index_of_zero(const Matrix<Scalar>& vertices,
                  bool homogeneous = true,
                  Int index_shift = 0)
{
   SparseVector<Scalar> the_zero(vertices.cols());
   if (homogeneous)
      the_zero[0] = one_value<Scalar>();

   for (Int zero_index = 0; zero_index < vertices.rows(); ++zero_index) {
      if (vertices.row(zero_index) == the_zero)
         return zero_index + index_shift;
   }
   return -1;
}

template <typename Scalar>
Set<Set<Int>> star_of_zero_impl(const Matrix<Scalar>& vertices,
                                const Array<Set<Int>>& facets,
                                bool homogeneous = true)
{
   const Int ioz = index_of_zero(vertices, homogeneous);
   Set<Set<Int>> star_of_zero;
   for (const auto& f : facets)
      if (f.contains(ioz))
         star_of_zero += f;

   if (star_of_zero.empty()) { // find a simplex containing 0
      for (const auto& f : facets) {
         // solve T(V) lambda = 0
         const Vector<Scalar> coeffs = homogeneous
            ? lin_solve(T(vertices.minor(f, All)), unit_vector<Scalar>(vertices.cols(), 0))
            : lin_solve(ones_vector<Scalar>() / T(vertices.minor(f, All)), unit_vector<Scalar>(vertices.cols()+1, 0));
         if (accumulate(coeffs, operations::min()) >= 0) { // the origin is a nonnegative linear combination of the vertices
            star_of_zero += f;
         }
      }
   }
   return star_of_zero;
}

template <typename HDType>
Map<Set<Int>, std::vector<Int>> links_of_ridges(const HDType& HD)
{
   Map<Set<Int>, std::vector<Int>> link_of;
   for (const auto r : HD.nodes_of_rank(HD.rank()-2)) {
      for (const auto f : HD.out_adjacent_nodes(r)) {
         link_of[HD.face(r)].push_back((HD.face(f)-HD.face(r)).front());
      }
   }
   return link_of;
}

// Quickly calculate the boundary of a simplicial ball
template <typename Container>
Set<Set<Int>> boundary_of(const Container& ball)
{
   Set<Set<Int>> boundary;
   for (const auto& b : ball) {
      for (auto rit = entire(all_subsets_less_1(b)); !rit.at_end(); ++rit) {
         if (boundary.contains(*rit)) // if we see a ridge for the second time, it's not in the boundary
            boundary -= *rit;
         else
            boundary += *rit;
      }
   }
   return boundary;
}

// this function takes a facet F and glues it around the boundary of
// the ball defined by the WEB. It creates a new simplicial complex
// which contains all the facets indicated by WEB.
// implemented in sum_triangulation.cc
void glue_facet(const Set<Int>& _F,
                const Array<Int>& F_vertex_indices,
                const Array<Set<Int>>& facets,
                const Array<Int>& facets_vertex_indices,
                const Set<Int>& web,
                Int shift,
                bool shift_facet,
                std::vector<Set<Int>>& result);


template<typename Scalar>
BigObject sum_triangulation_impl(BigObject p_in,
                                    BigObject q_in,
                                    const IncidenceMatrix<> webOfStars_in,
                                    OptionSet options)
{
   const Matrix<Scalar>
      pVert = p_in.give("COORDINATES"),
      qVert = q_in.give("COORDINATES");

   const Array<Set<Int>>
      facetsP = p_in.give("FACETS"),
      facetsQ = q_in.give("FACETS");

   Map<Set<Int>, Int> index_of;
   Int facet_index(0);
   for (const auto& r : facetsQ)
      index_of[r] = facet_index++;

   const bool origin_first = options["origin_first"];

   Array<Int> pVertexIndices, qVertexIndices;
   if (!(p_in.lookup("VERTEX_INDICES") >> pVertexIndices)) {
      pVertexIndices = sequence(0, pVert.rows());
   }
   if (!(q_in.lookup("VERTEX_INDICES") >> qVertexIndices)) {
      qVertexIndices = sequence(0, qVert.rows());
   }


   const Set<Set<Int>> star_Q_0 = star_of_zero_impl(qVert, facetsQ, false);
   Set<Int> indices_of_star_Q_0;
   for (const auto& s : star_Q_0)
      indices_of_star_Q_0 += index_of[s];

   // make sure webOfStars has the right dimensions (fill with 0s if needed)
   IncidenceMatrix<> webOfStars;
   if (webOfStars_in.rows()) {
      webOfStars = webOfStars_in;
      webOfStars.resize(facetsP.size(), facetsQ.size());
   } else {
      webOfStars = IncidenceMatrix<>(facetsP.size(), facetsQ.size());
      for (auto rit = entire(rows(webOfStars)); !rit.at_end(); ++rit)
         *rit = indices_of_star_Q_0;
   }

   std::vector<Set<Int>> output_list;

   bool is_P_sum(true);

   // build simplices from p_in to q_in
   // according to WEB
   for (Int i = 0; i < facetsP.size(); ++i) {
      const Set<Int>&
         F(facetsP[i]),
         web(webOfStars.row(i));
      if (web.empty()) {
         is_P_sum = false; // if it were a P-sum-triangulation, web wouldn't be empty
         continue;
      }
      glue_facet(F, pVertexIndices, facetsQ, qVertexIndices, web, pVert.rows(), false, output_list);
   }

   // build simplices from q_in to p_in
   // deduce the web in the other direction by using WEB
   // compatibility tells us that the inverse web function is
   // invert(transpose(web))

   const IncidenceMatrix<> negatedWebOfStars(~webOfStars);

   for (Int i = 0; i < facetsQ.size(); ++i) {
      const Set<Int>& F = facetsQ[i];
      const Set<Int>& web = negatedWebOfStars.col(i);
      if (web.empty()) {
         // if it is a Q-sum-triangulation this wouldn't happen
         if (!is_P_sum) throw std::runtime_error("sum_triangulation: web of stars do not belong to a compatible pair");
         continue;
      }

      glue_facet(F, qVertexIndices, facetsP, pVertexIndices, web, pVert.rows(), true, output_list);
   }


   // OUTPUT
   BigObject pSumTri("topaz::GeometricSimplicialComplex", mlist<Scalar>());
   pSumTri.set_description() << "a P sum triangulation of " << p_in.name() << " and " << q_in.name() << "." << endl;

   Matrix<Scalar> sumVert = (pVert | zero_matrix<Scalar>(pVert.rows(), qVert.cols())) /
                            (zero_matrix<Scalar>(qVert.rows(), pVert.cols()) | qVert);
   IncidenceMatrix<> output_facets(output_list);
   output_facets.resize(output_facets.rows(), pVert.rows()+qVert.rows());

   const Int
      p_zero_index(index_of_zero(pVert, false)),
      q_zero_index(index_of_zero(qVert, false, pVert.rows()));

   // Take care of the origin
   if (!origin_first) {
      Int zero_index = is_P_sum ? q_zero_index : p_zero_index;

      if (zero_index != -1) {
         sumVert = sumVert.minor(~scalar2set(zero_index), All);
         output_facets = output_facets.minor(All, ~scalar2set(zero_index));
      }
   } else {
      Set<Int> ball_of_zero;
      if (q_zero_index != -1) {
         sumVert = sumVert.minor(~scalar2set(q_zero_index), All);
         if (!is_P_sum) ball_of_zero = Set<Int>(output_facets.col(q_zero_index));
         output_facets = output_facets.minor(All, ~scalar2set(q_zero_index));
      }
      if (p_zero_index != -1) {
         sumVert = sumVert.minor(~scalar2set(p_zero_index), All);
         if (is_P_sum) ball_of_zero = Set<Int>(output_facets.col(p_zero_index));
         output_facets = output_facets.minor(All, ~scalar2set(p_zero_index));
      }

      sumVert = zero_vector<Scalar>() / sumVert;
      output_facets = ball_of_zero | output_facets;

   }

   pSumTri.take("COORDINATES") << sumVert;
   pSumTri.take("INPUT_FACES") << rows(output_facets);

   return pSumTri;
}


// implemented in web_of_stars.cc
IncidenceMatrix<> web_of_stars(const Array<Int>& poset_hom,
                               const Array<Set<Set<Int>>>& star_shaped_balls,
                               const Array<Set<Int>>& simplices);

} }

#endif // POLYMAKE_TOPAZ_SUM_TRIANGULATION_TOOLS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
