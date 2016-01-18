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
#include "polymake/PuiseuxFraction.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/client.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( indices_X8, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturnAnch( 1, (arg0), indices(arg0.get<T0>()) );
   };

   FunctionInstance4perl(indices_X8, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<int, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(indices_X8, perl::Canned< const SparseVector< Rational > >);
   FunctionInstance4perl(indices_X8, perl::Canned< const SparseVector< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(indices_X8, perl::Canned< const SparseVector< PuiseuxFraction< Min, PuiseuxFraction< Min, Rational, Rational >, Rational > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
