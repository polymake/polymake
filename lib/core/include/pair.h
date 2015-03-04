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

#ifndef POLYMAKE_PAIR_H
#define POLYMAKE_PAIR_H

#include "polymake/internal/comparators_basic_defs.h"
#include "polymake/internal/nothing.h"
#include <utility>

namespace pm {
using std::pair;

template <typename U1, typename U2, typename Target, bool _whole=isomorphic_types<pair<U1,U2>, Target>::value>
struct pair_chooser {
   typedef const pair<U1,U2>& first_type;
   typedef first_type second_type;
   static first_type first(first_type u) { return u; }
   static first_type second(first_type u) { return u; }
};

template <typename U1, typename U2, typename Target>
struct pair_chooser<U1, U2, Target, false> {
   typedef typename attrib<U1>::plus_const_ref first_type;
   typedef typename attrib<U2>::plus_const_ref second_type;
   static first_type first(const pair<U1,U2>& u) { return u.first; }
   static second_type second(const pair<U1,U2>& u) { return u.second; }
};

template <typename Target, typename U1, typename U2> inline
typename pair_chooser<U1, U2, Target>::first_type
pair_init_first(const pair<U1,U2>& u)
{
   return pair_chooser<U1, U2, Target>::first(u);
}

template <typename Target, typename U1, typename U2> inline
typename pair_chooser<U1, U2, Target>::first_type
pair_init_second(const pair<U1,U2>& u)
{
   return pair_chooser<U1, U2, Target>::second(u);
}

template <typename Data, typename NonPair>
struct isomorphic_to_first : False {};

template <typename Data, typename T1, typename T2>
struct isomorphic_to_first< Data, pair<T1,T2> > : isomorphic_types<typename deref<T1>::type, Data> {};

template <typename Data, typename NonPair>
struct isomorphic_to_second : False {};

template <typename Data, typename T1, typename T2>
struct isomorphic_to_second< Data, pair<T1,T2> > : isomorphic_types<typename deref<T2>::type, Data> {};
}
namespace std {

template <typename T1, typename T2>
struct pair<T1&, T2> {
   typedef T1& first_type;
   typedef T2 second_type;
   /// data types referred to by the components
   typedef T1 first_data_type;
   typedef T2 second_data_type;

   first_type first;
   second_type second;

   pair(T1& first_arg, const T2& second_arg) : first(first_arg), second(second_arg) {}

   template <typename U1, typename U2>
   explicit pair(const pair<U1&,U2>& p) : first(p.first), second(p.second) {}
};

template <typename T1, typename T2>
struct pair<T1, T2&> {
   typedef T1 first_type;
   typedef T2& second_type;

   typedef T1 first_data_type;
   typedef T2 second_data_type;

   first_type first;
   second_type second;

   pair(const T1& first_arg, T2& second_arg) : first(first_arg), second(second_arg) {}

   template <typename U1, typename U2>
   explicit pair(const pair<U1,U2&>& p) : first(p.first), second(p.second) {}
};

template <typename T1, typename T2>
struct pair<T1&, T2&> {
   typedef T1& first_type;
   typedef T2& second_type;

   typedef T1 first_data_type;
   typedef T2 second_data_type;

   first_type first;
   second_type second;

   pair(T1& first_arg, T2& second_arg) : first(first_arg), second(second_arg) {}

   template <typename U1, typename U2>
   explicit pair(const pair<U1&,U2&>& p) : first(p.first), second(p.second) {}
};

template <typename T1>
struct pair<T1, pm::nothing> {
   typedef T1 first_type;
   typedef pm::nothing second_type;

   typedef T1 first_data_type;
   typedef pm::nothing second_data_type;

   first_type first;
   static pm::nothing second;

   pair() {}

   explicit pair(const T1& first_arg) : first(first_arg) {}

   pair(const T1& first_arg, const pm::nothing&) : first(first_arg) {}

   template <typename U1, typename U2>
   explicit pair(const pair<U1,U2>& u) : first(pm::pair_init_first<T1>(u)) {}
};

template <typename T1> pm::nothing pair<T1,pm::nothing>::second;

template <typename T1>
struct pair<T1&, pm::nothing> {
   typedef T1& first_type;
   typedef pm::nothing second_type;

