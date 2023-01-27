/* Copyright (c) 1997-2023
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

#pragma once

#include <type_traits>
#include "polymake/meta_function.h"

namespace polymake {

/***********************************************************************
 *
 *  Elementary manipulations on meta-lists
 */

/// construct a meta-list with given elements unless it's already a meta-list itself
template <typename... T>
struct mlist_wrap {
   using type = mlist<T...>;
};

template <>
struct mlist_wrap<void> {
   using type = mlist<>;
};

template <typename... T>
struct mlist_wrap<mlist<T...>> {
   using type = mlist<T...>;
};


/// extract the single element from a meta-list, preserve the list if it contains more than one element
template <typename T>
struct mlist_unwrap {
   using type = T;
};

template <typename T>
struct mlist_unwrap<mlist<T>> {
   using type = T;
};

template <>
struct mlist_unwrap<mlist<>> {
   using type = void;
};


/// extract the leading element of a meta-list
/// for an empty list, return void
template <typename T>
struct mlist_head
   : mlist_unwrap<T> {};

template <typename T, typename... Tail>
struct mlist_head< mlist<T, Tail...> > {
   using type = T;
};


/// extract all but the leading element of a meta-list
/// as a new meta-list
template <typename T>
struct mlist_tail {
   using type = mlist<>;
};

template <typename T, typename... Tail>
struct mlist_tail<mlist<T, Tail...>> {
   using type = mlist<Tail...>;
};


/// An apparently endless meta-list consisting of arbitrarily many copies of the same value.
/// Can be used as a second argument of mlist_filter_binary, mlist_transform_binary,
/// and similar operations involving two meta-lists.
template <typename T>
struct mrepeat {};

template <typename T>
struct mlist_wrap< mrepeat<T> > {
   using type = mrepeat<T>;
};

template <typename T>
struct mlist_unwrap< mrepeat<T> > {
   using type = T;
};

template <typename T>
struct mlist_head< mrepeat<T> > {
   using type = T;
};

template <typename T>
struct mlist_tail< mrepeat<T> > {
   using type = mrepeat<T>;
};


/// compute the length of a meta-list
template <typename T>
struct mlist_length
   : int_constant<!std::is_same<T, void>::value> {};

template <typename... Elements>
struct mlist_length<mlist<Elements...>>
   : int_constant<int(sizeof...(Elements))> {};

template <typename T>
using mlist_is_empty
   = bool_constant<mlist_length<T>::value==0>;


/// retrieve the element at the given position;
/// positions are counted starting with 0.
/// For an out-of-range position no result is defined.
template <typename T, int Pos>
struct mlist_at {};

template <typename T>
struct mlist_at<T, 0>
   : mlist_head<T> {};

template <typename T, typename... Tail>
struct mlist_at<mlist<T, Tail...>, 0> {
   using type = T;
};

template <typename T, typename... Tail, int Pos>
struct mlist_at<mlist<T, Tail...>, Pos>
   : mlist_at<mlist<Tail...>, Pos-1> {};

/// counting position backwards
template <typename T, int Pos>
using mlist_at_rev = mlist_at<T, mlist_length<T>::value-Pos-1>;


template <typename T1, typename T2>
struct mlist_concat2;

template <typename T1, typename... T2>
struct mlist_concat2<T1, mlist<T2...>> {
   using type = mlist<T1, T2...>;
};

template <typename... T1, typename... T2>
struct mlist_concat2<mlist<T1...>, mlist<T2...>> {
   using type = mlist<T1..., T2...>;
};

/// Concatenate single elements and meta-lists into one meta-list.
/// `void' entries and empty meta-lists are elided.
/// The result is always a instance of mlist regardless of the number of contained elements.
template <typename... T>
struct mlist_concat
   : mlist_wrap<T...> {};

template <typename T1, typename T2, typename... Tail>
struct mlist_concat<T1, T2, Tail...>
   : mlist_concat2<typename mlist_wrap<T1>::type,
                   typename mlist_concat<T2, Tail...>::type> {};


/// Concatenate elements conditionally.
/// If Condition is `true', the function is equivalent to mlist_concat.
/// If Condition is `false',List is returned.
template <bool Condition, typename List, typename Suffix>
struct mlist_append_if
   : mlist_concat<List, Suffix> {};

template <typename List, typename Suffix>
struct mlist_append_if<false, List, Suffix>
   : mlist_wrap<List> {};


/// Concatenate elements conditionally.
/// If Condition is `true', the function is equivalent to mlist_concat.
/// If Condition is `false', List is returned.
template <bool Condition, typename Prefix, typename List>
struct mlist_prepend_if
   : mlist_concat<Prefix, List> {};

template <typename Prefix, typename List>
struct mlist_prepend_if<false, Prefix, List>
   : mlist_wrap<List> {};


