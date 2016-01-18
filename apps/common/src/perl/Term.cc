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
#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   ClassTemplate4perl("Polymake::common::Term");
   Class4perl("Polymake::common::Term_A_Rational_I_Int_Z", Term< Rational, int >);
   OperatorInstance4perl(Binary_add, int, perl::Canned< const Term< Rational, int > >);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const Term< Rational, int > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Term< Rational, int > >, perl::Canned< const Term< Rational, int > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Term< Rational, int > >, perl::Canned< const Polynomial< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Term< Rational, int > >, perl::Canned< const Monomial< Rational, int > >);
   FunctionInstance4perl(new, Term< Rational, int >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Term< Rational, int > >, perl::Canned< const Term< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Term< Rational, int > >, perl::Canned< const Polynomial< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Term< Rational, int > >, perl::Canned< const Term< Rational, int > >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Term< Rational, int > >);
   Class4perl("Polymake::common::Term_A_TropicalNumber_A_Min_I_Rational_Z_I_Int_Z", Term< TropicalNumber< Min, Rational >, int >);
   Class4perl("Polymake::common::Term_A_TropicalNumber_A_Max_I_Rational_Z_I_Int_Z", Term< TropicalNumber< Max, Rational >, int >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
