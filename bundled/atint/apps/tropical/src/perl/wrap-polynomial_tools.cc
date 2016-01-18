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

#include "polymake/Polynomial.h"
#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"

namespace polymake { namespace tropical { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( tolerant_multiplication_T_X_X, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (tolerant_multiplication<T0>(arg0.get<T1>(), arg1.get<T2>())) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( polynomial_degree_T_X, T0,T1 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (polynomial_degree<T0>(arg0.get<T1>())) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( is_homogeneous_T_X, T0,T1 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (is_homogeneous<T0>(arg0.get<T1>())) );
   };

   FunctionInstance4perl(is_homogeneous_T_X, TropicalNumber< Min, Rational >, perl::Canned< const Polynomial< TropicalNumber< Min, Rational >, int > >);
   FunctionInstance4perl(polynomial_degree_T_X, TropicalNumber< Min, Rational >, perl::Canned< const Polynomial< TropicalNumber< Min, Rational >, int > >);
   FunctionInstance4perl(polynomial_degree_T_X, TropicalNumber< Max, Rational >, perl::Canned< const Polynomial< TropicalNumber< Max, Rational >, int > >);
   FunctionInstance4perl(is_homogeneous_T_X, TropicalNumber< Max, Rational >, perl::Canned< const Polynomial< TropicalNumber< Max, Rational >, int > >);
   FunctionInstance4perl(tolerant_multiplication_T_X_X, TropicalNumber< Min, Rational >, perl::Canned< const Polynomial< TropicalNumber< Min, Rational >, int > >, perl::Canned< const Polynomial< TropicalNumber< Min, Rational >, int > >);
   FunctionInstance4perl(tolerant_multiplication_T_X_X, TropicalNumber< Max, Rational >, perl::Canned< const Polynomial< TropicalNumber< Max, Rational >, int > >, perl::Canned< const Polynomial< TropicalNumber< Max, Rational >, int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
