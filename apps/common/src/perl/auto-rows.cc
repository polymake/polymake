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
#include "polymake/linalg.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/Integer.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Graph.h"
#include "polymake/Vector.h"
#include "polymake/QuadraticExtension.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( rows_X8, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturnAnch( 1, (arg0), rows(arg0.get<T0>()) );
   };

   template <typename T0>
   FunctionInterface4perl( rows_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( rows(arg0.get<T0>()) );
   };

   template <typename T0>
   FunctionInterface4perl( rows_f1, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( arg0.get<T0>().rows() );
   };

   FunctionInstance4perl(rows_f1, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const IncidenceMatrix< NonSymmetric > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const Matrix< int > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const Matrix< Integer > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::Transposed<pm::Matrix<pm::Integer> > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const Matrix< double > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::Directed> > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Integer>&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const SparseMatrix< double, NonSymmetric > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const SparseMatrix< int, NonSymmetric > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const SparseMatrix< Rational, Symmetric > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const SparseMatrix< Integer, NonSymmetric > >);
   FunctionInstance4perl(rows_X, perl::Canned< const IncidenceMatrix< NonSymmetric > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::RowChain<pm::SingleRow<pm::Vector<pm::Rational> const&> const&, pm::RowChain<pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::RowChain<pm::SingleRow<pm::Vector<pm::Rational> const&> const&, pm::RowChain<pm::SingleRow<pm::Vector<pm::Rational> const&> const&, pm::RowChain<pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&> const&> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::RowChain<pm::SingleRow<pm::Vector<pm::Rational> const&> const&, pm::RowChain<pm::SingleRow<pm::Vector<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<int const&>, true> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::DirectedMulti>, true> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Integer>&, pm::all_selector const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(rows_X8, perl::Canned< const IncidenceMatrix< NonSymmetric > >);
   FunctionInstance4perl(rows_X, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::RowChain<pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::RowChain<pm::RowChain<pm::Matrix<pm::Integer> const&, pm::Matrix<pm::Integer> const&> const&, pm::Matrix<pm::Integer> const&> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational>&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