/** Extract a contiguous slice of a meta-list.
 *  @tparam Start position of the first element to extract.
 *  @tparam End position behind the last element to extract.
 *              If omitted, the selected subset stretches up to the end of the source meta-list.
 *  Positions are counted starting with 0.
 */
template <typename List, int Start, int End = mlist_length<List>::value, int TotalSize = mlist_length<List>::value>
struct mlist_slice
   : mlist_slice<typename mlist_wrap<List>::type, Start, End, TotalSize> {};

template <typename... Elements, int TotalSize>
struct mlist_slice<mlist<Elements...>, 0, TotalSize, TotalSize> {
  static_assert(sizeof...(Elements) == TotalSize,
                "mlist_slice: total list size mismatch");
  using type = mlist<Elements...>;
};

template <typename... Elements, int TotalSize>
struct mlist_slice<mlist<Elements...>, 0, 0, TotalSize> {
  static_assert(sizeof...(Elements) == TotalSize,
                "mlist_slice: total list size mismatch");
  using type = mlist<>;
};

template <>
struct mlist_slice<mlist<>, 0, 0, 0> {
  using type = mlist<>;
};

template <typename... Elements, int End, int TotalSize>
struct mlist_slice<mlist<Elements...>, 0, End, TotalSize>
   : mlist_concat<typename mlist_head<mlist<Elements...>>::type,
                  typename mlist_slice<typename mlist_tail<mlist<Elements...>>::type, 0, End-1, TotalSize-1>::type> {
  static_assert(End > 0 && TotalSize > 0, "mlist_slice - invalid end index");
};

template <typename... Elements, int Start, int End, int TotalSize>
struct mlist_slice<mlist<Elements...>, Start, End, TotalSize>
   : mlist_slice<typename mlist_tail<mlist<Elements...>>::type, Start-1, End-1, TotalSize-1> {
  static_assert(Start > 0, "mlist_slice - invalid start index");
  static_assert(End > 0 && TotalSize > 0, "mlist_slice - invalid end index");
};


/// Extract elements at even positions
template <typename List>
struct mlist_even_subset
   : mlist_even_subset<typename mlist_wrap<List>::type> {};

template <typename T1, typename T2, typename... Tail>
struct mlist_even_subset<mlist<T1, T2, Tail...>>
   : mlist_concat<T1, typename mlist_even_subset<mlist<Tail...>>::type> {};

template <>
struct mlist_even_subset<mlist<>> {
   using type = mlist<>;
};

/// Extract elements at odd positions
template <typename List>
struct mlist_odd_subset
   : mlist_odd_subset<typename mlist_wrap<List>::type> {};

template <typename T1, typename T2, typename... Tail>
struct mlist_odd_subset<mlist<T1, T2, Tail...>>
   : mlist_concat<T2, typename mlist_odd_subset<mlist<Tail...>>::type> {};

template <>
struct mlist_odd_subset<mlist<>> {
   using type = mlist<>;
};


/// Check whether one list coincides with the tail of another list
template <typename List1, typename List2, bool valid=(mlist_length<List2>::value >= mlist_length<List1>::value)>
struct mlist_is_tail_of : std::false_type {};

template <typename List1, typename List2>
struct mlist_is_tail_of<List1, List2, true>
   : std::is_same<List1, typename mlist_slice<List2, (mlist_length<List2>::value - mlist_length<List1>::value)>::type> {};


/// Replace a slice in a meta-list with new elements.
/// The slice is specified like in mlist_slice.
/// If Start==End, no elements are removed; the new elements are inserted at the given position.
template <typename List, int Start, int End, typename... Insert>
using mlist_replace_between
   = mlist_concat<typename mlist_slice<List, 0, Start>::type,
                  Insert...,
                  typename mlist_slice<List, End>::type>;


/// Replace a single element in a meta-list with new elements.
/// When Pos==-1, nothing is changed.
template <typename List, int Pos, typename... Insert>
struct mlist_replace_at
   : mlist_replace_between<List, Pos, Pos+1, Insert...> {};

template <typename List, typename... Insert>
struct mlist_replace_at<List, -1, Insert...> {
   using type = List;
};


/// Remove an element from a meta-list at the given position.
/// When Pos==-1, nothing is changed.
template <typename List, int Pos>
using mlist_remove_at
   = mlist_replace_at<List, Pos>;


/// Construct a meta-list consisting of N copies of the same element
template <typename T, size_t N>
struct mreplicate {
   using type = typename mlist_concat2< typename mreplicate<T, N/2>::type,
                                        typename mreplicate<T, N-N/2>::type >::type;
};

template <typename T>
struct mreplicate<T, 0> {
   using type = mlist<>;
};

