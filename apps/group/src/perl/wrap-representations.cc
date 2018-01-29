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
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"

namespace polymake { namespace group { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( isotypic_basis_T_x_x_int_o, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      WrapperReturn( (isotypic_basis<T0>(arg0, arg1, arg2.get<int>(), arg3)) );
   };

   template <typename T0>
   FunctionInterface4perl( isotypic_projector_T_x_x_int_o, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      WrapperReturn( (isotypic_projector<T0>(arg0, arg1, arg2.get<int>(), arg3)) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( to_orbit_order_X_X, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (to_orbit_order(arg0.get<T0>(), arg1.get<T1>())) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( irreducible_decomposition_T_C_x, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (irreducible_decomposition<T0>(arg0.get<T1, T0>(), arg1)) );
   };

   FunctionWrapper4perl( pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric> (perl::Object, perl::Object, int, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      IndirectWrapperReturn( arg0, arg1, arg2, arg3 );
   }
   FunctionWrapperInstance4perl( pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric> (perl::Object, perl::Object, int, perl::OptionSet) );

   FunctionWrapper4perl( pm::SparseMatrix<pm::Rational, pm::NonSymmetric> (perl::Object, perl::Object, pm::Array<int> const&) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0, arg1, arg2.get< perl::TryCanned< const Array< int > > >() );
   }
   FunctionWrapperInstance4perl( pm::SparseMatrix<pm::Rational, pm::NonSymmetric> (perl::Object, perl::Object, pm::Array<int> const&) );

   FunctionInstance4perl(irreducible_decomposition_T_C_x, Array< QuadraticExtension< Rational > >, perl::Canned< const Array< QuadraticExtension< Rational > > >);
   FunctionWrapper4perl( pm::IncidenceMatrix<pm::NonSymmetric> (perl::Object, perl::Object, pm::Array<pm::Set<int, pm::operations::cmp>> const&, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      IndirectWrapperReturn( arg0, arg1, arg2.get< perl::TryCanned< const Array< Set< int > > > >(), arg3 );
   }
   FunctionWrapperInstance4perl( pm::IncidenceMatrix<pm::NonSymmetric> (perl::Object, perl::Object, pm::Array<pm::Set<int, pm::operations::cmp>> const&, perl::OptionSet) );

   FunctionWrapper4perl( pm::IncidenceMatrix<pm::NonSymmetric> (perl::Object, perl::Object, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      IndirectWrapperReturn( arg0, arg1, arg2.get< perl::TryCanned< const SparseMatrix< Rational, NonSymmetric > > >(), arg3 );
   }
   FunctionWrapperInstance4perl( pm::IncidenceMatrix<pm::NonSymmetric> (perl::Object, perl::Object, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, perl::OptionSet) );

   FunctionWrapper4perl( pm::Array<int> (pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&) ) {
      perl::Value arg0(stack[0]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const SparseMatrix< Rational, NonSymmetric > > >() );
   }
   FunctionWrapperInstance4perl( pm::Array<int> (pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&) );

   FunctionInstance4perl(irreducible_decomposition_T_C_x, Array< int >, perl::Canned< const Array< int > >);
   FunctionInstance4perl(irreducible_decomposition_T_C_x, Vector< QuadraticExtension< Rational > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::QuadraticExtension<pm::Rational> > const&>, pm::Series<int, true>, mlist<> > >);
   FunctionInstance4perl(to_orbit_order_X_X, perl::Canned< const Array< Array< int > > >, perl::Canned< const Array< int > >);
   FunctionWrapper4perl( perl::Object (perl::Object) ) {
      perl::Value arg0(stack[0]);
      IndirectWrapperReturn( arg0 );
   }
   FunctionWrapperInstance4perl( perl::Object (perl::Object) );

   FunctionInstance4perl(isotypic_projector_T_x_x_int_o, Rational);
   FunctionInstance4perl(isotypic_basis_T_x_x_int_o, QuadraticExtension< Rational >);
   FunctionInstance4perl(isotypic_projector_T_x_x_int_o, QuadraticExtension< Rational >);
   FunctionInstance4perl(isotypic_basis_T_x_x_int_o, Rational);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
