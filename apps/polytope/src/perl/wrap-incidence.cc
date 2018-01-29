/* Copyright (c) 1997-2018
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
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/QuadraticExtension.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( incidence_matrix_X_X, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( incidence_matrix(arg0.get<T0>(), arg1.get<T1>()) );
   };

   FunctionInstance4perl(incidence_matrix_X_X, perl::Canned< const Matrix< Rational > >, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(incidence_matrix_X_X, perl::Canned< const Matrix< double > >, perl::Canned< const Matrix< double > >);
   FunctionInstance4perl(incidence_matrix_X_X, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(incidence_matrix_X_X, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<int, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> const&, pm::all_selector const&> >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> >);
   FunctionInstance4perl(incidence_matrix_X_X, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(incidence_matrix_X_X, perl::Canned< const Matrix<Rational> >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(incidence_matrix_X_X, perl::Canned< const SparseMatrix< double, NonSymmetric > >, perl::Canned< const Matrix<double> >);
   FunctionInstance4perl(incidence_matrix_X_X, perl::Canned< const Matrix< double > >, perl::Canned< const SparseMatrix< double, NonSymmetric > >);
   FunctionInstance4perl(incidence_matrix_X_X, perl::Canned< const Matrix< QuadraticExtension< Rational > > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
