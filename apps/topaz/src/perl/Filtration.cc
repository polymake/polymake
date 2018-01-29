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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/client.h"
#include "polymake/topaz/Filtration.h"

namespace polymake { namespace topaz { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( new_X2_X_x, T0,T1,T2 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]), arg2(stack[3]);
      WrapperReturnNew(T0, (arg0.get<T1>(), arg1.get<T2>(), arg2) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( new_x_X, T0,T1 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]);
      WrapperReturnNew(T0, (arg0, arg1.get<T1>()) );
   };

   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   ClassTemplate4perl("Polymake::topaz::Filtration");
   Class4perl("Polymake::topaz::Filtration__SparseMatrix_A_Integer_I_NonSymmetric_Z", Filtration< SparseMatrix< Integer, NonSymmetric > >);
   Class4perl("Polymake::topaz::Filtration__SparseMatrix_A_Rational_I_NonSymmetric_Z", Filtration< SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(new, Filtration< SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(new, Filtration< SparseMatrix< Integer, NonSymmetric > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Filtration< SparseMatrix< Rational, NonSymmetric > > >, perl::Canned< const Filtration< SparseMatrix< Rational, NonSymmetric > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Filtration< SparseMatrix< Integer, NonSymmetric > > >, perl::Canned< const Filtration< SparseMatrix< Integer, NonSymmetric > > >);
   FunctionInstance4perl(new_x_X, Filtration< SparseMatrix< Rational, NonSymmetric > >, perl::Canned< const Array< int > >);
   FunctionInstance4perl(new_x_X, Filtration< SparseMatrix< Integer, NonSymmetric > >, perl::Canned< const Array< int > >);
   FunctionInstance4perl(new_X2_X_x, Filtration< SparseMatrix< Rational, NonSymmetric > >, perl::Canned< Array< Cell > >, perl::Canned< const Array< SparseMatrix< Rational, NonSymmetric > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
