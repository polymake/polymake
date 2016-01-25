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

#include "polymake/PuiseuxFraction.h"
#include "polymake/Rational.h"
#include "polymake/RationalFunction.h"
#include "polymake/TropicalNumber.h"
#include "polymake/client.h"

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

   ClassTemplate4perl("Polymake::common::RationalFunction");
   Class4perl("Polymake::common::RationalFunction_A_Rational_I_Int_Z", RationalFunction< Rational, int >);
   FunctionInstance4perl(new, RationalFunction< Rational, int >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const RationalFunction< Rational, int > >, perl::Canned< const RationalFunction< Rational, int > >);
   FunctionInstance4perl(new_X, RationalFunction< Rational, int >, perl::Canned< const UniMonomial< Rational, int > >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const RationalFunction< Rational, int > >);
   Class4perl("Polymake::common::RationalFunction_A_Rational_I_Rational_Z", RationalFunction< Rational, Rational >);
   Class4perl("Polymake::common::RationalFunction_A_PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_I_Rational_Z", RationalFunction< PuiseuxFraction< Min, Rational, Rational >, Rational >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
