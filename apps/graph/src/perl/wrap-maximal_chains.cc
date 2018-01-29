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

#include "polymake/graph/Decoration.h"
#include "polymake/tropical/covectors.h"

namespace polymake { namespace graph { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( lattice_of_chains_T_x, T0,T1 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (lattice_of_chains<T0,T1>(arg0)) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( maximal_chains_of_lattice_T_x_o, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (maximal_chains_of_lattice<T0,T1>(arg0, arg1)) );
   };

   FunctionInstance4perl(maximal_chains_of_lattice_T_x_o, graph::lattice::BasicDecoration, graph::lattice::Sequential);
   FunctionCrossAppInstance4perl(maximal_chains_of_lattice_T_x_o, (1, "tropical"), CovectorDecoration, graph::lattice::Nonsequential);
   FunctionInstance4perl(lattice_of_chains_T_x, graph::lattice::BasicDecoration, graph::lattice::Sequential);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
