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
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseVector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/linalg.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   OperatorInstance4perl(Binary__ora, perl::Canned< const pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   OperatorInstance4perl(convert, Vector< QuadraticExtension< Rational > >, perl::Canned< const SparseVector< Rational > >);
   Class4perl("Polymake::common::Vector__Bool", Vector< bool >);
   Class4perl("Polymake::common::Vector__Set__Int", Vector< Set< int > >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, void>, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   OperatorInstance4perl(assign, pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, void>, perl::Canned< const pm::VectorChain<pm::SingleElementVector<pm::Rational>, pm::SameElementVector<pm::Rational const&> const&> >);
   Class4perl("Polymake::common::Vector__TropicalNumber_A_Min_I_Rational_Z", Vector< TropicalNumber< Min, Rational > >);
   FunctionInstance4perl(new, Vector< TropicalNumber< Min, Rational > >);
   OperatorInstance4perl(BinaryAssign_sub, perl::Canned< Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, false>, void> > >, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   OperatorInstance4perl(BinaryAssign_mul, perl::Canned< Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, false>, void> > >, int);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, false>, void> > >, perl::Canned< const pm::SameElementVector<pm::Rational const&> >);
   OperatorInstance4perl(BinaryAssign_sub, perl::Canned< Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, false>, void> > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Wary< pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, false>, void> > >, perl::Canned< const Vector< Rational > >);
   OperatorInstance4perl(convert, Vector< QuadraticExtension< Rational > >, perl::Canned< const Vector< Rational > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
