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
#include "polymake/PowerSet.h"
#include "polymake/Set.h"
#include "polymake/list"
#include "polymake/IncidenceMatrix.h"
#include "polymake/FacetList.h"
#include "polymake/color.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"

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

   ClassTemplate4perl("Polymake::common::Array");
   Class4perl("Polymake::common::Array__PowerSet__Int", Array< PowerSet< int > >);
   Class4perl("Polymake::common::Array__Array__Set__Int", Array< Array< Set< int > > >);
   Class4perl("Polymake::common::Array__Set__Int", Array< Set< int > >);
   Class4perl("Polymake::common::Array__Pair_A_Set__Int_I_Set__Int_Z", Array< std::pair< Set< int >, Set< int > > >);
   FunctionInstance4perl(new_X, Array< Set< int > >, int);
   FunctionInstance4perl(new, Array< Set< int > >);
   FunctionInstance4perl(new_X, Array< Set< int > >, perl::Canned< const std::list< Set< int > > >);
   Class4perl("Polymake::common::Array__Int", Array< int >);
   FunctionInstance4perl(new_X, Array< Set< int > >, perl::Canned< const FacetList >);
   FunctionInstance4perl(new_X, Array< Set< int > >, perl::Canned< const PowerSet< int > >);
   FunctionInstance4perl(new, Array< Array< Set< int > > >);
   FunctionInstance4perl(new_X, Array< Set< int > >, perl::Canned< const Array< Set< int > > >);
   Class4perl("Polymake::common::Array__String", Array< std::string >);
   FunctionInstance4perl(new, Array< std::pair< Set< int >, Set< int > > >);
   Class4perl("Polymake::common::Array__RGB", Array< RGB >);
   Class4perl("Polymake::common::Array__Array__Int", Array< Array< int > >);
   Class4perl("Polymake::common::Array__IncidenceMatrix__NonSymmetric", Array< IncidenceMatrix< NonSymmetric > >);
   FunctionInstance4perl(new_X, Array< IncidenceMatrix< NonSymmetric > >, int);
   Class4perl("Polymake::common::Array__List__Set__Int", Array< std::list< Set< int > > >);
   FunctionInstance4perl(new, Array< IncidenceMatrix< NonSymmetric > >);
   FunctionInstance4perl(new_X, Array< Array< Set< int > > >, perl::Canned< const Array< std::list< Set< int > > > >);
   Class4perl("Polymake::common::Array__Rational", Array< Rational >);
   FunctionInstance4perl(new, Array< Rational >);
   Class4perl("Polymake::common::Array__Array__Array__Int", Array< Array< Array< int > > >);
   Class4perl("Polymake::common::Array__Pair_A_Vector__Rational_I_Set__Int_Z", Array< std::pair< Vector< Rational >, Set< int > > >);
   FunctionInstance4perl(new, Array< std::pair< Vector< Rational >, Set< int > > >);
   Class4perl("Polymake::common::Array__Bool", Array< bool >);
   Class4perl("Polymake::common::Array__List__Int", Array< std::list< int > >);
   Class4perl("Polymake::common::Array__Pair_A_Array__Int_I_Array__Int_Z", Array< std::pair< Array< int >, Array< int > > >);
   Class4perl("Polymake::common::Array__Array__Float", Array< Array< double > >);
   Class4perl("Polymake::common::Array__Array__List__Int", Array< Array< std::list< int > > >);
   FunctionInstance4perl(new, Array< std::string >);
   FunctionInstance4perl(new, Array< Array< int > >);
   FunctionInstance4perl(new_X, Array< Array< int > >, perl::Canned< const Array< std::list< int > > >);
   FunctionInstance4perl(new, Array< Array< Array< int > > >);
   FunctionInstance4perl(new_X, Array< std::string >, perl::Canned< const Array< std::string > >);
   FunctionInstance4perl(new, Array< std::pair< Array< int >, Array< int > > >);
   FunctionInstance4perl(new, Array< Array< std::list< int > > >);
   FunctionInstance4perl(new, Array< bool >);
   FunctionInstance4perl(new_X, Array< int >, perl::Canned< const Array< int > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
