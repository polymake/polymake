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
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/hash_map"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   ClassTemplate4perl("Polymake::common::HashMap");
   Class4perl("Polymake::common::HashMap_A_SparseVector__Int_I_Rational_Z", hash_map< SparseVector< int >, Rational >);
   Class4perl("Polymake::common::HashMap_A_Int_I_Rational_Z", hash_map< int, Rational >);
   Class4perl("Polymake::common::HashMap_A_SparseVector__Int_I_TropicalNumber_A_Max_I_Rational_Z_Z", hash_map< SparseVector< int >, TropicalNumber< Max, Rational > >);
   Class4perl("Polymake::common::HashMap_A_SparseVector__Int_I_TropicalNumber_A_Min_I_Rational_Z_Z", hash_map< SparseVector< int >, TropicalNumber< Min, Rational > >);
   Class4perl("Polymake::common::HashMap_A_Rational_I_Rational_Z", hash_map< Rational, Rational >);
   Class4perl("Polymake::common::HashMap_A_SparseVector__Int_I_PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_Z", hash_map< SparseVector< int >, PuiseuxFraction< Min, Rational, Rational > >);
   Class4perl("Polymake::common::HashMap_A_Rational_I_PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_Z", hash_map< Rational, PuiseuxFraction< Min, Rational, Rational > >);
   Class4perl("Polymake::common::HashMap_A_Set__Int_I_Int_Z", hash_map< Set< int >, int >);
   FunctionInstance4perl(new, hash_map< Set< int >, int >);
   OperatorInstance4perl(Binary_brk, perl::Canned< hash_map< Set< int >, int > >, perl::Canned< const Set< int > >);
   Class4perl("Polymake::common::HashMap_A_Vector__Rational_I_Int_Z", hash_map< Vector< Rational >, int >);
   FunctionInstance4perl(new, hash_map< Vector< Rational >, int >);
   OperatorInstance4perl(Binary_brk, perl::Canned< hash_map< Vector< Rational >, int > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> >);
   Class4perl("Polymake::common::HashMap_A_Array__Int_I_Int_Z", hash_map< Array< int >, int >);
   FunctionInstance4perl(new, hash_map< Array< int >, int >);
   Class4perl("Polymake::common::HashMap_A_Set__Int_I_Rational_Z", hash_map< Set< int >, Rational >);
   FunctionInstance4perl(new, hash_map< Set< int >, Rational >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const hash_map< Set< int >, int > >, perl::Canned< const hash_map< Set< int >, int > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const hash_map< Set< int >, Rational > >, perl::Canned< const hash_map< Set< int >, Rational > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
