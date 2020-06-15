/* Copyright (c) 1997-2020
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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

#ifndef POLYMAKE_INTERNAL_TYPE_MANIP_H
#define POLYMAKE_INTERNAL_TYPE_MANIP_H

#include "polymake/internal/pool_allocator.h"
#include "polymake/meta_list.h"
#include "polymake/type_utils.h"

#include <new>
#include <memory>
#include <limits>
#include <cstddef>

#if defined(__GLIBCXX__) || defined(_LIBCPP_VERSION)
#  define POLYMAKE_ALIGN(what,n) what __attribute__ ((aligned (n)))
#elif defined(__INTEL_COMPILER)
#  define POLYMAKE_ALIGN(what,n) __declspec(align(n)) what
#endif

#if defined(__clang__) && __clang_major__ >= (defined(__APPLE__) ? 8 : 4)
# define PmNoSanitize(issue) __attribute__((no_sanitize(#issue)))
#else
# define PmNoSanitize(issue)
#endif

namespace pm {

// For the time of transition to a proper C++14 implementation
using polymake::int_constant;
using polymake::size_constant;
using polymake::bool_constant;
using polymake::char_constant;
using polymake::mselect;
using polymake::mlist_and;
using polymake::mlist_or;
using polymake::bool_not;
using polymake::mvalue_of;
using polymake::mis_equal;
using polymake::mis_less;
using polymake::mis_greater;
using polymake::mminimum;
using polymake::mmaximum;
using polymake::madd;
using polymake::msubtract;
using polymake::mlist_and_nonempty;
using polymake::is_instance_of;
using polymake::is_derived_from_instance_of;
using polymake::least_derived_class;
using polymake::most_derived_class;
using polymake::mprefer1st;
using polymake::mprefer2nd;
using polymake::mproject1st;
using polymake::mproject2nd;
using polymake::accept_valid_type;
using polymake::accept_valid_template;
using polymake::mlist;
using polymake::mlist_head;
using polymake::mlist_tail;
using polymake::mlist_length;
using polymake::mlist_is_empty;
using polymake::mlist_wrap;
using polymake::mlist_unwrap;
using polymake::mlist_at;
using polymake::mlist_at_rev;
using polymake::mlist_concat;
using polymake::mlist_contains;
using polymake::mlist_slice;
using polymake::mlist_even_subset;
using polymake::mlist_odd_subset;
using polymake::mlist_prepend_if;
using polymake::mlist_append_if;
using polymake::mlist_find_if;
using polymake::mlist_find;
using polymake::mlist_remove_duplicates;
using polymake::mlist_flatten;
using polymake::mlist_reverse;
using polymake::mlist_fold;
using polymake::mlist_fold_transform;
using polymake::mlist_is_included;
using polymake::mlist_match;
using polymake::mlist_match_all;
using polymake::mlist_union;
using polymake::mlist_intersection;
using polymake::mlist_difference;
using polymake::mlist_symdifference;
using polymake::mlist_replace;
using polymake::mlist_remove;
using polymake::mlist_insert;
using polymake::mlist_insert_unique;
using polymake::mlist_filter_unary;
using polymake::mlist_filter_binary;
using polymake::mlist_transform_unary;
using polymake::mlist_transform_binary;
using polymake::mrepeat;
using polymake::mreplicate;
using polymake::mlist_sort;
using polymake::muntag_t;
using polymake::mtagged_list_extract;
using polymake::tagged_list_extract_integral;
using polymake::mtagged_list_replace;
using polymake::mtagged_list_add_default;
using polymake::mtagged_list_remove;
using polymake::is_among;
using polymake::is_derived_from;
using polymake::is_derived_from_any;
using polymake::can_construct_any;
using polymake::is_instance_of_any;
using polymake::can_assign_to;
using polymake::forward_arg_of_type;
using polymake::forward_arg_derived_from;
using polymake::forward_arg_satisfying;
using polymake::forward_all_args_of_type;
using polymake::forward_all_args_derived_from;
using polymake::forward_all_args_satisfying;
using polymake::is_direct_constructible;
using polymake::is_lossless_convertible;
using polymake::mget_template_parameter;
using polymake::mset_template_parameter;
using polymake::mreplace_template_parameter;
using polymake::mget_func_signature;
using polymake::func_args_t;
using polymake::func_return_t;
using polymake::msafely_eval;
using polymake::mbind_const;
using polymake::mbind1st;
using polymake::mbind2nd;
using polymake::mswap_args;
using polymake::mnegate_unary;
using polymake::mnegate_binary;
using polymake::legible_typename;
using polymake::pure_type_t;
using polymake::same_pure_type;
using polymake::is_class_or_union;
using polymake::remove_unsigned_t;
using polymake::inherit_signed;
using polymake::inherit_signed_t;
using polymake::is_const;
// not yet! using polymake::add_const;
using polymake::add_const_t;
// not yet! using polymake::inherit_const;
using polymake::mlist_pure_types;
using polymake::is_class_or_union;
using polymake::inherit_const_t;
using polymake::inherit_reference;
using polymake::inherit_reference_t;
using polymake::private_mutable_t;
using polymake::ensure_private_mutable;
using polymake::reverse_cast;
using polymake::is_comparable;
using polymake::are_comparable;
using polymake::is_less_greater_comparable;
using polymake::are_less_greater_comparable;
using polymake::mlist2tuple;
using polymake::tuple2mlist;
using polymake::index_sequence_for;
using polymake::make_index_sequence_for;
using polymake::make_reverse_index_sequence;
using polymake::foreach_in_tuple;

using polymake::Int;

template <typename Class, typename Member, Member Class::*Ptr>
struct ptr2type {
   typedef Member Class::* constant_type;
};

template <typename T>
struct type2type {
   typedef T type;
};

/** Definition of the input argument type

    For a given type, a suitable type of the function input argument is defined.
    Objects of user-defined classes should be passed as a const reference, while
    built-in types are more efficiently passed per value.
*/
template <typename T>
struct function_argument {
   typedef typename std::conditional<(std::is_pod<T>::value && sizeof(T)<=sizeof(double)), const T, const T&>::type type;
   typedef type const_type;
};

template <typename T>
struct function_argument<T&> {
   typedef T& type;
   typedef const T& const_type;
};

template <typename T>
struct function_argument<const T&> : function_argument<T&> {
   typedef const T& type;
};

template <>
struct function_argument<void> {
   typedef void* type;
   typedef void* const_type;
};

