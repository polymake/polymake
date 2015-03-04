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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Rational.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Set.h"
#include "polymake/list"
#include "polymake/Polynomial.h"
#include "polymake/Graph.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"

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

   Class4perl("Polymake::common::Array__Set__Array__Set__Int", Array< Set< Array< Set< int > > > >);
   FunctionInstance4perl(new_X, Array< Array< Set< int > > >, perl::Canned< const Array< Set< Set< int > > > >);
   FunctionInstance4perl(new_X, Array< Set< Array< Set< int > > > >, perl::Canned< const Array< Set< Set< Set< int > > > > >);
   FunctionInstance4perl(new, Array< Set< Array< Set< int > > > >);
   OperatorInstance4perl(convert, Array< int >, perl::Canned< const pm::Series<int, true> >);
   Class4perl("Polymake::common::Array__Vector__Rational", Array< Vector< Rational > >);
   OperatorInstance4perl(assign, Array< Matrix< Rational > >, perl::Canned< const Array< Matrix< Integer > > >);
   FunctionInstance4perl(new, Array< Vector< Rational > >);
   OperatorInstance4perl(convert, Array< int >, perl::Canned< const Set< int > >);
   FunctionInstance4perl(new_X, Array< Polynomial< Rational, int > >, perl::Canned< const Array< Polynomial< Rational, int > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< int > >, perl::Canned< const Array< int > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< Set< int > > >, perl::Canned< const Array< Set< int > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< std::string > >, perl::Canned< const Array< std::string > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< bool > >, perl::Canned< const Array< bool > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< Polynomial< Rational, int > > >, perl::Canned< const Array< Polynomial< Rational, int > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< Array< int > > >, perl::Canned< const Array< Array< int > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< std::pair< Array< int >, Array< int > > > >, perl::Canned< const Array< std::pair< Array< int >, Array< int > > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< Array< std::list< int > > > >, perl::Canned< const Array< Array< std::list< int > > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< std::pair< Set< int >, Set< int > > > >, perl::Canned< const Array< std::pair< Set< int >, Set< int > > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< std::list< int > > >, perl::Canned< const Array< std::list< int > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< Rational > >, perl::Canned< const Array< Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< QuadraticExtension< Rational > > >, perl::Canned< const Array< QuadraticExtension< Rational > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< Array< Set< int > > > >, perl::Canned< const Array< Array< Set< int > > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< Set< Array< Set< int > > > > >, perl::Canned< const Array< Set< Array< Set< int > > > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< IncidenceMatrix< NonSymmetric > > >, perl::Canned< const Array< IncidenceMatrix< NonSymmetric > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Array< std::pair< Vector< Rational >, Set< int > > > >, perl::Canned< const Array< std::pair< Vector< Rational >, Set< int > > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
