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
#include "polymake/Matrix.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/list"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Set< Vector< double > > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double> const&>, pm::Series<int, true>> >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Set< Vector< double > > >, perl::Canned< const Vector< double > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Set< Vector< Rational > > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(new_X, Set< int >, perl::Canned< const Array< int > >);
   OperatorInstance4perl(BinaryAssign_sub, perl::Canned< Set< int > >, perl::Canned< const Set< int > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&> >, int);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&> >, perl::Canned< const Set< int > >);
   OperatorInstance4perl(assign, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)2>, false, (pm::sparse2d::restriction_kind)2> > >, perl::Canned< const pm::Series<int, true> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Set< int > >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Set< Vector< Integer > > >, perl::Canned< const Set< Vector< Integer > > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Set< Vector< Integer > > >, perl::Canned< const Set< Vector< Integer > > >);
   FunctionInstance4perl(new_X, Set< Vector< Integer > >, perl::Canned< const Vector< Integer > >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Set< int > >, perl::Canned< const Set< int > >);
   OperatorInstance4perl(convert, Set< int >, perl::Canned< const std::list< int > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const pm::Series<int, true> >, perl::Canned< const pm::Indices<pm::SparseVector<pm::PuiseuxFraction<pm::Min, pm::Rational, pm::Rational> > const&> >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Set< int > >, int);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Set< Vector< double > > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double>&>, pm::Series<int, true>, mlist<> > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Set< Vector< Rational > > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, mlist<> > >);
   FunctionInstance4perl(new_X, Set< Vector< Integer > >, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Integer> const&>, pm::Series<int, true>, mlist<> >, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&, mlist<> > >);
   Class4perl("Polymake::common::Set__Matrix_A_PuiseuxFraction_A_Max_I_Rational_I_Rational_Z_I_NonSymmetric_Z", Set< Matrix< PuiseuxFraction< Max, Rational, Rational > > >);
   Class4perl("Polymake::common::Set__Matrix_A_PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_I_NonSymmetric_Z", Set< Matrix< PuiseuxFraction< Min, Rational, Rational > > >);
   Class4perl("Polymake::common::Set__Matrix_A_Float_I_NonSymmetric_Z", Set< Matrix< double > >);
   Class4perl("Polymake::common::Set__Matrix_A_QuadraticExtension__Rational_I_NonSymmetric_Z", Set< Matrix< QuadraticExtension< Rational > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
