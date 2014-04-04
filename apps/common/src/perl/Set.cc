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
#include "polymake/Array.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Rational.h"

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

   Class4perl("Polymake::common::Set__Set__Set__Int", Set< Set< Set< int > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Set< Set< Set< int > > > >, perl::Canned< const Set< Set< Set< int > > > >);
   OperatorInstance4perl(assign, Set< int >, perl::Canned< const Array< Array< Set< int > > > >);
   OperatorInstance4perl(BinaryAssign_sub, perl::Canned< Set< int > >, int);
   OperatorInstance4perl(Unary_com, perl::Canned< const pm::Series<int, true> >);
   OperatorInstance4perl(assign, Set< int >, perl::Canned< const pm::SingleElementSet<int> >);
   OperatorInstance4perl(Unary_com, perl::Canned< const pm::SingleElementSet<int> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Set< int > >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Set< Vector< Rational > > >, perl::Canned< const Set< Vector< Rational > > >);
   FunctionInstance4perl(new_X, Set< Set< int > >, perl::Canned< const Array< Set< int > > >);
   Class4perl("Polymake::common::Set__Array__Set__Int", Set< Array< Set< int > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Set< Array< Set< int > > > >, perl::Canned< const Set< Array< Set< int > > > >);
   Class4perl("Polymake::common::Set__Vector__Int", Set< Vector< int > >);
   FunctionInstance4perl(new, Set< Vector< int > >);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Set< Vector< int > > >, perl::Canned< const Vector< int > >);
   OperatorInstance4perl(convert, Set< Set< int > >, perl::Canned< const Array< Set< int > > >);
   Class4perl("Polymake::common::Set__String", Set< std::string >);
   FunctionInstance4perl(new, Set< std::string >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Set< std::string > >, perl::Canned< const Set< std::string > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
