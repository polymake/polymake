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
*/

#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"

namespace polymake { namespace tropical { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( tdiam_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (tdiam(arg0.get<T0>())) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( tdist_X_X, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (tdist(arg0.get<T0>(), arg1.get<T1>())) );
   };

   template <typename T0>
   FunctionInterface4perl( second_tdet_and_perm_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (second_tdet_and_perm(arg0.get<T0>())) );
   };

   template <typename T0>
   FunctionInterface4perl( tdet_and_perm_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (tdet_and_perm(arg0.get<T0>())) );
   };

   template <typename T0>
   FunctionInterface4perl( cramer_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (cramer(arg0.get<T0>())) );
   };

   template <typename T0>
   FunctionInterface4perl( tdet_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (tdet(arg0.get<T0>())) );
   };

   FunctionInstance4perl(tdet_X, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(tdet_X, perl::Canned< const Matrix< TropicalNumber< Max, Rational > > >);
   FunctionInstance4perl(tdet_X, perl::Canned< const SparseMatrix< TropicalNumber< Max, Rational >, Symmetric > >);
   FunctionInstance4perl(cramer_X, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(cramer_X, perl::Canned< const Matrix< TropicalNumber< Max, Rational > > >);
   FunctionInstance4perl(tdet_and_perm_X, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(second_tdet_and_perm_X, perl::Canned< const Matrix< TropicalNumber< Max, Rational > > >);
   FunctionInstance4perl(second_tdet_and_perm_X, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(tdist_X_X, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::TropicalNumber<pm::Min, pm::Rational> >&>, pm::Series<int, false>, mlist<> > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::TropicalNumber<pm::Min, pm::Rational> >&>, pm::Series<int, false>, mlist<> > >);
   FunctionInstance4perl(tdiam_X, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(tdiam_X, perl::Canned< const Matrix< TropicalNumber< Max, Rational > > >);
   FunctionInstance4perl(tdist_X_X, perl::Canned< const Vector< TropicalNumber< Min, Rational > > >, perl::Canned< const Vector< TropicalNumber< Min, Rational > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
