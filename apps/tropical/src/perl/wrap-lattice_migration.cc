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

namespace polymake { namespace tropical { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   FunctionWrapper4perl( pm::graph::NodeMap<pm::graph::Directed, pm::IncidenceMatrix<pm::NonSymmetric> > (pm::graph::Graph<pm::graph::Directed> const&, pm::graph::NodeMap<pm::graph::Directed, tropical::CovectorDecoration> const&) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Graph< Directed > > >(), arg1.get< perl::TryCanned< const NodeMap< Directed, CovectorDecoration > > >() );
   }
   FunctionWrapperInstance4perl( pm::graph::NodeMap<pm::graph::Directed, pm::IncidenceMatrix<pm::NonSymmetric> > (pm::graph::Graph<pm::graph::Directed> const&, pm::graph::NodeMap<pm::graph::Directed, tropical::CovectorDecoration> const&) );

///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
