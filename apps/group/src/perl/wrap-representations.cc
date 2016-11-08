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

#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/hash_map"

namespace polymake { namespace group { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( irreducible_decomposition_T_C_x, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (irreducible_decomposition<T0>(arg0.get<T1, T0>(), arg1)) );
   };

   FunctionWrapper4perl( pm::Array<int> (pm::Array<int> const&, perl::Object) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Array< int > > >(), arg1 );
   }
   FunctionWrapperInstance4perl( pm::Array<int> (pm::Array<int> const&, perl::Object) );

   FunctionWrapper4perl( pm::SparseMatrix<pm::Rational, pm::NonSymmetric> (perl::Object, perl::Object, pm::Array<int> const&) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0, arg1, arg2.get< perl::TryCanned< const Array< int > > >() );
   }
   FunctionWrapperInstance4perl( pm::SparseMatrix<pm::Rational, pm::NonSymmetric> (perl::Object, perl::Object, pm::Array<int> const&) );

   FunctionWrapper4perl( pm::Array<int> (pm::Array<pm::Array<int>> const&, pm::Array<int> const&) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Array< Array< int > > > >(), arg1.get< perl::TryCanned< const Array< int > > >() );
   }
   FunctionWrapperInstance4perl( pm::Array<int> (pm::Array<pm::Array<int>> const&, pm::Array<int> const&) );

   FunctionWrapper4perl( pm::SparseMatrix<pm::Rational, pm::NonSymmetric> (perl::Object, perl::Object, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0, arg1, arg2 );
   }
   FunctionWrapperInstance4perl( pm::SparseMatrix<pm::Rational, pm::NonSymmetric> (perl::Object, perl::Object, int) );

   FunctionWrapper4perl( pm::IncidenceMatrix<pm::NonSymmetric> (perl::Object, perl::Object, pm::Array<pm::Set<int, pm::operations::cmp>> const&) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0, arg1, arg2.get< perl::TryCanned< const Array< Set< int > > > >() );
   }
   FunctionWrapperInstance4perl( pm::IncidenceMatrix<pm::NonSymmetric> (perl::Object, perl::Object, pm::Array<pm::Set<int, pm::operations::cmp>> const&) );

   FunctionWrapper4perl( pm::IncidenceMatrix<pm::NonSymmetric> (perl::Object, perl::Object, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0, arg1, arg2.get< perl::TryCanned< const SparseMatrix< Rational, NonSymmetric > > >() );
   }
   FunctionWrapperInstance4perl( pm::IncidenceMatrix<pm::NonSymmetric> (perl::Object, perl::Object, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&) );

   FunctionWrapper4perl( pm::Array<int> (pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&) ) {
      perl::Value arg0(stack[0]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const SparseMatrix< Rational, NonSymmetric > > >() );
   }
   FunctionWrapperInstance4perl( pm::Array<int> (pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&) );

   FunctionInstance4perl(irreducible_decomposition_T_C_x, Array< int >, perl::Canned< const Array< int > >);
   FunctionInstance4perl(irreducible_decomposition_T_C_x, Vector< Rational >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
