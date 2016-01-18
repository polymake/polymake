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

#include "polymake/Array.h"
#include "polymake/Polynomial.h"
#include "polymake/Rational.h"
#include "polymake/Ring.h"
#include "polymake/TropicalNumber.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_std__string_P, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   ClassTemplate4perl("Polymake::common::Ring");
   Class4perl("Polymake::common::Ring_A_Rational_I_Int_Z", Ring< Rational, int >);
   FunctionInstance4perl(new_X, Ring< Rational, int >, perl::Canned< const Array< std::string > >);
   Class4perl("Polymake::common::Ring_A_Rational_I_Rational_Z", Ring< Rational, Rational >);
   FunctionInstance4perl(new_X, Ring< Rational, Rational >, perl::Canned< const Array< std::string > >);
   FunctionInstance4perl(new_X, Ring< Rational, int >, int);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Ring< Rational, int > >, perl::Canned< const Ring< Rational, int > >);
   FunctionInstance4perl(new_X, Ring< Rational, int >, perl::Canned< const Ring< Rational, int > >);
   FunctionInstance4perl(new, Ring< Rational, int >);
   FunctionInstance4perl(new_std__string_P, Ring< Rational, int >, perl::Canned< const Array< std::string > >);
   Class4perl("Polymake::common::Ring_A_TropicalNumber_A_Min_I_Rational_Z_I_Int_Z", Ring< TropicalNumber< Min, Rational >, int >);
   FunctionInstance4perl(new_X, Ring< TropicalNumber< Min, Rational >, int >, perl::Canned< const Array< std::string > >);
   Class4perl("Polymake::common::Ring_A_TropicalNumber_A_Max_I_Rational_Z_I_Int_Z", Ring< TropicalNumber< Max, Rational >, int >);
   FunctionInstance4perl(new_X, Ring< TropicalNumber< Min, Rational >, int >, int);
   FunctionInstance4perl(new_X, Ring< TropicalNumber< Max, Rational >, int >, int);
   FunctionInstance4perl(new_std__string_P, Ring< TropicalNumber< Max, Rational >, int >, perl::Canned< const Array< std::string > >);
   FunctionInstance4perl(new_X, Ring< Rational, Rational >, int);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