template <typename T>
struct mreplicate<T, 1> {
   using type = mlist<T>;
};


/***********************************************************************
 *
 *  Search in and comparison of meta-lists
 */

/// Find the first element in a meta-list evaluating the given unary boolean meta-function to true
/// Avoids instantiation of the meta-function for the tail after the match.
/// @return value true or false
/// @return match list element or void
/// @return pos position of match in the list
/// @return match_with_tail list tail starting with match
template <typename List, template <typename> class Func,
          bool Match = msafely_eval_boolean<typename mlist_head<List>::type, Func>::value>
struct mlist_find_first
  : std::true_type {
  static constexpr int pos = 0;
  using match = typename mlist_head<List>::type;
  using match_with_tail = List;
};

template <template <typename> class Func>
struct mlist_find_first<mlist<>, Func, false>
  : std::false_type {
  static constexpr int pos = -1;
  using match = void;
  using match_with_tail = mlist<>;
};

template <typename List, template <typename> class Func>
struct mlist_find_first<List, Func, false>
  : mlist_find_first<typename mlist_tail<List>::type, Func> {
  using base_t = mlist_find_first<typename mlist_tail<List>::type, Func>;
  static constexpr int pos = base_t::pos + (base_t::pos >= 0);
};


/// Find the first element in a meta-list satisying the given unary or binary boolean meta-function
/// @return @see mlist_find_first
template <typename List, template <typename...> class Func, typename... Args>
struct mlist_find_if;

template <typename List, template <typename> class UnaryFunc>
struct mlist_find_if<List, UnaryFunc>
  : mlist_find_first<typename mlist_wrap<List>::type, UnaryFunc> {};

template <typename List, template <typename, typename> class BinaryFunc, typename RightArg>
struct mlist_find_if<List, BinaryFunc, RightArg>
  : mlist_find_first<typename mlist_wrap<List>::type, mbind2nd<BinaryFunc, RightArg>::template func> {};

/// Find the first occurrence of an element in a meta-list
template <typename List, typename Value>
using mlist_find = mlist_find_if<List, std::is_same, Value>;

/// Tell whether an element is contained in a meta-list
template <typename List, typename Value, template <typename, typename> class Compare = std::is_same>
using mlist_contains
   = bool_constant<mlist_find_if<List, Compare, Value>::value>;


/// Replace an element in a meta-list
/// If it does not occur in the list, the result equals the input list.
template <typename List, typename Element, typename NewElement, template <typename, typename> class Compare = std::is_same>
using mlist_replace
   = mlist_replace_at<List, mlist_find_if<List, Compare, Element>::pos, NewElement>;

/// Remove an element from a meta-list.
/// If it does not occur in the list, the result equals the input list.
template <typename List, typename Element, template <typename, typename> class Compare = std::is_same>
using mlist_remove
   = mlist_remove_at<List, mlist_find_if<List, Compare, Element>::pos>;

/// Tell whether the first type occurs among the rest.
/// Convenience wrapper for SFINAE-overloaded functions.
template <typename T, typename... Expected>
using is_among
   = mlist_contains<typename mlist_wrap<Expected...>::type, T>;


/** Find all pairs of elements of two meta-lists evaluating a binary meta-function to true.
 *  Every element may be used only once.
 *  The search is greedy, it does not try to maximize the mapping.
 *  Following results are defined:
 *    type  list of elements from List1 satisfying Compare
 *    type2 list of corresponding elements from List2
 *    complement  list of elements from List1 left without matching mate
 *    complement2 list of elements from List2 left without matching mate
 */
template <typename List1, typename List2, template <typename, typename> class Compare>
struct mlist_match_impl {
   using head2 = typename mlist_head<List2>::type;
   using find_head2 = mlist_find_if<List1, Compare, head2>;
   static constexpr bool head2_matched = find_head2::pos >= 0;
   using rest1 = typename mlist_remove_at<List1, find_head2::pos>::type;
   using match_rest = mlist_match_impl<rest1, typename mlist_tail<List2>::type, Compare>;
   using type = typename mlist_prepend_if<head2_matched, typename find_head2::match, typename match_rest::type>::type;
   using type2 = typename mlist_prepend_if<head2_matched, head2, typename match_rest::type2>::type;
   using complement = typename match_rest::complement;
   using complement2 = typename mlist_prepend_if<!head2_matched, head2, typename match_rest::complement2>::type;
};

template <typename List1, template <typename, typename> class Compare>
struct mlist_match_impl<List1, mlist<>, Compare> {
   using type = mlist<>;
   using complement = List1;
   using type2 = mlist<>;
   using complement2 = mlist<>;
};

