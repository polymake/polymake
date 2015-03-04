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

///==== this line controls the automatic file splitting: max.instances=20

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new_int_int, T0 ) {
      perl::Value arg0(stack[1]), arg1(stack[2]);
      WrapperReturnNew(T0, (arg0.get<int>(), arg1.get<int>()) );
   };

   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   FunctionInstance4perl(new, Matrix< Rational >);
   FunctionInstance4perl(new, Matrix< double >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const Matrix< double > >);
   FunctionInstance4perl(new, Matrix< Integer >);
   FunctionInstance4perl(new_X, Matrix< int >, perl::Canned< const pm::SameElementSparseMatrix<pm::IncidenceMatrix<pm::NonSymmetric> const&, int> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const Matrix< Integer > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> >);
   FunctionInstance4perl(new, Matrix< int >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const Matrix< int > >);
   FunctionInstance4perl(new_int_int, Matrix< Rational >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(new_X, Matrix< int >, perl::Canned< const Matrix< int > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::Series<int, true> const&> const&> >);
   FunctionInstance4perl(new_X, Matrix< Integer >, perl::Canned< const pm::Transposed<pm::Matrix<pm::Integer> > >);
   FunctionInstance4perl(new_int_int, Matrix< int >);
   FunctionInstance4perl(new_X, Matrix< Integer >, perl::Canned< const Matrix< int > >);
   FunctionInstance4perl(new_X, Matrix< Integer >, perl::Canned< const Matrix< Integer > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::Matrix<pm::Rational> const&, pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Set<int, pm::operations::cmp> const&, pm::Series<int, true> const&> const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
