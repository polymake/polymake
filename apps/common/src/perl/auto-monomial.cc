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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/Polynomial.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/Rational.h"
#include "polymake/RationalFunction.h"
#include "polymake/TropicalNumber.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( Polynomial__monomial_int_int_f1, T0 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]);
      WrapperReturn( T0::monomial(arg0.get<int>(), arg1.get<int>()) );
   };

   template <typename T0>
   FunctionInterface4perl( UniPolynomial__monomial_f1, T0 ) {
      WrapperReturn( T0::monomial() );
   };

   FunctionInstance4perl(Polynomial__monomial_int_int_f1, Polynomial< Rational, int >);
   FunctionInstance4perl(UniPolynomial__monomial_f1, UniPolynomial< Rational, int >);
   FunctionInstance4perl(UniPolynomial__monomial_f1, UniPolynomial< Rational, Rational >);
   FunctionInstance4perl(Polynomial__monomial_int_int_f1, Polynomial< PuiseuxFraction< Min, Rational, Rational >, int >);
   FunctionInstance4perl(UniPolynomial__monomial_f1, UniPolynomial< PuiseuxFraction< Min, Rational, Rational >, Rational >);
   FunctionInstance4perl(UniPolynomial__monomial_f1, UniPolynomial< UniPolynomial< Rational, int >, Rational >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
