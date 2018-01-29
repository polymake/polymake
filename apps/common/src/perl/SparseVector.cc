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
#include "polymake/PuiseuxFraction.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
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

   ClassTemplate4perl("Polymake::common::SparseVector");
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< pm::SameElementSparseVector<pm::SingleElementSetCmp<int, pm::operations::cmp>, pm::Rational> > >, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Integer, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer>&>, pm::Series<int, true>, mlist<> > >);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const Wary< pm::IndexedSlice<pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> const&, pm::Series<int, true>, mlist<> > > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< SparseVector< double > > >, perl::Canned< const SparseVector< double > >);
   Class4perl("Polymake::common::SparseVector__PuiseuxFraction_A_Min_I_Rational_I_Rational_Z", SparseVector< PuiseuxFraction< Min, Rational, Rational > >);
   FunctionInstance4perl(new_X, SparseVector< PuiseuxFraction< Min, Rational, Rational > >, perl::Canned< const SparseVector< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(new, SparseVector< PuiseuxFraction< Min, Rational, Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< SparseVector< PuiseuxFraction< Min, Rational, Rational > > > >, perl::Canned< const SparseVector< PuiseuxFraction< Min, Rational, Rational > > >);
   Class4perl("Polymake::common::SparseVector__PuiseuxFraction_A_Max_I_Rational_I_Rational_Z", SparseVector< PuiseuxFraction< Max, Rational, Rational > >);
   FunctionInstance4perl(new_X, SparseVector< PuiseuxFraction< Min, Rational, Rational > >, perl::Canned< const Vector< PuiseuxFraction< Min, Rational, Rational > > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSetCmp<int, pm::operations::cmp>, pm::Rational> >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSetCmp<int, pm::operations::cmp>, pm::Rational> >);
   FunctionInstance4perl(new_X, SparseVector< QuadraticExtension< Rational > >, perl::Canned< const Vector< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_X, SparseVector< PuiseuxFraction< Max, Rational, Rational > >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSetCmp<int, pm::operations::cmp>, pm::PuiseuxFraction<pm::Max, pm::Rational, pm::Rational> > >);
   FunctionInstance4perl(new_X, SparseVector< PuiseuxFraction< Min, Rational, Rational > >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSetCmp<int, pm::operations::cmp>, pm::PuiseuxFraction<pm::Min, pm::Rational, pm::Rational> > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Wary< pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<double, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double>&>, pm::Series<int, true>, mlist<> > >);
   OperatorInstance4perl(assign, pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<double, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric>, perl::Canned< const Vector< double > >);
   FunctionInstance4perl(new_X, SparseVector< double >, perl::Canned< const SparseVector< double > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