template <typename List2, template <typename, typename> class Compare>
struct mlist_match_impl<mlist<>, List2, Compare> {
   using type = mlist<>;
   using complement = mlist<>;
   using type2 = mlist<>;
   using complement2 = List2;
};

template <template <typename, typename> class Compare>
struct mlist_match_impl<mlist<>, mlist<>, Compare> {
   using type = mlist<>;
   using complement = mlist<>;
   using type2 = mlist<>;
   using complement2 = mlist<>;
};

template <typename List1, typename List2, template <typename, typename> class Compare = std::is_same>
using mlist_match = mlist_match_impl<typename mlist_wrap<List1>::type, typename mlist_wrap<List2>::type, Compare>;

/// Shortcut wrappers around mlist_match

/// Tell whether meta-lists are equal regardless the element order
template <typename List1, typename List2, template <typename, typename> class Compare = std::is_same>
using mlists_are_equivalent
   = bool_constant<(mlist_is_empty<typename mlist_match<List1, List2, Compare>::complement>::value &&
                    mlist_is_empty<typename mlist_match<List1, List2, Compare>::complement2>::value)>;

/// Tell whether meta-lists have any elements in common
template <typename List1, typename List2, template <typename, typename> class Compare = std::is_same>
using mlists_are_intersecting
   = bool_not<mlist_is_empty<typename mlist_match<List1, List2, Compare>::type>>;

/// Tell whether meta-lists do not have any elements in common
template <typename List1, typename List2, template <typename, typename> class Compare = std::is_same>
using mlists_are_disjoint
   = mlist_is_empty<typename mlist_match<List1, List2, Compare>::type>;

/// Tell whether one meta-list is completely contained in another one regardless the element order
template <typename List1, typename List2, template <typename, typename> class Compare = std::is_same>
using mlist_is_included
   = mlist_is_empty<typename mlist_match<List1, List2, Compare>::complement>;


/***********************************************************************
 *
 *  Logical operations on meta-lists
 */

/// Compute the boolean conjunction (AND) of one or more constants.
template <typename... T>
using mlist_and
   = bool_not<mlist_find_first<typename mlist_concat<T...>::type, bool_not>>;

/// Compute the boolean disjunction (OR) of one or more constants.
template <typename... T>
using mlist_or
   = mlist_find_first<typename mlist_concat<T...>::type, mvalue_of>;

/// like mlist_and, but delivers false_type on empty input
template <typename... T>
struct mlist_and_nonempty
   : mlist_and<T...> {};

template <>
struct mlist_and_nonempty<>
   : std::false_type {};

/***********************************************************************
 *
 *  Meta-list transformation
 */

/// Reverse the elements in a meta-list
template <typename T>
struct mlist_reverse {
   using type = T;
};

template <typename T, typename... Tail>
struct mlist_reverse<mlist<T, Tail...>>
   : mlist_concat<typename mlist_reverse<mlist<Tail...>>::type, T> {};


template <typename T, int Depth>
struct mlist_flatten_impl {
   using type = T;
};

template <typename T, typename... Tail>
struct mlist_flatten_impl<mlist<T, Tail...>, 0> {
   using type = mlist<T, Tail...>;
};

template <typename T, typename... Tail, int Depth>
struct mlist_flatten_impl<mlist<T, Tail...>, Depth>
   : mlist_concat< typename mlist_flatten_impl<T, Depth-1>::type,
                   typename mlist_flatten_impl<Tail, Depth-1>::type... > {};

/*! Create a flattened list from a list of (possibly) nested lists.
 *  Empty elements are elided.
 *  TDepth::value sets the descending depth limit:
 *  0 means no conversion at all; -1 means unlimited descending
 */
template <typename List, typename Depth=int_constant<-1>>
using mlist_flatten
   = mlist_flatten_impl<typename mlist_wrap<List>::type, Depth::value>;


/// Apply a unary meta-function to the elements of a meta-list
template <typename T, template <typename> class UnaryFunc>
struct mlist_transform_unary
   : mlist_transform_unary<typename mlist_wrap<T>::type, UnaryFunc> {};

template <typename... T, template <typename> class UnaryFunc>
struct mlist_transform_unary<mlist<T...>, UnaryFunc> {
   using type = mlist<typename UnaryFunc<T>::type...>;
};


/// Apply a binary meta-function pairwise to the elements of two meta-lists
template <typename T1, typename T2, template <typename, typename> class BinaryFunc>
struct mlist_transform_binary
   : mlist_transform_binary<typename mlist_wrap<T1>::type, typename mlist_wrap<T2>::type, BinaryFunc> {};

template <typename... T1, typename... T2, template <typename, typename> class BinaryFunc>
struct mlist_transform_binary<mlist<T1...>, mlist<T2...>, BinaryFunc> {
   static_assert(sizeof...(T1) == sizeof...(T2), "mlist_transform_binary - list size mismatch");
   using type = mlist<typename BinaryFunc<T1, T2>::type...>;
};

