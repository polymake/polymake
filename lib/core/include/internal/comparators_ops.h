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

#ifndef POLYMAKE_INTERNAL_COMPARATORS_OPS_H
#define POLYMAKE_INTERNAL_COMPARATORS_OPS_H

#include <cmath>
#include <cstdlib>

#include "polymake/internal/comparators_basic_defs.h"
#include "polymake/internal/converters_basic_defs.h"
#include "polymake/internal/operations.h"

namespace pm {

template <>
struct spec_object_traits<double> :
   spec_object_traits< cons<double, int2type<object_classifier::is_scalar> > > {
public:
   static double global_epsilon;
   static bool is_zero(double x) { return abs(x) <= global_epsilon; }
};

template <>
struct spec_object_traits<float> :
   spec_object_traits< cons<float, int2type<object_classifier::is_scalar> > > {
public:
   static bool is_zero(float x) { return abs(x) <= spec_object_traits<double>::global_epsilon; }
};

class local_epsilon_keeper {
   mutable double saved;
public:
   local_epsilon_keeper(double new_val)
      : saved(spec_object_traits<double>::global_epsilon)
   {
      spec_object_traits<double>::global_epsilon=new_val;
   }

   local_epsilon_keeper(const local_epsilon_keeper& le)
      : saved(le.saved)
   {
      le.saved=spec_object_traits<double>::global_epsilon;
   }

   ~local_epsilon_keeper()
   {
      spec_object_traits<double>::global_epsilon=saved;
   }
};

inline
local_epsilon_keeper local_epsilon(double new_val)
{
   return local_epsilon_keeper(new_val);
}

namespace operations {

template <typename OpRef>
struct positive {
   typedef OpRef argument_type;
   typedef bool result_type;
   result_type operator() (typename function_argument<OpRef>::const_type x) const
   {
      return pm::sign(x)==cmp_gt;
   }
};

template <typename OpRef>
struct negative {
   typedef OpRef argument_type;
   typedef bool result_type;
   result_type operator() (typename function_argument<OpRef>::const_type x) const
   {
      return pm::sign(x)==cmp_lt;
   }
};

template <typename OpRef>
struct non_zero {
   typedef OpRef argument_type;
   typedef bool result_type;
   result_type operator() (typename function_argument<OpRef>::const_type x) const
   {
      return !is_zero(x);
   }
};

template <typename OpRef>
struct equals_to_zero {
   typedef OpRef argument_type;
   typedef bool result_type;
   result_type operator() (typename function_argument<OpRef>::const_type x) const
   {
      return is_zero(x);
   }
};

template <typename T1, typename T2, typename ComparatorFamily,
          typename Model1=typename object_traits<T1>::model, typename Model2=typename object_traits<T2>::model,
          typename _discr=typename isomorphic_types<T1,T2>::discriminant,
          bool _enabled=isomorphic_types<T1,T2>::value>
struct build_comparator;

template <typename ComparatorFamily>
struct generic_comparator : incomplete {

   template <typename Left, typename Right>
   cmp_value operator()(const Left& l, const Right& r) const
   {
      return typename build_comparator<Left, Right, ComparatorFamily>::type()(l, r);
   }

   template <typename Left, typename Iterator2>
   cmp_value operator() (typename enable_if<partial_left, build_comparator<Left, Left, ComparatorFamily>::partially_defined>::type,
                         const Left& l, const Iterator2& r) const
   {
      return typename build_comparator<Left, Left, ComparatorFamily>::type()(partial_left(), l, r);
   }

