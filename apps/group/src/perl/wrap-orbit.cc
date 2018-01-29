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

#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Polynomial.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/group/orbit.h"

namespace polymake { namespace group { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( action_T_X_X, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (action<T0>(arg0.get<T1>(), arg1.get<T2>())) );
   };

   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( action_inv_T_X_X, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (action_inv<T0>(arg0.get<T1>(), arg1.get<T2>())) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( orbit_representatives_T_X, T0,T1 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (orbit_representatives<T0>(arg0.get<T1>())) );
   };

   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( orbit_T_X_X, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (orbit<T0>(arg0.get<T1>(), arg1.get<T2>())) );
   };

   FunctionInstance4perl(orbit_representatives_T_X, Array< int >, perl::Canned< const Array< Array< int > > >);
   FunctionInstance4perl(orbit_T_X_X, on_elements, perl::Canned< const Array< Matrix< Rational > > >, perl::Canned< const SparseVector< Rational > >);
   FunctionInstance4perl(orbit_T_X_X, on_elements, perl::Canned< const Array< Matrix< Rational > > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(orbit_T_X_X, on_container, perl::Canned< const Array< Array< int > > >, perl::Canned< const Set< int > >);
   FunctionInstance4perl(orbit_T_X_X, on_elements, perl::Canned< const Array< Array< int > > >, perl::Canned< const Set< Set< int > > >);
   FunctionInstance4perl(orbit_T_X_X, on_cols, perl::Canned< const Array< Array< int > > >, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(orbit_T_X_X, on_rows, perl::Canned< const Array< Array< int > > >, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(orbit_T_X_X, on_elements, perl::Canned< const Array< Array< int > > >, perl::Canned< const std::pair< Set< int >, Set< Set< int > > > >);
   FunctionInstance4perl(orbit_T_X_X, on_container, perl::Canned< const Array< Array< int > > >, perl::Canned< const Array< int > >);
   FunctionInstance4perl(orbit_T_X_X, on_elements, perl::Canned< const Array< Array< int > > >, perl::Canned< const Array< int > >);
   FunctionInstance4perl(orbit_T_X_X, on_container, perl::Canned< const Array< Array< int > > >, perl::Canned< const Vector< int > >);
   FunctionInstance4perl(orbit_T_X_X, on_elements, perl::Canned< const Array< Array< int > > >, perl::Canned< const Vector< int > >);
   FunctionInstance4perl(orbit_T_X_X, on_elements, perl::Canned< const Array< Array< int > > >, perl::Canned< const Matrix< int > >);
   FunctionInstance4perl(orbit_T_X_X, on_container, perl::Canned< const Array< Array< int > > >, perl::Canned< const Polynomial< Rational, int > >);
   FunctionInstance4perl(action_T_X_X, on_container, perl::Canned< const Array< int > >, perl::Canned< const Array< int > >);
   FunctionInstance4perl(action_inv_T_X_X, on_container, perl::Canned< const Array< int > >, perl::Canned< const Array< int > >);
   FunctionInstance4perl(orbit_T_X_X, on_nonhomog_container, perl::Canned< const Array< Array< int > > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(orbit_T_X_X, on_container, perl::Canned< const Array< Array< int > > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(orbit_T_X_X, Rational, perl::Canned< const Array< Matrix< Rational > > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(orbit_T_X_X, QuadraticExtension< Rational >, perl::Canned< const Array< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const Vector< QuadraticExtension< Rational > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
