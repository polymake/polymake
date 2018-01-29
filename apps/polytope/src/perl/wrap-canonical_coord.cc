/* Copyright (c) 1997-2018
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or4 FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( orthogonalize_subspace_X2_f16, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturnVoid( (orthogonalize_subspace(arg0.get<T0>())) );
   };

   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( find_representation_permutation_X_X_X_x, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      WrapperReturn( find_representation_permutation(arg0.get<T0>(), arg1.get<T1>(), arg2.get<T2>(), arg3) );
   };

   template <typename T0>
   FunctionInterface4perl( canonicalize_rays_X2_f16, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturnVoid( canonicalize_rays(arg0.get<T0>()) );
   };

   template <typename T0>
   FunctionInterface4perl( rotate_hyperplane_X_x, T0 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( rotate_hyperplane(arg0.get<T0>(), arg1) );
   };

   template <typename T0>
   FunctionInterface4perl( dehomogenize_X, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( dehomogenize(arg0.get<T0>()) );
   };

   template <typename T0>
   FunctionInterface4perl( orthogonalize_affine_subspace_X2_f16, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturnVoid( orthogonalize_affine_subspace(arg0.get<T0>()) );
   };

   template <typename T0>
   FunctionInterface4perl( canonicalize_facets_X2_f16, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturnVoid( canonicalize_facets(arg0.get<T0>()) );
   };

   FunctionInstance4perl(orthogonalize_affine_subspace_X2_f16, perl::Canned< Matrix< Rational > >);
   FunctionInstance4perl(orthogonalize_affine_subspace_X2_f16, perl::Canned< SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(orthogonalize_affine_subspace_X2_f16, perl::Canned< Matrix< double > >);
   FunctionInstance4perl(orthogonalize_affine_subspace_X2_f16, perl::Canned< SparseMatrix< Rational, Symmetric > >);
   FunctionInstance4perl(orthogonalize_affine_subspace_X2_f16, perl::Canned< SparseMatrix< double, NonSymmetric > >);
   FunctionInstance4perl(orthogonalize_affine_subspace_X2_f16, perl::Canned< Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(orthogonalize_affine_subspace_X2_f16, perl::Canned< SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   FunctionInstance4perl(dehomogenize_X, perl::Canned< const Matrix< double > >);
   FunctionInstance4perl(dehomogenize_X, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> >);
   FunctionInstance4perl(dehomogenize_X, perl::Canned< const Vector< double > >);
   FunctionInstance4perl(dehomogenize_X, perl::Canned< const SparseMatrix< double, NonSymmetric > >);
   FunctionInstance4perl(dehomogenize_X, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(rotate_hyperplane_X_x, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> >);
   FunctionInstance4perl(rotate_hyperplane_X_x, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(canonicalize_rays_X2_f16, perl::Canned< Matrix< Rational > >);
   FunctionInstance4perl(canonicalize_rays_X2_f16, perl::Canned< Vector< Rational > >);
   FunctionInstance4perl(canonicalize_rays_X2_f16, perl::Canned< Matrix< double > >);
   FunctionInstance4perl(canonicalize_rays_X2_f16, perl::Canned< SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(canonicalize_rays_X2_f16, perl::Canned< Vector< double > >);
   FunctionInstance4perl(canonicalize_rays_X2_f16, perl::Canned< SparseMatrix< Rational, Symmetric > >);
   FunctionInstance4perl(canonicalize_rays_X2_f16, perl::Canned< SparseVector< Rational > >);
   FunctionInstance4perl(canonicalize_rays_X2_f16, perl::Canned< SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   FunctionInstance4perl(canonicalize_rays_X2_f16, perl::Canned< Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(canonicalize_rays_X2_f16, perl::Canned< SparseVector< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(canonicalize_rays_X2_f16, perl::Canned< SparseMatrix< double, NonSymmetric > >);
   FunctionInstance4perl(canonicalize_facets_X2_f16, perl::Canned< Matrix< Rational > >);
   FunctionInstance4perl(find_representation_permutation_X_X_X_x, perl::Canned< const Matrix<double> >, perl::Canned< const Matrix<double> >, perl::Canned< const Matrix<double> >);
   FunctionInstance4perl(find_representation_permutation_X_X_X_x, perl::Canned< const Matrix<Rational> >, perl::Canned< const Matrix<Rational> >, perl::Canned< const Matrix<Rational> >);
   FunctionInstance4perl(find_representation_permutation_X_X_X_x, perl::Canned< const Matrix<Rational> >, perl::Canned< const SparseMatrix< Rational, NonSymmetric > >, perl::Canned< const Matrix<Rational> >);
   FunctionInstance4perl(find_representation_permutation_X_X_X_x, perl::Canned< const Matrix< QuadraticExtension< Rational > > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(find_representation_permutation_X_X_X_x, perl::Canned< const Matrix< QuadraticExtension< Rational > > >, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(canonicalize_rays_X2_f16, perl::Canned< Vector< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(dehomogenize_X, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(dehomogenize_X, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(dehomogenize_X, perl::Canned< const pm::RowChain<pm::RowChain<pm::Matrix<double> const&, pm::Matrix<double> const&> const&, pm::Matrix<double> const&> >);
   FunctionInstance4perl(dehomogenize_X, perl::Canned< const pm::RowChain<pm::Matrix<double> const&, pm::Matrix<double> const&> >);
   FunctionInstance4perl(dehomogenize_X, perl::Canned< const SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   FunctionInstance4perl(canonicalize_rays_X2_f16, perl::Canned< SparseVector< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(orthogonalize_affine_subspace_X2_f16, perl::Canned< SparseMatrix< PuiseuxFraction< Max, Rational, Rational >, NonSymmetric > >);
   FunctionInstance4perl(orthogonalize_affine_subspace_X2_f16, perl::Canned< Matrix< PuiseuxFraction< Max, Rational, Rational > > >);
   FunctionInstance4perl(canonicalize_rays_X2_f16, perl::Canned< Matrix< PuiseuxFraction< Max, Rational, Rational > > >);
   FunctionInstance4perl(orthogonalize_affine_subspace_X2_f16, perl::Canned< Matrix< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(canonicalize_rays_X2_f16, perl::Canned< Vector< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(canonicalize_rays_X2_f16, perl::Canned< Matrix< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(orthogonalize_subspace_X2_f16, perl::Canned< Matrix< Rational > >);
   FunctionInstance4perl(orthogonalize_subspace_X2_f16, perl::Canned< Matrix< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(orthogonalize_subspace_X2_f16, perl::Canned< Matrix< double > >);
   FunctionInstance4perl(orthogonalize_subspace_X2_f16, perl::Canned< SparseMatrix< Rational, NonSymmetric > >);
   FunctionInstance4perl(orthogonalize_subspace_X2_f16, perl::Canned< SparseMatrix< QuadraticExtension< Rational >, NonSymmetric > >);
   FunctionInstance4perl(orthogonalize_subspace_X2_f16, perl::Canned< Matrix< PuiseuxFraction< Max, Rational, Rational > > >);
   FunctionInstance4perl(orthogonalize_subspace_X2_f16, perl::Canned< Matrix< PuiseuxFraction< Min, Rational, Rational > > >);
   FunctionInstance4perl(orthogonalize_subspace_X2_f16, perl::Canned< SparseMatrix< double, NonSymmetric > >);
   FunctionInstance4perl(dehomogenize_X, perl::Canned< const SparseVector< double > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
