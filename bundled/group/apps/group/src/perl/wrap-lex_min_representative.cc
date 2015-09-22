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
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"

namespace polymake { namespace group { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( orbit_supports_x_X, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (orbit_supports(arg0, arg1.get<T0>())) );
   };

   template <typename T0>
   FunctionInterface4perl( orbit_support_sets_x_X, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (orbit_support_sets(arg0, arg1.get<T0>())) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( lex_min_representative_T_x_C, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (lex_min_representative<T0>(arg0, arg1.get<T1, T0>())) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( orbit_representatives_T_x_X, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (orbit_representatives<T0>(arg0, arg1.get<T1>())) );
   };

   FunctionInstance4perl(orbit_representatives_T_x_X, Set< int >, perl::Canned< const Array< Set< int > > >);
   FunctionInstance4perl(lex_min_representative_T_x_C, Set< int >, perl::Canned< const Set< int > >);
   FunctionInstance4perl(orbit_supports_x_X, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(orbit_support_sets_x_X, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
