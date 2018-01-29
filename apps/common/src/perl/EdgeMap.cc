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
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   ClassTemplate4perl("Polymake::common::EdgeMap");
   Class4perl("Polymake::common::EdgeMap_A_Undirected_I_Float_Z", EdgeMap< Undirected, double >);
   Class4perl("Polymake::common::EdgeMap_A_Directed_I_Vector__Rational_Z", EdgeMap< Directed, Vector< Rational > >);
   Class4perl("Polymake::common::EdgeMap_A_Undirected_I_Int_Z", EdgeMap< Undirected, int >);
   FunctionInstance4perl(new_X, EdgeMap< Undirected, double >, perl::Canned< const Graph< Undirected > >);
   Class4perl("Polymake::common::EdgeMap_A_Undirected_I_Vector__Rational_Z", EdgeMap< Undirected, Vector< Rational > >);
   FunctionInstance4perl(new_X, EdgeMap< Directed, Vector< Rational > >, perl::Canned< const Graph< Directed > >);
   FunctionInstance4perl(new_X, EdgeMap< Undirected, Vector< Rational > >, perl::Canned< const Graph< Undirected > >);
   FunctionInstance4perl(new_X, EdgeMap< Undirected, int >, perl::Canned< const Graph< Undirected > >);
   Class4perl("Polymake::common::EdgeMap_A_Undirected_I_Rational_Z", EdgeMap< Undirected, Rational >);
   FunctionInstance4perl(new_X, EdgeMap< Undirected, Rational >, perl::Canned< const Graph< Undirected > >);
   Class4perl("Polymake::common::EdgeMap_A_Undirected_I_Integer_Z", EdgeMap< Undirected, Integer >);
   FunctionInstance4perl(new_X, EdgeMap< Undirected, Integer >, perl::Canned< const Graph< Undirected > >);
   Class4perl("Polymake::common::EdgeMap_A_DirectedMulti_I_Int_Z", EdgeMap< DirectedMulti, int >);
   FunctionInstance4perl(new_X, EdgeMap< DirectedMulti, int >, perl::Canned< const Graph< DirectedMulti > >);
   Class4perl("Polymake::common::EdgeMap_A_UndirectedMulti_I_Int_Z", EdgeMap< UndirectedMulti, int >);
   Class4perl("Polymake::common::EdgeMap_A_Directed_I_Int_Z", EdgeMap< Directed, int >);
   FunctionInstance4perl(new_X, EdgeMap< Directed, int >, perl::Canned< const Graph< Directed > >);
   FunctionInstance4perl(new_X, EdgeMap< UndirectedMulti, int >, perl::Canned< const Graph< UndirectedMulti > >);
   Class4perl("Polymake::common::EdgeMap_A_Directed_I_Rational_Z", EdgeMap< Directed, Rational >);
   FunctionInstance4perl(new_X, EdgeMap< Directed, Rational >, perl::Canned< const Graph< Directed > >);
   Class4perl("Polymake::common::EdgeMap_A_Undirected_I_Vector__QuadraticExtension__Rational_Z", EdgeMap< Undirected, Vector< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_X, EdgeMap< Undirected, Vector< QuadraticExtension< Rational > > >, perl::Canned< const Graph< Undirected > >);
   Class4perl("Polymake::common::EdgeMap_A_Undirected_I_QuadraticExtension__Rational_Z", EdgeMap< Undirected, QuadraticExtension< Rational > >);
   FunctionInstance4perl(new_X, EdgeMap< Undirected, QuadraticExtension< Rational > >, perl::Canned< const Graph< Undirected > >);
   Class4perl("Polymake::common::EdgeMap_A_Undirected_I_Vector__Float_Z", EdgeMap< Undirected, Vector< double > >);
   Class4perl("Polymake::common::EdgeMap_A_Undirected_I_PuiseuxFraction_A_Max_I_Rational_I_Rational_Z_Z", EdgeMap< Undirected, PuiseuxFraction< Max, Rational, Rational > >);
   Class4perl("Polymake::common::EdgeMap_A_Undirected_I_PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_Z", EdgeMap< Undirected, PuiseuxFraction< Min, Rational, Rational > >);
   Class4perl("Polymake::common::EdgeMap_A_Undirected_I_Vector__PuiseuxFraction_A_Max_I_Rational_I_Rational_Z_Z", EdgeMap< Undirected, Vector< PuiseuxFraction< Max, Rational, Rational > > >);
   Class4perl("Polymake::common::EdgeMap_A_Undirected_I_Vector__PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_Z", EdgeMap< Undirected, Vector< PuiseuxFraction< Min, Rational, Rational > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
