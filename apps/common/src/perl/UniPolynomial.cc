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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/client.h"
#include "polymake/Polynomial.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Ring.h"
#include "polymake/RationalFunction.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   template <typename T0, typename T1, typename T2, typename T3>
   FunctionInterface4perl( new_X_X_X, T0,T1,T2,T3 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]), arg2(stack[3]);
      WrapperReturnNew(T0, (arg0.get<T1>(), arg1.get<T2>(), arg2.get<T3>()) );
   };

   ClassTemplate4perl("Polymake::common::UniPolynomial");
   Class4perl("Polymake::common::UniPolynomial_A_Rational_I_Rational_Z", UniPolynomial< Rational, Rational >);
   FunctionInstance4perl(new_X_X_X, UniPolynomial< Rational, Rational >, perl::Canned< const Array< Rational > >, perl::Canned< const Array< Rational > >, perl::Canned< const Ring< Rational, Rational > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const UniPolynomial< Rational, Rational > >, perl::Canned< const UniPolynomial< Rational, Rational > >);
   FunctionInstance4perl(new, UniPolynomial< Rational, int >);
   Class4perl("Polymake::common::UniPolynomial_A_Rational_I_Int_Z", UniPolynomial< Rational, int >);
   OperatorInstance4perl(Binary_add, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const UniMonomial< Rational, int > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const UniTerm< Rational, int > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const UniPolynomial< Rational, int > >, int);
   OperatorInstance4perl(Binary_add, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const UniTerm< Rational, int > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const UniPolynomial< Rational, int > >, int);
   OperatorInstance4perl(Binary_mul, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const UniPolynomial< Rational, int > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const UniPolynomial< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const UniPolynomial< Rational, int > >, int);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const UniPolynomial< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary_div, perl::Canned< const UniPolynomial< Rational, int > >, int);
   OperatorInstance4perl(Binary_div, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const UniPolynomial< Rational, int > >);
   OperatorInstance4perl(Binary_add, int, perl::Canned< const UniPolynomial< Rational, int > >);
   OperatorInstance4perl(Binary_sub, int, perl::Canned< const UniPolynomial< Rational, int > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const UniMonomial< Rational, int > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const UniPolynomial< Rational, int > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const UniPolynomial< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const UniMonomial< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const UniTerm< Rational, int > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const UniMonomial< Rational, int > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const UniTerm< Rational, int > >);
   OperatorInstance4perl(Binary_div, int, perl::Canned< const UniPolynomial< Rational, int > >);
   FunctionInstance4perl(new_X_X_X, UniPolynomial< Rational, int >, perl::Canned< const Array< Rational > >, perl::Canned< const Array< int > >, perl::Canned< const Ring< Rational, int > >);
   FunctionInstance4perl(new_X, UniPolynomial< Rational, int >, perl::Canned< const UniMonomial< Rational, int > >);
   FunctionInstance4perl(new_X, UniPolynomial< Rational, int >, int);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
