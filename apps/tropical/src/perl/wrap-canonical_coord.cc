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

#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace tropical { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( canonicalize_to_nonnegative_X2_f16, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturnVoid( canonicalize_to_nonnegative(arg0.get<T0>()) );
   };

   template <typename T0>
   FunctionInterface4perl( dehomogenize_trop_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( dehomogenize_trop(arg0.get<T0>()) );
   };

   template <typename T0>
   FunctionInterface4perl( canonicalize_to_leading_zero_X2_f16, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturnVoid( canonicalize_to_leading_zero(arg0.get<T0>()) );
   };

   FunctionInstance4perl(canonicalize_to_leading_zero_X2_f16, perl::Canned< Matrix< Rational > >);
   FunctionInstance4perl(dehomogenize_trop_X, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, void> >);
   FunctionInstance4perl(dehomogenize_trop_X, perl::Canned< const Matrix< double > >);
   FunctionInstance4perl(canonicalize_to_nonnegative_X2_f16, perl::Canned< Matrix< Rational > >);
   FunctionInstance4perl(canonicalize_to_leading_zero_X2_f16, perl::Canned< pm::MatrixMinor<pm::Matrix<pm::Rational>&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
