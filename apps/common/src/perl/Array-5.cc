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
#include "polymake/Bitset.h"
#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/Polynomial.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/hash_map"
#include "polymake/hash_set"
#include "polymake/list"

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

   Class4perl("Polymake::common::Array__Float", Array< double >);
   FunctionInstance4perl(new_X, Array< double >, perl::Canned< const EdgeMap< Undirected, double > >);
   FunctionInstance4perl(new, Array< double >);
   FunctionInstance4perl(new_X, Array< int >, perl::Canned< const pm::Series<int, true> >);
   FunctionInstance4perl(new_X, Array< Array< int > >, perl::Canned< const Set< Array< int > > >);
   FunctionInstance4perl(new_X, Array< Array< Array< int > > >, int);
   FunctionInstance4perl(new_X, Array< Set< Array< int > > >, int);
   OperatorInstance4perl(convert, Array< Set< Array< int > > >, perl::Canned< const Array< Array< Array< int > > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< Array< Array< int > > > >, perl::Canned< const Array< Array< Array< int > > > >);
   FunctionInstance4perl(new_X, Array< int >, perl::Canned< const Vector< int > >);
   FunctionInstance4perl(new_X, Array< Set< Array< int > > >, perl::Canned< const Array< Array< Array< int > > > >);
   OperatorInstance4perl(assign, Array< int >, perl::Canned< const Vector< int > >);
   OperatorInstance4perl(convert, Array< Array< Array< int > > >, perl::Canned< const Array< Set< Array< int > > > >);
   FunctionInstance4perl(new_X, Array< Array< int > >, perl::Canned< const Array< Set< int > > >);
   FunctionInstance4perl(new_X, Array< Vector< Rational > >, int);
   Class4perl("Polymake::common::Array__HashSet__Int", Array< hash_set< int > >);
   FunctionInstance4perl(new, Array< hash_set< int > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< hash_set< int > > >, perl::Canned< const Array< hash_set< int > > >);
   FunctionInstance4perl(new_X, Array< Matrix< Integer > >, perl::Canned< const Array< Matrix< Integer > > >);
   Class4perl("Polymake::common::Array__List__Pair_A_Int_I_Int_Z", Array< std::list< std::pair< int, int > > >);
   FunctionInstance4perl(new, Array< std::list< std::pair< int, int > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< std::list< std::pair< int, int > > > >, perl::Canned< const Array< std::list< std::pair< int, int > > > >);
   Class4perl("Polymake::common::Array__SparseMatrix_A_Rational_I_NonSymmetric_Z", Array< SparseMatrix< Rational, NonSymmetric > >);
   Class4perl("Polymake::common::Array__SparseMatrix_A_Integer_I_NonSymmetric_Z", Array< SparseMatrix< Integer, NonSymmetric > >);
   FunctionInstance4perl(new_X, Array< hash_set< int > >, perl::Canned< const Array< hash_set< int > > >);
   Class4perl("Polymake::common::Array__Pair_A_Set__Int_I_Int_Z", Array< std::pair< Set< int >, int > >);
   Class4perl("Polymake::common::Array__Pair_A_SparseMatrix_A_Integer_I_NonSymmetric_Z_I_Array__Int_Z", Array< std::pair< SparseMatrix< Integer, NonSymmetric >, Array< int > > >);
   FunctionInstance4perl(new_X, Array< std::string >, int);
   FunctionInstance4perl(new, Array< SparseMatrix< Integer, NonSymmetric > >);
   Class4perl("Polymake::common::Array__Array__Array__Array__Int", Array< Array< Array< Array< int > > > >);
   Class4perl("Polymake::common::Array__Polynomial_A_Rational_I_Int_Z", Array< Polynomial< Rational, int > >);
   FunctionInstance4perl(new, Array< Polynomial< Rational, int > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< Polynomial< Rational, int > > >, perl::Canned< const Array< Polynomial< Rational, int > > >);
   FunctionInstance4perl(new_X, Array< Polynomial< Rational, int > >, perl::Canned< const Array< Polynomial< Rational, int > > >);
   Class4perl("Polymake::common::Array__Pair_A_Bitset_I_HashMap_A_Bitset_I_Rational_Z_Z", Array< std::pair< Bitset, hash_map< Bitset, Rational > > >);
   Class4perl("Polymake::common::Array__Array__Bitset", Array< Array< Bitset > >);
   Class4perl("Polymake::common::Array__Bitset", Array< Bitset >);
   Class4perl("Polymake::common::Array__HashMap_A_Bitset_I_Rational_Z", Array< hash_map< Bitset, Rational > >);
   FunctionInstance4perl(new, Array< Bitset >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< Bitset > >, perl::Canned< const Array< Bitset > >);
   FunctionInstance4perl(new, Array< Array< Bitset > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< Array< Bitset > > >, perl::Canned< const Array< Array< Bitset > > >);
   FunctionInstance4perl(new, Array< hash_map< Bitset, Rational > >);
   FunctionInstance4perl(new_X, Array< Bitset >, perl::Canned< const Array< Bitset > >);
   FunctionInstance4perl(new_X, Array< Array< Set< int > > >, perl::Canned< const Array< Array< Bitset > > >);
   OperatorInstance4perl(assign, Array< int >, perl::Canned< const Bitset >);
   FunctionInstance4perl(new, Array< std::pair< Bitset, hash_map< Bitset, Rational > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< std::pair< Bitset, hash_map< Bitset, Rational > > > >, perl::Canned< const Array< std::pair< Bitset, hash_map< Bitset, Rational > > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
