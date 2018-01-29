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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/Polynomial.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_C, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1, T0>()) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   template <typename T0>
   FunctionInterface4perl( new_int, T0 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<int>()) );
   };

   template <typename T0, typename T1, typename T2, typename T3>
   FunctionInterface4perl( new_X_X_X, T0,T1,T2,T3 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]), arg2(stack[3]);
      WrapperReturnNew(T0, (arg0.get<T1>(), arg1.get<T2>(), arg2.get<T3>()) );
   };

   OperatorInstance4perl(Unary_not, perl::Canned< const QuadraticExtension< Rational > >);
   FunctionInstance4perl(new_X_X_X, QuadraticExtension< Rational >, perl::Canned< const Rational >, perl::Canned< const Rational >, int);
   FunctionInstance4perl(new_X_X_X, QuadraticExtension< Rational >, int, perl::Canned< const Rational >, int);
   FunctionInstance4perl(new_int, QuadraticExtension< Rational >);
   FunctionInstance4perl(new_X, QuadraticExtension< Rational >, perl::Canned< const Rational >);
   FunctionInstance4perl(new_X_X_X, QuadraticExtension< Rational >, int, int, int);
   FunctionInstance4perl(new_X_X_X, QuadraticExtension< Rational >, perl::Canned< const Rational >, perl::Canned< const Rational >, perl::Canned< const Rational >);
   FunctionInstance4perl(new_C, QuadraticExtension< Rational >, int);
   FunctionInstance4perl(new_C, QuadraticExtension< Rational >, perl::Canned< const Rational >);
   OperatorInstance4perl(assign, QuadraticExtension< Rational >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Polynomial< QuadraticExtension< Rational >, int > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const QuadraticExtension< Rational > >, perl::Canned< const Polynomial< QuadraticExtension< Rational >, int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