template <typename... T1, typename T2, template <typename, typename> class BinaryFunc>
struct mlist_transform_binary<mlist<T1...>, mrepeat<T2>, BinaryFunc> {
   using type = mlist<typename BinaryFunc<T1, T2>::type...>;
};

template <typename T1, typename... T2, template <typename, typename> class BinaryFunc>
struct mlist_transform_binary<mrepeat<T1>, mlist<T2...>, BinaryFunc> {
   using type = mlist<typename BinaryFunc<T1, T2>::type...>;
};

template <typename T1, typename T2, template <typename, typename> class BinaryFunc>
struct mlist_transform_binary<mrepeat<T1>, mrepeat<T2>, BinaryFunc>
   : mlist_wrap<typename BinaryFunc<T1, T2>::type> {};


template <typename... T>
struct mlist_remove_void;

template <typename T, typename... Tail>
struct mlist_remove_void<T, Tail...>
   : mlist_concat2<T, typename mlist_remove_void<Tail...>::type> {};

template <typename... Tail>
struct mlist_remove_void<void, Tail...>
   : mlist_remove_void<Tail...> {};

template <>
struct mlist_remove_void<> {
   using type = mlist<>;
};


/// Select elements of a meta-list satisfying the given unary boolean meta-function.
/// The order of elements is preserved.
template <typename T, template <typename> class UnaryFunc>
struct mlist_filter_unary
   : mlist_filter_unary<typename mlist_wrap<T>::type, UnaryFunc> {};

template <typename... T, template <typename> class UnaryFunc>
struct mlist_filter_unary<mlist<T...>, UnaryFunc>
   : mlist_remove_void<std::conditional_t<UnaryFunc<T>::value, T, void>...> {};


/** Split the elements of two meta-lists depending on results of a binary meta-function.
 *  Following results are defined:
 *    type  list of elements from List1 evaluating BinaryFunc to true
 *    type2 list of corresponding elements from List2
 *    complement  list of elements from List1 evaluating BinaryFunc to false
 *    complement2 list of corresponding elements from List2
 */
template <typename List1, typename List2, template <typename, typename> class BinaryFunc>
struct mlist_filter_binary
   : mlist_filter_binary<typename mlist_wrap<List1>::type, typename mlist_wrap<List2>::type, BinaryFunc> {};

template <typename... T1, typename... T2, template <typename, typename> class BinaryFunc>
struct mlist_filter_binary<mlist<T1...>, mlist<T2...>, BinaryFunc> {
   static_assert(sizeof...(T1) == sizeof...(T2), "mlist_filter_binary - meta-lists size mismatch");
   using type = typename mlist_remove_void<std::conditional_t<BinaryFunc<T1, T2>::value, T1, void>...>::type;
   using type2 = typename mlist_remove_void<std::conditional_t<BinaryFunc<T1, T2>::value, T2, void>...>::type;
   using complement = typename mlist_remove_void<std::conditional_t<BinaryFunc<T1, T2>::value, void, T1>...>::type;
   using complement2 = typename mlist_remove_void<std::conditional_t<BinaryFunc<T1, T2>::value, void, T2>...>::type;
};

template <typename... T1, typename T2, template <typename, typename> class BinaryFunc>
struct mlist_filter_binary<mlist<T1...>, mrepeat<T2>, BinaryFunc> {
   using type = typename mlist_remove_void<std::conditional_t<BinaryFunc<T1, T2>::value, T1, void>...>::type;
   using complement = typename mlist_remove_void<std::conditional_t<BinaryFunc<T1, T2>::value, void, T1>...>::type;
   using type2 = std::conditional_t<mlist_is_empty<type>::value, mlist<>, typename mlist_wrap<T2>::type>;
   using complement2 = std::conditional_t<mlist_is_empty<type>::value, typename mlist_wrap<T2>::type, mlist<>>;
};

template <typename T1, typename... T2, template <typename, typename> class BinaryFunc>
struct mlist_filter_binary<mrepeat<T1>, mlist<T2...>, BinaryFunc> {
   using type2 = typename mlist_remove_void<std::conditional_t<BinaryFunc<T1, T2>::value, T2, void>...>::type;
   using complement2 = typename mlist_remove_void<std::conditional_t<BinaryFunc<T1, T2>::value, void, T2>...>::type;
   using type = std::conditional_t<mlist_is_empty<type2>::value, mlist<>, typename mlist_wrap<T1>::type>;
   using complement = std::conditional_t<mlist_is_empty<type2>::value, typename mlist_wrap<T1>::type, mlist<>>;
};

