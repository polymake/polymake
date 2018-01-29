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
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   ClassTemplate4perl("Polymake::common::NodeMap");
   Class4perl("Polymake::common::NodeMap_A_Directed_I_Set__Int_Z", NodeMap< Directed, Set< int > >);
   Class4perl("Polymake::common::NodeMap_A_Undirected_I_Vector__Rational_Z", NodeMap< Undirected, Vector< Rational > >);
   Class4perl("Polymake::common::NodeMap_A_Undirected_I_Int_Z", NodeMap< Undirected, int >);
   FunctionInstance4perl(new_X, NodeMap< Undirected, int >, perl::Canned< const Graph< Undirected > >);
   FunctionInstance4perl(new_X, NodeMap< Directed, Set< int > >, perl::Canned< const Graph< Directed > >);
   FunctionInstance4perl(new_X, NodeMap< Undirected, Vector< Rational > >, perl::Canned< const Graph< Undirected > >);
   Class4perl("Polymake::common::NodeMap_A_Undirected_I_Vector__QuadraticExtension__Rational_Z", NodeMap< Undirected, Vector< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_X, NodeMap< Undirected, Vector< QuadraticExtension< Rational > > >, perl::Canned< const Graph< Undirected > >);
   Class4perl("Polymake::common::NodeMap_A_Directed_I_IncidenceMatrix__NonSymmetric_Z", NodeMap< Directed, IncidenceMatrix< NonSymmetric > >);
   FunctionInstance4perl(new_X, NodeMap< Directed, IncidenceMatrix< NonSymmetric > >, perl::Canned< const Graph< Directed > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
