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
#include "polymake/PowerSet.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/hash_set"
#include "polymake/list"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( size_f1, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( arg0.get<T0>().size() );
   };

   FunctionInstance4perl(size_f1, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<int, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >);
   FunctionInstance4perl(size_f1, perl::Canned< const Set< int > >);
   FunctionInstance4perl(size_f1, perl::Canned< const PowerSet< int > >);
   FunctionInstance4perl(size_f1, perl::Canned< const Set< Set< int > > >);
   FunctionInstance4perl(size_f1, perl::Canned< const Set< Vector< Rational > > >);
   FunctionInstance4perl(size_f1, perl::Canned< const Set< Vector< QuadraticExtension< Rational > > > >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, false, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&> >);
   FunctionInstance4perl(size_f1, perl::Canned< const Set< std::pair< Set< int >, Set< int > > > >);
   FunctionInstance4perl(size_f1, perl::Canned< const Set< std::string > >);
   FunctionInstance4perl(size_f1, perl::Canned< const Set< Array< int > > >);
   FunctionInstance4perl(size_f1, perl::Canned< const hash_set< Vector< Rational > > >);
   FunctionInstance4perl(size_f1, perl::Canned< const Set< Vector< Integer > > >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::Rows<pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::Undirected>, false> > >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::Rows<pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::Directed>, false> > >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::graph::multi_adjacency_line<pm::AVL::tree<pm::sparse2d::traits<pm::graph::traits_base<pm::graph::DirectedMulti, true, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > > >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::Rows<pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::DirectedMulti>, true> > >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::Rows<pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::UndirectedMulti>, true> > >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::graph::multi_adjacency_line<pm::AVL::tree<pm::sparse2d::traits<pm::graph::traits_base<pm::graph::UndirectedMulti, false, (pm::sparse2d::restriction_kind)0>, true, (pm::sparse2d::restriction_kind)0> > > >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Integer, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> >);
   FunctionInstance4perl(size_f1, perl::Canned< const SparseVector< int > >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::QuadraticExtension<pm::Rational>, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(size_f1, perl::Canned< const SparseVector< Rational > >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<int, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> >);
   FunctionInstance4perl(size_f1, perl::Canned< const SparseVector< double > >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<double, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(size_f1, perl::Canned< const SparseVector< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(size_f1, perl::Canned< const SparseVector< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::PuiseuxFraction<pm::Max, pm::Rational, pm::Rational>, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::QuadraticExtension<pm::Rational>, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> >);
   FunctionInstance4perl(size_f1, perl::Canned< const SparseVector< Integer > >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Integer, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::TropicalNumber<pm::Max, pm::Rational>, false, true, (pm::sparse2d::restriction_kind)0>, true, (pm::sparse2d::restriction_kind)0> >&, pm::Symmetric> >);
   FunctionInstance4perl(size_f1, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::TropicalNumber<pm::Min, pm::Rational>, false, true, (pm::sparse2d::restriction_kind)0>, true, (pm::sparse2d::restriction_kind)0> >&, pm::Symmetric> >);
   FunctionInstance4perl(size_f1, perl::Canned< const SparseVector< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(size_f1, perl::Canned< const Set< Matrix< QuadraticExtension< Rational > > > >);
   FunctionInstance4perl(size_f1, perl::Canned< const hash_set< Set< int > > >);
   FunctionInstance4perl(size_f1, perl::Canned< const Set< Matrix< Rational > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
