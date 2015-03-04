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

#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( bounding_box_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( bounding_box(arg0.get<T0>()) );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( extend_bounding_box_X2_X_f16, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturnVoid( extend_bounding_box(arg0.get<T0>(), arg1.get<T1>()) );
   };

   FunctionInstance4perl(bounding_box_X, perl::Canned< const Matrix< double > >);
   FunctionInstance4perl(extend_bounding_box_X2_X_f16, perl::Canned< Matrix< double > >, perl::Canned< const Matrix< double > >);
   FunctionInstance4perl(bounding_box_X, perl::Canned< const pm::MatrixMinor<pm::Matrix<double>&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(bounding_box_X, perl::Canned< const SparseMatrix< double, NonSymmetric > >);
   FunctionInstance4perl(bounding_box_X, perl::Canned< const pm::MatrixMinor<pm::Matrix<double>&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
