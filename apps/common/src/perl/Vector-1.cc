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

#include "polymake/client.h"
#include "polymake/Vector.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/SparseVector.h"
#include "polymake/Set.h"
#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new_int, T0 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<int>()) );
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

   ClassTemplate4perl("Polymake::common::Vector");
   Class4perl("Polymake::common::Vector__Integer", Vector< Integer >);
   Class4perl("Polymake::common::Vector__Rational", Vector< Rational >);
   Class4perl("Polymake::common::Vector__Float", Vector< double >);
   Class4perl("Polymake::common::Vector__Int", Vector< int >);
   FunctionInstance4perl(new, Vector< double >);
   FunctionInstance4perl(new, Vector< Rational >);
   FunctionInstance4perl(new_X, Vector< double >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(new_X, Vector< int >, perl::Canned< const pm::SameElementSparseVector<pm::Set<int, pm::operations::cmp> const&, int> >);
   FunctionInstance4perl(new, Vector< Integer >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(new_int, Vector< Rational >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>>, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(new, Vector< int >);
   OperatorInstance4perl(assign, Vector< Rational >, perl::Canned< const SparseVector< Rational > >);
   FunctionInstance4perl(new_int, Vector< Integer >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer>&>, pm::Series<int, true>>, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>> >);
   FunctionInstance4perl(new_X, Vector< Integer >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>> >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer>&>, pm::Series<int, true>>, perl::Canned< const Vector< Integer > >);
   FunctionInstance4perl(new_X, Vector< Integer >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const Vector< Integer > >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>>, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> >);
   OperatorInstance4perl(convert, Vector< double >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< Rational > > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< int > > >, perl::Canned< const Vector< int > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< Integer > > >, perl::Canned< const Vector< Integer > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Vector< Rational > > >, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Rational>, pm::Vector<pm::Rational> const&> >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< Integer > > >, perl::Canned< const Vector< int > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< Rational > > >, perl::Canned< const SparseVector< Rational > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double>&>, pm::Series<int, true>>, perl::Canned< const Vector< double > >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>>, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> >);
   OperatorInstance4perl(assign, Vector< double >, perl::Canned< const pm::IndexedSlice<pm::Vector<pm::Rational>&, pm::Nodes<pm::graph::Graph<pm::graph::Undirected> > const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
