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
#include "polymake/PuiseuxFraction.h"
#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   template <typename T0>
   FunctionInterface4perl( jarvis_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( jarvis(arg0.get<T0>()) );
   };

   FunctionWrapper4perl( pm::ListMatrix<pm::Vector<pm::Rational> > (pm::Matrix<pm::Rational> const&) ) {
      perl::Value arg0(stack[0]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Matrix< Rational > > >() );
   }
   FunctionWrapperInstance4perl( pm::ListMatrix<pm::Vector<pm::Rational> > (pm::Matrix<pm::Rational> const&) );

   FunctionInstance4perl(jarvis_X, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(jarvis_X, perl::Canned< const Matrix< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(new_X, Matrix< PuiseuxFraction< Min, Rational, Rational > >, perl::Canned< const pm::ListMatrix<pm::Vector<pm::PuiseuxFraction<pm::Min, pm::Rational, pm::Rational> > > >);
   FunctionInstance4perl(jarvis_X, perl::Canned< const Matrix< double > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