template <typename T1, typename T2, template <typename, typename> class BinaryFunc>
struct mlist_filter_binary<mrepeat<T1>, mrepeat<T2>, BinaryFunc> {
   static constexpr bool match = BinaryFunc<T1, T2>::value;
   using type = std::conditional_t<match, typename mlist_wrap<T1>::type, mlist<>>;
   using type2 = std::conditional_t<match, typename mlist_wrap<T2>::type, mlist<>>;
   using complement = std::conditional_t<match, mlist<>, typename mlist_wrap<T1>::type>;
   using complement2 = std::conditional_t<match, mlist<>, typename mlist_unwrap<T2>::type>;
};


/** Fold a meta-list applying a binary meta-function to its elements.
 *  Folding starts at the tail of the list.
 *  If it should start at the head, the input list must be reversed (@see mlist_reverse).
 *  The elements of the list are passed as the left argument of BinaryFunc,
 *  the accumulated intermediate result as the right argument.
 *  The initial value is passed as the right argument for the very first application of BinaryFunc.
 *  If it is void, the tail input element is taken as is.
 */
template <typename T, template <typename, typename> class BinaryFunc, typename InitVal=void>
struct mlist_fold
   : mlist_fold<typename mlist_wrap<T>::type, BinaryFunc, InitVal> {};

template <typename T, typename... Tail, template <typename, typename> class BinaryFunc, typename InitVal>
struct mlist_fold<mlist<T, Tail...>, BinaryFunc, InitVal> {
   using folded_tail = typename mlist_fold<mlist<Tail...>, BinaryFunc, InitVal>::type;
   using type = typename BinaryFunc<T, folded_tail>::type;
};

template <typename T, template <typename, typename> class BinaryFunc>
struct mlist_fold<mlist<T>, BinaryFunc, void> {
   using type = T;
};

template <template <typename, typename> class BinaryFunc, typename InitVal>
struct mlist_fold<mlist<>, BinaryFunc, InitVal> {
   using type = InitVal;
};


/** Transform and fold a meta-list.
 *  Every element is transformed with a given unary meta-function Transform, the result is passed
 *  as a left argument to a binary meta-function Fold;  the right argument is the accumulated result
 *  for the processed part of the list.
 *  Folding starts at the tail of the list.
 *  If it should start at the head, the input list must be reversed (@see mlist_reverse).
 */
template <typename T, template <typename> class Transform, template <typename, typename> class Fold>
struct mlist_fold_transform
   : mlist_fold_transform<typename mlist_wrap<T>::type, Transform, Fold> {};

template <typename T, template <typename> class Transform, template <typename, typename> class Fold>
struct mlist_fold_transform<mlist<T>, Transform, Fold> {
   using type = typename Transform<T>::type;
};

template <template <typename> class Transform, template <typename, typename> class Fold>
struct mlist_fold_transform<mlist<>, Transform, Fold> {
   using type = void;
};

template <typename T, typename T2, typename... Tail, template <typename> class Transform, template <typename, typename> class Fold>
struct mlist_fold_transform<mlist<T, T2, Tail...>, Transform, Fold> {
   using folded_tail = typename mlist_fold_transform<mlist<T2, Tail...>, Transform, Fold>::type;
   using type = typename Fold<typename Transform<T>::type, folded_tail>::type;
};


template <template <typename, typename> class Compare>
struct mlist_remove_duplicates_impl {
   template <typename Element, typename List>
   using func
      = mlist_prepend_if<!mlist_contains<List, Element, Compare>::value, Element, List>;
};

/// Remove duplicate elements from a meta-list
template <typename List, template <typename, typename> class Compare = std::is_same>
using mlist_remove_duplicates
   = mlist_wrap<typename mlist_fold<List, mlist_remove_duplicates_impl<Compare>::template func>::type>;


/// Compute a union of two meta-lists.
/// Lists are supposed to be free from duplicates.
template <typename List1, typename List2>
using mlist_union
   = mlist_fold<List1, mlist_remove_duplicates_impl<std::is_same>::template func, List2>;

/// Compute an intersection of two meta-lists.
/// Lists are supposed to be free from duplicates.
template <typename List1, typename List2>
struct mlist_intersection {
   using type = typename mlist_match<List1, List2>::type;
};

/// Compute a difference of two meta-lists.
/// Lists are supposed to be free from duplicates.
template <typename List1, typename List2>
struct mlist_difference {
   using type = typename mlist_match<List1, List2>::complement;
};

/// Compute a symmetric difference of two meta-lists.
/// Lists are supposed to be free from duplicates.
template <typename List1, typename List2>
using mlist_symdifference
   = mlist_concat<typename mlist_match<List1, List2>::complement,
                  typename mlist_match<List1, List2>::complement2>;


