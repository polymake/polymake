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
#include "polymake/IncidenceMatrix.h"
#include "polymake/Map.h"
#include "polymake/Matrix.h"
#include "polymake/Polynomial.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/RationalFunction.h"
#include "polymake/TropicalNumber.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( substitute_X_f1, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( arg0.get<T0>().substitute(arg1.get<T1>()) );
   };

   FunctionInstance4perl(substitute_X_f1, perl::Canned< const UniPolynomial< TropicalNumber< Max, Rational >, int > >, perl::Canned< const TropicalNumber< Max, Rational > >);
   FunctionInstance4perl(substitute_X_f1, perl::Canned< const UniPolynomial< TropicalNumber< Max, Rational >, int > >, perl::Canned< const UniPolynomial< TropicalNumber< Max, Rational >, int > >);
   FunctionInstance4perl(substitute_X_f1, perl::Canned< const UniPolynomial< QuadraticExtension< Rational >, int > >, int);
   FunctionInstance4perl(substitute_X_f1, perl::Canned< const UniPolynomial< QuadraticExtension< Rational >, int > >, perl::Canned< const Rational >);
   FunctionInstance4perl(substitute_X_f1, perl::Canned< const UniPolynomial< QuadraticExtension< Rational >, int > >, perl::Canned< const QuadraticExtension< Rational > >);
   FunctionInstance4perl(substitute_X_f1, perl::Canned< const UniPolynomial< Rational, int > >, int);
   FunctionInstance4perl(substitute_X_f1, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const Rational >);
   FunctionInstance4perl(substitute_X_f1, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const QuadraticExtension< Rational > >);
   FunctionInstance4perl(substitute_X_f1, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(substitute_X_f1, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const UniPolynomial< Rational, int > >);
   FunctionInstance4perl(substitute_X_f1, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const UniPolynomial< QuadraticExtension< Rational >, int > >);
   FunctionInstance4perl(substitute_X_f1, perl::Canned< const UniPolynomial< QuadraticExtension< Rational >, int > >, perl::Canned< const UniPolynomial< Rational, int > >);
   FunctionInstance4perl(substitute_X_f1, perl::Canned< const UniPolynomial< QuadraticExtension< Rational >, int > >, perl::Canned< const UniPolynomial< QuadraticExtension< Rational >, int > >);
   FunctionInstance4perl(substitute_X_f1, perl::Canned< const UniPolynomial< Rational, int > >, perl::Canned< const Polynomial< Rational, int > >);
   FunctionInstance4perl(substitute_X_f1, perl::Canned< const Polynomial< Rational, int > >, perl::Canned< const Map< int, Rational > >);
   FunctionInstance4perl(substitute_X_f1, perl::Canned< const Polynomial< Rational, int > >, perl::Canned< const Map< int, QuadraticExtension< Rational > > >);
   FunctionInstance4perl(substitute_X_f1, perl::Canned< const Polynomial< Rational, int > >, perl::Canned< const Array< int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }