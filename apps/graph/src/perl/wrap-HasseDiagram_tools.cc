/* Copyright (c) 1997-2014
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

namespace polymake { namespace graph { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( permuted_atoms_x_X, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( permuted_atoms(arg0, arg1.get<T0>()) );
   };

   template <typename T0>
   FunctionInterface4perl( permuted_coatoms_x_X, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( permuted_coatoms(arg0, arg1.get<T0>()) );
   };

   FunctionInstance4perl(permuted_coatoms_x_X, perl::TryCanned< const Array< int > >);
   FunctionInstance4perl(permuted_atoms_x_X, perl::TryCanned< const Array< int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
