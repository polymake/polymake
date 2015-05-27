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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Polynomial.h"
#include "polymake/Rational.h"
#include "polymake/Ring.h"
#include "polymake/SparseMatrix.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( new_X_X, T0,T1,T2 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]);
      WrapperReturnNew(T0, (arg0.get<T1>(), arg1.get<T2>()) );
   };

   ClassTemplate4perl("Polymake::common::Monomial");
   Class4perl("Polymake::common::Monomial_A_Rational_I_Int_Z", Monomial< Rational, int >);
   OperatorInstance4perl(Binary_xor, perl::Canned< const Monomial< Rational, int > >, int);
   OperatorInstance4perl(Binary_add, perl::Canned< const Monomial< Rational, int > >, perl::Canned< const Monomial< Rational, int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Monomial< Rational, int > >, perl::Canned< const Monomial< Rational, int > >);
   OperatorInstance4perl(BinaryAssign_mul, perl::Canned< Monomial< Rational, int > >, perl::Canned< const Monomial< Rational, int > >);
   FunctionInstance4perl(new_X_X, Monomial< Rational, int >, perl::Canned< const pm::VectorChain<pm::VectorChain<pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<int, false, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&, pm::NonSymmetric> const&, pm::Vector<int> const&> const&, pm::Vector<int> const&> >, perl::Canned< const Ring< Rational, int > >);
   FunctionInstance4perl(new_X_X, Monomial< Rational, int >, perl::Canned< const Vector< int > >, perl::Canned< const Ring< Rational, int > >);
   OperatorInstance4perl(Binary_mul, int, perl::Canned< const Monomial< Rational, int > >);
   Class4perl("Polymake::common::Monomial_A_TropicalNumber_A_Min_I_Rational_Z_I_Int_Z", Monomial< TropicalNumber< Min, Rational >, int >);
   Class4perl("Polymake::common::Monomial_A_TropicalNumber_A_Max_I_Rational_Z_I_Int_Z", Monomial< TropicalNumber< Max, Rational >, int >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Monomial< TropicalNumber< Max, Rational >, int > >, perl::Canned< const Term< TropicalNumber< Max, Rational >, int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
