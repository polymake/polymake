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

#include "polymake/Array.h"
#include "polymake/Set.h"

namespace polymake { namespace topaz { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( vertex_graph_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( vertex_graph(arg0.get<T0>()) );
   };

   FunctionWrapper4perl( pm::graph::Graph<pm::graph::Undirected> (pm::FacetList const&) ) {
      perl::Value arg0(stack[0]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const FacetList > >() );
   }
   FunctionWrapperInstance4perl( pm::graph::Graph<pm::graph::Undirected> (pm::FacetList const&) );

   FunctionInstance4perl(vertex_graph_X, perl::Canned< const Array< Set< int > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
