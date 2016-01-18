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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( bounding_box_T_X_x_x, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      WrapperReturn( (bounding_box<T0>(arg0.get<T1>(), arg1, arg2)) );
   };

   FunctionInstance4perl(bounding_box_T_X_x_x, Rational, perl::Canned< const pm::RowChain<pm::Matrix<pm::Rational> const&, pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Set<int, pm::operations::cmp> const&, pm::Series<int, true> const&> const&> >);
   FunctionInstance4perl(bounding_box_T_X_x_x, Rational, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(bounding_box_T_X_x_x, double, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<double const&> const&>, pm::Matrix<double> const&> >);
   FunctionInstance4perl(bounding_box_T_X_x_x, Rational, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(bounding_box_T_X_x_x, double, perl::Canned< const Matrix< double > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
