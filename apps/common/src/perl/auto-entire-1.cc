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
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/linalg.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( entire_R_X32, T0 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnPkg( (entire(arg0.get<T0>())), arg0 );
   };

   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::Edges<pm::graph::Graph<pm::graph::Undirected> > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::Edges<pm::graph::Graph<pm::graph::Directed> > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::Edges<pm::graph::Graph<pm::graph::UndirectedMulti> > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::Edges<pm::graph::Graph<pm::graph::DirectedMulti> > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const NodeMap< Undirected, int > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const EdgeMap< Directed, Vector< Rational > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const NodeMap< Undirected, Vector< Rational > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const EdgeMap< Undirected, double > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const EdgeMap< Undirected, Vector< Rational > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const EdgeMap< Undirected, Vector< QuadraticExtension< Rational > > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const NodeMap< Directed, Set< int > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::graph::incident_edge_list<pm::AVL::tree<pm::sparse2d::traits<pm::graph::traits_base<pm::graph::Undirected, false, (pm::sparse2d::restriction_kind)0>, true, (pm::sparse2d::restriction_kind)0> > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const EdgeMap< Undirected, Integer > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const EdgeMap< Undirected, Rational > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const EdgeMap< Undirected, QuadraticExtension< Rational > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const Set< std::pair< Set< int >, Set< int > > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const Set< int > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::graph::incident_edge_list<pm::AVL::tree<pm::sparse2d::traits<pm::graph::traits_base<pm::graph::Directed, true, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const EdgeMap< Undirected, int > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const NodeMap< Directed, IncidenceMatrix< NonSymmetric > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const SparseVector< int > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<int, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::QuadraticExtension<pm::Rational>, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::Nodes<pm::graph::Graph<pm::graph::Directed> > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const SparseVector< Rational > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const SparseVector< double > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<double, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<int, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const SparseVector< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::Nodes<pm::graph::Graph<pm::graph::Undirected> > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::QuadraticExtension<pm::Rational>, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const SparseVector< Integer > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Integer, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Integer, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::TropicalNumber<pm::Min, pm::Rational>, false, true, (pm::sparse2d::restriction_kind)0>, true, (pm::sparse2d::restriction_kind)0> >&, pm::Symmetric> >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const SparseVector< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(entire_R_X32, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::TropicalNumber<pm::Max, pm::Rational>, false, true, (pm::sparse2d::restriction_kind)0>, true, (pm::sparse2d::restriction_kind)0> >&, pm::Symmetric> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
