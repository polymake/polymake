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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/client.h"
#include "polymake/list"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   ClassTemplate4perl("Polymake::common::List");
   Class4perl("Polymake::common::List__Pair_A_Integer_I_Int_Z", std::list< std::pair< Integer, int > >);
   Class4perl("Polymake::common::List__Set__Int", std::list< Set< int > >);
   Class4perl("Polymake::common::List__Integer", std::list< Integer >);
   Builtin4perl("Polymake::common::List__String", std::list< std::string >);
   Class4perl("Polymake::common::List__List__Pair_A_Int_I_Int_Z", std::list< std::list< std::pair< int, int > > >);
   Class4perl("Polymake::common::List__Pair_A_Int_I_Int_Z", std::list< std::pair< int, int > >);
   Class4perl("Polymake::common::List__Int", std::list< int >);
   FunctionInstance4perl(new_X, std::list< int >, int);
   Class4perl("Polymake::common::List__Pair_A_Integer_I_SparseMatrix_A_Integer_I_NonSymmetric_Z_Z", std::list< std::pair< Integer, SparseMatrix< Integer, NonSymmetric > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
