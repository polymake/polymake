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

#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Map.h"
#include "polymake/Matrix.h"
#include "polymake/Polynomial.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   Class4perl("Polymake::common::Map_A_Set__Int_I_Vector__Rational_Z", Map< Set< int >, Vector< Rational > >);
   Class4perl("Polymake::common::Map_A_Vector__Integer_I_Vector__Integer_Z", Map< Vector< Integer >, Vector< Integer > >);
   Class4perl("Polymake::common::Map_A_Vector__Rational_I_Vector__Rational_Z", Map< Vector< Rational >, Vector< Rational > >);
   FunctionInstance4perl(new, Map< Vector< Rational >, Array< Vector< Rational > > >);
   FunctionInstance4perl(new, Map< Vector< Rational >, Matrix< Rational > >);
   FunctionInstance4perl(new, Map< Vector< Rational >, Vector< Rational > >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Vector< Rational >, Vector< Rational > > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, void> >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Vector< Rational >, Matrix< Rational > > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, void> >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Vector< Rational >, Array< Vector< Rational > > > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, void> >);
   FunctionInstance4perl(new, Map< Set< int >, Vector< Rational > >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Set< int >, Vector< Rational > > >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >);
   OperatorInstance4perl(Binary_brk, perl::Canned< const Map< Set< int >, Vector< Rational > > >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >);
   Class4perl("Polymake::common::Map_A_Set__Int_I_Polynomial_A_Rational_I_Int_Z_Z", Map< Set< int >, Polynomial< Rational, int > >);
   Class4perl("Polymake::common::Map_A_Int_I_Map_A_Int_I_Vector__Integer_Z_Z", Map< int, Map< int, Vector< Integer > > >);
   Class4perl("Polymake::common::Map_A_Int_I_Vector__Rational_Z", Map< int, Vector< Rational > >);
   Class4perl("Polymake::common::Map_A_Int_I_Map_A_Int_I_Vector__Rational_Z_Z", Map< int, Map< int, Vector< Rational > > >);
   Class4perl("Polymake::common::Map_A_Set__Int_I_Set__Int_Z", Map< Set< int >, Set< int > >);
   Class4perl("Polymake::common::Map_A_Int_I_Vector__Integer_Z", Map< int, Vector< Integer > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