/** Removal of the pointer/reference modifier

    For a given pointer/reference type, the data type being referred to is revealed.
    For the sake of completeness, non-reference types yield themselves as result.
*/
template <typename C> struct attrib {
   typedef C minus_const_ref;
   typedef C minus_const;
   typedef C minus_ref;
   typedef const C& plus_const_ref;
   typedef const C plus_const;
   typedef C& plus_ref;
   static const bool is_reference=false, is_const=false, is_rvref=false;
};
template <>
struct attrib<void> {
   typedef void minus_const_ref;
   typedef void minus_const;
   typedef void minus_ref;
   typedef void plus_const_ref;
   typedef void plus_const;
   typedef void plus_ref;
   static const bool is_reference=false, is_const=false;
};
template <typename C>
struct attrib<const C> : attrib<C> {
   typedef const C minus_ref;
   typedef const C& plus_ref;
   static const bool is_const=true;
};
template <typename C>
struct attrib<C&> : attrib<C> {
   typedef typename attrib<C>::plus_const& plus_const;
   typedef typename attrib<C>::minus_const& minus_const;
   static const bool is_reference=true;
};
template <typename C>
struct attrib<C&&> : attrib<C> {
   typedef typename attrib<C>::plus_const&& plus_const;
   typedef typename attrib<C>::minus_const&& minus_const;
   static const bool is_reference=true, is_rvref=true;
};

template <typename C>
struct deref : attrib<C> {
   typedef typename attrib<C>::minus_const_ref type;
};

template <typename T>
struct add_const {
   typedef typename deref<T>::plus_const type;
};

template <typename T, typename From>
struct inherit_const {
   typedef typename std::conditional<deref<From>::is_const, typename deref<T>::plus_const, T>::type type;
};

template <typename T, typename From>
struct inherit_ref {
   typedef typename inherit_const<typename deref<T>::minus_ref, From>::type QT;
   typedef typename mselect< std::enable_if<deref<T>::is_rvref || deref<From>::is_rvref, QT&&>,
                             std::enable_if<deref<T>::is_reference || deref<From>::is_reference, QT&>,
                             QT >::type type;
};

template <typename T, typename From>
struct inherit_ref_norv {
   typedef typename inherit_const<typename deref<T>::minus_ref, From>::type QT;
   typedef typename mselect< std::enable_if<deref<T>::is_rvref || deref<From>::is_rvref, QT&>,
                             std::enable_if<deref<T>::is_reference || deref<From>::is_reference, QT&>,
                             QT >::type type;
};

template <typename T, bool make_const>
struct assign_const {
   typedef typename std::conditional<make_const, typename deref<T>::plus_const, typename deref<T>::minus_const>::type type;
};

template <typename T, bool make_ref>
struct assign_ref {
   typedef typename std::conditional<make_ref, typename attrib<T>::plus_ref, typename attrib<T>::minus_ref>::type type;
};

template <typename Head, typename Tail>
struct cons {
   typedef Head head;
   typedef Head type;
   typedef Tail tail;
};

template <typename T>
struct list_head {
   typedef T type;
};
template <typename Head, typename Tail>
struct list_head< cons<Head, Tail> > {
   typedef Head type;
};

