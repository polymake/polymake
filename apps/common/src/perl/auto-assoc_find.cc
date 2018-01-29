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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/Array.h"
#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Map.h"
#include "polymake/Matrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/Vector.h"
#include "polymake/client.h"
#include "polymake/hash_map"
#include "polymake/list"
#include "polymake/perl/assoc.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( assoc_find_X32_X, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (pm::perl::find_element(arg0.get<T0>(), arg1.get<T1>())), arg0 );
   };

   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const Map< Vector< double >, int > >, perl::Canned< const Vector< double > >);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const Map< Set< int >, Vector< Rational > > >, perl::Canned< const pm::incidence_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::nothing, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&> >);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const Map< Vector< double >, perl::Array > >, perl::Canned< const Vector< double > >);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const NodeHashMap< Undirected, bool > >, int);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const Map< Vector< Rational >, int > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>, mlist<> > >);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const NodeHashMap< Directed, bool > >, int);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const EdgeHashMap< Directed, bool > >, int);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const Map< int, int > >, int);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const Map< std::pair< int, int >, int > >, perl::Canned< const std::pair< int, int > >);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const Map< Vector< double >, int > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double> const&>, pm::Series<int, true>, mlist<> > >);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const Map< std::string, std::string > >, std::string);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const hash_map< Set< int >, int > >, perl::Canned< const Set< int > >);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const Map< int, std::list< int > > >, int);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const Map< int, std::pair< int, int > > >, int);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const hash_map< SparseVector< int >, Rational > >, perl::Canned< const SparseVector< int > >);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const Map< Set< int >, Integer > >, perl::Canned< const Set< int > >);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const hash_map< Vector< QuadraticExtension< Rational > >, int > >, perl::Canned< const Vector< QuadraticExtension< Rational > > >);
   FunctionInstance4perl(assoc_find_X32_X, perl::Canned< const hash_map< Vector< Rational >, int > >, perl::Canned< const Vector< Rational > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
