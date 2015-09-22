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

///==== this line controls the automatic file splitting: max.instances=20

#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/Vector.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   FunctionInstance4perl(new_X, Matrix< PuiseuxFraction< Min, Rational, Rational > >, perl::Canned< const SparseMatrix< PuiseuxFraction< Min, Rational, Rational >, NonSymmetric > >);
   FunctionInstance4perl(new, Matrix< PuiseuxFraction< Max, Rational, Rational > >);
   OperatorInstance4perl(convert, Matrix< PuiseuxFraction< Max, Rational, Rational > >, perl::Canned< const SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric > >);
   FunctionInstance4perl(new_X, Matrix< PuiseuxFraction< Max, Rational, Rational > >, perl::Canned< const Matrix< PuiseuxFraction< Max, Rational, Rational > > >);
   Class4perl("Polymake::common::Matrix_A_PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_I_NonSymmetric_Z", Matrix< PuiseuxFraction< Min, Rational, Rational > >);
   Class4perl("Polymake::common::Matrix_A_PuiseuxFraction_A_Max_I_Rational_I_Rational_Z_I_NonSymmetric_Z", Matrix< PuiseuxFraction< Max, Rational, Rational > >);
   OperatorInstance4perl(convert, Matrix< PuiseuxFraction< Min, Rational, Rational > >, perl::Canned< const SparseMatrix< PuiseuxFraction< Min, Rational, Rational >, NonSymmetric > >);
   FunctionInstance4perl(new_X, Matrix< PuiseuxFraction< Min, Rational, Rational > >, perl::Canned< const Matrix< PuiseuxFraction< Min, Rational, Rational > > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< Matrix< PuiseuxFraction< Min, Rational, Rational > > > >, perl::Canned< const Matrix< PuiseuxFraction< Min, Rational, Rational > > >);
   Class4perl("Polymake::common::Matrix_A_PuiseuxFraction_A_Min_I_Rational_I_Int_Z_I_NonSymmetric_Z", Matrix< PuiseuxFraction< Min, Rational, int > >);
   FunctionInstance4perl(new_X, Matrix< PuiseuxFraction< Min, Rational, int > >, perl::Canned< const Matrix< PuiseuxFraction< Min, Rational, int > > >);
   OperatorInstance4perl(convert, Matrix< QuadraticExtension< Rational > >, perl::Canned< const Matrix< double > >);
   OperatorInstance4perl(convert, Matrix< double >, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   Class4perl("Polymake::common::Matrix_A_TropicalNumber_A_Max_I_Rational_Z_I_NonSymmetric_Z", Matrix< TropicalNumber< Max, Rational > >);
   FunctionInstance4perl(new, Matrix< TropicalNumber< Min, Rational > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< PuiseuxFraction< Min, Rational, Rational > > > >, perl::Canned< const SparseVector< PuiseuxFraction< Min, Rational, Rational > > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const SparseVector< Rational > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< PuiseuxFraction< Min, Rational, Rational > > > >, perl::Canned< const Vector< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(new, Matrix< PuiseuxFraction< Min, Rational, Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< PuiseuxFraction< Min, Rational, Rational > > > >, perl::Canned< const Matrix< PuiseuxFraction< Min, Rational, Rational > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
