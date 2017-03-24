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
#include "polymake/Matrix.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/TropicalNumber.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( centroid_volume_x_X_X_f16, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturnVoid( (centroid_volume(arg0, arg1.get<T0>(), arg2.get<T1>())) );
   };

   FunctionInstance4perl(centroid_volume_x_X_X_f16, perl::Canned< const Matrix< Rational > >, perl::Canned< const Array< Set< int > > >);
   FunctionInstance4perl(centroid_volume_x_X_X_f16, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >, perl::Canned< const Array< Set< int > > >);
   FunctionInstance4perl(centroid_volume_x_X_X_f16, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >, perl::Canned< const Array< Set< int > > >);
   FunctionInstance4perl(centroid_volume_x_X_X_f16, perl::Canned< const Matrix< QuadraticExtension< Rational > > >, perl::Canned< const Array< Set< int > > >);
   FunctionInstance4perl(centroid_volume_x_X_X_f16, perl::Canned< const Matrix< PuiseuxFraction< Min, Rational, Rational > > >, perl::Canned< const Array< Set< int > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
