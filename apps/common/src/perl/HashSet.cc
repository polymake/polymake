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
#include "polymake/Polynomial.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/hash_set"

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

   ClassTemplate4perl("Polymake::common::HashSet");
   Class4perl("Polymake::common::HashSet__Vector__Rational", hash_set< Vector< Rational > >);
   FunctionInstance4perl(new, hash_set< Vector< Rational > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< hash_set< Vector< Rational > > >, perl::Canned< const Vector< Rational > >);
   Class4perl("Polymake::common::HashSet__Int", hash_set< int >);
   Class4perl("Polymake::common::HashSet__Matrix_A_Rational_I_NonSymmetric_Z", hash_set< Matrix< Rational > >);
   Class4perl("Polymake::common::HashSet__Matrix_A_Int_I_NonSymmetric_Z", hash_set< Matrix< int > >);
   Class4perl("Polymake::common::HashSet__Polynomial_A_Rational_I_Int_Z", hash_set< Polynomial< Rational, int > >);
   Class4perl("Polymake::common::HashSet__Monomial_A_Rational_I_Int_Z", hash_set< Monomial< Rational, int > >);
   Class4perl("Polymake::common::HashSet__Set__Int", hash_set< Set< int > >);
   Class4perl("Polymake::common::HashSet__SparseVector__Rational", hash_set< SparseVector< Rational > >);
   Class4perl("Polymake::common::HashSet__Set__Set__Int", hash_set< Set< Set< int > > >);
   Class4perl("Polymake::common::HashSet__Array__Int", hash_set< Array< int > >);
   Class4perl("Polymake::common::HashSet__Vector__Int", hash_set< Vector< int > >);
   Class4perl("Polymake::common::HashSet__Pair_A_Set__Int_I_Set__Set__Int_Z", hash_set< std::pair< Set< int >, Set< Set< int > > > >);
   FunctionInstance4perl(new, hash_set< Matrix< Rational > >);
   FunctionInstance4perl(new, hash_set< SparseVector< Rational > >);
   FunctionInstance4perl(new, hash_set< Set< int > >);
   FunctionInstance4perl(new, hash_set< Set< Set< int > > >);
   FunctionInstance4perl(new, hash_set< std::pair< Set< int >, Set< Set< int > > > >);
   FunctionInstance4perl(new, hash_set< Vector< int > >);
   FunctionInstance4perl(new, hash_set< Array< int > >);
   FunctionInstance4perl(new, hash_set< Matrix< int > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const hash_set< SparseVector< Rational > > >, perl::Canned< const hash_set< SparseVector< Rational > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const hash_set< Vector< Rational > > >, perl::Canned< const hash_set< Vector< Rational > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const hash_set< Set< int > > >, perl::Canned< const hash_set< Set< int > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const hash_set< Set< Set< int > > > >, perl::Canned< const hash_set< Set< Set< int > > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const hash_set< Matrix< Rational > > >, perl::Canned< const hash_set< Matrix< Rational > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const hash_set< std::pair< Set< int >, Set< Set< int > > > > >, perl::Canned< const hash_set< std::pair< Set< int >, Set< Set< int > > > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const hash_set< Array< int > > >, perl::Canned< const hash_set< Array< int > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const hash_set< Vector< int > > >, perl::Canned< const hash_set< Vector< int > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const hash_set< Matrix< int > > >, perl::Canned< const hash_set< Matrix< int > > >);
   FunctionInstance4perl(new, hash_set< Polynomial< Rational, int > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const hash_set< Polynomial< Rational, int > > >, perl::Canned< const hash_set< Polynomial< Rational, int > > >);
   FunctionInstance4perl(new, hash_set< Monomial< Rational, int > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const hash_set< Monomial< Rational, int > > >, perl::Canned< const hash_set< Monomial< Rational, int > > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< hash_set< Vector< Rational > > >, perl::Canned< const hash_set< Vector< Rational > > >);
   FunctionInstance4perl(new, hash_set< int >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< hash_set< int > >, int);
   FunctionInstance4perl(new_X, hash_set< Set< int > >, perl::Canned< const Array< Set< int > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
