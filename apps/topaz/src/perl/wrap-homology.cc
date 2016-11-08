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
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/SparseMatrix.h"
#include "polymake/topaz/ChainComplex.h"

namespace polymake { namespace topaz { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( homology_T_X_x_x_x, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      WrapperReturn( (homology<T0,T1>(arg0.get<T2>(), arg1, arg2, arg3)) );
   };

   FunctionWrapper4perl( pm::Array<polymake::topaz::HomologyGroup<pm::Integer>> (pm::Array<pm::Set<int, pm::operations::cmp>> const&, bool, int, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Array< Set< int > > > >(), arg1, arg2, arg3 );
   }
   FunctionWrapperInstance4perl( pm::Array<polymake::topaz::HomologyGroup<pm::Integer>> (pm::Array<pm::Set<int, pm::operations::cmp>> const&, bool, int, int) );

   FunctionWrapper4perl( pm::perl::ListReturn (pm::Array<pm::Set<int, pm::operations::cmp>> const&, bool, int, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      IndirectWrapperReturnVoid( arg0.get< perl::TryCanned< const Array< Set< int > > > >(), arg1, arg2, arg3 );
   }
   FunctionWrapperInstance4perl( pm::perl::ListReturn (pm::Array<pm::Set<int, pm::operations::cmp>> const&, bool, int, int) );

   FunctionInstance4perl(homology_T_X_x_x_x, Integer, SparseMatrix< Integer, NonSymmetric >, perl::Canned< const ChainComplex< SparseMatrix< Integer, NonSymmetric > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
