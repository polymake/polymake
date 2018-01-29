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

#include "polymake/Bitset.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/hash_set"
#include "polymake/linalg.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   FunctionInstance4perl(new_X, hash_set< Vector< Rational > >, perl::Canned< const pm::Rows<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> > >);
   Class4perl("Polymake::common::HashSet__Bitset", hash_set< Bitset >);
   FunctionInstance4perl(new, hash_set< Bitset >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const hash_set< Bitset > >, perl::Canned< const hash_set< Bitset > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< hash_set< Vector< Rational > > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, mlist<> > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< hash_set< Set< int > > >, perl::Canned< const Set< int > >);
   Class4perl("Polymake::common::HashSet__Vector__QuadraticExtension__Rational", hash_set< Vector< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new, hash_set< Vector< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< hash_set< Vector< QuadraticExtension< Rational > > > >, perl::Canned< const Vector< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(BinaryAssign_sub, perl::Canned< hash_set< Vector< QuadraticExtension< Rational > > > >, perl::Canned< const hash_set< Vector< QuadraticExtension< Rational > > > >);
   OperatorInstance4perl(BinaryAssign_sub, perl::Canned< hash_set< Vector< Rational > > >, perl::Canned< const hash_set< Vector< Rational > > >);
   Class4perl("Polymake::common::HashSet__Matrix_A_QuadraticExtension__Rational_I_NonSymmetric_Z", hash_set< Matrix< QuadraticExtension< Rational > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
