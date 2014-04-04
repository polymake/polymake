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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Integer.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/SparseVector.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   ClassTemplate4perl("Polymake::common::Pair");
   Class4perl("Polymake::common::Pair_A_Set__Int_I_Set__Int_Z", std::pair< Set< int >, Set< int > >);
   Class4perl("Polymake::common::Pair_A_Integer_I_Int_Z", std::pair< Integer, int >);
   Class4perl("Polymake::common::Pair_A_Bool_I_Vector__Rational_Z", std::pair< bool, Vector< Rational > >);
   Class4perl("Polymake::common::Pair_A_Vector__Rational_I_Set__Int_Z", std::pair< Vector< Rational >, Set< int > >);
   Class4perl("Polymake::common::Pair_A_Array__Int_I_Array__Int_Z", std::pair< Array< int >, Array< int > >);
   FunctionInstance4perl(new, std::pair< Array< int >, Array< int > >);
   Class4perl("Polymake::common::Pair_A_Matrix_A_Rational_I_NonSymmetric_Z_I_Array__Set__Int_Z", std::pair< Matrix<Rational>, Array< Set< int > > >);
   FunctionInstance4perl(new, std::pair< Set< int >, Set< int > >);
   Class4perl("Polymake::common::Pair_A_SparseVector__Int_I_Rational_Z", std::pair< SparseVector< int >, Rational >);
   OperatorInstance4perl(assign, std::pair< SparseVector< int >, Rational >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<int>&>, pm::Series<int, true>, void> >);
   Class4perl("Polymake::common::Pair_A_Int_I_Set__Int_Z", std::pair< int, Set< int > >);
   FunctionInstance4perl(new, std::pair< int, Set< int > >);
   Class4perl("Polymake::common::Pair_A_Array__Set__Int_I_Array__Set__Int_Z", std::pair< Array< Set< int > >, Array< Set< int > > >);
   Class4perl("Polymake::common::Pair_A_Int_I_Rational_Z", std::pair< int, Rational >);
   Class4perl("Polymake::common::Pair_A_Rational_I_Rational_Z", std::pair< Rational, Rational >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
