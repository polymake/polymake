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

#include "polymake/Graph.h"

namespace polymake { namespace graph { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( eigenvalues_laplacian_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (eigenvalues_laplacian(arg0.get<T0>())) );
   };

   template <typename T0>
   FunctionInterface4perl( laplacian_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (laplacian(arg0.get<T0>())) );
   };

   template <typename T0>
   FunctionInterface4perl( eigenvalues_laplacian_T_x, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (eigenvalues_laplacian<T0>(arg0)) );
   };

   template <typename T0>
   FunctionInterface4perl( laplacian_T_x, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (laplacian<T0>(arg0)) );
   };

   FunctionInstance4perl(eigenvalues_laplacian_T_x, Undirected);
   FunctionInstance4perl(laplacian_T_x, Undirected);
   FunctionInstance4perl(eigenvalues_laplacian_X, perl::Canned< const Graph< Undirected > >);
   FunctionInstance4perl(laplacian_X, perl::Canned< const Graph< Undirected > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
