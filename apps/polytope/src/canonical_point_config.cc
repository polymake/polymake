/* Copyright (c) 1997-2021
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
#include "polymake/linalg.h"
#include "polymake/permutations.h"
#include "polymake/polytope/canonicalize.h"

namespace polymake { namespace polytope {

template <typename TVector>
typename std::enable_if<pm::check_container_feature<TVector, pm::sparse>::value, void>::type
canonicalize_point_configuration(GenericVector<TVector>& V)
{
   auto it=V.top().begin();
   if (!it.at_end()) {
      if (it.index()==0) {
         if (!is_one(*it)) {
            const auto first=*it;
            V.top() /= first;
         }
      } else {
         canonicalize_oriented(it);
      }
   }
}

template <typename TVector>
typename std::enable_if<!pm::check_container_feature<TVector, pm::sparse>::value, void>::type
canonicalize_point_configuration(GenericVector<TVector>& V)
{
   if (!V.top().empty() && !is_one(V.top().front())) {
      if (!is_zero(V.top().front())) {
         const auto first=V.top().front();
         V.top() /= first;
      } else {
         canonicalize_oriented(find_in_range_if(entire(V.top()), operations::non_zero()));
      }
   }
}

template <typename TMatrix> inline
void canonicalize_point_configuration(GenericMatrix<TMatrix>& M)
{
   Set<Int> neg;
   Int i = 0;
   for (auto r = entire(rows(M)); !r.at_end();  ++r, ++i) {
      if ( r->top()[0] < 0 )
         neg.push_back(i);
      else
         canonicalize_point_configuration(r->top());
   }
   M = M.minor(~neg,All);
}

FunctionTemplate4perl("canonicalize_point_configuration(Vector&)");
FunctionTemplate4perl("canonicalize_point_configuration(Matrix&)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
