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

#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Vector.h"
#include "polymake/hash_map"

namespace polymake { namespace group { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1, typename T2, typename T3>
   FunctionInterface4perl( induced_permutations_T_X_X_X_o, T0,T1,T2,T3 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      WrapperReturn( (induced_permutations<T0>(arg0.get<T1>(), arg1.get<T2>(), arg2.get<T3>(), arg3)) );
   };

   FunctionWrapper4perl( pm::Array<pm::Array<int>> (pm::Array<pm::Array<int>> const&, pm::IncidenceMatrix<pm::NonSymmetric> const&, pm::hash_map<pm::Set<int, pm::operations::cmp>, int> const&) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Array< Array< int > > > >(), arg1.get< perl::TryCanned< const IncidenceMatrix< NonSymmetric > > >(), arg2.get< perl::TryCanned< const hash_map< Set< int >, int > > >() );
   }
   FunctionWrapperInstance4perl( pm::Array<pm::Array<int>> (pm::Array<pm::Array<int>> const&, pm::IncidenceMatrix<pm::NonSymmetric> const&, pm::hash_map<pm::Set<int, pm::operations::cmp>, int> const&) );

   FunctionInstance4perl(induced_permutations_T_X_X_X_o, Rational, perl::Canned< const Array< Array< int > > >, perl::Canned< const Matrix< Rational > >, perl::Canned< const hash_map< Vector< Rational >, int > >);
   FunctionInstance4perl(induced_permutations_T_X_X_X_o, Rational, perl::Canned< const Array< Array< int > > >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >, perl::Canned< const hash_map< Vector< Rational >, int > >);
   FunctionWrapper4perl( pm::Array<pm::Array<int>> (pm::Array<pm::Array<int>> const&, pm::IncidenceMatrix<pm::NonSymmetric> const&, pm::hash_map<pm::Set<int, pm::operations::cmp>, int> const&, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Array< Array< int > > > >(), arg1.get< perl::TryCanned< const IncidenceMatrix< NonSymmetric > > >(), arg2.get< perl::TryCanned< const hash_map< Set< int >, int > > >(), arg3 );
   }
   FunctionWrapperInstance4perl( pm::Array<pm::Array<int>> (pm::Array<pm::Array<int>> const&, pm::IncidenceMatrix<pm::NonSymmetric> const&, pm::hash_map<pm::Set<int, pm::operations::cmp>, int> const&, perl::OptionSet) );

   FunctionInstance4perl(induced_permutations_T_X_X_X_o, Set< int >, perl::Canned< const Array< Array< int > > >, perl::Canned< const Array< Set< int > > >, perl::Canned< const hash_map< Set< int >, int > >);
   FunctionInstance4perl(induced_permutations_T_X_X_X_o, Rational, perl::Canned< const Array< Matrix< Rational > > >, perl::Canned< const Matrix< Rational > >, perl::Canned< const hash_map< Vector< Rational >, int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
