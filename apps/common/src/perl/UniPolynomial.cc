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

#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/RationalFunction.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/linalg.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( new_X_X, T0,T1,T2 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]);
      WrapperReturnNew(T0, (arg0.get<T1>(), arg1.get<T2>()) );
   };

   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   OperatorInstance4perl(Binary_div, int, perl::Canned< const UniPolynomial< Rational, int > >);
   FunctionInstance4perl(new, UniPolynomial< Rational, int >);
   FunctionInstance4perl(new_X_X, UniPolynomial< Rational, int >, perl::Canned< const Vector< int > >, perl::Canned< const Array< int > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< UniPolynomial< Rational, int > >, perl::Canned< const UniPolynomial< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const UniPolynomial< UniPolynomial< Rational, int >, Rational > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const UniPolynomial< UniPolynomial< Rational, int >, Rational > >, int);
   OperatorInstance4perl(Binary_sub, perl::Canned< const UniPolynomial< UniPolynomial< Rational, int >, Rational > >, perl::Canned< const UniPolynomial< UniPolynomial< Rational, int >, Rational > >);
   Class4perl("Polymake::common::UniPolynomial_A_UniPolynomial_A_Rational_I_Int_Z_I_Rational_Z", UniPolynomial< UniPolynomial< Rational, int >, Rational >);
   FunctionInstance4perl(new, UniPolynomial< UniPolynomial< Rational, int >, Rational >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const UniPolynomial< UniPolynomial< Rational, int >, Rational > >, perl::Canned< const UniPolynomial< UniPolynomial< Rational, int >, Rational > >);
   FunctionInstance4perl(new_X_X, UniPolynomial< Rational, int >, perl::Canned< const Array< int > >, perl::Canned< const Array< int > >);
   Class4perl("Polymake::common::UniPolynomial_A_TropicalNumber_A_Min_I_Rational_Z_I_Int_Z", UniPolynomial< TropicalNumber< Min, Rational >, int >);
   FunctionInstance4perl(new, UniPolynomial< TropicalNumber< Min, Rational >, int >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const UniPolynomial< TropicalNumber< Min, Rational >, int > >, perl::Canned< const UniPolynomial< TropicalNumber< Min, Rational >, int > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const UniPolynomial< TropicalNumber< Max, Rational >, int > >, perl::Canned< const TropicalNumber< Max, Rational > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const UniPolynomial< TropicalNumber< Max, Rational >, int > >, perl::Canned< const UniPolynomial< TropicalNumber< Max, Rational >, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const UniPolynomial< TropicalNumber< Max, Rational >, int > >, perl::Canned< const UniPolynomial< TropicalNumber< Max, Rational >, int > >);
   Class4perl("Polymake::common::UniPolynomial_A_TropicalNumber_A_Max_I_Rational_Z_I_Int_Z", UniPolynomial< TropicalNumber< Max, Rational >, int >);
   Class4perl("Polymake::common::UniPolynomial_A_QuadraticExtension__Rational_I_Int_Z", UniPolynomial< QuadraticExtension< Rational >, int >);
   FunctionInstance4perl(new_X_X, UniPolynomial< QuadraticExtension< Rational >, int >, perl::Canned< const Array< QuadraticExtension< Rational > > >, perl::Canned< const Array< int > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const UniPolynomial< QuadraticExtension< Rational >, int > >, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const UniPolynomial< Rational, int > >);
   FunctionInstance4perl(new, UniPolynomial< QuadraticExtension< Rational >, int >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const UniPolynomial< QuadraticExtension< Rational >, int > >, perl::Canned< const UniPolynomial< QuadraticExtension< Rational >, int > >);
   OperatorInstance4perl(Binary_xor, perl::Canned< const UniPolynomial< TropicalNumber< Max, Rational >, int > >, int);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
