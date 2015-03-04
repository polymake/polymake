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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/client.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/QuadraticExtension.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( permuted_rows_X_X, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( permuted_rows(arg0.get<T0>(), arg1.get<T1>()) );
   };

   FunctionInstance4perl(permuted_rows_X_X, perl::Canned< const IncidenceMatrix< NonSymmetric > >, perl::Canned< const Array< int > >);
   FunctionInstance4perl(permuted_rows_X_X, perl::Canned< const Matrix< Rational > >, perl::Canned< const Array< int > >);
   FunctionInstance4perl(permuted_rows_X_X, perl::Canned< const Matrix< double > >, perl::Canned< const Array< int > >);
   FunctionInstance4perl(permuted_rows_X_X, perl::Canned< const SparseMatrix< int, NonSymmetric > >, perl::Canned< const Array< int > >);
   FunctionInstance4perl(permuted_rows_X_X, perl::Canned< const Matrix< QuadraticExtension< Rational > > >, perl::Canned< const Array< int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
