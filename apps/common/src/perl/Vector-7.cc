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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/linalg.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new_int, T0 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<int>()) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   OperatorInstance4perl(convert, Vector< QuadraticExtension< Rational > >, perl::Canned< const SparseVector< Rational > >);
   Class4perl("Polymake::common::Vector__Bool", Vector< bool >);
   Class4perl("Polymake::common::Vector__Set__Int", Vector< Set< int > >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>>, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>>, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Rational>, pm::SameElementVector<pm::Rational const&> const&> >);
   Class4perl("Polymake::common::Vector__TropicalNumber_A_Min_I_Rational_Z", Vector< TropicalNumber< Min, Rational > >);
   FunctionInstance4perl(new, Vector< TropicalNumber< Min, Rational > >);
   OperatorInstance4perl(BinaryAssign_sub, perl::Canned< Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, false>> > >, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   OperatorInstance4perl(BinaryAssign_mul, perl::Canned< Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, false>> > >, int);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, false>> > >, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   OperatorInstance4perl(BinaryAssign_sub, perl::Canned< Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, false>> > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, false>> > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(convert, Vector< QuadraticExtension< Rational > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Vector< Rational > >, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   OperatorInstance4perl(Binary_div, perl::Canned< const Wary< pm::SameElementVector<pm::Rational const&> > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary__ora, int, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary__ora, int, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>> const&, pm::Series<int, true>> >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< pm::VectorChain<pm::SingleElementVector<pm::Integer>, pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>> const&, pm::Series<int, true>> const&> > >, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Integer>, pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>> const&, pm::Series<int, true>> const&> >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< Rational > > >, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Integer>, pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>> const&, pm::Series<int, true>> const&> >);
   OperatorInstance4perl(Binary__ora, int, perl::Canned< const Vector< Integer > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< pm::VectorChain<pm::SingleElementVector<pm::Integer>, pm::Vector<pm::Integer> const&> > >, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< Rational > > >, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Integer>, pm::Vector<pm::Integer> const&> >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< Integer > > >, perl::Canned< const SparseVector< Integer > >);
   FunctionInstance4perl(new_X, Vector< Integer >, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Integer>, pm::Vector<pm::Integer> const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Integer >, perl::Canned< const Vector< Integer > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< pm::VectorChain<pm::SingleElementVector<pm::Integer const&>, pm::Vector<pm::Integer> const&> > >, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Integer const&>, pm::Vector<pm::Integer> const&> >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< Rational > > >, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Integer const&>, pm::Vector<pm::Integer> const&> >);
   Class4perl("Polymake::common::Vector__TropicalNumber_A_Max_I_Rational_Z", Vector< TropicalNumber< Max, Rational > >);
   FunctionInstance4perl(new, Vector< TropicalNumber< Max, Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< TropicalNumber< Max, Rational > > > >, perl::Canned< const Vector< TropicalNumber< Max, Rational > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< TropicalNumber< Min, Rational > > > >, perl::Canned< const Vector< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(new_X, Vector< TropicalNumber< Min, Rational > >, perl::Canned< const Vector< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(new_int, Vector< TropicalNumber< Min, Rational > >);
   FunctionInstance4perl(new, Vector< bool >);
   Class4perl("Polymake::common::Vector__String", Vector< std::string >);
   FunctionInstance4perl(new_int, Vector< TropicalNumber< Max, Rational > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Vector< Rational > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const pm::VectorChain<pm::Vector<pm::Rational> const&, pm::Vector<pm::Rational> const&> >);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const Wary< Vector< Rational > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
