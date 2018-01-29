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

#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/IndexedSubgraph.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
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
   template <typename T0>
   FunctionInterface4perl( dim_f1, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( arg0.get<T0>().dim() );
   };

   FunctionInstance4perl(dim_f1, perl::Canned< const SparseVector< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const Graph< UndirectedMulti > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Undirected> const&, pm::Set<int, pm::operations::cmp> const&, mlist<> > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::IndexedSubgraph<pm::graph::Graph<pm::graph::Undirected> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, mlist<> > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const SparseVector< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const Vector< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const Vector< int > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double>&>, pm::Series<int, true>, mlist<> > >);
   FunctionInstance4perl(dim_f1, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::QuadraticExtension<pm::Rational> >&>, pm::Series<int, true>, mlist<> >, pm::Series<int, true> const&, mlist<> > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
