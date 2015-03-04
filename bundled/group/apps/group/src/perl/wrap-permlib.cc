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

#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Integer.h"

namespace polymake { namespace group { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( orbit_coord_action_x_X, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( orbit_coord_action(arg0, arg1.get<T0>()) );
   };

   template <typename T0>
   FunctionInterface4perl( cols_f1, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( arg0.get<T0>().cols() );
   };

   template <typename T0>
   FunctionInterface4perl( orbits_coord_action_complete_x_X, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( orbits_coord_action_complete(arg0, arg1.get<T0>()) );
   };

   template <typename T0=void>
   FunctionInterface4perl( all_group_elements_x ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( all_group_elements(arg0) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( are_in_same_orbit_x_X_X, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( are_in_same_orbit(arg0, arg1.get<T0>(), arg2.get<T1>()) );
   };

   template <typename T0>
   FunctionInterface4perl( stabilizer_of_vector_x_X, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( stabilizer_of_vector(arg0, arg1.get<T0>()) );
   };

   FunctionWrapper4perl( void (pm::Array<pm::Array<int, void>, void> const&, perl::Object) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturnVoid( arg0.get< perl::TryCanned< const Array< Array< int > > > >(), arg1 );
   }
   FunctionWrapperInstance4perl( void (pm::Array<pm::Array<int, void>, void> const&, perl::Object) );

   FunctionWrapper4perl( perl::Object (pm::Array<std::string, void> const&, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Array< std::string > > >(), arg1 );
   }
   FunctionWrapperInstance4perl( perl::Object (pm::Array<std::string, void> const&, int) );

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

   FunctionWrapper4perl( pm::Array<pm::Set<int, pm::operations::cmp>, void> (perl::Object, pm::IncidenceMatrix<pm::NonSymmetric> const&) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0, arg1.get< perl::TryCanned< const IncidenceMatrix< NonSymmetric > > >() );
   }
   FunctionWrapperInstance4perl( pm::Array<pm::Set<int, pm::operations::cmp>, void> (perl::Object, pm::IncidenceMatrix<pm::NonSymmetric> const&) );

   FunctionInstance4perl(orbits_coord_action_complete_x_X, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(orbit_coord_action_x_X, perl::Canned< const Matrix< Integer > >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::ListMatrix<pm::Vector<pm::Rational> > >);
   FunctionInstance4perl(all_group_elements_x);
   FunctionInstance4perl(are_in_same_orbit_x_X_X, perl::Canned< const Vector< int > >, perl::Canned< const Vector< int > >);
   FunctionInstance4perl(stabilizer_of_vector_x_X, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(stabilizer_of_vector_x_X, perl::Canned< const Vector< int > >);
   FunctionInstance4perl(orbits_coord_action_complete_x_X, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(cols_f1, perl::Canned< const pm::ListMatrix<pm::Vector<pm::QuadraticExtension<pm::Rational> > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
