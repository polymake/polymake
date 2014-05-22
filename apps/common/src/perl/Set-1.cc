/* Copyright (c) 1997-2014
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

#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Graph.h"
#include "polymake/Vector.h"
#include "polymake/Integer.h"
#include "polymake/Array.h"
#include "polymake/list"
#include "polymake/Rational.h"
#include "polymake/QuadraticExtension.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   ClassTemplate4perl("Polymake::common::Set");
   Class4perl("Polymake::common::Set__Int", Set< int >);
   FunctionInstance4perl(new, Set< int >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Set< int > >, perl::Canned< const Set< int > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Set< int > >, perl::Canned< const Set< int > >);
   OperatorInstance4perl(Unary_com, perl::Canned< const Set< int > >);
   OperatorInstance4perl(assign, Set< int >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >);
   FunctionInstance4perl(new_X, Set< int >, perl::Canned< const Set< int > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const pm::Series<int, true> >, perl::Canned< const Set< int > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::graph::traits_base<pm::graph::Undirected, false, (pm::sparse2d::restriction_kind)0>, true, (pm::sparse2d::restriction_kind)0> > > >, perl::Canned< const Set< int > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Set< int > >, perl::Canned< const Set< int > >);
   Class4perl("Polymake::common::Set__Vector__Integer", Set< Vector< Integer > >);
   FunctionInstance4perl(new, Set< Vector< Integer > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Set< Vector< Integer > > >, perl::Canned< const Vector< Integer > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Set< int > >, int);
   FunctionInstance4perl(new_X, Set< int >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >);
   OperatorInstance4perl(Binary_add, int, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >);
   OperatorInstance4perl(Unary_com, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const pm::Series<int, true> >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >);
   OperatorInstance4perl(assign, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)2>, false, (pm::sparse2d::restriction_kind)2> > >
, perl::Canned< const std::list< Set< int > > >);
   OperatorInstance4perl(assign, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)2>, false, (pm::sparse2d::restriction_kind)2> > >
, perl::Canned< const Array< Set< int > > >);
   OperatorInstance4perl(assign, Set< int >, perl::Canned< const IncidenceMatrix< NonSymmetric > >);
   OperatorInstance4perl(assign, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, false, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&>, perl::Canned< const Set< int > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Set< int > >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >);
   OperatorInstance4perl(assign, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)2>, false, (pm::sparse2d::restriction_kind)2> > >, perl::Canned< const Set< int > >);
   OperatorInstance4perl(assign, Set< int >, perl::Canned< const pm::Series<int, true> >);
   FunctionInstance4perl(new_X, Set< int >, int);
   Class4perl("Polymake::common::Set__Pair_A_Set__Int_I_Set__Int_Z", Set< std::pair< Set< int >, Set< int > > >);
   Class4perl("Polymake::common::Set__Set__Int", Set< Set< int > >);
   FunctionInstance4perl(new, Set< Set< int > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Set< std::pair< Set< int >, Set< int > > > >, perl::Canned< const Set< std::pair< Set< int >, Set< int > > > >);
   FunctionInstance4perl(new, Set< std::pair< Set< int >, Set< int > > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Set< Set< int > > >, perl::Canned< const Set< int > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Set< Set< int > > >, perl::Canned< const Set< Set< int > > >);
   Class4perl("Polymake::common::Set__Vector__Rational", Set< Vector< Rational > >);
   FunctionInstance4perl(new, Set< Vector< Rational > >);
   OperatorInstance4perl(BinaryAssign_sub, perl::Canned< Set< Vector< Rational > > >, perl::Canned< const Set< Vector< Rational > > >);
   Class4perl("Polymake::common::Set__Vector__QuadraticExtension__Rational", Set< Vector< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(new, Set< Vector< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(BinaryAssign_sub, perl::Canned< Set< Vector< QuadraticExtension< Rational > > > >, perl::Canned< const Set< Vector< QuadraticExtension< Rational > > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
