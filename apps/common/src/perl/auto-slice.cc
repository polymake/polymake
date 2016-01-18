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
#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Vector.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( slice_X8_f5, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturnLvalueAnch( 2, (arg0)(arg1), T0, arg0.get<T0>().slice(arg1.get<T1>()) );
   };

   template <typename T0>
   FunctionInterface4perl( slice_x_x_f5, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturnLvalueAnch( 1, (arg0), T0, arg0.get<T0>().slice(arg1, arg2) );
   };

   FunctionInstance4perl(slice_X8_f5, perl::Canned< Wary< Vector< Rational > > >, perl::Canned< const pm::Nodes<pm::graph::Graph<pm::graph::Undirected> > >);
   FunctionInstance4perl(slice_X8_f5, perl::Canned< const Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double> const&>, pm::Series<int, true>, void> > >, int);
   FunctionInstance4perl(slice_X8_f5, perl::Canned< const Wary< Vector< Rational > > >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >);
   FunctionInstance4perl(slice_X8_f5, perl::Canned< const Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, void> > >, int);
   FunctionInstance4perl(slice_X8_f5, perl::Canned< const Wary< Vector< Rational > > >, int);
   FunctionInstance4perl(slice_x_x_f5, perl::Canned< Wary< Vector< double > > >);
   FunctionInstance4perl(slice_X8_f5, perl::Canned< Wary< Vector< double > > >, int);
   FunctionInstance4perl(slice_x_x_f5, perl::Canned< Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double>&>, pm::Series<int, true>, void> > >);
   FunctionInstance4perl(slice_X8_f5, perl::Canned< const Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>, void> > >, int);
   FunctionInstance4perl(slice_X8_f5, perl::Canned< Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer>&>, pm::Series<int, true>, void> > >, int);
   FunctionInstance4perl(slice_x_x_f5, perl::Canned< const Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, void> > >);
   FunctionInstance4perl(slice_X8_f5, perl::Canned< Wary< Vector< Rational > > >, int);
   FunctionInstance4perl(slice_X8_f5, perl::Canned< const Wary< pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> > >, int);
   FunctionInstance4perl(slice_x_x_f5, perl::Canned< const Wary< Vector< Integer > > >);
   FunctionInstance4perl(slice_x_x_f5, perl::Canned< const Wary< Vector< Rational > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