   template <typename Right, typename Iterator1>
   cmp_value operator() (typename enable_if<partial_right, build_comparator<Right, Right, ComparatorFamily>::partially_defined>::type,
                         const Iterator1& l, const Right& r) const
   {
      return typename build_comparator<Right, Right, ComparatorFamily>::type()(partial_right(), l, r);
   }
};

struct cmp : generic_comparator<cmp> {};

struct cmp_with_leeway : generic_comparator<cmp_with_leeway> {};

template <typename T1, typename T2>
struct build_comparator<T1, T2, cmp, is_scalar, is_scalar, cons<is_scalar, is_scalar>, true> {
   typedef cmp_scalar<T1,T2> type;
   static const bool partially_defined=true;
};

template <typename T1, typename T2>
struct build_comparator<T1, T2, cmp_with_leeway, is_scalar, is_scalar, cons<is_scalar, is_scalar>, true> {
   typedef cmp_scalar_with_leeway<T1,T2> type;
   static const bool partially_defined=true;
};

template <typename T, typename Tag>
struct build_comparator<T, T, cmp, is_opaque, is_opaque, cons<Tag,Tag>, true> {
   typedef cmp_opaque<T> type;
   static const bool partially_defined=is_partially_defined<type>::value;
};

template <typename T>
struct build_comparator<T*, T*, cmp, is_not_object, is_not_object, pm::cons<pm::is_not_object, pm::is_not_object>, true> {
   typedef cmp_pointer<T> type;
   static const bool partially_defined=false;
};

template <typename Comparator, typename LeftRef, typename RightRef, cmp_value good>
struct cmp_adapter {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef bool result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type a,
                           typename function_argument<RightRef>::const_type b) const
   {
      Comparator cmp_op;
      return cmp_op(a,b) == good;
   }
};

template <typename Comparator, typename T, typename T2=T>
struct cmp2eq : cmp_adapter<Comparator, T, T2, cmp_eq> {};

template <typename LeftRef, typename RightRef>
struct eq : cmp2eq<cmp, LeftRef, RightRef> {};

template <typename LeftRef, typename RightRef>
struct lt : cmp_adapter<cmp, LeftRef, RightRef, cmp_lt> {};

template <typename LeftRef, typename RightRef>
struct gt : cmp_adapter<cmp, LeftRef, RightRef, cmp_gt> {};

template <typename LeftRef, typename RightRef>
struct ne : composed21<eq<LeftRef, RightRef>, std::logical_not<bool> > {};

template <typename LeftRef, typename RightRef>
struct le : composed21<gt<LeftRef, RightRef>, std::logical_not<bool> > {};

template <typename LeftRef, typename RightRef>
struct ge : composed21<lt<LeftRef, RightRef>, std::logical_not<bool> > {};

template <typename Left, typename Right,
          typename TagL=typename object_traits<Left>::generic_tag,
          typename TagR=typename object_traits<Right>::generic_tag,
          bool _is_numeric=std::numeric_limits<Left>::is_specialized && std::numeric_limits<Right>::is_specialized>
struct minmax : sub_result<Left,Right> {};

template <typename Left, typename Right, typename Tag1, typename Tag2>
struct minmax<Left,Right,Tag1,Tag2,false> : least_derived< cons<Left,Right> > {};

template <typename Left, typename Right, typename Tag>
struct minmax<Left,Right,Tag,Tag,false> : identical<typename object_traits<Left>::persistent_type, typename object_traits<Right>::persistent_type> {};

template <typename T, typename Tag>
struct minmax<T,T,Tag,Tag,false> {
   typedef T type;
};

template <typename LeftRef, typename RightRef>
struct max {
   typedef typename deref<LeftRef>::type first_argument_type;
   typedef typename deref<RightRef>::type second_argument_type;
   typedef typename minmax<first_argument_type, second_argument_type>::type
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type a,
                           typename function_argument<RightRef>::const_type b) const
   {
      if (a>=b) return a;
      return b;
   }

   void assign(typename lvalue_arg<LeftRef>::type a, typename function_argument<RightRef>::const_type b) const
   {
      if (a<b) a=b;
   }
};

template <typename LeftRef, typename RightRef>
struct min {
   typedef typename deref<LeftRef>::type first_argument_type;
   typedef typename deref<RightRef>::type second_argument_type;
   typedef typename minmax<first_argument_type, second_argument_type>::type
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type a,
                           typename function_argument<RightRef>::const_type b) const
   {
      if (a<=b) return a;
      return b;
   }

   void assign(typename lvalue_arg<LeftRef>::type a, typename function_argument<RightRef>::const_type b) const
   {
      if (a>b) a=b;
   }
};

template <typename OpRef>
struct abs_value {
   typedef OpRef argument_type;
   typedef typename deref<OpRef>::type result_type;

   result_type operator() (typename function_argument<OpRef>::const_type a) const
   {
      return abs(a);
   }

   void assign(typename lvalue_arg<OpRef>::type a) const
   {
      if (negative<OpRef>()(a)) negate(a);
   }
};

} // end namespace operations

// Tag for various parameter lists
template <typename> class Comparator;

template <typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder<operations::cmp, Iterator1, Iterator2, Reference1, Reference2> :
   empty_op_builder<typename operations::build_comparator<typename deref<Reference1>::type, typename deref<Reference2>::type, operations::cmp>::type> {};

template <typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder<operations::cmp_with_leeway, Iterator1, Iterator2, Reference1, Reference2> :
   empty_op_builder<typename operations::build_comparator<typename deref<Reference1>::type, typename deref<Reference2>::type, operations::cmp_with_leeway>::type> {};

} // end namespace pm

#endif // POLYMAKE_INTERNAL_COMPARATORS_OPS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
