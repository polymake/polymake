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
   ClassTemplate4perl("Polymake::common::UniTerm");
   Class4perl("Polymake::common::UniTerm_A_Rational_I_Int_Z", UniTerm< Rational, int >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const UniTerm< Rational, int > >, perl::Canned< const UniTerm< Rational, int > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const UniTerm< Rational, int > >, int);
   OperatorInstance4perl(Binary_add, perl::Canned< const UniTerm< Rational, int > >, perl::Canned< const UniMonomial< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const UniTerm< Rational, int > >, int);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const UniTerm< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const UniTerm< Rational, int > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary_div, perl::Canned< const UniTerm< Rational, int > >, int);
   OperatorInstance4perl(Binary_div, perl::Canned< const UniTerm< Rational, int > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const UniTerm< Rational, int > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const UniTerm< Rational, int > >, int);
   OperatorInstance4perl(Binary_add, int, perl::Canned< const UniTerm< Rational, int > >);
   OperatorInstance4perl(Binary_sub, int, perl::Canned< const UniTerm< Rational, int > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const UniTerm< Rational, int > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const UniTerm< Rational, int > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const UniTerm< Rational, int > >, perl::Canned< const UniMonomial< Rational, int > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const UniTerm< Rational, int > >, perl::Canned< const UniTerm< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const UniTerm< Rational, int > >, perl::Canned< const UniMonomial< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const UniTerm< Rational, int > >, perl::Canned< const UniTerm< Rational, int > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const UniTerm< Rational, int > >, perl::Canned< const UniPolynomial< Rational, int > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const UniTerm< Rational, int > >, perl::Canned< const UniPolynomial< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const UniTerm< Rational, int > >, perl::Canned< const UniPolynomial< Rational, int > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const UniTerm< Rational, int > >, perl::Canned< const UniMonomial< Rational, int > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const UniTerm< Rational, int > >, perl::Canned< const UniTerm< Rational, int > >);
   OperatorInstance4perl(Binary_div, int, perl::Canned< const UniTerm< Rational, int > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const UniTerm< Rational, int > >, perl::Canned< const UniPolynomial< Rational, int > >);
   Class4perl("Polymake::common::UniTerm_A_Rational_I_Rational_Z", UniTerm< Rational, Rational >);
   Class4perl("Polymake::common::UniTerm_A_TropicalNumber_A_Min_I_Rational_Z_I_Int_Z", UniTerm< TropicalNumber< Min, Rational >, int >);
   Class4perl("Polymake::common::UniTerm_A_PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_I_Rational_Z", UniTerm< PuiseuxFraction< Min, Rational, Rational >, Rational >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const UniTerm< Rational, Rational > >, int);
   OperatorInstance4perl(Binary_add, perl::Canned< const UniTerm< Rational, Rational > >, perl::Canned< const UniMonomial< Rational, Rational > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const UniTerm< Rational, Rational > >, perl::Canned< const UniMonomial< Rational, Rational > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const UniTerm< Rational, Rational > >, perl::Canned< const UniTerm< Rational, Rational > >);
   OperatorInstance4perl(Binary_div, int, perl::Canned< const UniTerm< Rational, Rational > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const UniTerm< Rational, Rational > >, int);
   OperatorInstance4perl(Binary_div, perl::Canned< const UniTerm< Rational, Rational > >, perl::Canned< const UniPolynomial< Rational, Rational > >);
   Class4perl("Polymake::common::UniTerm_A_PuiseuxFraction_A_Min_I_PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_I_Rational_Z_I_Rational_Z", UniTerm< PuiseuxFraction< Min, PuiseuxFraction< Min, Rational, Rational >, Rational >, Rational >);
   Class4perl("Polymake::common::UniTerm_A_UniPolynomial_A_Rational_I_Int_Z_I_Int_Z", UniTerm< UniPolynomial< Rational, int >, int >);
   OperatorInstance4perl(Binary_add, perl::Canned< const UniTerm< PuiseuxFraction< Min, Rational, Rational >, Rational > >, perl::Canned< const UniMonomial< PuiseuxFraction< Min, Rational, Rational >, Rational > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
