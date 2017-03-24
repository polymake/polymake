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
#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/Polynomial.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/hash_set"
#include "polymake/list"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   Class4perl("Polymake::common::Array__Set__Matrix_A_Rational_I_NonSymmetric_Z", Array< Set< Matrix< Rational > > >);
   FunctionInstance4perl(new_X, Array< Set< int > >, perl::Canned< const pm::Rows<pm::IncidenceMatrix<pm::NonSymmetric> > >);
   Class4perl("Polymake::common::Array__PuiseuxFraction_A_Max_I_Rational_I_Rational_Z", Array< PuiseuxFraction< Max, Rational, Rational > >);
   Class4perl("Polymake::common::Array__PuiseuxFraction_A_Min_I_Rational_I_Rational_Z", Array< PuiseuxFraction< Min, Rational, Rational > >);
   Class4perl("Polymake::common::Array__Matrix_A_PuiseuxFraction_A_Max_I_Rational_I_Rational_Z_I_NonSymmetric_Z", Array< Matrix< PuiseuxFraction< Max, Rational, Rational > > >);
   Class4perl("Polymake::common::Array__Vector__PuiseuxFraction_A_Max_I_Rational_I_Rational_Z", Array< Vector< PuiseuxFraction< Max, Rational, Rational > > >);
   Class4perl("Polymake::common::Array__Set__Matrix_A_Float_I_NonSymmetric_Z", Array< Set< Matrix< double > > >);
   Class4perl("Polymake::common::Array__Vector__QuadraticExtension__Rational", Array< Vector< QuadraticExtension< Rational > > >);
   Class4perl("Polymake::common::Array__Matrix_A_Float_I_NonSymmetric_Z", Array< Matrix< double > >);
   Class4perl("Polymake::common::Array__Vector__Float", Array< Vector< double > >);
   Class4perl("Polymake::common::Array__Set__Matrix_A_PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_I_NonSymmetric_Z", Array< Set< Matrix< PuiseuxFraction< Min, Rational, Rational > > > >);
   Class4perl("Polymake::common::Array__Vector__PuiseuxFraction_A_Min_I_Rational_I_Rational_Z", Array< Vector< PuiseuxFraction< Min, Rational, Rational > > >);
   Class4perl("Polymake::common::Array__Matrix_A_PuiseuxFraction_A_Min_I_Rational_I_Rational_Z_I_NonSymmetric_Z", Array< Matrix< PuiseuxFraction< Min, Rational, Rational > > >);
   Class4perl("Polymake::common::Array__Set__Matrix_A_QuadraticExtension__Rational_I_NonSymmetric_Z", Array< Set< Matrix< QuadraticExtension< Rational > > > >);
   Class4perl("Polymake::common::Array__Matrix_A_QuadraticExtension__Rational_I_NonSymmetric_Z", Array< Matrix< QuadraticExtension< Rational > > >);
   Class4perl("Polymake::common::Array__Set__Matrix_A_PuiseuxFraction_A_Max_I_Rational_I_Rational_Z_I_NonSymmetric_Z", Array< Set< Matrix< PuiseuxFraction< Max, Rational, Rational > > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
