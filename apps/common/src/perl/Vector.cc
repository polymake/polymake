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
#include "polymake/PuiseuxFraction.h"
#include "polymake/Rational.h"
#include "polymake/RationalFunction.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
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

   Class4perl("Polymake::common::Vector__PuiseuxFraction_A_Max_I_Rational_I_Rational_Z", Vector< PuiseuxFraction< Max, Rational, Rational > >);
   Class4perl("Polymake::common::Vector__PuiseuxFraction_A_Min_I_Rational_I_Rational_Z", Vector< PuiseuxFraction< Min, Rational, Rational > >);
   FunctionInstance4perl(new, Vector< PuiseuxFraction< Min, Rational, Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< PuiseuxFraction< Min, Rational, Rational > > > >, perl::Canned< const Vector< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(new, Vector< PuiseuxFraction< Max, Rational, Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< PuiseuxFraction< Max, Rational, Rational > > > >, perl::Canned< const Vector< PuiseuxFraction< Max, Rational, Rational > > >);
   Class4perl("Polymake::common::Vector__UniPolynomial_A_Rational_I_Int_Z", Vector< UniPolynomial< Rational, int > >);
   FunctionInstance4perl(new_int, Vector< PuiseuxFraction< Min, Rational, Rational > >);
   FunctionInstance4perl(new_X, Vector< PuiseuxFraction< Min, Rational, Rational > >, perl::Canned< const Vector< PuiseuxFraction< Min, Rational, Rational > > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::Rational const&> >, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Vector< Rational > >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::Rational const&> >, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double> const&>, pm::Series<int, true>, mlist<> > const&, pm::Series<int, true>, mlist<> > > >, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double> const&>, pm::Series<int, true>, mlist<> > const&, pm::Series<int, true>, mlist<> > >);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const Wary< pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double> const&>, pm::Series<int, true>, mlist<> > const&, pm::Series<int, true>, mlist<> > > >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::Vector<pm::Rational>&, pm::Series<int, true>, mlist<> >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Vector< Rational > >, perl::Canned< const pm::RowChain<pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> const&, pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::IndexedSlice<pm::Vector<pm::Rational>&, pm::Series<int, true>, mlist<> > > >, perl::Canned< const pm::IndexedSlice<pm::Vector<pm::Rational>&, pm::Series<int, true>, mlist<> > >);
   OperatorInstance4perl(assign, Vector< Integer >, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>, mlist<> >, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&, mlist<> > >);
   OperatorInstance4perl(assign, Vector< Integer >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<int>&>, pm::Series<int, true>, mlist<> > >);
   OperatorInstance4perl(assign, Vector< Integer >, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<int, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::Rational const&> >, perl::Canned< const pm::RowChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> const&, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> const&> >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Vector< Rational > > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>, mlist<> > >);
   FunctionInstance4perl(new_X, Vector< Rational >, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Rational>, pm::Vector<pm::Rational> const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
