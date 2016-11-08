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

#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Map.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   FunctionInstance4perl(new, Map< Vector< int >, Integer >);
   FunctionInstance4perl(new, Map< Set< int >, Integer >);
   Class4perl("Polymake::common::Map_A_Set__Set__Int_I_Matrix_A_Rational_I_NonSymmetric_Z_Z", Map< Set< Set< int > >, Matrix< Rational > >);
   Class4perl("Polymake::common::Map_A_Set__Set__Int_I_Int_Z", Map< Set< Set< int > >, int >);
   Class4perl("Polymake::common::Map_A_Set__Int_I_Matrix_A_Rational_I_NonSymmetric_Z_Z", Map< Set< int >, Matrix< Rational > >);
   Class4perl("Polymake::common::Map_A_Int_I_Array__Set__Int_Z", Map< int, Array< Set< int > > >);
   Class4perl("Polymake::common::Map_A_String_I_String_Z", Map< std::string, std::string >);
   FunctionInstance4perl(new, Map< std::string, std::string >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< std::string, std::string > >, std::string);
   OperatorInstance4perl(Binary_brk, perl::Canned< const Map< Set< int >, Integer > >, perl::Canned< const Set< int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