template <typename T>
struct list_tail {
   typedef void type;
};
template <typename Head, typename Tail>
struct list_tail< cons<Head, Tail> > {
   typedef Tail type;
};

}
namespace polymake {

template <typename Head, typename Tail>
struct mlist_wrap< pm::cons<Head, Tail> >
   : mlist_concat<Head, typename mlist_wrap<Tail>::type> {};

}
namespace pm {

template <typename> struct same {};

template <typename List>
struct list_length {
   static const int value=!std::is_same<List, void>::value;
};

template <typename Head, typename Tail>
struct list_length< cons<Head,Tail> > {
   static const int value= list_length<Head>::value + list_length<Tail>::value;
};

template <typename List, int n> struct n_th;

template <typename T>
struct n_th<T,0> {
   typedef T type;
   typedef void tail;
   typedef T list;
};

template <typename Head, typename Tail, int n>
struct n_th<cons<Head, Tail>, n> : n_th<Tail, n-1> {};

template <typename Head, typename Tail>
struct n_th<cons<Head, Tail>, 0> {
   typedef Head type;
   typedef Tail tail;
   typedef cons<Head, Tail> list;
};


template <size_t n>
struct size_discriminant {
   typedef char (&type)[n];
};

template <typename T1, typename T2,
          bool are_objects=(is_class_or_union<pure_type_t<T1>>::value &&
                            is_class_or_union<pure_type_t<T2>>::value),
          bool are_arithmetic=(std::is_arithmetic<pure_type_t<T1>>::value &&
                               std::is_arithmetic<pure_type_t<T2>>::value)>
struct compatible_helper
   : std::false_type {
   using type = void;
};

template <typename T1, typename T2>
struct compatible_helper<T1, T2, true, false> {
   using common_base = typename least_derived_class<typename deref<T1>::type, typename deref<T2>::type>::type;
   static constexpr bool value=!std::is_same<common_base, void>::value;
   using value_type = typename assign_const<common_base, value && (is_const<T1>::value || is_const<T2>::value)>::type;
   using type = typename assign_ref<value_type, value && std::is_reference<T1>::value && std::is_reference<T2>::value>::type;
};

template <typename T1, typename T2>
struct compatible_helper<T1, T2, false, true>
   : std::true_type {
   static const bool keep_attribs=std::is_same<pure_type_t<T1>, pure_type_t<T2>>::value;
   using common_type = typename std::common_type<pure_type_t<T1>, pure_type_t<T2>>::type;
   using value_type = typename assign_const<common_type, keep_attribs && (is_const<T1>::value || is_const<T2>::value)>::type;
   using type = typename assign_ref<value_type, keep_attribs && std::is_reference<T1>::value && std::is_reference<T2>::value>::type;
};

template <typename T1, typename T2>
struct compatible : compatible_helper<T1, T2> {};

template <typename T1, typename T2>
struct compatible<T1*, T2*> : compatible<T1, T2> {
   using type = typename compatible<T1,T2>::type*;
};

template <typename T1, typename T2>
void assert_overloaded(T1, T2)
{
   static_assert(!std::is_same<T1, T2>::value, "method must be overloaded");
}

#define AssertOVERLOADED(base, child, method) \
   if (false) assert_overloaded(&base::method, &child::method)


/* ---------------------------------------------
 *  Building blocks for flexible traits classes
 * --------------------------------------------- */

struct list;

template <typename T>
struct list2cons {
   typedef T type;
   typedef list return_type;
};

template <typename R>
struct list2cons< R() > {
   typedef void type;
   typedef R return_type;
};

template <typename R, typename T>
struct list2cons< R(T) > {
   typedef T type;
   typedef R return_type;
};

template <typename R, typename List>
struct list2tail : list2cons<List> {};

template <typename List>
struct list2tail<list,List*> : list2cons<List> {};

template <typename R, typename T1, typename T2>
struct list2cons< R(T1,T2) > {
   typedef cons<T1, typename list2tail<R,T2>::type> type;
   typedef R return_type;
};

template <typename R, typename T1, typename T2, typename T3>
struct list2cons< R(T1,T2,T3) > {
   typedef cons<T1, cons<T2, typename list2tail<R,T3>::type> > type;
   typedef R return_type;
};

template <typename R, typename T1, typename T2, typename T3, typename T4>
struct list2cons< R(T1,T2,T3,T4) > {
   typedef cons<T1, cons<T2, cons<T3, typename list2tail<R,T4>::type> > > type;
   typedef R return_type;
};

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
struct list2cons< R(T1,T2,T3,T4,T5) > {
   typedef cons<T1, cons<T2, cons<T3, cons<T4, typename list2tail<R,T5>::type> > > > type;
   typedef R return_type;
};

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
struct list2cons< R(T1,T2,T3,T4,T5,T6) > {
   typedef cons<T1, cons<T2, cons<T3, cons<T4, cons<T5, typename list2tail<R,T6>::type> > > > > type;
   typedef R return_type;
};

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
struct list2cons< R(T1,T2,T3,T4,T5,T6,T7) > {
   typedef cons<T1, cons<T2, cons<T3, cons<T4, cons<T5, cons<T6, typename list2tail<R,T7>::type> > > > > > type;
   typedef R return_type;
};

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
struct list2cons< R(T1,T2,T3,T4,T5,T6,T7,T8) > {
   typedef cons<T1, cons<T2, cons<T3, cons<T4, cons<T5, cons<T6, cons<T7, typename list2tail<R,T8>::type> > > > > > > type;
   typedef R return_type;
};

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
struct list2cons< R(T1,T2,T3,T4,T5,T6,T7,T8,T9) > {
   typedef cons<T1, cons<T2, cons<T3, cons<T4, cons<T5, cons<T6, cons<T7, cons<T8, typename list2tail<R,T9>::type> > > > > > > > type;
   typedef R return_type;
};

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
struct list2cons< R(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10) > {
   typedef cons<T1, cons<T2, cons<T3, cons<T4, cons<T5, cons<T6, cons<T7, cons<T8, cons<T9, typename list2tail<R,T10>::type> > > > > > > > > type;
   typedef R return_type;
};

template <typename T1, typename T2>
struct concat_list {
   typedef typename std::conditional<std::is_same<T1, void>::value, T2,
                                     typename std::conditional<std::is_same<T2, void>::value, T1, cons<T1, T2> >::type>::type
      type;
};

template <typename Head, typename Tail, typename T>
struct concat_list<cons<Head,Tail>, T>
   : concat_list<Head, typename concat_list<Tail, T>::type> {};

template <typename Head, typename Tail>
struct concat_list<cons<Head,Tail>, void> {
   typedef cons<Head, Tail> type;
};

/** Determine element pairs yielding Predicate::value==true
    The primary template handles the case of $First$ and $Second$ being scalars,
    while the general case of $First$ being a list is handled in specializations.

    list_search is a simplified function, it terminates as soon as one matching
    pair is found.
 */
template <typename First, typename Second, template <typename,typename> class Predicate>
struct list_search {
   /// whether at least one pair found
   static const bool value=Predicate<First,Second>::value;
   /// where it was found
   static const int pos= value ? 0 : -1;
   /// elements from First matching the predicate
   typedef typename std::conditional<value, First, void>::type positive;
   /// complement
   typedef typename std::conditional<value, void, First>::type negative;
   /// Second if there is at least one matching pair
   typedef typename std::conditional<value, Second, void>::type positive2;
   /// complement
   typedef typename std::conditional<value, void, Second>::type negative2;
};

template <typename Second, template <typename,typename> class Predicate>
struct list_search<void, Second, Predicate> : std::false_type {
   static const int pos=-1;
   typedef void positive;
   typedef void negative;
   typedef void positive2;
   typedef Second negative2;
};

template <typename First, template <typename,typename> class Predicate>
struct list_search<First, void, Predicate> : std::false_type {
   static const int pos=-1;
   typedef void positive;
   typedef First negative;
   typedef void positive2;
   typedef void negative2;
};

// to avoid ambiguity
template <template <typename,typename> class Predicate>
struct list_search<void, void, Predicate> : std::false_type {
   static const int pos=-1;
   typedef void positive;
   typedef void negative;
   typedef void positive2;
   typedef void negative2;
};

template <typename Head, typename Tail, typename Second, template <typename,typename> class Predicate,
          bool _head_matches=Predicate<Head,Second>::value>
struct list_search_helper {
   static const bool value=_head_matches; // true
   static const int pos=0;
   typedef Head positive;
   typedef Tail negative;
};

template <typename Head, typename Tail, typename Second, template <typename,typename> class Predicate>
struct list_search_helper<Head, Tail, Second, Predicate, false>
   : list_search<Tail, Second, Predicate> {
   typedef list_search<Tail, Second, Predicate> _super;
   static const int pos= _super::value ? _super::pos+1 : -1;
   typedef typename concat_list<Head, typename _super::negative>::type negative;
};

template <typename Head, typename Tail, typename Second, template <typename,typename> class Predicate>
struct list_search<cons<Head, Tail>, Second, Predicate>
   : list_search_helper<Head, Tail, Second, Predicate> {};

// to avoid ambiguity
template <typename Head, typename Tail, template <typename,typename> class Predicate>
struct list_search<cons<Head, Tail>, void, Predicate> : std::false_type {
   static const int pos=-1;
   typedef void positive;
   typedef cons<Head,Tail> negative;
   typedef void positive2;
   typedef void negative2;
};

/** More powerful than list_search: find all pairs from $First$ and $Second$
    (both may be lists or scalars.) */
template <typename First, typename Second, template <typename,typename> class Predicate>
struct list_search_all : list_search<First, Second, Predicate> {};

template <typename Head, typename Tail, typename Second, template <typename,typename> class Predicate>
struct list_search_all_first_helper {
   typedef list_search_all<Head,Second,Predicate> head;
   typedef list_search_all<Tail,Second,Predicate> tail;
   static const bool value=head::value || tail::value;
   typedef typename concat_list<typename head::positive, typename tail::positive>::type positive;
   typedef typename concat_list<typename head::negative, typename tail::negative>::type negative;
   typedef typename std::conditional<value, Second, void>::type positive2;
   typedef typename std::conditional<value, void, Second>::type negative2;
};

template <typename First, typename Head, typename Tail, template <typename,typename> class Predicate>
struct list_search_all_second_helper {
   typedef list_search_all<First,Head,Predicate> head;
   typedef list_search_all<First,Tail,Predicate> tail;
   static const bool value=head::value || tail::value;
   typedef typename std::conditional<value,First,void>::type positive;
   typedef typename std::conditional<value,void,First>::type negative;
   typedef typename concat_list<typename head::positive2, typename tail::positive2>::type positive2;
   typedef typename concat_list<typename head::negative2, typename tail::negative2>::type negative2;
};

template <typename Head, typename Tail, typename Second, template <typename,typename> class Predicate>
struct list_search_all<cons<Head,Tail>, Second, Predicate>
   : list_search_all_first_helper<Head, Tail, Second, Predicate> {};

template <typename First, typename Head, typename Tail, template <typename,typename> class Predicate>
struct list_search_all<First, cons<Head, Tail>, Predicate>
   : list_search_all_second_helper<First, Head, Tail, Predicate> {};

template <typename Head, typename Tail, typename Head2, typename Tail2, template <typename,typename> class Predicate>
struct list_search_all<cons<Head,Tail>, cons<Head2, Tail2>, Predicate>
   : list_search_all_first_helper<Head, Tail, cons<Head2, Tail2>, Predicate> {
   typedef list_search_all_second_helper<cons<Head,Tail>, Head2, Tail2, Predicate> h2;
   typedef typename h2::positive2 positive2;
   typedef typename h2::negative2 negative2;
};

/// Eliminate the elements of Second that evaluate Predicate::value==TRUE
///  when combined with some element(s) from First.
template <typename First, typename Second, template <typename,typename> class Predicate>
struct merge_list
   : concat_list<First, typename list_search_all<First, Second, Predicate>::negative2> {};

template <template <typename> class Operation, typename T>
struct list_transform_unary : Operation<T> {};

template <template <typename> class Operation>
struct list_transform_unary<Operation, void> {
   typedef void type;
};

template <template <typename> class Operation, typename Head, typename Tail>
struct list_transform_unary<Operation, cons<Head,Tail> >
   : concat_list<typename Operation<Head>::type,
                 typename list_transform_unary<Operation, Tail>::type> {};

template <template <typename,typename> class Operation, typename T1, typename T2>
struct list_transform_binary : Operation<T1,T2> {};

template <template <typename,typename> class Operation, typename T1, typename T2>
struct list_transform_binary<Operation, T1, same<T2> > : Operation<T1,T2> {};

template <template <typename,typename> class Operation, typename T1, typename T2>
struct list_transform_binary<Operation, same<T1>, T2> : Operation<T1,T2> {};

template <template <typename,typename> class Operation, typename T1, typename T2>
struct list_transform_binary<Operation, same<T1>, same<T2> > : Operation<T1,T2> {};

template <template <typename,typename> class Operation, typename Head1, typename Tail1, typename Head2, typename Tail2>
struct list_transform_binary<Operation, cons<Head1,Tail1>, cons<Head2,Tail2> >
   : concat_list<typename Operation<Head1, Head2>::type,
                 typename list_transform_binary<Operation, Tail1, Tail2>::type> {};

template <template <typename,typename> class Operation, typename Head1, typename Tail1, typename T2>
struct list_transform_binary<Operation, cons<Head1,Tail1>, same<T2> >
   : concat_list<typename Operation<Head1, T2>::type,
                 typename list_transform_binary<Operation, Tail1, same<T2> >::type> {};

template <template <typename,typename> class Operation, typename T1, typename Head2, typename Tail2>
struct list_transform_binary<Operation, same<T1>, cons<Head2,Tail2> >
   : concat_list<typename Operation<T1, Head2>::type,
                 typename list_transform_binary<Operation, same<T1>, Tail2>::type> {};

template <template <typename> class Operation>
struct apply_unary {
   template <typename T>
   struct op {
      typedef Operation<T> type;
   };
};

template <template <typename,typename> class Operation>
struct apply_binary {
   template <typename T1, typename T2>
   struct op {
      typedef Operation<T1,T2> type;
   };
};

template <template <typename, typename> class Operation, typename T>
struct list_accumulate : T {};

template <template <typename, typename> class Operation>
struct list_accumulate<Operation, void> : Operation<void,void> {};

template <template <typename, typename> class Operation, typename Head, typename Tail>
struct list_accumulate<Operation, cons<Head,Tail> >
   : Operation<Head, list_accumulate<Operation, Tail> > {};

template <typename T1, typename T2>
struct list_count {
   static const int value= T1::value + T2::value;
};

template <>
struct list_count<void,void> {
   static const int value=0;
};

template <typename T1, typename T2>
struct list_and {
   static const bool value= T1::value && T2::value;
};

template <>
struct list_and<void,void> {
   static const bool value=false;
};

template <typename T1, typename T2>
struct list_or {
   static const bool value= T1::value || T2::value;
};

template <>
struct list_or<void,void> {
   static const bool value=false;
};

template <template <typename,typename> class AccumulatingOperation, template <typename> class Operation, typename T>
struct list_accumulate_unary
   : list_accumulate<AccumulatingOperation, typename list_transform_unary<apply_unary<Operation>::template op, T>::type> {};

template <template <typename,typename> class AccumulatingOperation, template <typename,typename> class Operation, typename T1, typename T2>
struct list_accumulate_binary
   : list_accumulate<AccumulatingOperation, typename list_transform_binary<apply_binary<Operation>::template op, T1, T2>::type> {};

template <typename T1, typename T2>
struct list_logical_or {
   typedef T1 type;
};

template <typename T2>
struct list_logical_or<void, T2> {
   typedef T2 type;
};

template <typename T1, typename T2>
struct list_logical_and {
   typedef T2 type;
};

template <typename T2>
struct list_logical_and<void, T2> {
   typedef void type;
};

template <typename First, typename Second, template <typename, typename> class Predicate=std::is_same>
struct list_mapping {
   typedef int_constant< list_search<Second, First, Predicate>::pos > type;
   static const bool mismatch= list_search<Second, First, Predicate>::pos < 0;
};

template <typename Head, typename Tail, typename Second, template <typename, typename> class Predicate>
struct list_mapping< cons<Head, Tail>, Second, Predicate> {
   typedef list_mapping<Head, Second, Predicate> first;
   typedef list_mapping<Tail, Second, Predicate> second;
   typedef cons< typename first::type, typename second::type > type;
   static const bool mismatch= first::mismatch || second::mismatch;
};

/* -----------------------------------
 *  Structural description of objects
 * ----------------------------------- */

struct is_scalar {};
struct is_container {};
struct is_composite {};
struct is_opaque {};
struct is_not_object {};

namespace object_classifier {
   enum { is_not_object, is_opaque, is_scalar };

