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
#include "polymake/Matrix.h"
#include "polymake/PowerSet.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/Vector.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Set< int > >, perl::Canned< const Set< int > >);
   Class4perl("Polymake::common::Set__SparseVector__Rational", Set< SparseVector< Rational > >);
   FunctionInstance4perl(new, Set< SparseVector< Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Set< SparseVector< Rational > > >, perl::Canned< const Set< SparseVector< Rational > > >);
   Class4perl("Polymake::common::Set__Matrix_A_Rational_I_NonSymmetric_Z", Set< Matrix< Rational > >);
   FunctionInstance4perl(new, Set< Matrix< Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Set< Matrix< Rational > > >, perl::Canned< const Set< Matrix< Rational > > >);
   Class4perl("Polymake::common::Set__Pair_A_Set__Int_I_Set__Set__Int_Z", Set< std::pair< Set< int >, Set< Set< int > > > >);
   FunctionInstance4perl(new, Set< std::pair< Set< int >, Set< Set< int > > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Set< std::pair< Set< int >, Set< Set< int > > > > >, perl::Canned< const Set< std::pair< Set< int >, Set< Set< int > > > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