/** Find all pairs of elements of two meta-lists evaluating a binary meta-function to true.
 *  Elements can be paired multiple times with different mates.
 *  Results are structured like in mlist_match.
 *  type and type2 may have different length, the order of their elements is unrelated to each other.
 */
template <typename List1, typename List2, template <typename, typename> class Compare = std::is_same>
struct mlist_match_all {
   using head2 = typename mlist_head<List2>::type;
   using match_head = mlist_filter_binary<List1, mrepeat<head2>, Compare>;
   using match_tail = mlist_match_all<List1, typename mlist_tail<List2>::type, Compare>;
   using type        = typename mlist_union<typename match_head::type, typename match_tail::type>::type;
   using complement  = typename mlist_intersection<typename match_head::complement, typename match_tail::complement>::type;
   using type2       = typename mlist_concat<typename match_head::type2, typename match_tail::type2>::type;
   using complement2 = typename mlist_concat<typename match_head::complement2, typename match_tail::complement2>::type;
};

template <typename List1, template <typename, typename> class Compare>
struct mlist_match_all<List1, mlist<>, Compare> {
   using type = mlist<>;
   using complement = typename mlist_wrap<List1>::type;
   using type2 = mlist<>;
   using complement2 = mlist<>;
};


/***********************************************************************
 *
 *  Tagged meta-lists
 */

template <template <typename> class Tag, typename T>
struct mtag_value {
   using type = Tag<T>;
};

template <template <typename> class Tag>
struct mtag_value<Tag, void> {
   using type = void;
};

template <typename T>
struct muntag {
   using type = T;
};

template <template <typename> class Tag, typename T>
struct muntag<Tag<T>> {
   using type = T;
};

template <typename T>
using muntag_t = typename muntag<T>::type;

template <template <typename> class Tag>
struct mmatching_tag {
   template <typename T>
   using func = is_instance_of<T, Tag>;
};

template <typename T>
struct mis_properly_tagged
   : std::false_type {};

template <template <typename> class... Tag, typename... T>
struct mis_properly_tagged< mlist<Tag<T>...> >
   : std::true_type {};


/** Extract an element with a desired tag from a tagged meta-list.
 *  Defines following results:
 *  value = matching value or Default if nothing found
 *  is_specified = true If the tag occurs in the list
 *  tagged_value = Tag<Value> or void if nothing found
 */
template <typename List, template <typename> class Tag, typename Default = void>
struct mtagged_list_extract {
   static_assert(mis_properly_tagged<List>::value, "not a proper tagged meta-list");

   using finder = mlist_find_if<List, mmatching_tag<Tag>::template func>;
   static constexpr bool is_specified = finder::value;
   using tagged_value = typename finder::match;
   using tagged_list = typename mlist_wrap<tagged_value>::type;
   using type = std::conditional_t<is_specified, muntag_t<tagged_value>, Default>;

   static_assert(!mlist_find_if<typename mlist_tail<typename finder::match_with_tail>::type, mmatching_tag<Tag>::template func>::value,
                 "a tag occurs multiple times in a meta-list");
};

template <typename List, template <typename> class Tag, typename Value>
constexpr
Value tagged_list_extract_integral(const Value deflt)
{
   using extractor = mtagged_list_extract<List, Tag, std::integral_constant<Value, Value(0)>>;
   return extractor::is_specified ? extractor::type::value : deflt;
}


/** Modify a tagged meta-list: Replace the element with the given tag by the result of a binary meta-function
 *  applied to the element and a second value.
 *  If the tag does not occur in the source list, TOperation<void, TValue2>::type is appended to the result list.
 */
template <typename List, template <typename> class Tag, template <typename...> class Operation, typename... MoreOperands>
struct mtagged_list_modify
   : mlist_concat< typename mlist_head<List>::type,
                   typename mtagged_list_modify<typename mlist_tail<List>::type, Tag, Operation, MoreOperands...>::type >
{
   static_assert(mis_properly_tagged<List>::value, "not a proper tagged meta-list");
};

template <typename Value, typename... Tail, template <typename> class Tag,
          template <typename...> class Operation, typename... MoreOperands>
struct mtagged_list_modify<mlist<Tag<Value>, Tail...>, Tag, Operation, MoreOperands...>
   : mlist_concat< typename mtag_value<Tag, typename Operation<Value, MoreOperands...>::type>::type, mlist<Tail...> > {
   static_assert(!mtagged_list_extract<mlist<Tail...>, Tag>::is_specified, "a tag occurs multiple times in a meta-list");
};

template <template <typename> class Tag, template <typename...> class Operation, typename... MoreOperands>
struct mtagged_list_modify<mlist<>, Tag, Operation, MoreOperands...>
   : mlist_wrap<typename mtag_value<Tag, typename Operation<void, MoreOperands...>::type>::type> {};