   namespace _impl {
      struct bait {};
      size_discriminant<is_opaque>::type analyzer_f(...);

      template <typename T>
      typename std::enable_if<std::numeric_limits<T>::is_specialized, size_discriminant<is_scalar>::type>::type analyzer_f(const T*, bait*);
   }
   typedef _impl::bait *bait;

   template <typename T, typename = void> struct kind_of;

   template <typename T>
   struct kind_of<T, typename std::enable_if<!(std::is_enum<T>::value || std::is_pointer<T>::value)>::type>
   {
      static constexpr int value = sizeof(analyzer_f((const T*)nullptr, (bait)nullptr));
   };

   template <typename T>
   struct kind_of<T*, void>
   {
      static const int value=is_not_object;
   };

   template <typename T>
   struct kind_of<T, typename std::enable_if<std::is_enum<T>::value>::type>
   {
      static constexpr int check_numeric = sizeof(analyzer_f((const T*)nullptr, (bait)nullptr));
      static constexpr int value = check_numeric == is_scalar ? is_scalar : is_not_object;
   };

   template <>
   struct kind_of<void, void>
   {
      static constexpr int value = is_not_object;
   };

   template <typename T, int kind = kind_of<T>::value>
   struct what_is {
      static const int value = kind;
   };
} // end namespace object_classifier

// to be specialized for overwriting model-specific properties
template <typename T> struct spec_object_traits;

struct default_object_traits {
   static const bool is_lazy=false, is_temporary=false, is_always_const=false, is_persistent=true;
   static const bool IO_ends_with_eol=false;
   // allow creation of static objects, destructor wouldn't make any troubles when called in the global exit sequence
   static const bool allow_static=true;
   typedef void masquerade_for;
   typedef void proxy_for;
};

template <>
struct spec_object_traits<is_scalar> : default_object_traits {
   typedef is_scalar model;
};

template <>
struct spec_object_traits<is_not_object> : default_object_traits {
   typedef is_not_object model;
};

template <>
struct spec_object_traits<is_opaque> : default_object_traits {
   typedef is_opaque model;
};

template <typename T>
class Serialized : public T {
public:
   Serialized& operator= (const T& x) { static_cast<T&>(*this)=x; return *this; }
protected:
   Serialized() noexcept;
   ~Serialized() noexcept;
};

// TODO: use some auto-detection based on decltype(serialized(X))
template <typename T, bool enabled=is_class_or_union<T>::value>
struct has_serialized_impl : is_among<typename spec_object_traits<Serialized<T>>::model, is_composite, is_container> {};

template <typename T>
struct has_serialized_impl<T, false> : std::false_type {};

template <typename T>
struct has_serialized : has_serialized_impl<T> {};

template <typename T>
struct has_serialized< Serialized<T> > : std::false_type {};

template <typename T, typename enabled=typename std::enable_if<has_serialized<T>::value>::type> inline
Serialized<T>& serialize(T& x)
{
   return static_cast<Serialized<T>&>(x);
}

template <typename T, typename enabled=typename std::enable_if<has_serialized<T>::value>::type> inline
const Serialized<T>& serialize(const T& x)
{
   return static_cast<const Serialized<T>&>(x);
}

template <>
struct spec_object_traits<is_composite> {
   typedef is_composite model;
   static const bool is_lazy=false, is_temporary=false, is_always_const=false, is_persistent=true;
   static const bool allow_static=true;
   typedef void masquerade_for;
   typedef void proxy_for;
};

// Serialized should not be used unless specialized for certain data types
template <typename T>
struct spec_object_traits<Serialized<T>> : spec_object_traits<is_not_object> {};

template <typename T>
struct spec_object_traits< cons<T, int_constant<object_classifier::is_not_object>> > : spec_object_traits<is_not_object> {};

template <typename T>
struct spec_object_traits< cons<T, int_constant<object_classifier::is_opaque>> > : spec_object_traits<is_opaque> {};

template <typename T>
struct spec_object_traits< cons<T, int_constant<object_classifier::is_scalar>> > : spec_object_traits<is_scalar> {
   static
   bool is_zero(const T& x)
   {
      return !x;
   }

