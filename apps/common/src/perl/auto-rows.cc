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

#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/IndexedSubgraph.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/TropicalNumber.h"
#include "polymake/client.h"
#include "polymake/linalg.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( rows_R_X32, T0 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnPkg( (rows(arg0.get<T0>())), arg0 );
   };

   template <typename T0>
   FunctionInterface4perl( rows_f1, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( arg0.get<T0>().rows() );
   };

   FunctionInstance4perl(rows_f1, perl::Canned< const SparseMatrix< TropicalNumber< Min, Rational >, Symmetric > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const SparseMatrix< TropicalNumber< Max, Rational >, Symmetric > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::RowChain<pm::RowChain<pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::RowChain<pm::RowChain<pm::RowChain<pm::RowChain<pm::RowChain<pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Set<int, pm::operations::cmp> const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const Matrix< TropicalNumber< Max, Rational > > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::Undirected>, false> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const Matrix< std::pair< double, double > > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::MatrixMinor<pm::Matrix<double>&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::MatrixMinor<pm::SparseMatrix<double, pm::NonSymmetric>&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Series<int, true> const&> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::AdjacencyMatrix<pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Undirected> const&, pm::Series<int, true> const&, mlist<pm::RenumberTag<std::integral_constant<bool, true> > > >, false> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::UndirectedMulti>, true> >);
   FunctionInstance4perl(rows_f1, perl::Canned< const Matrix< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric > >);
   FunctionInstance4perl(rows_f1, perl::Canned< const Matrix< PuiseuxFraction< Max, Rational, Rational > > >);
   FunctionInstance4perl(rows_R_X32, perl::Canned< const pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::Undirected>, false> >);
   FunctionInstance4perl(rows_R_X32, perl::Canned< const pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::Directed>, false> >);
   FunctionInstance4perl(rows_R_X32, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(rows_R_X32, perl::Canned< const pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::DirectedMulti>, true> >);
   FunctionInstance4perl(rows_R_X32, perl::Canned< const pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::UndirectedMulti>, true> >);
   FunctionInstance4perl(rows_R_X32, perl::Canned< const Matrix< int > >);
   FunctionInstance4perl(rows_R_X32, perl::Canned< const IncidenceMatrix< NonSymmetric > >);
   FunctionInstance4perl(rows_R_X32, perl::Canned< const Matrix< double > >);
   FunctionInstance4perl(rows_R_X32, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Integer> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(rows_R_X32, perl::Canned< const Matrix< Integer > >);
   FunctionInstance4perl(rows_R_X32, perl::Canned< const SparseMatrix< int, NonSymmetric > >);
   FunctionInstance4perl(rows_R_X32, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::Matrix<pm::Rational> const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
