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

///==== this line controls the automatic file splitting: max.instances=60

#include "polymake/client.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/QuadraticExtension.h"

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

   Class4perl("Polymake::common::Integer", Integer);
   FunctionInstance4perl(new_X, Integer, int);
   FunctionInstance4perl(new, Integer);
   OperatorInstance4perl(Binary_add, perl::Canned< const Integer >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Integer >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Integer >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary_div, perl::Canned< const Integer >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary_mod, perl::Canned< const Integer >, perl::Canned< const Integer >);
   OperatorInstance4perl(UnaryAssign_inc, perl::Canned< Integer >);
   OperatorInstance4perl(UnaryAssign_dec, perl::Canned< Integer >);
   OperatorInstance4perl(Binary_lsh, perl::Canned< const Integer >, int);
   OperatorInstance4perl(Binary_rsh, perl::Canned< const Integer >, int);
   OperatorInstance4perl(Unary_neg, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary__lt, perl::Canned< const Integer >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary__gt, perl::Canned< const Integer >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Integer >, long);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Integer >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Integer >, long);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< Integer >, perl::Canned< const Integer >);
   OperatorInstance4perl(BinaryAssign_mul, perl::Canned< Integer >, perl::Canned< const Integer >);
   OperatorInstance4perl(BinaryAssign_sub, perl::Canned< Integer >, perl::Canned< const Integer >);
   OperatorInstance4perl(BinaryAssign_div, perl::Canned< Integer >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary_add, long, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary_sub, long, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Integer >, long);
   OperatorInstance4perl(Binary_mul, long, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary_div, perl::Canned< const Integer >, long);
   OperatorInstance4perl(Binary_div, long, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Integer >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Integer >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Integer >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary_div, perl::Canned< const Integer >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary__lt, perl::Canned< const Integer >, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary__gt, perl::Canned< const Integer >, perl::Canned< const Rational >);
   FunctionInstance4perl(new_X, Integer, double);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< pm::GMP::Proxy<(pm::GMP::proxy_kind)0, true> >, long);
   OperatorInstance4perl(BinaryAssign_add, perl::Canned< pm::GMP::Proxy<(pm::GMP::proxy_kind)1, true> >, long);
   OperatorInstance4perl(Binary__lt, int, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary__ne, perl::Canned< const Integer >, int);
   OperatorInstance4perl(Binary__eq, perl::Canned< const Integer >, int);
   FunctionInstance4perl(new_X, Integer, perl::Canned< const Rational >);
   OperatorInstance4perl(Binary__gt, perl::Canned< const Integer >, int);
   OperatorInstance4perl(Binary__ne, perl::Canned< const Integer >, perl::Canned< const Integer >);
   FunctionInstance4perl(new_X, Integer, perl::Canned< const Integer >);
   OperatorInstance4perl(assign, Integer, perl::Canned< const Rational >);
   OperatorInstance4perl(Unary_boo, perl::Canned< const Integer >);
   OperatorInstance4perl(BinaryAssign_mul, perl::Canned< Integer >, long);
   OperatorInstance4perl(Binary__ne, perl::Canned< const Integer >, perl::Canned< const Rational >);
   OperatorInstance4perl(BinaryAssign_sub, perl::Canned< Integer >, long);
   OperatorInstance4perl(Binary__eq, int, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary_mod, perl::Canned< const pm::sparse_elem_proxy<pm::sparse_proxy_base<pm::sparse2d::line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Integer, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > >, pm::unary_transform_iterator<pm::AVL::tree_iterator<pm::sparse2d::it_traits<pm::Integer, true, false>, (pm::AVL::link_index)1>, std::pair<pm::BuildUnary<pm::sparse2d::cell_accessor>, pm::BuildUnaryIt<pm::sparse2d::cell_index_accessor> > > >, pm::Integer, pm::NonSymmetric> >, perl::Canned< const pm::sparse_elem_proxy<pm::sparse_proxy_base<pm::sparse2d::line<pm::AVL::tree<pm::sparse2d::traits<pm::sparse2d::traits_base<pm::Integer, true, false, (pm::sparse2d::restriction_kind)0>, false, (pm::sparse2d::restriction_kind)0> > >, pm::unary_transform_iterator<pm::AVL::tree_iterator<pm::sparse2d::it_traits<pm::Integer, true, false>, (pm::AVL::link_index)1>, std::pair<pm::BuildUnary<pm::sparse2d::cell_accessor>, pm::BuildUnaryIt<pm::sparse2d::cell_index_accessor> > > >, pm::Integer, pm::NonSymmetric> >);
   OperatorInstance4perl(Binary_add, perl::Canned< const Integer >, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary_sub, perl::Canned< const Integer >, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary_mul, perl::Canned< const Integer >, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary_div, perl::Canned< const Integer >, perl::Canned< const QuadraticExtension< Rational > >);
   OperatorInstance4perl(Binary__lt, perl::Canned< const Integer >, int);
   OperatorInstance4perl(Binary__ge, perl::Canned< const Integer >, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary__ge, int, perl::Canned< const Integer >);
   OperatorInstance4perl(Binary__ge, perl::Canned< const Integer >, int);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