   static
   bool is_one(const T& x)
   {
      return x==1;
   }

   static const T& zero()
   {
      static const T zero_v(0);
      return zero_v;
   }
   static const T& one()
   {
      static const T one_v(1);
      return one_v;
   }
};

template <typename T>
struct spec_object_traits :
   spec_object_traits< cons<T, int_constant<object_classifier::what_is<T>::value> > > {};

template <typename T, typename Model,
          bool _fulfilled=is_derived_from< spec_object_traits<T>, spec_object_traits<Model> >::value>
struct spec_or_model_traits : spec_object_traits<T> {
   static const bool is_specialized=true;
};

template <typename T, typename Model>
struct spec_or_model_traits<T, Model, false> : spec_object_traits<Model> {
   static const bool is_specialized=false;
};

template <typename Top>
class Generic {
protected:
   Generic() = default;
   Generic(const Generic&) = default;
public:
   using top_type = Top;
   using generic_type = void;
   using persistent_type = Top;
   using concrete_type = Top;

   const Top& top() const &
   {
      return static_cast<const Top&>(*this);
   }
   Top& top() &
   {
      return static_cast<Top&>(*this);
   }
   Top&& top() &&
   {
      return static_cast<Top&&>(*this);
   }
};

// to be specialized for overwriting generic family-specific properties
template <typename T>
struct generic_object_traits : spec_object_traits<typename T::generic_type> {};

DeclTypedefCHECK(generic_type);
DeclTypedefCHECK(persistent_type);
DeclTypedefCHECK(element_type);

template <typename T, bool _has_generic=has_generic_type<T>::value,
                      bool _has_persistent=has_persistent_type< spec_object_traits<T> >::value>
struct choose_generic_object_traits : spec_object_traits<T> {
   typedef void generic_type;
   typedef T persistent_type;
   typedef typename spec_object_traits<T>::model generic_tag;
};

template <typename T>
struct choose_generic_object_traits<T, false, true> : spec_object_traits<T> {
   typedef void generic_type;
   typedef typename spec_object_traits<T>::model generic_tag;
};

template <typename T>
struct choose_generic_object_traits<T, true, false> : generic_object_traits<T> {
   typedef typename T::generic_type generic_type;
   typedef generic_object_traits<T> gen_traits;
   static const bool is_persistent=
      gen_traits::is_persistent && std::is_same<typename gen_traits::masquerade_for, void>::value && !(gen_traits::is_temporary || gen_traits::is_lazy);
   typedef typename std::conditional<is_persistent, T, typename generic_type::persistent_type>::type persistent_type;
};

template <typename T>
struct choose_generic_object_traits<T, true, true> : generic_object_traits<T> {
   typedef typename T::generic_type generic_type;
   typedef generic_object_traits<T> gen_traits;
   static const bool is_persistent=std::is_same<T, typename spec_object_traits<T>::persistent_type>::value;
};

// to be specialized by "proxy" types like pseudo-containers
template <typename T>
struct redirect_object_traits : choose_generic_object_traits<T> {};

template <typename T, typename Model=typename redirect_object_traits<T>::model>
struct object_traits : redirect_object_traits<T> {
   static const int total_dimension=0, nesting_level=0;
};

enum IO_separator_kind { IO_sep_inherit, IO_sep_containers, IO_sep_enforce };

template <typename T, bool take_elements=has_element_type<T>::value>
struct container_element_type {
   typedef typename T::value_type type;
};

template <typename T>
struct container_element_type<T, true> {
   typedef typename T::element_type type;
};

template <typename T>
struct object_traits<T, is_container>
   : redirect_object_traits<T> {
   typedef typename container_element_type<T>::type element_type;
   static const int
      total_dimension = object_traits<element_type>::total_dimension + object_traits<T>::dimension,
      nesting_level = object_traits<element_type>::nesting_level+1;
   static const int is_resizeable= object_traits<T>::is_temporary || object_traits<T>::is_always_const ? 0 : redirect_object_traits<T>::is_resizeable;
   static const bool
      IO_separate_elements_with_eol=
         object_traits<T>::IO_separator==IO_sep_inherit &&
            object_traits<element_type>::IO_ends_with_eol
      || object_traits<T>::IO_separator==IO_sep_containers &&
            object_traits<element_type>::total_dimension >= 1
      || object_traits<T>::IO_separator==IO_sep_enforce,
      IO_ends_with_eol=
         IO_separate_elements_with_eol
      || object_traits<element_type>::IO_ends_with_eol
      || object_traits<T>::dimension>1;
};

template <>
struct spec_object_traits<is_container> {
   typedef is_container model;
   static const int dimension=1, is_resizeable=1;
   static const bool is_lazy=false, is_temporary=false, is_always_const=false, is_persistent=true, allow_sparse=false;
   static const bool allow_static=true;
   static const IO_separator_kind IO_separator=IO_sep_containers;
   typedef void masquerade_for;
   typedef void proxy_for;
};

template <typename T, typename Model = typename object_traits<T>::model>
struct get_scalar_type {
   using type = void;
};

template <typename T>
struct get_scalar_type<T, is_scalar> {
   using type = typename object_traits<T>::persistent_type;
};

template <typename T>
struct get_scalar_type<T, is_container>
   : get_scalar_type<typename object_traits<T>::element_type> {};

template <typename T>
using matching_primitive_number_t = std::conditional_t<std::is_same<typename get_scalar_type<pure_type_t<T>>::type, double>::value, double, Int>;

template <typename Eref, typename Match = Int>
using prevent_int_element = std::conditional_t<std::is_same<pure_type_t<Eref>, int>::value,
                                               inherit_const_t<matching_primitive_number_t<Match>, Eref>, Eref>;

template <typename T>
struct max_dimension {
   static const int value=object_traits<typename deref<T>::type>::total_dimension;
   static const bool IO_ends_with_eol=object_traits<typename deref<T>::type>::IO_ends_with_eol;
};

template <int X, int Y, bool _ge=(X>=Y)>
struct const_compare {
   static const int max=X, min=Y;
};
template <int X, int Y>
struct const_compare<X,Y,false> {
   static const int max=Y, min=X;
};

template <typename Head, typename Tail>
struct max_dimension< cons<Head, Tail> > {
   static const int value=const_compare<max_dimension<Head>::value, max_dimension<Tail>::value>::max;
   static const bool IO_ends_with_eol=max_dimension<Head>::IO_ends_with_eol || max_dimension<Tail>::IO_ends_with_eol;
};

template <typename Object>
struct object_traits<Object,is_composite> : redirect_object_traits<Object> {
   static const int total_dimension=max_dimension<typename object_traits::elements>::value,
                    nesting_level=0;
   static const bool IO_ends_with_eol=max_dimension<typename object_traits::elements>::IO_ends_with_eol;
};

template <typename ObjectRef, int n, int m=0,
          int l=list_length<typename object_traits<typename attrib<ObjectRef>::minus_const_ref>::elements>::value>
struct visitor_n_th : visitor_n_th<ObjectRef, n, m+1, l> {
   typedef visitor_n_th<ObjectRef, n, m+1, l> super;
   super&
   operator<< (typename attrib<typename n_th<typename super::elements, m>::type>::plus_const_ref)
   {
      return *this;
   }
};

template <typename ObjectRef, int n, int l>
struct visitor_n_th<ObjectRef, n, n, l> : visitor_n_th<ObjectRef, n, n+1, l> {
   typedef visitor_n_th<ObjectRef, n, n+1, l> super;
   super&
   operator<< (typename super::value_ref val)
   {
      this->value=&val;
      return *this;
   }
};

template <typename ObjectRef, int n, int l>
struct visitor_n_th<ObjectRef, n, l, l> {
   typedef typename object_traits<typename attrib<ObjectRef>::minus_const_ref>::elements elements;
   typedef typename n_th<elements, n>::type element;
   typedef typename inherit_const<typename attrib<element>::minus_const_ref, ObjectRef>::type* value_ptr;
   typedef typename inherit_const<typename attrib<element>::minus_const_ref, ObjectRef>::type& value_ref;
   value_ptr value;

