/* Copyright (c) 1997-2018
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

#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"

namespace polymake { namespace tropical { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( normalized_first_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (normalized_first(arg0.get<T0>())) );
   };

   template <typename T0>
   FunctionInterface4perl( tdehomog_vec_X_x_x, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (tdehomog_vec(arg0.get<T0>(), arg1, arg2)) );
   };

   template <typename T0>
   FunctionInterface4perl( thomog_X_x_x, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (thomog(arg0.get<T0>(), arg1, arg2)) );
   };

   template <typename T0>
   FunctionInterface4perl( tdehomog_X_x_x, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (tdehomog(arg0.get<T0>(), arg1, arg2)) );
   };

   FunctionInstance4perl(tdehomog_X_x_x, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(thomog_X_x_x, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(tdehomog_X_x_x, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational>&, pm::all_selector const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(tdehomog_X_x_x, perl::Canned< const pm::MatrixMinor<pm::MatrixMinor<pm::Matrix<pm::Rational>&, pm::all_selector const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&>&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(tdehomog_vec_X_x_x, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(tdehomog_vec_X_x_x, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Rational>, pm::Vector<pm::Rational> const&> >);
   FunctionInstance4perl(tdehomog_vec_X_x_x, perl::Canned< const pm::IndexedSlice<pm::VectorChain<pm::SingleElementVector<pm::Rational>, pm::Vector<pm::Rational> const&> const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&, mlist<> > >);
   FunctionInstance4perl(normalized_first_X, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(normalized_first_X, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::TropicalNumber<pm::Min, pm::Rational> >&, pm::Array<int> const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(normalized_first_X, perl::Canned< const Matrix< TropicalNumber< Max, Rational > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
