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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/linalg.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   template <typename T0>
   FunctionInterface4perl( new_int, T0 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<int>()) );
   };

   FunctionInstance4perl(new_int, Vector< Set< int > >);
   FunctionInstance4perl(new_X, Vector< TropicalNumber< Min, Rational > >, perl::Canned< const pm::SameElementVector<pm::TropicalNumber<pm::Min, pm::Rational> const&> >);
   FunctionInstance4perl(new_X, Vector< TropicalNumber< Min, Rational > >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::TropicalNumber<pm::Min, pm::Rational> > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Vector< Integer > > >, perl::Canned< const pm::SameElementVector<pm::Integer const&> >);
   FunctionInstance4perl(new_X, Vector< TropicalNumber< Max, Rational > >, perl::Canned< const pm::SameElementVector<pm::TropicalNumber<pm::Max, pm::Rational> const&> >);
   FunctionInstance4perl(new_X, Vector< TropicalNumber< Max, Rational > >, perl::Canned< const Vector< TropicalNumber< Max, Rational > > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Wary< Vector< Rational > > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, void> >);
   FunctionInstance4perl(new_X, Vector< int >, perl::Canned< const Vector< int > >);
   FunctionInstance4perl(new_X, Vector< Integer >, perl::Canned< const Vector< Integer > >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, false>, void> >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, false>, void> > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Vector< Rational > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(new_X, Vector< bool >, perl::Canned< const Vector< bool > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< bool > > >, perl::Canned< const Vector< bool > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