   visitor_n_th() : value(nullptr) {}
};

template <typename T, int n> inline
typename visitor_n_th<T,n>::value_ref
visit_n_th(T& obj, int_constant<n>)
{
   visitor_n_th<T, n> vis;
   object_traits<typename attrib<T>::minus_const_ref>::visit_elements(obj,vis);
   return *vis.value;
}

template <typename ObjectRef, typename Model=typename object_traits<typename attrib<ObjectRef>::minus_const_ref>::model>
struct is_mutable_helper {
   static const bool value=!attrib<ObjectRef>::is_const;
};

template <typename ObjectRef>
struct is_mutable : is_mutable_helper<ObjectRef> {};

template <typename ObjectRef>
struct is_mutable_helper<ObjectRef, is_composite> {
   static const bool value=
      is_mutable_helper<ObjectRef, void>::value &&
      list_accumulate_unary<list_and, is_mutable, typename object_traits<typename attrib<ObjectRef>::minus_const_ref>::elements>::value;
};

template <typename T,
          typename _hidden=typename object_traits<typename deref<T>::type>::masquerade_for>
struct is_masquerade : std::true_type {
   typedef typename std::conditional<is_masquerade<_hidden>::value, typename is_masquerade<_hidden>::hidden_type,
                                                                    typename attrib<_hidden>::minus_const_ref>::type
      hidden_type;
   // the rest is for the appropriate specialization of the class alias
   typedef typename std::conditional<attrib<_hidden>::is_const || attrib<_hidden>::is_reference, _hidden, T>::type
      inherit_from;
   typedef typename inherit_ref< typename inherit_ref<hidden_type,_hidden>::type,
                                 typename std::conditional< object_traits<hidden_type>::is_always_const, typename attrib<T>::plus_const, T>::type>::type
      hidden_stored_type;
};

template <typename T>
struct is_masquerade<T, void> : std::false_type {
   typedef void hidden_type;
};

// general case: everything is different
struct anisomorphic_types : std::false_type {
   using tags = mlist<>;
   // TODO: remove discriminant here and in all specializations below when operators are refactored
   typedef void discriminant;
};

template <typename T1, typename T2,
          typename Model1=typename object_traits<T1>::model, typename Model2=typename object_traits<T2>::model>
struct isomorphic_types_impl : anisomorphic_types {};

template <typename T1, typename T2, typename Model>
struct isomorphic_types_impl<T1, T2, Model, Model> : std::true_type {
   using tags = mlist<typename object_traits<T1>::generic_tag, typename object_traits<T2>::generic_tag>;
   typedef cons<typename object_traits<T1>::generic_tag, typename object_traits<T2>::generic_tag> discriminant;
};

template <typename T, typename TBehind=typename object_traits<T>::proxy_for>
struct type_behind
   : type_behind<TBehind> {};

template <typename T>
struct type_behind<T, void> {
   using type = T;
};

template <typename T>
using type_behind_t = typename type_behind<pure_type_t<T>>::type;

template <typename T1, typename T2>
struct isomorphic_types
   : isomorphic_types_impl<type_behind_t<T1>, type_behind_t<T2>> {};

template <typename T1, typename T2>
struct isomorphic_types_impl<T1, T2, is_container, is_container> {
   using V1 = typename container_element_type<T1>::type;
   using V2 = typename container_element_type<T2>::type;
   static constexpr bool value=isomorphic_types<V1, V2>::value;
   using type = bool_constant<value>;
   using value_type = bool;

