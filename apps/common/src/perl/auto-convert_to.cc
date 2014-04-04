/* Copyright (c) 1997-2014
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
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/Integer.h"
#include "polymake/Vector.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Array.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/SparseVector.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( convert_to_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturn( convert_to<T0>(arg0.get<T1>()) );
   };

   FunctionInstance4perl(convert_to_X, double, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(convert_to_X, double, perl::Canned< const Matrix< Integer > >);
   FunctionInstance4perl(convert_to_X, double, perl::Canned< const pm::RowChain<pm::RowChain<pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> const&, pm::SingleRow<pm::Vector<pm::Rational> const&> > const&, pm::SingleRow<pm::Vector<pm::Rational> const&> > >);
   FunctionInstance4perl(convert_to_X, double, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(convert_to_X, double, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(convert_to_X, double, perl::Canned< const pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(convert_to_X, double, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(convert_to_X, int, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(convert_to_X, Rational, perl::Canned< const Matrix< int > >);
   FunctionInstance4perl(convert_to_X, int, perl::Canned< const Matrix<Integer> >);
   FunctionInstance4perl(convert_to_X, int, perl::Canned< const Vector< Integer > >);
   FunctionInstance4perl(convert_to_X, Integer, perl::Canned< const Matrix<Rational> >);
   FunctionInstance4perl(convert_to_X, Rational, perl::Canned< const pm::ColChain<pm::MatrixMinor<pm::Matrix<pm::Integer>&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> const&, pm::SingleCol<pm::IndexedSlice<pm::Vector<pm::Integer> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, void> const&> > >);
   FunctionInstance4perl(convert_to_X, double, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Array<int, void> const&> >);
   FunctionInstance4perl(convert_to_X, double, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(convert_to_X, double, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   FunctionInstance4perl(convert_to_X, double, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, void> const&, pm::Series<int, true>, void> >);
   FunctionInstance4perl(convert_to_X, double, perl::Canned< const pm::IndexedSlice<pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> const&, pm::Series<int, true>, void> >);
   FunctionInstance4perl(convert_to_X, double, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, void> >);
   FunctionInstance4perl(convert_to_X, double, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::QuadraticExtension<pm::Rational> > const&>, pm::Series<int, true>, void> const&, pm::Series<int, true>, void> >);
   FunctionInstance4perl(convert_to_X, Rational, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>, void> >);
   FunctionInstance4perl(convert_to_X, Rational, perl::Canned< const Matrix< Integer > >);
   FunctionInstance4perl(convert_to_X, Rational, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Integer>&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> >);
   FunctionInstance4perl(convert_to_X, int, perl::Canned< const SparseMatrix< Integer, NonSymmetric > >);
   FunctionInstance4perl(convert_to_X, int, perl::Canned< const SparseVector< Integer > >);
   FunctionInstance4perl(convert_to_X, double, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Integer> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(convert_to_X, double, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational>&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
