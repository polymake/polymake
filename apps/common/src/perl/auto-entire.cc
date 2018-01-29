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
#include "polymake/IndexedSubgraph.h"
#include "polymake/Map.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/client.h"
#include "polymake/list"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( entire_R_X32, T0 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnPkg( (entire(arg0.get<T0>())), arg0 );
   };

   FunctionInstance4perl(entire_R_X32, perl::Canned< const Array< int > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const Map< int, std::list< int > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::graph::multi_adjacency_line<pm::AVL::tree<pm::sparse2d::traits<pm::graph::traits_base<pm::graph::DirectedMulti, true, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::Nodes<pm::graph::Graph<pm::graph::DirectedMulti> > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::Nodes<pm::graph::Graph<pm::graph::UndirectedMulti> > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::graph::multi_adjacency_line<pm::AVL::tree<pm::sparse2d::traits<pm::graph::traits_base<pm::graph::UndirectedMulti, false, (pm::sparse2d::restriction_kind)0>, true, (pm::sparse2d::restriction_kind)0> > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const Map< int, std::pair< int, int > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::Nodes<pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Undirected> const&, pm::Set<int, pm::operations::cmp> const&, mlist<> > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::Nodes<pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Undirected> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, mlist<> > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const SparseVector< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::PuiseuxFraction<pm::Max, pm::Rational, pm::Rational>, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::Rows<pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::Undirected>, false> > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::Rows<pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::Directed>, false> > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::Rows<pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::DirectedMulti>, true> > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::Rows<pm::AdjacencyMatrix<pm::graph::Graph<pm::graph::UndirectedMulti>, true> > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const Array< Set< int > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::graph::incident_edge_list<pm::AVL::tree<pm::sparse2d::traits<pm::graph::traits_base<pm::graph::Directed, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const EdgeMap< Directed, int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
