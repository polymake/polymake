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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/RationalFunction.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/linalg.h"

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

   Class4perl("Polymake::common::SparseMatrix_A_PuiseuxFraction_A_Max_I_Rational_I_Rational_Z_I_NonSymmetric_Z", SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric >);
   FunctionInstance4perl(new_X, SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric >, perl::Canned< const SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric > >);
   FunctionInstance4perl(new, SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric >);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Wary< SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric > > >, perl::Canned< const SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric > > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::PuiseuxFraction<pm::Max, pm::Rational, pm::Rational> > const&>, pm::Series<int, true>, mlist<> > >);
   Class4perl("Polymake::common::SparseMatrix_A_PuiseuxFraction_A_Max_I_Rational_I_Rational_Z_I_Symmetric_Z", SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, Symmetric >);
   Class4perl("Polymake::common::SparseMatrix_A_RationalFunction_A_Rational_I_Int_Z_I_Symmetric_Z", SparseMatrix< RationalFunction< Rational, int >, Symmetric >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< SparseMatrix< Integer, NonSymmetric > > >, perl::Canned< const SparseMatrix< Integer, NonSymmetric > >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::Vector<pm::Rational> const&>, pm::RowChain<pm::MatrixMinor<pm::Matrix<pm::Rational> const&, pm::all_selector const&, pm::Complement<pm::SingleElementSetCmp<int, pm::operations::cmp>, int, pm::operations::cmp> const&> const&, pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> const&> >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::Transposed<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric> > >);
   OperatorInstance4perl(Binary__ora, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Transposed<pm::MatrixMinor<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric>&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> > const&> > >, perl::Canned< const pm::Transposed<pm::MatrixMinor<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric>&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> > >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::ColChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Transposed<pm::MatrixMinor<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric>&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> > const&> const&, pm::Transposed<pm::MatrixMinor<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric>&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> > const&> >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::QuadraticExtension<pm::Rational> const&> const&>, pm::Transposed<pm::MatrixMinor<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric>&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> > const&> >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Wary< pm::Transposed<pm::MatrixMinor<pm::SparseMatrix<pm::QuadraticExtension<pm::Rational>, pm::NonSymmetric>&, pm::Set<int, pm::operations::cmp> const&, pm::all_selector const&> > > >, perl::Canned< const pm::Transposed<pm::Matrix<pm::QuadraticExtension<pm::Rational> > > >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::MatrixMinor<pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::MatrixMinor<pm::SparseMatrix<double, pm::NonSymmetric>&, pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> const&, pm::all_selector const&> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> > >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::RowChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> const&, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> const&> >);
   OperatorInstance4perl(Binary_diva, perl::Canned< const Wary< pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> > >, perl::Canned< const pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&> >);
   FunctionInstance4perl(new_X, SparseMatrix< Rational, NonSymmetric >, perl::Canned< const pm::RowChain<pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::DiagMatrix<pm::SameElementVector<pm::Rational const&>, true> const&> const&, pm::ColChain<pm::SingleCol<pm::SameElementVector<pm::Rational const&> const&>, pm::SparseMatrix<pm::Rational, pm::NonSymmetric> const&> const&> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
