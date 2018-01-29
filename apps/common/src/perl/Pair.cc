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
#include "polymake/Bitset.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/hash_map"
#include "polymake/hash_set"
#include "polymake/list"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   Class4perl("Polymake::common::Pair_A_Pair_A_Int_I_Int_Z_I_Vector__Rational_Z", std::pair< std::pair< int, int >, Vector< Rational > >);
   Class4perl("Polymake::common::Pair_A_Vector__Rational_I_Vector__Rational_Z", std::pair< Vector< Rational >, Vector< Rational > >);
   Class4perl("Polymake::common::Pair_A_Array__Set__Int_I_SparseMatrix_A_Rational_I_NonSymmetric_Z_Z", std::pair< Array< Set< int > >, SparseMatrix< Rational, NonSymmetric > >);
   Class4perl("Polymake::common::Pair_A_Matrix_A_Rational_I_NonSymmetric_Z_I_Array__HashSet__Int_Z", std::pair< Matrix< Rational >, Array< hash_set< int > > >);
   Class4perl("Polymake::common::Pair_A_Vector__Int_I_Integer_Z", std::pair< Vector< int >, Integer >);
   Class4perl("Polymake::common::Pair_A_Set__Int_I_Integer_Z", std::pair< Set< int >, Integer >);
   Class4perl("Polymake::common::Pair_A_TropicalNumber_A_Min_I_Rational_Z_I_Array__Int_Z", std::pair< TropicalNumber< Min, Rational >, Array< int > >);
   FunctionInstance4perl(new, std::pair< TropicalNumber< Min, Rational >, Array< int > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const std::pair< TropicalNumber< Min, Rational >, Array< int > > >, perl::Canned< const std::pair< TropicalNumber< Min, Rational >, Array< int > > >);
   Class4perl("Polymake::common::Pair_A_TropicalNumber_A_Max_I_Rational_Z_I_Array__Int_Z", std::pair< TropicalNumber< Max, Rational >, Array< int > >);
   Class4perl("Polymake::common::Pair_A_SparseMatrix_A_Integer_I_NonSymmetric_Z_I_List__Pair_A_Integer_I_SparseMatrix_A_Integer_I_NonSymmetric_Z_Z_Z", std::pair< SparseMatrix< Integer, NonSymmetric >, std::list< std::pair< Integer, SparseMatrix< Integer, NonSymmetric > > > >);
   Class4perl("Polymake::common::Pair_A_Integer_I_SparseMatrix_A_Integer_I_NonSymmetric_Z_Z", std::pair< Integer, SparseMatrix< Integer, NonSymmetric > >);
   FunctionInstance4perl(new, std::pair< SparseMatrix< Integer, NonSymmetric >, std::list< std::pair< Integer, SparseMatrix< Integer, NonSymmetric > > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const std::pair< SparseMatrix< Integer, NonSymmetric >, std::list< std::pair< Integer, SparseMatrix< Integer, NonSymmetric > > > > >, perl::Canned< const std::pair< SparseMatrix< Integer, NonSymmetric >, std::list< std::pair< Integer, SparseMatrix< Integer, NonSymmetric > > > > >);
   Class4perl("Polymake::common::Pair_A_String_I_String_Z", std::pair< std::string, std::string >);
   Class4perl("Polymake::common::Pair_A_Int_I_Pair_A_Int_I_Int_Z_Z", std::pair< int, std::pair< int, int > >);
   Class4perl("Polymake::common::Pair_A_Int_I_List__Int_Z", std::pair< int, std::list< int > >);
	Class4perl("Polymake::common::Pair_A_SparseVector__Int_I_PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_Z", std::pair< SparseVector< int >, PuiseuxFraction< Min, Rational, Rational > >);
	Class4perl("Polymake::common::Pair_A_Rational_I_PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_Z", std::pair< Rational, PuiseuxFraction< Min, Rational, Rational > >);
   Class4perl("Polymake::common::Pair_A_Matrix_A_Float_I_NonSymmetric_Z_I_Matrix_A_Float_I_NonSymmetric_Z_Z", std::pair< Matrix< double >, Matrix< double > >);
   Class4perl("Polymake::common::Pair_A_Array__Set__Int_I_Array__Int_Z", std::pair< Array< Set< int > >, Array< int > >);
   Class4perl("Polymake::common::Pair_A_Bitset_I_HashMap_A_Bitset_I_Rational_Z_Z", std::pair< Bitset, hash_map< Bitset, Rational > >);
   Class4perl("Polymake::common::Pair_A_Array__Bitset_I_Array__Bitset_Z", std::pair< Array< Bitset >, Array< Bitset > >);
   Class4perl("Polymake::common::Pair_A_Array__Array__Matrix_A_QuadraticExtension__Rational_I_NonSymmetric_Z_I_Array__Matrix_A_QuadraticExtension__Rational_I_NonSymmetric_Z_Z", std::pair< Array< Array< Matrix< QuadraticExtension< Rational > > > >, Array< Matrix< QuadraticExtension< Rational > > > >);
   Class4perl("Polymake::common::Pair_A_Array__Set__Matrix_A_QuadraticExtension__Rational_I_NonSymmetric_Z_I_Array__Matrix_A_QuadraticExtension__Rational_I_NonSymmetric_Z_Z", std::pair< Array< Set< Matrix< QuadraticExtension< Rational > > > >, Array< Matrix< QuadraticExtension< Rational > > > >);
   Class4perl("Polymake::common::Pair_A_Array__Set__Array__Int_I_Array__Array__Int_Z", std::pair< Array< Set< Array< int > > >, Array< Array< int > > >);
   Class4perl("Polymake::common::Pair_A_Array__Set__Matrix_A_Rational_I_NonSymmetric_Z_I_Array__Matrix_A_Rational_I_NonSymmetric_Z_Z", std::pair< Array< Set< Matrix< Rational > > >, Array< Matrix< Rational > > >);
   Class4perl("Polymake::common::Pair_A_Bool_I_Matrix_A_Rational_I_NonSymmetric_Z_Z", std::pair< bool, Matrix< Rational > >);
   FunctionInstance4perl(new, std::pair< Rational, Rational >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const std::pair< Rational, Rational > >, perl::Canned< const std::pair< Rational, Rational > >);
   Class4perl("Polymake::common::Pair_A_Matrix_A_TropicalNumber_A_Min_I_Rational_Z_I_NonSymmetric_Z_I_IncidenceMatrix__NonSymmetric_Z", std::pair< Matrix< TropicalNumber< Min, Rational > >, IncidenceMatrix< NonSymmetric > >);
   FunctionInstance4perl(new, std::pair< Matrix< TropicalNumber< Min, Rational > >, IncidenceMatrix< NonSymmetric > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const std::pair< Matrix< TropicalNumber< Min, Rational > >, IncidenceMatrix< NonSymmetric > > >, perl::Canned< const std::pair< Matrix< TropicalNumber< Min, Rational > >, IncidenceMatrix< NonSymmetric > > >);
   Class4perl("Polymake::common::Pair_A_Vector__TropicalNumber_A_Min_I_Rational_Z_I_Int_Z", std::pair< Vector< TropicalNumber< Min, Rational > >, int >);
   FunctionInstance4perl(new, std::pair< Vector< TropicalNumber< Min, Rational > >, int >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const std::pair< Vector< TropicalNumber< Min, Rational > >, int > >, perl::Canned< const std::pair< Vector< TropicalNumber< Min, Rational > >, int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
