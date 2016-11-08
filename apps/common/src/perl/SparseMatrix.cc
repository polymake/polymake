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
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/TropicalNumber.h"
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

   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::MatrixMinor<pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(new, SparseMatrix< TropicalNumber< Min, Rational >, Symmetric >);
   Class4perl("Polymake::common::SparseMatrix_A_TropicalNumber_A_Max_I_Rational_Z_I_Symmetric_Z", SparseMatrix< TropicalNumber< Max, Rational >, Symmetric >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< SparseMatrix< TropicalNumber< Min, Rational >, Symmetric > > >, perl::Canned< const SparseMatrix< TropicalNumber< Min, Rational >, Symmetric > >);
   FunctionInstance4perl(new, SparseMatrix< TropicalNumber< Max, Rational >, Symmetric >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< SparseMatrix< TropicalNumber< Max, Rational >, Symmetric > > >, perl::Canned< const SparseMatrix< TropicalNumber< Max, Rational >, Symmetric > >);
   Class4perl("Polymake::common::SparseMatrix_A_PuiseuxFraction_A_Max_I_Rational_I_Rational_Z_I_NonSymmetric_Z", SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric >);
   FunctionInstance4perl(new, SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric >);
   Class4perl("Polymake::common::SparseMatrix_A_PuiseuxFraction_A_Max_I_Rational_I_Rational_Z_I_Symmetric_Z", SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, Symmetric >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(convert, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< SparseMatrix< double, NonSymmetric > > >, perl::Canned< const SparseMatrix< double, NonSymmetric > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< SparseMatrix< Rational, NonSymmetric > > >, perl::Canned< const pm::RepeatedRow<pm::Vector<pm::Rational> const&> >);
   FunctionInstance4perl(new, SparseMatrix< Integer, NonSymmetric >);
   Class4perl("Polymake::common::SparseMatrix_A_TropicalNumber_A_Min_I_Int_Z_I_Symmetric_Z", SparseMatrix< TropicalNumber< Min, int >, Symmetric >);
   FunctionInstance4perl(new_X, SparseMatrix< double, NonSymmetric >, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Wary< SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > > >, perl::Canned< const pm::RepeatedRow<pm::Vector<pm::QuadraticExtension<pm::Rational> > const&> >);
   FunctionInstance4perl(new_X, SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric >, perl::Canned< const SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric > > >, perl::Canned< const SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
