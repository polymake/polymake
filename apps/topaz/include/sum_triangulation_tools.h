/* Copyright (c) 1997-2015
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

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

#ifndef __POLYMAKE_TOPAZ_SUM_TRIANGULATION_TOOLS_H__
#define __POLYMAKE_TOPAZ_SUM_TRIANGULATION_TOOLS_H__

#include "polymake/Matrix.h"
#include "polymake/graph/HasseDiagram.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/Map.h"
#include "polymake/linalg.h"

namespace polymake { namespace topaz {

// is 0 a vertex of the configuration? Return its index if found, else -1
// the return value is shifted if necessary
template<typename Scalar>
int index_of_zero(const Matrix<Scalar>& vertices,
                  bool homogeneous = true,
                  int index_shift = 0)
{
   bool found_zero = false;
   int zero_index(0);
   SparseVector<Scalar> the_zero;
   if (homogeneous)
      the_zero = unit_vector<Scalar>(vertices.cols(), 0);
   else 
      the_zero = zero_vector<Scalar>(vertices.cols());

   for(; zero_index < vertices.rows(); ++zero_index) {
      if (vertices.row(zero_index) == the_zero) {
         found_zero = true;
         break;
      }
   }
   return found_zero ? zero_index + index_shift : -1;
}

template<typename Scalar>
Set<Set<int> > star_of_zero_impl(const Matrix<Scalar>& vertices,
                                 const Array<Set<int> >& facets,
                                 bool homogeneous = true)
{
   const int ioz = index_of_zero(vertices, homogeneous);
   Set<Set<int> > star_of_zero;
   for (Entire<Array<Set<int> > >::const_iterator fit = entire(facets); !fit.at_end(); ++fit)
      if ((*fit).contains(ioz))
         star_of_zero += *fit;

   if (!star_of_zero.size()) { // find a simplex containing 0
      for (Entire<Array<Set<int> > >::const_iterator fit = entire(facets); !fit.at_end(); ++fit) {
         // solve T(V) lambda = 0
         const Vector<Scalar> coeffs = homogeneous
            ? lin_solve(T(vertices.minor(*fit, All)), unit_vector<Scalar>(vertices.cols(), 0))
            : lin_solve(ones_vector<Scalar>() / T(vertices.minor(*fit, All)), unit_vector<Scalar>(vertices.cols() + 1, 0));
         if (accumulate(coeffs, operations::min()) >= 0) { // the origin is a nonnegative linear combination of the vertices
            star_of_zero += *fit;
         }
      }
   }
   return star_of_zero;
}

template <typename HDType>
Map<Set<int>, std::vector<int> > links_of_ridges(const HDType& HD)
{
   Map<Set<int>, std::vector<int> > link_of;
   for (Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator r = entire(HD.node_range_of_dim(-2)); !r.at_end(); ++r) {
      for (graph::HasseDiagram::graph_type::out_adjacent_node_list::const_iterator f = HD.out_adjacent_nodes(*r).begin(); !f.at_end(); ++f) {
         link_of[HD.face(*r)].push_back((HD.face(*f)-HD.face(*r)).front());
      }
   }
   return link_of;
}

// Quickly calculate the boundary of a simplicial ball
template <typename Container>
Set<Set<int> > boundary_of(const Container& ball)
{
   Set<Set<int> > boundary;
   for (typename Entire<Container>::const_iterator bit = entire(ball); !bit.at_end(); ++bit) {
      for (Entire<Subsets_less_1<const Set<int>&> >::const_iterator rit = entire(all_subsets_less_1(*bit)); !rit.at_end(); ++rit) {
         if (boundary.contains(*rit)) // if we see a ridge for the second time, it's not in the boundary
            boundary -= *rit;
         else
            boundary += *rit;
      }
   }
   return boundary;
}

#endif // __POLYMAKE_TOPAZ_SUM_TRIANGULATION_TOOLS_H__


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
