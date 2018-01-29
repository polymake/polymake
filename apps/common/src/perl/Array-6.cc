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
#include "polymake/Bitset.h"
#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/Polynomial.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/hash_map"
#include "polymake/hash_set"
#include "polymake/list"

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

   OperatorInstance4perl(assign, Array< Array< Array< int > > >, perl::Canned< const Array< Set< Array< int > > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< hash_map< Bitset, Rational > > >, perl::Canned< const Array< hash_map< Bitset, Rational > > >);
   FunctionInstance4perl(new_X, Array< Set< int > >, perl::Canned< const Array< hash_set< int > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< Matrix< Rational > > >, perl::Canned< const Array< Matrix< Rational > > >);
   FunctionInstance4perl(new_X, Array< Matrix< Rational > >, perl::Canned< const Array< Matrix< Rational > > >);
   FunctionInstance4perl(new, Array< Matrix< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const Array< Matrix< QuadraticExtension< Rational > > > >);
   Class4perl("Polymake::common::Array__Array__Matrix_A_QuadraticExtension__Rational_I_NonSymmetric_Z", Array< Array< Matrix< QuadraticExtension< Rational > > > >);
   FunctionInstance4perl(new_X, Array< Set< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const Array< Set< Matrix< QuadraticExtension< Rational > > > > >);
   FunctionInstance4perl(new_X, Array< Matrix< QuadraticExtension< Rational > > >, perl::Canned< const Array< Matrix< QuadraticExtension< Rational > > > >);
   FunctionInstance4perl(new, Array< Set< Matrix< QuadraticExtension< Rational > > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< Set< Matrix< QuadraticExtension< Rational > > > > >, perl::Canned< const Array< Set< Matrix< QuadraticExtension< Rational > > > > >);
   FunctionInstance4perl(new_X, Array< QuadraticExtension< Rational > >, perl::Canned< const Array< int > >);
   FunctionInstance4perl(new_X, Array< Set< Array< int > > >, perl::Canned< const Array< Set< Array< int > > > >);
   Class4perl("Polymake::common::Array__Array__Vector__QuadraticExtension__Rational", Array< Array< Vector< QuadraticExtension< Rational > > > >);
   Class4perl("Polymake::common::Array__Array__Vector__Rational", Array< Array< Vector< Rational > > >);
   FunctionInstance4perl(new_X, Array< Matrix< Rational > >, perl::Canned< const hash_set< Matrix< Rational > > >);
   Class4perl("Polymake::common::Array__Array__Vector__Float", Array< Array< Vector< double > > >);
   Class4perl("Polymake::common::Array__Array__Vector__PuiseuxFraction_A_Min_I_Rational_I_Rational_Z", Array< Array< Vector< PuiseuxFraction< Min, Rational, Rational > > > >);
   Class4perl("Polymake::common::Array__Array__Vector__PuiseuxFraction_A_Max_I_Rational_I_Rational_Z", Array< Array< Vector< PuiseuxFraction< Max, Rational, Rational > > > >);
   FunctionInstance4perl(new_X, Array< Set< Matrix< Rational > > >, perl::Canned< const Array< Set< Matrix< Rational > > > >);
   Class4perl("Polymake::common::Array__Array__Matrix_A_Rational_I_NonSymmetric_Z", Array< Array< Matrix< Rational > > >);
   OperatorInstance4perl(convert, Array< Array< Matrix< Rational > > >, perl::Canned< const Array< Set< Matrix< Rational > > > >);
   FunctionInstance4perl(new, Array< Set< Matrix< Rational > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< Set< Matrix< Rational > > > >, perl::Canned< const Array< Set< Matrix< Rational > > > >);
   OperatorInstance4perl(convert, Array< Array< Matrix< QuadraticExtension< Rational > > > >, perl::Canned< const Array< Set< Matrix< QuadraticExtension< Rational > > > > >);
   FunctionInstance4perl(new_X, Array< QuadraticExtension< Rational > >, perl::Canned< const Array< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new_X, Array< Matrix< QuadraticExtension< Rational > > >, perl::Canned< const hash_set< Matrix< QuadraticExtension< Rational > > > >);
   FunctionInstance4perl(new_X, Array< Matrix< QuadraticExtension< Rational > > >, perl::Canned< const Set< Matrix< QuadraticExtension< Rational > > > >);
   FunctionInstance4perl(new_X, Array< Matrix< Rational > >, perl::Canned< const Set< Matrix< Rational > > >);
   FunctionInstance4perl(new_X, Array< Array< int > >, perl::Canned< const hash_set< Array< int > > >);
   OperatorInstance4perl(convert, Array< hash_set< int > >, perl::Canned< const Array< Set< int > > >);
   OperatorInstance4perl(convert, Array< Set< int > >, perl::Canned< const pm::Rows<pm::IncidenceMatrix<pm::NonSymmetric> > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
