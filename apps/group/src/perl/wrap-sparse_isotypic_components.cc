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

namespace polymake { namespace group { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   FunctionWrapper4perl( pm::Array<pm::hash_map<pm::Set<int, pm::operations::cmp>, pm::Rational>> (perl::Object const&, perl::Object const&, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0, arg1, arg2 );
   }
   FunctionWrapperInstance4perl( pm::Array<pm::hash_map<pm::Set<int, pm::operations::cmp>, pm::Rational>> (perl::Object const&, perl::Object const&, int) );

   FunctionWrapper4perl( pm::Array<pm::hash_map<pm::Bitset, pm::Rational>> (perl::Object const&, perl::Object const&, int) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0, arg1, arg2 );
   }
   FunctionWrapperInstance4perl( pm::Array<pm::hash_map<pm::Bitset, pm::Rational>> (perl::Object const&, perl::Object const&, int) );

   FunctionWrapper4perl( bool (perl::Object, pm::Array<pm::hash_map<pm::Bitset, pm::Rational>> const&, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]);
      IndirectWrapperReturn( arg0, arg1.get< perl::TryCanned< const Array< hash_map< pm::Bitset, Rational > > > >(), arg2 );
   }
   FunctionWrapperInstance4perl( bool (perl::Object, pm::Array<pm::hash_map<pm::Bitset, pm::Rational>> const&, perl::OptionSet) );
   FunctionWrapper4perl( pm::Array<pm::hash_map<pm::Bitset, pm::Rational>> (perl::Object const&, perl::Object const&, int, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      IndirectWrapperReturn( arg0, arg1, arg2, arg3 );
   }
   FunctionWrapperInstance4perl( pm::Array<pm::hash_map<pm::Bitset, pm::Rational>> (perl::Object const&, perl::Object const&, int, perl::OptionSet) );

   FunctionWrapper4perl( pm::Array<int> (pm::Array<pm::Array<int>> const&, pm::Set<int, pm::operations::cmp> const&) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      IndirectWrapperReturn( arg0.get< perl::TryCanned< const Array< Array< int > > > >(), arg1.get< perl::TryCanned< const Set< int > > >() );
   }
   FunctionWrapperInstance4perl( pm::Array<int> (pm::Array<pm::Array<int>> const&, pm::Set<int, pm::operations::cmp> const&) );

   FunctionWrapper4perl( pm::hash_set<pm::Bitset> (perl::Object const&, perl::Object const&, int, perl::OptionSet) ) {
      perl::Value arg0(stack[0]), arg1(stack[1]), arg2(stack[2]), arg3(stack[3]);
      IndirectWrapperReturn( arg0, arg1, arg2, arg3 );
   }
   FunctionWrapperInstance4perl( pm::hash_set<pm::Bitset> (perl::Object const&, perl::Object const&, int, perl::OptionSet) );

   template <typename T0, typename T1, typename T2>
   FunctionInterface4perl( span_same_subspace_T_X_X, T0,T1,T2 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (span_same_subspace<T0>(arg0.get<T1>(), arg1.get<T2>())) );
   };

   FunctionInstance4perl(span_same_subspace_T_X_X, pm::Bitset, perl::Canned< const pm::Array< pm::hash_map< pm::Bitset, pm::Rational > > >, perl::Canned< const pm::Array< pm::hash_map< pm::Bitset, pm::Rational > > >);

///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