   using tags = typename mselect< std::enable_if< value, mlist<typename object_traits<T1>::generic_tag, typename object_traits<T2>::generic_tag> >,
                                  std::enable_if< isomorphic_types<T1, V2>::value, mlist<is_scalar, typename object_traits<T2>::generic_tag> >,
                                  std::enable_if< isomorphic_types<V1, T2>::value, mlist<typename object_traits<T1>::generic_tag, is_scalar> >,
                                  mlist<> >::type;
   typedef typename mselect< std::enable_if< value, cons<typename object_traits<T1>::generic_tag, typename object_traits<T2>::generic_tag> >,
                             std::enable_if< isomorphic_types<T1, V2>::value, cons<is_scalar, typename object_traits<T2>::generic_tag> >,
                             std::enable_if< isomorphic_types<V1, T2>::value, cons<typename object_traits<T1>::generic_tag, is_scalar> >,
                             void >::type
      discriminant;
};

template <typename T1, typename T2, typename Model1>
struct isomorphic_types_impl<T1, T2, Model1, is_container>
   : std::false_type {
   using tags = std::conditional_t< isomorphic_types<T1, typename container_element_type<T2>::type>::value,
                                    mlist<is_scalar, typename object_traits<T2>::generic_tag>,
                                    mlist<> >;
   typedef typename std::conditional< isomorphic_types<T1, typename container_element_type<T2>::type>::value,
                                      cons<is_scalar, typename object_traits<T2>::generic_tag>, void>::type
      discriminant;
};

template <typename T1, typename T2, typename Model2>
struct isomorphic_types_impl<T1, T2, is_container, Model2>
   : std::false_type {
   using tags = std::conditional_t< isomorphic_types<typename container_element_type<T1>::type, T2>::value,
                                    mlist<typename object_traits<T1>::generic_tag, is_scalar>,
                                    mlist<> >;
   typedef typename std::conditional< isomorphic_types<typename container_element_type<T1>::type, T2>::value,
                                      cons<typename object_traits<T1>::generic_tag, is_scalar>, void>::type
      discriminant;
};

template <typename T1, typename T2>
struct isomorphic_types_deref : isomorphic_types<typename deref<T1>::type, typename deref<T2>::type> {};

template <typename T1, typename T2>
struct isomorphic_types_impl<T1, T2, is_composite, is_composite> {
   static const bool value=
      list_length<typename object_traits<T1>::elements>::value == list_length<typename object_traits<T2>::elements>::value &&
      list_accumulate_binary<list_and, isomorphic_types_deref, typename object_traits<T1>::elements, typename object_traits<T2>::elements>::value;
   using type = bool_constant<value>;
   using value_type = bool;

