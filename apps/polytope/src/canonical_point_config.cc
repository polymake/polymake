/* Copyright (c) 1997-2014
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

#include "polymake/client.h"
#include "polymake/linalg.h"
#include "polymake/permutations.h"
#include "polymake/polytope/canonicalize.h"

namespace polymake { namespace polytope {

template <typename Vector>
typename pm::enable_if<void, pm::check_container_feature<Vector, pm::sparse>::value>::type
canonicalize_point_configuration(GenericVector<Vector>& V)
{
   typename Vector::iterator it=V.top().begin();
   if (!it.at_end()) {
      if (it.index()==0) {
         if (!is_one(*it)) {
            const typename Vector::element_type first=*it;
            V.top() /= first;
         }
      } else {
         canonicalize_oriented(it);
      }
   }
}

template <typename Vector>
typename pm::disable_if<void, pm::check_container_feature<Vector,pm::sparse>::value>::type
canonicalize_point_configuration(GenericVector<Vector>& V)
{
   if (!V.top().empty() && !is_one(V.top().front())) {
      if (!is_zero(V.top().front())) {
         const typename Vector::element_type first=V.top().front();
         V.top() /= first;
      } else {
         canonicalize_oriented(find_if(entire(V.top()), operations::non_zero()));
      }
   }
}

template <typename Matrix> inline
void canonicalize_point_configuration(GenericMatrix<Matrix>& M)
{
   Set<int> neg;
   int i = 0;
   for (typename Entire< Rows<Matrix> >::iterator r=entire(rows(M)); !r.at_end();  ++r, ++i) {
      if ( r->top()[0] < 0 )
         neg.push_back(i);
      else
      canonicalize_point_configuration(r->top());
   }
   M = M.minor(~neg,All);
}

FunctionTemplate4perl("canonicalize_point_configuration(Vector&) : void");
FunctionTemplate4perl("canonicalize_point_configuration(Matrix&) : void");



} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
