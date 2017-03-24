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

///==== this line controls the automatic file splitting: max.instances=20

#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Vector.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( minor_X32_X32_f37, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturnLvalue( T0, arg0.get<T0>().minor(arg1.get<T1>(), arg2.get<T2>()), arg0, arg1, arg2 );
   };

   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< Wary< Matrix< double > > >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >, perl::Enum<pm::all_selector>);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< Wary< Matrix< Rational > > >, perl::Enum<pm::all_selector>, perl::Canned< const pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> >);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >, perl::Enum<pm::all_selector>);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< Matrix< int > > >, perl::Canned< const pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> >, perl::Enum<pm::all_selector>);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const Array< int > >, perl::Enum<pm::all_selector>);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< Wary< Matrix< Integer > > >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >, perl::Enum<pm::all_selector>);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const Set< int > >, perl::Canned< const pm::Series<int, true> >);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< Matrix< Rational > > >, perl::Enum<pm::all_selector>, perl::Canned< const pm::Series<int, true> >);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> >, perl::Canned< const pm::Series<int, true> >);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< SparseMatrix< Rational, NonSymmetric > > >, perl::Canned< const Set< int > >, perl::Enum<pm::all_selector>);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const Set< int > >, perl::Enum<pm::all_selector>);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< Wary< Matrix< Integer > > >, perl::Enum<pm::all_selector>, perl::Canned< const Array< int > >);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< Wary< Matrix< Rational > > >, perl::Canned< const pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> >, perl::Enum<pm::all_selector>);
      FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> >, perl::Enum<pm::all_selector>);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< Wary< Matrix< double > > >, perl::Canned< const Set< int > >, perl::Enum<pm::all_selector>);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >, perl::Canned< const pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> >);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> > >, perl::Canned< const Set< int > >, perl::Enum<pm::all_selector>);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< Matrix< Rational > > >, perl::Enum<pm::all_selector>, perl::Canned< const pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> >);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> >, perl::Enum<pm::all_selector>);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< Wary< SparseMatrix< double, NonSymmetric > > >, perl::Canned< const Set< int > >, perl::Enum<pm::all_selector>);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< Wary< Matrix< Integer > > >, perl::Enum<pm::all_selector>, perl::Canned< const pm::Series<int, true> >);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> const&> > >, perl::Canned< const Set< int > >, perl::Enum<pm::all_selector>);
   ///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
