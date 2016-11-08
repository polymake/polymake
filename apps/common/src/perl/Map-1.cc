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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/client.h"
#include "polymake/Map.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0>
   FunctionInterface4perl( new, T0 ) {
      WrapperReturnNew(T0, () );
   };

   ClassTemplate4perl("Polymake::common::Map");
   Class4perl("Polymake::common::Map_A_Int_I_Int_Z", Map< int, int >);
   Class4perl("Polymake::common::Map_A_Rational_I_Int_Z", Map< Rational, int >);
   Class4perl("Polymake::common::Map_A_Vector__Float_I_Int_Z", Map< Vector< double >, int >);
   FunctionInstance4perl(new, Map< Vector< double >, int >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Vector< double >, int > >, perl::Canned< const Vector< double > >);
   FunctionInstance4perl(new, Map< int, int >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< int, int > >, int);
   FunctionInstance4perl(new, Map< Rational, int >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Rational, int > >, perl::Canned< const Rational >);
   Class4perl("Polymake::common::Map_A_Vector__Rational_I_Bool_Z", Map< Vector< Rational >, bool >);
   Class4perl("Polymake::common::Map_A_Vector__Rational_I_String_Z", Map< Vector< Rational >, std::string >);
   FunctionInstance4perl(new, Map< Vector< Rational >, bool >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Vector< Rational >, bool > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational>&>, pm::Series<int, true>> >);
   FunctionInstance4perl(new, Map< Vector< Rational >, std::string >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Vector< Rational >, std::string > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> >);
   Class4perl("Polymake::common::Map_A_Vector__Float_I_ARRAY_Z", Map< Vector< double >, perl::Array >);
   FunctionInstance4perl(new, Map< Vector< double >, perl::Array >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Vector< double >, perl::Array > >, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double> const&>, pm::Series<int, true>> const&, pm::Series<int, true>> >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Vector< double >, int > >, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double> const&>, pm::Series<int, true>> const&, pm::Series<int, true>> >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Vector< Rational >, bool > >, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Vector< Rational >, bool > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>> >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Vector< Rational >, std::string > >, perl::Canned< const pm::sparse_matrix_line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Rational, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > const&, pm::NonSymmetric> >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Integer, int > >, perl::Canned< const Integer >);
   Class4perl("Polymake::common::Map_A_Integer_I_Int_Z", Map< Integer, int >);
   FunctionInstance4perl(new, Map< Integer, int >);
   Class4perl("Polymake::common::Map_A_Rational_I_Rational_Z", Map< Rational, Rational >);
   FunctionInstance4perl(new, Map< Rational, Rational >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Vector< Rational >, std::string > >, perl::Canned< const pm::IndexedSlice<pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<pm::Rational> const&>, pm::Series<int, true>>, pm::Complement<pm::SingleElementSet<int>, int, pm::operations::cmp> const&> >);
   Class4perl("Polymake::common::Map_A_Vector__Float_I_String_Z", Map< Vector< double >, std::string >);
   Class4perl("Polymake::common::Map_A_Vector__Float_I_Bool_Z", Map< Vector< double >, bool >);
   FunctionInstance4perl(new, Map< Vector< double >, bool >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Vector< double >, bool > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double> const&>, pm::Series<int, true>> >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Vector< double >, bool > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double>&>, pm::Series<int, true>> >);
   FunctionInstance4perl(new, Map< Vector< double >, std::string >);
   OperatorInstance4perl(Binary_brk, perl::Canned< Map< Vector< double >, std::string > >, perl::Canned< const pm::IndexedSlice<pm::masquerade<pm::ConcatRows, pm::Matrix_base<double> const&>, pm::Series<int, true>> >);
   Class4perl("Polymake::common::Map_A_Set__Int_I_Int_Z", Map< Set< int >, int >);
   Class4perl("Polymake::common::Map_A_Vector__Rational_I_Array__Vector__Rational_Z", Map< Vector< Rational >, Array< Vector< Rational > > >);
   Class4perl("Polymake::common::Map_A_Vector__Rational_I_Matrix_A_Rational_I_NonSymmetric_Z_Z", Map< Vector< Rational >, Matrix< Rational > >);
   Class4perl("Polymake::common::Map_A_Vector__Rational_I_Rational_Z", Map< Vector< Rational >, Rational >);
   Class4perl("Polymake::common::Map_A_Vector__Integer_I_Rational_Z", Map< Vector< Integer >, Rational >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
