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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Vector.h"
#include "polymake/graph/Decoration.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( separable_T_x_X_o, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (separable<T0>(arg0, arg1.get<T1>(), arg2)) );
   };

   template <typename T0>
   FunctionInterface4perl( separating_hyperplane_T_x_x_o, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (separating_hyperplane<T0>(arg0, arg1, arg2)) );
   };

   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( separating_hyperplane_T_X_X, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (separating_hyperplane<T0>(arg0.get<T1>(), arg1.get<T2>())) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( cone_contains_point_T_x_X_o, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (cone_contains_point<T0>(arg0, arg1.get<T1>(), arg2)) );
   };

   FunctionInstance4perl(cone_contains_point_T_x_X_o, Rational, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSetCmp<int, pm::operations::cmp>, pm::Rational> >);
   FunctionInstance4perl(separating_hyperplane_T_X_X, Rational, perl::Canned< const Vector< Rational > >, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(separable_T_x_X_o, Rational, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(separating_hyperplane_T_x_x_o, Rational);
   FunctionInstance4perl(separating_hyperplane_T_X_X, QuadraticExtension< Rational >, perl::Canned< const Vector< QuadraticExtension< Rational > > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(separating_hyperplane_T_x_x_o, QuadraticExtension< Rational >);
   FunctionInstance4perl(cone_contains_point_T_x_X_o, Rational, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(cone_contains_point_T_x_X_o, QuadraticExtension< Rational >, perl::Canned< const Vector< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(cone_contains_point_T_x_X_o, QuadraticExtension< Rational >, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSetCmp<int, pm::operations::cmp>, pm::QuadraticExtension<pm::Rational> > >);
   FunctionInstance4perl(separable_T_x_X_o, Rational, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, mlist<> > >);
   FunctionInstance4perl(separating_hyperplane_T_X_X, Rational, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, mlist<> > >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Series<int, true> const&, pm::all_selector const&> >);
   FunctionInstance4perl(cone_contains_point_T_x_X_o, Rational, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, mlist<> > >);
   FunctionInstance4perl(cone_contains_point_T_x_X_o, Rational, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(separating_hyperplane_T_X_X, Rational, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, mlist<> > >, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(separable_T_x_X_o, Rational, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, mlist<> > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
