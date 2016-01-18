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
   FunctionInterface4perl( dim_f1, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( arg0.get<T0>().dim() );
   };

   FunctionInstance4perl(dim_f1, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const Vector< double > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const Graph< Undirected > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const Graph< Directed > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, void> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const SparseVector< double > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const SparseVector< Rational > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const Vector< Integer > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const SparseVector< int > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::VectorChain<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, void> const&, pm::Vector<pm::Rational> const&> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const Graph< DirectedMulti > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::graph::multi_adjacency_line<pm::AVL::tree<pm::sparse2d::traits<pm::graph::traits_base<pm::graph::DirectedMulti, true, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const Vector< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const SparseVector< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::VectorChain<pm::SameElementVector<pm::Rational const&> const&, pm::SameElementVector<pm::Rational const&> const&> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::VectorChain<pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> const&, pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Rational> const&> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const SparseVector< Integer > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::SameElementSparseVector<pm::SingleElementSet<int>, pm::Integer> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::VectorChain<pm::VectorChain<pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<int, false, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> const&, pm::Vector<int> const&> const&, pm::Vector<int> const&> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::VectorChain<pm::Vector<int> const&, pm::Vector<int> const&> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer>&>, pm::Series<int, true>, void> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>, void> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, void>, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&, void> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, void>&, pm::Series<int, true>, void> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::VectorChain<pm::SparseVector<pm::Rational> const&, pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, void>&, pm::Series<int, true>, void> const&> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::VectorChain<pm::VectorChain<pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, void>&, pm::Series<int, true>, void> const&, pm::SparseVector<pm::Rational> const&> const&, pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, void>&, pm::Series<int, true>, void> const&> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::VectorChain<pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, void>&, pm::Series<int, true>, void> const&, pm::SparseVector<pm::Rational> const&> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, void> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Rational>, pm::SameElementVector<pm::Rational const&> const&> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Integer>, pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>, void> const&, pm::Series<int, true>, void> const&> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Integer>, pm::Vector<pm::Integer> const&> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Integer const&>, pm::Vector<pm::Integer> const&> >);
   FunctionInstance4perl(dim_f1, perl::Canned< const Vector< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const Vector< TropicalNumber< Max, Rational > > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const SparseVector< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::QuadraticExtension<pm::Rational>, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
