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

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====

   ClassTemplate4perl("Polymake::common::Matrix");
   Class4perl("Polymake::common::Matrix_A_Float_I_NonSymmetric_Z", Matrix<double>);
   Class4perl("Polymake::common::Matrix_A_Integer_I_NonSymmetric_Z", Matrix<Integer>);
   Class4perl("Polymake::common::Matrix_A_Int_I_NonSymmetric_Z", Matrix<int>);
   Class4perl("Polymake::common::Matrix_A_Rational_I_NonSymmetric_Z", Matrix<Rational>);
   OperatorInstance4perl(convert, Matrix< Rational >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   OperatorInstance4perl(convert, Matrix< Rational >, perl::Canned< const Matrix< int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const Matrix< Rational > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Wary< Matrix< Integer > > >);
   OperatorInstance4perl(BinaryAssign_div, perl::Canned< Wary< Matrix< Integer > > >, perl::Canned< const pm::RowChain<pm::Matrix<pm::Integer> const&, pm::Matrix<pm::Integer> const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
