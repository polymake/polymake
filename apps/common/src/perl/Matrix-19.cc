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

///==== this line controls the automatic file splitting: max.instances=20

#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/linalg.h"

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

   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(new_X, Matrix< Integer >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Integer const&> const&>, pm::Matrix<pm::Integer> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   FunctionInstance4perl(new_X, Matrix< QuadraticExtension< Rational > >, perl::Canned< const pm::RowChain<pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&, pm::Matrix<pm::QuadraticExtension<pm::Rational> > const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::RowChain<pm::Matrix<double> const&, pm::Matrix<double> const&> >);
   FunctionInstance4perl(new_X, Matrix< TropicalNumber< Min, Rational > >, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< TropicalNumber< Min, Rational > > > >, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(new_X, Matrix< TropicalNumber< Max, Rational > >, perl::Canned< const Matrix< TropicalNumber< Max, Rational > > >);
   FunctionInstance4perl(new, Matrix< TropicalNumber< Max, Rational > >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< Matrix< TropicalNumber< Max, Rational > > > >, perl::Canned< const Matrix< TropicalNumber< Max, Rational > > >);
   OperatorInstance4perl(BinaryAssign_div, perl::Canned< Wary< Matrix< Rational > > >, perl::Canned< const pm::VectorChain<pm::Vector<pm::Rational> const&, pm::Vector<pm::Rational> const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const Matrix< TropicalNumber< Min, Rational > > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const Matrix< TropicalNumber< Max, Rational > > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<pm::Rational>&, pm::all_selector const&, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   FunctionInstance4perl(new_X, Matrix< double >, perl::Canned< const pm::MatrixMinor<pm::Matrix<double> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> >);
   FunctionInstance4perl(new_X, Matrix< double >, perl::Canned< const pm::MatrixMinor<pm::Matrix<double> const&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::MatrixMinor<pm::Matrix<double> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> >);
   FunctionInstance4perl(new_X, Matrix< Rational >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::Vector<int> const&>, pm::MatrixMinor<pm::SparseMatrix<int, pm::NonSymmetric> const&, pm::Complement<pm::Set<int, pm::operations::cmp>, int, pm::operations::cmp> const&, pm::all_selector const&> const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< Matrix< Rational > > >, perl::Canned< const pm::Transposed<pm::ColChain<pm::Matrix<pm::Rational> const&, pm::SingleCol<pm::Vector<pm::Rational> const&> > > >);
   FunctionInstance4perl(new_X, Matrix< double >, perl::Canned< const pm::MatrixMinor<pm::Matrix<double> const&, pm::Array<int> const&, pm::all_selector const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
