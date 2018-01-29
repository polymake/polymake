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
#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/Polynomial.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( convert_to_T_X, T0,T1 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (convert_to<T0>(arg0.get<T1>())) );
   };

   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const Matrix< Integer > >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::RowChain<pm::RowChain<pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> const&, pm::SingleRow<pm::Vector<pm::Rational> const&> > const&, pm::SingleRow<pm::Vector<pm::Rational> const&> > >);
   FunctionInstance4perl(convert_to_T_X, Rational, perl::Canned< const Matrix< Integer > >);
   FunctionInstance4perl(convert_to_T_X, Rational, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Integer>&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   FunctionInstance4perl(convert_to_T_X, int, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational>&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::MatrixMinor<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Array<int> const&, pm::all_selector const&> >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::MatrixMinor<pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, polymake::mlist<> > >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::IndexedSlice<pm::Vector<pm::Rational>&, pm::Nodes<pm::graph::Graph<pm::graph::Undirected> > const&, polymake::mlist<> > >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::IndexedSlice<pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> const&, pm::Series<int, true>, mlist<> > >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::QuadraticExtension<pm::Rational> > const&>, pm::Series<int, true>, mlist<> > const&, pm::Series<int, true>, mlist<> > >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, mlist<> > const&, pm::Series<int, true>, mlist<> > >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const SparseVector< Rational > >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::all_selector const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::IndexedSlice<pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::QuadraticExtension<pm::Rational>, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> const&, pm::Series<int, true>, mlist<> > >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const Vector< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::IndexedSlice<pm::Vector<pm::Rational>&, pm::Series<int, true>, mlist<> > >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const Matrix< PuiseuxFraction< Max, Rational, Rational > > >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational>&, pm::all_selector const&, pm::Array<int> const&> >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Set<int, pm::operations::cmp> const&, pm::Series<int, true> const&> >);
   FunctionInstance4perl(convert_to_T_X, double, perl::Canned< const pm::MatrixMinor<pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> const&, pm::Array<int> const&, pm::all_selector const&> >);
   FunctionInstance4perl(convert_to_T_X, Rational, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Integer> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(convert_to_T_X, QuadraticExtension< Rational >, perl::Canned< const Polynomial< Rational, int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
