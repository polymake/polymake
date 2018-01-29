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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"

namespace polymake { namespace group { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( orbits_of_coordinate_action_T_x_X, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (orbits_of_coordinate_action<T0>(arg0, arg1.get<T1>())) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( are_in_same_orbit_x_X_X, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (are_in_same_orbit(arg0, arg1.get<T0>(), arg2.get<T1>())) );
   };

   template <typename T0>
   FunctionInterface4perl( stabilizer_of_vector_x_X, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (stabilizer_of_vector(arg0, arg1.get<T0>())) );
   };

   FunctionWrapper4perl( pm::Array<pm::Set<int, pm::operations::cmp>> (perl::Object) ) {
      perl::Value arg0(stack[0]);
      IndirectWrapperReturn( arg0 );
   }
   FunctionWrapperInstance4perl( pm::Array<pm::Set<int, pm::operations::cmp>> (perl::Object) );

   FunctionInstance4perl(are_in_same_orbit_x_X_X, perl::Canned< const Vector< int > >, perl::Canned< const Vector< int > >);
   FunctionWrapper4perl( perl::Object (pm::Array<std::string> const&, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Array< std::string > > >(), arg1 );
   }
   FunctionWrapperInstance4perl( perl::Object (pm::Array<std::string> const&, int) );

   FunctionWrapper4perl( std::string (perl::Object) ) {
      perl::Value arg0(stack[0]);
      IndirectWrapperReturn( arg0 );
   }
   FunctionWrapperInstance4perl( std::string (perl::Object) );

   FunctionWrapper4perl( perl::Object (perl::Object, pm::Set<int, pm::operations::cmp> const&) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0, arg1.get< perl::TryCanned< const Set< int > > >() );
   }
   FunctionWrapperInstance4perl( perl::Object (perl::Object, pm::Set<int, pm::operations::cmp> const&) );

   FunctionInstance4perl(stabilizer_of_vector_x_X, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(stabilizer_of_vector_x_X, perl::Canned< const Vector< int > >);
   FunctionWrapper4perl( void (pm::Array<pm::Array<int>> const&, perl::Object, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturnVoid( arg0.get< perl::TryCanned< const Array< Array< int > > > >(), arg1, arg2 );
   }
   FunctionWrapperInstance4perl( void (pm::Array<pm::Array<int>> const&, perl::Object, perl::OptionSet) );

   FunctionWrapper4perl( pm::Array<pm::hash_set<int>> (perl::Object) ) {
      perl::Value arg0(stack[0]);
      IndirectWrapperReturn( arg0 );
   }
   FunctionWrapperInstance4perl( pm::Array<pm::hash_set<int>> (perl::Object) );

   FunctionInstance4perl(orbits_of_coordinate_action_T_x_X, Integer, perl::Canned< const Matrix< Integer > >);
   FunctionWrapper4perl( pm::Array<pm::Array<int>> (perl::Object) ) {
      perl::Value arg0(stack[0]);
      IndirectWrapperReturn( arg0 );
   }
   FunctionWrapperInstance4perl( pm::Array<pm::Array<int>> (perl::Object) );

///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