/// Replace values of elements of a tagged meta-list.
/// If some tag does not occur in the source list, the new value is appended to the result.
template <typename List, typename... Replacements>
struct mtagged_list_replace;

template <typename List>
struct mtagged_list_replace<List> {
   using type = List;
};

template <typename List, template <typename> class Tag, typename Value, typename... MoreReplacements>
struct mtagged_list_replace<List, Tag<Value>, MoreReplacements...>
   : mtagged_list_modify<typename mtagged_list_replace<List, MoreReplacements...>::type, Tag, mproject2nd, Value> {};


/// Append elements to a tagged meta-list unless another elements with identical tags are already there.
template <typename List, typename... Defaults>
struct mtagged_list_add_default;

template <typename List>
struct mtagged_list_add_default<List> {
   using type = List;
};

template <typename List, template <typename> class Tag, typename Value, typename... MoreDefaults>
struct mtagged_list_add_default<List, Tag<Value>, MoreDefaults...>
   : mtagged_list_modify<typename mtagged_list_add_default<List, MoreDefaults...>::type, Tag, mprefer1st, Value> {};


/// Remove an element with a given tag from a tagged meta-list.
/// If the tag does not occur in the source list, the result list is identical to the source one.
template <typename List, template <typename> class Tag>
struct mtagged_list_remove {
   using type = typename mlist_remove_at<List, mlist_find_if<List, mmatching_tag<Tag>::template func>::pos>::type;
   static_assert(!mtagged_list_extract<type, Tag>::is_specified, "a tag occurs multiple times in a meta-list");
};


template <typename T1, typename T2>
struct has_same_tag
   : std::false_type {};

template <typename T1, typename T2, template <typename> class Tag>
struct has_same_tag<Tag<T1>, Tag<T2>>
   : std::true_type {};

/// Find all elements with desired tags in a tagged meta-list.
/// The desired tags can be specified with arbitrary contents, e.g. with void.
template <typename List, typename Tags>
using mtagged_list_intersect
   = mlist_match_all<List, Tags, has_same_tag>;


/// Combine two tagged meta-lists.
/// Elements with equal tags are merged by concatenation and elimination of duplicates.
template <typename List1, typename List2>
struct mtagged_list_concat
   : mlist_concat<List1, List2> {};

template <template <typename> class Tag, typename Value, typename... Tail, typename List2>
struct mtagged_list_concat<mlist<Tag<Value>, Tail...>, List2> {
   using value2 = typename mtagged_list_extract<List2, Tag>::type;
   using combined_value = typename mlist_unwrap<typename mlist_remove_duplicates<typename mlist_concat<Value, value2>::type>::type>::type;
   using result_tail = typename mtagged_list_concat<mlist<Tail...>, typename mtagged_list_remove<List2, Tag>::type>::type;
   using type = typename mlist_concat<Tag<combined_value>, result_tail>::type;
};

/***********************************************************************
 *
 *  Ordered meta-lists
 */

/// Insert an element into a sorted meta-list at the appropriate position
/// Duplicates are allowed
template <typename List, typename AddElement, template <typename, typename> class Less = mis_less>
struct mlist_insert {
   using finder = mlist_find_if<List, mnegate_binary<Less>::template func, AddElement>;
   static constexpr int cnt_before = finder::value ? finder::pos : mlist_length<List>::value;
   using type = typename mlist_concat<
      typename mlist_slice<List, 0, cnt_before>::type,
      AddElement,
      typename finder::match_with_tail>::type;
};

/// Insert an element into a sorted meta-list at the appropriate position
/// unless it is already contained in the list
template <typename List, typename AddElement, template <typename, typename> class Less = mis_less>
struct mlist_insert_unique {
   using finder = mlist_find_if<List, mnegate_binary<Less>::template func, AddElement>;
   static constexpr int cnt_before = finder::value ? finder::pos : mlist_length<List>::value;
   using type = typename mlist_concat<
      typename mlist_slice<List, 0, cnt_before>::type,
      typename mlist_prepend_if<!std::is_same<AddElement, typename finder::match>::value,
                                AddElement, typename finder::match_with_tail>::type>::type;
};


/// Sort a meta-list according to the given element comparator
template <typename List, template <typename, typename> class Less = mis_less>
struct mlist_sort
   : mlist_sort<typename mlist_wrap<List>::type, Less> {};

template <typename T, typename... Tail, template <typename, typename> class Less>
struct mlist_sort<mlist<T, Tail...>, Less>
   : mlist_insert<typename mlist_sort<mlist<Tail...>, Less>::type, T, Less> {};

template <template <typename, typename> class Less>
struct mlist_sort<mlist<>, Less> {
  using type = mlist<>;
};

}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
