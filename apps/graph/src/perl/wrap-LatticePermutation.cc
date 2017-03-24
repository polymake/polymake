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
#include "polymake/graph/Decoration.h"

namespace polymake { namespace graph { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2, typename T3>
   FunctionInterface4perl( find_lattice_permutation_T_x_x_C, T0,T1,T2,T3 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (find_lattice_permutation<T0,T1,T2>(arg0, arg1, arg2.get<T3, T2>())) );
   };

   FunctionInstance4perl(find_lattice_permutation_T_x_x_C, graph::lattice::BasicDecoration, graph::lattice::Sequential, Array< int >, perl::Canned< const Array< int > >);
   FunctionInstance4perl(find_lattice_permutation_T_x_x_C, graph::lattice::BasicDecoration, graph::lattice::Nonsequential, Array< int >, perl::Canned< const Array< int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