   using tags = std::conditional_t<value, mlist<typename object_traits<T1>::generic_tag, typename object_traits<T2>::generic_tag>, mlist<>>;
   typedef typename std::conditional<value, cons<typename object_traits<T1>::generic_tag, typename object_traits<T2>::generic_tag>, void>::type
      discriminant;
};

// specializations thereof may define an internal type field_type pointing to itself or another type
// fulfilling the requirements to an algebraic field.
template <typename T>
struct algebraic_traits {};

template <>
struct algebraic_traits<double> {
   typedef double field_type;
};

template <typename T, typename field_type=T>
struct is_field_impl : std::false_type {};

template <typename T>
struct is_field_impl<T, typename algebraic_traits<T>::field_type> : std::true_type {};

template <typename T>
struct is_field : is_field_impl<T> {};

template <typename T, typename T2 = void>
struct has_zero_value_impl : std::false_type {};

// we assume that every class for which algebraic_traits is specialized has also a zero value
template <typename T>
struct has_zero_value_impl<T, accept_valid_type<typename algebraic_traits<T>::field_type>> : std::true_type {};

template <typename T>
struct has_zero_value : has_zero_value_impl<T> {};

template <typename T>
struct is_field_of_fractions : bool_constant<!std::numeric_limits<T>::is_integer & std::numeric_limits<T>::is_exact> {};

/* -----------------------------------------------
 *  conversion between generic and concrete types
 * ----------------------------------------------- */

template <typename T, bool=has_generic_type<typename deref<T>::type>::value, bool=false>
struct Concrete : std::true_type {
   using type = pure_type_t<T>;
};

template <typename T>
struct Concrete<T, true, false>
   : Concrete<T, false, std::is_same<typename deref<T>::type, typename deref<T>::type::generic_type>::value> {};

template <typename T>
struct Concrete<T, false, true> : std::false_type {
   using type = typename deref<T>::type::concrete_type;
};

template <typename T>
using concrete_t = inherit_reference_t<typename Concrete<pure_type_t<T>>::type, T>;

template <typename T>
decltype(auto) concrete(T&& x)
{
   return static_cast<concrete_t<T&&>>(std::forward<T>(x));
}

template <typename T>
constexpr bool is_lazy()
{
   return object_traits<typename Concrete<typename deref<T>::type>::type>::is_lazy;
}

template <typename T, bool=is_lazy<T>()>
struct Diligent {
   using type = typename Concrete<typename deref<T>::type>::type;
};

template <typename T>
struct Diligent<T, true> {
   using type = typename object_traits<typename Concrete<typename deref<T>::type>::type>::persistent_type;
};

template <typename T>
using diligent_t = typename Diligent<T>::type;

template <typename T>
using diligent_ref_t = std::conditional_t<is_lazy<T>(), diligent_t<T>,
                                          add_const_t<inherit_reference_t<diligent_t<T>, T>>>;

template <typename T>
decltype(auto) diligent(T&& x, std::enable_if_t<!is_lazy<T>(), void**> =nullptr)
{
   return concrete(std::forward<T>(x));
}

template <typename T>
decltype(auto) diligent(T&& x, std::enable_if_t<is_lazy<T>(), void**> =nullptr)
{
   return diligent_t<T>(std::forward<T>(x));
}

template <typename Result, typename Source, typename Generic=Source>
struct inherit_generic_helper {
   using type = typename Source::template rebind_generic<Result>::type;
};

struct generic_none {};

template <typename Result, typename Source>
struct inherit_generic_helper<Result, Source, void> {
   using type = generic_none;
};

template <typename Result, typename Source>
struct inherit_generic
   : inherit_generic_helper<Result, Source, typename object_traits<Source>::generic_type> {};

template <typename Result, typename... Sources>
struct inherit_generic<Result, mlist<Sources...> >
   : inherit_generic_helper<Result, typename least_derived_class<typename inherit_generic<Result, Sources>::type...>::type> {};

/* ----------------------------------------
 *  tools for alias and masquerade objects 
 * ---------------------------------------- */

template <typename ObjectRef>
using is_effectively_const
   = bool_constant<(object_traits<typename deref<ObjectRef>::type>::is_always_const
                    || deref<ObjectRef>::is_const
                    || (!std::is_lvalue_reference<ObjectRef>::value && object_traits<typename deref<ObjectRef>::type>::is_persistent))>;

template <typename ObjectRef>
using effectively_const_t = std::conditional_t<is_effectively_const<ObjectRef>::value, typename deref<ObjectRef>::plus_const, ObjectRef>;

template <template <typename> class Masquerade, typename OrigRef>
struct masquerade : inherit_ref<Masquerade<typename deref<OrigRef>::type>, OrigRef> {};

template <template <typename,typename> class Masquerade, typename OrigRef, typename Second>
struct masquerade2 : inherit_ref<Masquerade<typename deref<OrigRef>::type, Second>, OrigRef> {};

template <template <typename,typename,typename> class Masquerade, typename OrigRef, typename Second, typename Third>
struct masquerade3 : inherit_ref<Masquerade<typename deref<OrigRef>::type, Second, Third>, OrigRef> {};

template <template <typename> class Masquerade, typename OrigRef>
struct deref< masquerade<Masquerade,OrigRef> > : deref< typename masquerade<Masquerade,OrigRef>::type > {
   typedef masquerade<Masquerade, typename attrib<OrigRef>::minus_const> minus_const;
   typedef masquerade<Masquerade, typename attrib<OrigRef>::plus_const> plus_const;
};

template <template <typename,typename> class Masquerade, typename OrigRef, typename Second>
struct deref< masquerade2<Masquerade,OrigRef,Second> > : deref< typename masquerade2<Masquerade,OrigRef,Second>::type > {
   typedef masquerade2<Masquerade, typename attrib<OrigRef>::minus_const, Second> minus_const;
   typedef masquerade2<Masquerade, typename attrib<OrigRef>::plus_const, Second> plus_const;
};

template <template <typename,typename,typename> class Masquerade, typename OrigRef, typename Second, typename Third>
struct deref< masquerade3<Masquerade,OrigRef,Second,Third> > : deref< typename masquerade3<Masquerade,OrigRef,Second,Third>::type > {
  typedef masquerade3<Masquerade, typename attrib<OrigRef>::minus_const, Second, Third> minus_const;
  typedef masquerade3<Masquerade, typename attrib<OrigRef>::plus_const, Second, Third> plus_const;
};

template <typename ObjRef, bool _is_ref=(deref<ObjRef>::is_reference || is_masquerade<ObjRef>::value)>
struct lvalue_arg {
   typedef typename deref<ObjRef>::type object;
   typedef typename std::conditional< object_traits<object>::is_temporary &&
                                      !object_traits<object>::is_always_const &&
                                      !object_traits<object>::is_lazy,
                                      object, object&>::type
      type;
};

template <typename ObjRef>
struct lvalue_arg<ObjRef, true> {
   typedef typename deref<ObjRef>::minus_const type;
};

template <>
struct lvalue_arg<void, false> {
   typedef type2type<void> type;
};

/// These functions are defined in the standard libraries at non-portable locations under non-portable names.
/// Until they are unified, it's easier to provide an own implementation.

template <typename T, typename... Args>
T* construct_at(T* place, Args&&... args)
{
   return ::new((void*)place) T(std::forward<Args>(args)...);
}

#if __cplusplus <= 201402L

template <typename T>
void destroy_at(T* obj)
{
   obj->~T();
}

#else

using std::destroy_at;

#endif

template <typename T>
struct extract_element_type {
   using type = typename deref<T>::type::element_type;
};

template <typename T>
struct extract_lazy {
   using type = bool_constant<object_traits<typename deref<T>::type>::is_lazy>;
};

/* ----------------------
 *  memory layout tricks
 * ---------------------- */

template <typename T> inline
void relocate(T* from, T* to, std::true_type)
{
   *to=*from;
}

template <typename T> inline
void relocate(T* from, T* to, std::false_type)
{
   construct_at(to, *from);
   destroy_at(from);
}

template <typename T> inline
void relocate(T* from, T* to)
{
   relocate(from, to, std::is_pod<T>());
}

template <typename T>
bool is_zero(const T& x) { return object_traits<T>::is_zero(x); }

template <typename T>
bool is_one(const T& x) { return object_traits<T>::is_one(x); }

template <typename T>
const T& zero_value() { return object_traits<T>::zero(); }

template <typename T>
const T& one_value() { return object_traits<T>::one(); }

template <typename T>
struct is_gcd_domain
   : bool_constant<!is_among<T, bool, int>::value && std::numeric_limits<T>::is_integer && std::numeric_limits<T>::is_exact> {};

enum all_selector : bool { All };

} // end namespace pm

namespace polymake {
   using pm::ptr2type;
   using pm::type2type;
   using pm::is_zero;
   using pm::is_one;
   using pm::zero_value;
   using pm::one_value;
   using pm::is_gcd_domain;
   using pm::All;
}

#endif // POLYMAKE_INTERNAL_TYPE_MANIP_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
