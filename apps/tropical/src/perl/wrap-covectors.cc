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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"

namespace polymake { namespace tropical { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2, typename T3>
   FunctionInterface4perl( generalized_apex_covector_T_X_X, T0,T1,T2,T3 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (generalized_apex_covector<T0,T1>(arg0.get<T2>(), arg1.get<T3>())) );
   };

   template <typename T0, typename T1, typename T2, typename T3>
   FunctionInterface4perl( coarse_covectors_of_scalar_vertices_T_X_X, T0,T1,T2,T3 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (coarse_covectors_of_scalar_vertices<T0,T1>(arg0.get<T2>(), arg1.get<T3>())) );
   };

   template <typename T0, typename T1, typename T2, typename T3>
   FunctionInterface4perl( covectors_of_scalar_vertices_T_X_X, T0,T1,T2,T3 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (covectors_of_scalar_vertices<T0,T1>(arg0.get<T2>(), arg1.get<T3>())) );
   };

   FunctionInstance4perl(covectors_of_scalar_vertices_T_X_X, Min, Rational, perl::Canned< const Matrix< Rational > >, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(covectors_of_scalar_vertices_T_X_X, Max, Rational, perl::Canned< const Matrix< Rational > >, perl::Canned< const Matrix< TropicalNumber< Max, Rational > > >);
   FunctionInstance4perl(coarse_covectors_of_scalar_vertices_T_X_X, Min, Rational, perl::Canned< const Matrix< Rational > >, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(coarse_covectors_of_scalar_vertices_T_X_X, Max, Rational, perl::Canned< const Matrix< Rational > >, perl::Canned< const Matrix< TropicalNumber< Max, Rational > > >);
   FunctionInstance4perl(generalized_apex_covector_T_X_X, Min, Rational, perl::Canned< const Vector< TropicalNumber< Min, Rational > > >, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(generalized_apex_covector_T_X_X, Max, Rational, perl::Canned< const Vector< TropicalNumber< Max, Rational > > >, perl::Canned< const Matrix< TropicalNumber< Max, Rational > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
