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
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/TropicalNumber.h"
#include "polymake/client.h"
#include "polymake/linalg.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( minor_X32_X32_f37, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturnLvalue( T0, arg0.get<T0>().minor(arg1.get<T1>(), arg2.get<T2>()), arg0, arg1, arg2 );
   };

   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< pm::DiagMatrix<pm::SameElementVector<pm::PuiseuxFraction<pm::Min, pm::Rational, pm::Rational> const&>, true> > >, perl::Canned< const pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> >, perl::Enum<pm::all_selector>);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Enum<pm::all_selector>, perl::Canned< const pm::Series<int, true> >);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const Array< int > >, perl::Enum<pm::all_selector>);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Enum<pm::all_selector>, perl::Canned< const Set< int > >);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< pm::RowChain<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> > >, perl::Canned< const Set< int > >, perl::Enum<pm::all_selector>);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< const Wary< pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> > >, perl::Canned< const Array< int > >, perl::Enum<pm::all_selector>);
   FunctionInstance4perl(minor_X32_X32_f37, perl::Canned< Wary< Matrix< QuadraticExtension< Rational > > > >, perl::Enum<pm::all_selector>, perl::Canned< const pm::Series<int, true> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