   typedef T1 first_data_type;
   typedef pm::nothing second_data_type;

   first_type first;
   static pm::nothing second;

   explicit pair(T1& first_arg) : first(first_arg) {}

   pair(T1& first_arg, const pm::nothing&) : first(first_arg) {}

   template <typename U1, typename U2>
   explicit pair(const pair<U1&,U2>& u) : first(pm::pair_init_first<T1>(u)) {}
};

template <typename T1> pm::nothing pair<T1&, pm::nothing>::second;

template <typename T2>
struct pair<pm::nothing, T2> {
   typedef pm::nothing first_type;
   typedef T2 second_type;

   typedef pm::nothing first_data_type;
   typedef T2 second_data_type;

   static pm::nothing first;
   second_type second;

   pair() {}

   explicit pair(const T2& second_arg) : second(second_arg) {}

   pair(const pm::nothing&, const T2& second_arg) : second(second_arg) {}

   template <typename U1, typename U2>
   explicit pair(const pair<U1,U2>& u) : second(pm::pair_init_second<T2>(u)) {}
};

template <typename T2> pm::nothing pair<pm::nothing, T2>::first;

template <typename T2>
struct pair<pm::nothing, T2&> {
   typedef pm::nothing first_type;
   typedef T2& second_type;

   typedef pm::nothing first_data_type;
   typedef T2 second_data_type;

   static pm::nothing first;
   second_type second;

   explicit pair(T2& second_arg) : second(second_arg) {}

   pair(const pm::nothing&, T2& second_arg) : second(second_arg) {}

   template <typename U1, typename U2>
   explicit pair(const pair<U1,U2&>& u) : second(pm::pair_init_second<T2>(u)) {}
};

template <typename T2> pm::nothing pair<pm::nothing, T2&>::first;

template <>
struct pair<pm::nothing, pm::nothing> {
   typedef pm::nothing first_type;
   typedef pm::nothing second_type;

   typedef pm::nothing first_data_type;
   typedef pm::nothing second_data_type;

   static pm::nothing first, second;

   pair() {}
   explicit pair(const pm::nothing&) {}
   pair(const pm::nothing&, const pm::nothing&) {}

   template <typename U1, typename U2>
   explicit pair(const pair<U1,U2>&) {}
};

} // end namespace std

namespace pm {

template <typename T1, typename T2>
struct spec_object_traits< pair<T1,T2> > : spec_object_traits<is_composite> {
   typedef cons<T1,T2> elements;

   template <typename Me, typename Visitor>
   static void visit_elements(Me& x, Visitor& v)
   {
      v << x.first << x.second;
   }
};

namespace operations {

template <typename T1, typename T2>
struct pair_maker {
   typedef T1 first_argument_type;
   typedef T2 second_argument_type;
   typedef pair<T1,T2> result_type;

   result_type operator() (typename function_argument<T1>::type first_arg,
                           typename function_argument<T2>::type second_arg) const
   {
      return result_type(first_arg,second_arg);
   }
};

template <typename PairRef>
struct take_first {
   typedef PairRef argument_type;
   typedef typename inherit_ref<typename deref<PairRef>::type::first_type, PairRef>::type result_type;

   result_type operator() (typename function_argument<PairRef>::type arg) const { return arg.first; }
};

template <typename PairRef>
struct take_second {
   typedef PairRef argument_type;
   typedef typename inherit_ref<typename deref<PairRef>::type::second_type, PairRef>::type result_type;

   result_type operator() (typename function_argument<PairRef>::type arg) const { return arg.second; }
};

} // end namespace operations

template <typename Container> inline
pair<typename Container::const_reference, typename Container::const_reference>
front_pair(const Container& c)
{
   typename Container::const_iterator it=c.begin();
   typename Container::const_reference e1=*it, e2=*(++it);
   return pair<typename Container::const_reference, typename Container::const_reference>(e1,e2);
}

template <typename Container> inline
pair<typename Container::const_reference, typename Container::const_reference>
back_pair(const Container& c)
{
   typename Container::const_reverse_iterator it=c.rbegin();
   typename Container::const_reference e1=*it, e2=*(++it);
   return pair<typename Container::const_reference, typename Container::const_reference>(e1,e2);
}

} // end namespace pm

#endif // POLYMAKE_PAIR_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
