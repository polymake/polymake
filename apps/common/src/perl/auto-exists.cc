/* Copyright (c) 1997-2014
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

#include "polymake/client.h"
#include "polymake/Map.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/hash_set"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Integer.h"
#include "polymake/Graph.h"
#include "polymake/SparseMatrix.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( exists_X_f1, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( arg0.get<T0>().exists(arg1.get<T1>()) );
   };

   FunctionInstance4perl(exists_X_f1, perl::Canned< const Map< Vector< double >, int > >, perl::Canned< const Vector< double > >);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const hash_set< Vector< Rational > > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const Map< Vector< Rational >, bool > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, void> >);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const Set< int > >, int);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const Map< Vector< double >, int > >, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double> const&>, pm::Series<int, true>, void> const&, pm::Series<int, true>, void> >);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >, int);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const Set< Array< int > > >, perl::TryCanned< const Array< int > >);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const Set< Set< int > > >, perl::Canned< const Set< int > >);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const Set< Vector< Rational > > >, perl::Canned< const Vector< Integer > >);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::graph::traits_base<pm::graph::Undirected, false, (pm::sparse2d::restriction_kind)0>, true, (pm::sparse2d::restriction_kind)0> > > >, int);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> >&> >, int);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const Map< Vector< Rational >, bool > >, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const Set< int > >, perl::Canned< const Integer >);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const Map< Vector< Rational >, bool > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>, void> >);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const Set< Vector< Integer > > >, perl::Canned< const Vector< Integer > >);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const Set< Vector< Rational > > >, perl::Canned< const Vector< Rational > >);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const Map< Vector< double >, bool > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double>&>, pm::Series<int, true>, void> >);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const Set< Vector< Rational > > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, void> >);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const Set< std::string > >, std::string);
   FunctionInstance4perl(exists_X_f1, perl::Canned< const Map< Vector< double >, bool > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double> const&>, pm::Series<int, true>, void> >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
