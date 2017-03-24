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
#include "polymake/Vector.h"

namespace polymake { namespace fan { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2, typename T3>
   FunctionInterface4perl( tight_span_vertices_T_X_X_X, T0,T1,T2,T3 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (tight_span_vertices<T0>(arg0.get<T1>(), arg1.get<T2>(), arg2.get<T3>())) );
   };

   FunctionWrapper4perl( perl::Object (pm::IncidenceMatrix<pm::NonSymmetric> const&, pm::Array<pm::IncidenceMatrix<pm::NonSymmetric> > const&, int, pm::Array<int> const&, bool, bool) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]), arg4(stack[4]), arg5(stack[5]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const IncidenceMatrix< NonSymmetric > > >(), arg1.get< perl::TryCanned< const Array< IncidenceMatrix< NonSymmetric > > > >(), arg2, arg3.get< perl::TryCanned< const Array< int > > >(), arg4, arg5 );
   }
   FunctionWrapperInstance4perl( perl::Object (pm::IncidenceMatrix<pm::NonSymmetric> const&, pm::Array<pm::IncidenceMatrix<pm::NonSymmetric> > const&, int, pm::Array<int> const&, bool, bool) );

   FunctionInstance4perl(tight_span_vertices_T_X_X_X, Rational, perl::Canned< const Matrix< Rational > >, perl::Canned< const IncidenceMatrix< NonSymmetric > >, perl::Canned< const Vector< Rational > >);
   FunctionWrapper4perl( perl::Object (pm::IncidenceMatrix<pm::NonSymmetric> const&, pm::Array<pm::IncidenceMatrix<pm::NonSymmetric> > const&, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const IncidenceMatrix< NonSymmetric > > >(), arg1.get< perl::TryCanned< const Array< IncidenceMatrix< NonSymmetric > > > >(), arg2 );
   }
   FunctionWrapperInstance4perl( perl::Object (pm::IncidenceMatrix<pm::NonSymmetric> const&, pm::Array<pm::IncidenceMatrix<pm::NonSymmetric> > const&, int) );

///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
