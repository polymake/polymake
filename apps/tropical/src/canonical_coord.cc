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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace tropical {
namespace {

template <typename Iterator> inline
bool leading_non_zero(const Iterator& it, pm::False) { return !is_zero(*it); }

template <typename Iterator> inline
bool leading_non_zero(const Iterator& it, pm::True) { return !it.at_end() && it.index()==0; }
}

template <typename Vector>
void canonicalize_to_leading_zero(GenericVector<Vector>& V)
{
   typename Vector::iterator it=V.top().begin();
   if (leading_non_zero(it, bool2type<pm::check_container_feature<Vector,pm::sparse>::value>())) {
      const typename Vector::element_type first=*it;
      V-=same_element_vector(first, V.dim());
   }
}

template <typename Matrix>
void canonicalize_to_leading_zero(GenericMatrix<Matrix>& M)
{
   if (!M.rows())
      throw std::runtime_error("point matrix may not be empty");
   for (typename Entire< Rows<Matrix> >::iterator r=entire(rows(M)); !r.at_end();  ++r)
      canonicalize_to_leading_zero(r->top());
}

template <typename Vector>
void canonicalize_to_nonnegative(GenericVector<Vector>& V)
{
   const typename Vector::element_type x_min=accumulate(V.top(), operations::min());
   if (pm::check_container_feature<Vector,pm::sparse>::value
       ? x_min<0 || V.top().size()==V.dim()
       : !is_zero(x_min))
      V-=same_element_vector(x_min,V.dim());
}

template <typename Matrix>
void canonicalize_to_nonnegative(GenericMatrix<Matrix>& M)
{
   if (!M.rows())
      throw std::runtime_error("point matrix may not be empty");
   for (typename Entire< Rows<Matrix> >::iterator r=entire(rows(M)); !r.at_end();  ++r)
      canonicalize_to_nonnegative(r->top());
}

FunctionTemplate4perl("canonicalize_to_leading_zero(Vector&) : void");
FunctionTemplate4perl("canonicalize_to_leading_zero(Matrix&) : void");
FunctionTemplate4perl("canonicalize_to_nonnegative(Vector&) : void");
FunctionTemplate4perl("canonicalize_to_nonnegative(Matrix&) : void");

FunctionTemplate4perl("dehomogenize_trop(Vector)");
FunctionTemplate4perl("dehomogenize_trop(Matrix)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
