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
#include "polymake/linalg.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/SparseMatrix.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( new_X, T0,T1 ) {
      perl::Value arg0(stack[1]);
      WrapperReturnNew(T0, (arg0.get<T1>()) );
   };

   FunctionInstance4perl(new_X, Matrix<double>, perl::Canned< const pm::RowChain<pm::Matrix<double> const&, pm::SingleRow<pm::Vector<double> const&> > >);
   OperatorInstance4perl(convert, Matrix<double>, perl::Canned< const SparseMatrix< double, NonSymmetric > >);
   FunctionInstance4perl(new_X, Matrix<Rational>, perl::Canned< const pm::Transposed<pm::RowChain<pm::SingleRow<pm::Vector<pm::Rational> const&> const&, pm::RowChain<pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&> > >);
   FunctionInstance4perl(new_X, Matrix<Rational>, perl::Canned< const pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> >);
   FunctionInstance4perl(new_X, Matrix<Rational>, perl::Canned< const pm::Transposed<pm::RowChain<pm::SingleRow<pm::Vector<pm::Rational> const&> const&, pm::RowChain<pm::SingleRow<pm::Vector<pm::Rational> const&> const&, pm::RowChain<pm::RowChain<pm::Matrix<pm::Rational> const&, pm::Matrix<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&> const&> > >);
   FunctionInstance4perl(new_X, Matrix<Rational>, perl::Canned< const pm::Transposed<pm::RowChain<pm::SingleRow<pm::Vector<pm::Rational> const&> const&, pm::RowChain<pm::SingleRow<pm::Vector<pm::Rational> const&> const&, pm::Matrix<pm::Rational> const&> const&> > >);
   FunctionInstance4perl(new_X, Matrix<Rational>, perl::Canned< const pm::SingleRow<pm::Vector<pm::Rational> const&> >);
   FunctionInstance4perl(new_X, Matrix<double>, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<double const&> const&>, pm::RowChain<pm::MatrixMinor<pm::Matrix<double>&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> const&, pm::SingleRow<pm::Vector<double> const&> > const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
