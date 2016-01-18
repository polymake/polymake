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
#include "polymake/Set.h"
#include "polymake/list"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Polynomial.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Integer.h"
#include "polymake/PowerSet.h"
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

   FunctionInstance4perl(new_X, Array< std::string >, perl::TryCanned< const std::list< std::string > >);
   FunctionInstance4perl(new, Array< std::list< int > >);
   FunctionInstance4perl(new, Array< int >);
   FunctionInstance4perl(new_X, Array< int >, int);
   FunctionInstance4perl(new_X, Array< std::list< int > >, perl::Canned< const Array< std::list< int > > >);
   FunctionInstance4perl(new_X, Array< std::list< int > >, int);
   FunctionInstance4perl(new_X, Array< Array< int > >, perl::Canned< const Array< Array< int > > >);
   FunctionInstance4perl(new_X, Array< int >, perl::Canned< const Set< int > >);
   FunctionInstance4perl(new_X, Array< int >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >);
   FunctionInstance4perl(new_X, Array< Array< Array< int > > >, perl::Canned< const Array< Array< Array< int > > > >);
   Class4perl("Polymake::common::Array__Array__String", Array< Array< std::string > >);
   FunctionInstance4perl(new_X, Array< bool >, perl::Canned< const Array< bool > >);
   FunctionInstance4perl(new_X, Array< IncidenceMatrix< NonSymmetric > >, perl::Canned< const Array< IncidenceMatrix< NonSymmetric > > >);
   Class4perl("Polymake::common::Array__Polynomial_A_Rational_I_Int_Z", Array< Polynomial< Rational, int > >);
   FunctionInstance4perl(new, Array< Polynomial< Rational, int > >);
   Class4perl("Polymake::common::Array__Set__Set__Int", Array< Set< Set< int > > >);
   FunctionInstance4perl(new, Array< Set< Set< int > > >);
   FunctionInstance4perl(new_X, Array< int >, perl::Canned< const pm::IndexedSlice<pm::ConcatRows<pm::Matrix<int> > const&, pm::Series<int, false>, void> >);
   Class4perl("Polymake::common::Array__Set__Set__Set__Int", Array< Set< Set< Set< int > > > >);
   FunctionInstance4perl(new, Array< Set< Set< Set< int > > > >);
   OperatorInstance4perl(assign, Array< Array< int > >, perl::Canned< const Array< Set< int > > >);
   OperatorInstance4perl(convert, Array< Array< int > >, perl::Canned< const Array< Set< int > > >);
   Class4perl("Polymake::common::Array__Matrix_A_Integer_I_NonSymmetric_Z", Array< Matrix< Integer > >);
   FunctionInstance4perl(new, Array< Matrix< Integer > >);
   Class4perl("Polymake::common::Array__Matrix_A_Rational_I_NonSymmetric_Z", Array< Matrix< Rational > >);
   FunctionInstance4perl(new, Array< Matrix< Rational > >);
   FunctionInstance4perl(new, Array< PowerSet< int > >);
   FunctionInstance4perl(new_X, Array< Rational >, int);
   Class4perl("Polymake::common::Array__QuadraticExtension__Rational", Array< QuadraticExtension< Rational > >);
   FunctionInstance4perl(new, Array< QuadraticExtension< Rational > >);
   Class4perl("Polymake::common::Array__Array__Rational", Array< Array< Rational > >);
   FunctionInstance4perl(new, Array< Array< Rational > >);
   Class4perl("Polymake::common::Array__Integer", Array< Integer >);
   Class4perl("Polymake::common::Array__Pair_A_Int_I_Set__Int_Z", Array< std::pair< int, Set< int > > >);
   FunctionInstance4perl(new, Array< std::pair< int, Set< int > > >);
   FunctionInstance4perl(new_X, Array< Set< Set< int > > >, perl::Canned< const Array< Array< Set< int > > > >);
   FunctionInstance4perl(new, Array< std::pair< int, int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
