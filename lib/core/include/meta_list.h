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

#ifndef POLYMAKE_META_LIST_H
#define POLYMAKE_META_LIST_H

#include <type_traits>
#include <tuple>
#include "polymake/meta_function.h"

namespace polymake {

#if defined(__clang__) && defined(_LIBCPP_VERSION)

template <typename... T>
struct mlist_and
   : std::true_type {};

template <typename T, typename... TTail>
struct mlist_and<T, TTail...>
   : std::integral_constant<bool, T::value && mlist_and<TTail...>::value> {};

template <typename... T>
struct mlist_or
   : std::false_type {};

template <typename T, typename... TTail>
struct mlist_or<T, TTail...>
   : std::integral_constant<bool, T::value || mlist_or<TTail...>::value> {};

template <typename T>
struct bool_not
   : std::integral_constant<bool, !T::value> {};

#else // GNU libstdc++

/// combine all `value' members with boolean AND;  return true for an empty list
template <typename... T>
using mlist_and = std::__and_<T...>;

/// combine all `value' members with boolean OR;  return false for an empty list
template <typename... T>
using mlist_or = std::__or_<T...>;

/// logical inversion of a compile-time boolean constant
template <typename T>
using bool_not = std::__not_<T>;

#endif

/// like mlist_and, but delivers false_type on empty input
template <typename... T>
struct mlist_and_nonempty
   : mlist_and<T...> {};

template <>
struct mlist_and_nonempty<>
   : std::false_type {};

/***********************************************************************
 *
 *  Elementary manipulations on meta-lists
 */

/// compute the length of a meta-list
template <typename T>
struct mlist_length
   : int_constant<!std::is_same<T, void>::value> {};

template <typename... TElements>
struct mlist_length<mlist<TElements...>>
   : int_constant<sizeof...(TElements)> {};


/// retrieve the element at the given position;
/// positions are counted starting with 0.
/// For an out-of-range position no result is defined.
template <typename T, int TPos>
struct mlist_at {};

template <typename T>
struct mlist_at<T, 0> {
   typedef T type;
};

template <typename T, typename... TTail>
struct mlist_at<mlist<T, TTail...>, 0> {
   typedef T type;
};

template <typename T, typename... TTail, int TPos>
struct mlist_at<mlist<T, TTail...>, TPos>
   : mlist_at<mlist<TTail...>, TPos-1> {};


/// extract the leading element of a meta-list
/// for an empty list, return void
template <typename T>
struct mlist_head
   : mlist_at<T, 0> {};

template <>
struct mlist_head< mlist<> > {
   typedef void type;
};


/// extract all but the leading element of a meta-list
/// as a new meta-list
template <typename T>
struct mlist_tail {
   typedef mlist<> type;
};

template <typename T, typename... TTail>
struct mlist_tail<mlist<T, TTail...>> {
   typedef mlist<TTail...> type;
};


/// An apparently endless meta-list consisting of arbitrarily many copies of the same value.
/// Can be used as a second argument of mlist_filter_binary, mlist_transform_binary,
/// and similar operations involving two meta-lists.
template <typename T>
struct mrepeat {};

template <typename T>
struct mlist_head< mrepeat<T> > {
   typedef T type;
};

template <typename T>
struct mlist_tail< mrepeat<T> > {
   typedef mrepeat<T> type;
};


template <typename T1, typename T2, bool TDescend=(mlist_length<T1>::value > 1 || mlist_length<T2>::value > 1)>
struct mlist_coherent_tails {
   static_assert(is_instance_of<T1, mrepeat>::value ||
                 is_instance_of<T2, mrepeat>::value ||
                 mlist_length<T1>::value == mlist_length<T2>::value,
                 "attempt to combine meta-lists of different length in a binary operation");
   typedef typename mlist_tail<T1>::type type;
   typedef typename mlist_tail<T2>::type type2;
};

template <typename T1, typename T2>
struct mlist_coherent_tails<T1, T2, false> {
   typedef mlist<> type;
   typedef mlist<> type2;
};


/// construct a meta-list with given elements unless it's already a meta-list itself
template <typename... T>
struct mlist_wrap {
   typedef mlist<T...> type;
};

template <>
struct mlist_wrap<void> {
   typedef mlist<> type;
};

template <typename... T>
struct mlist_wrap<mlist<T...>> {
   typedef mlist<T...> type;
};


/// extract the single element from a meta-list, preserve the list if it contains more than one element
template <typename T>
struct mlist_unwrap {
   typedef T type;
};

template <typename T>
struct mlist_unwrap<mlist<T>> {
   typedef T type;
};

template <>
struct mlist_unwrap<mlist<>> {
   typedef void type;
};


template <typename T1, typename T2>
struct mlist_concat2;

template <typename... T1, typename... T2>
struct mlist_concat2<mlist<T1...>, mlist<T2...>> {
   typedef mlist<T1..., T2...> type;
};

/// Concatenate single elements and meta-lists into one meta-list.
/// `void' entries and empty meta-lists are elided.
/// The result is always a instance of mlist even if it contains zero or one element.
template <typename T, typename... TTail>
struct mlist_concat
   : mlist_concat2<typename mlist_wrap<T>::type,
                   typename mlist_concat<TTail...>::type> {};

template <typename T>
struct mlist_concat<T>
   : mlist_wrap<T> {};


/// Concatenate elements conditionally.
/// If TEnable is `true', the function is equivalent to mlist_concat.
/// If TEnable is `false', T1 is returned.
template <bool TEnable, typename T1, typename T2>
struct mlist_append_if
   : mlist_concat<T1, T2> {};

template <typename T1, typename T2>
struct mlist_append_if<false, T1, T2> {
   typedef T1 type;
};


/// Concatenate elements conditionally.
/// If TEnable is `true', the function is equivalent to mlist_concat.
/// If TEnable is `false', T2 is returned.
template <bool TEnable, typename T1, typename T2>
struct mlist_prepend_if
   : mlist_concat<T1, T2> {};

template <typename T1, typename T2>
struct mlist_prepend_if<false, T1, T2> {
   typedef T2 type;
};


/** Extract a subset of the given meta-list.
 *  @tparam TStart position of the first element to extract.
 *  @tparam TEnd position behind the last element to extract.
 *               If omitted, the selected subset stretches up to the end of the source meta-list.
 *  Positions are counted starting with 0.
 */
template <typename TList, int TStart,
          int TEnd=mlist_length<TList>::value,
          bool TValid=(0 <= TStart && TStart <= TEnd && TEnd <= mlist_length<TList>::value)>
struct mlist_subset;

template <typename TList>
struct mlist_subset<TList, 0, 0, true> {
   typedef mlist<> type;
};

template <typename TList, int TEnd>
struct mlist_subset<TList, 0, TEnd, true>
   : mlist_concat<typename mlist_head<TList>::type,
                  typename mlist_subset<typename mlist_tail<TList>::type, 0, TEnd-1, true>::type> {};

template <typename TList, int TStart, int TEnd>
struct mlist_subset<TList, TStart, TEnd, true>
   : mlist_subset<typename mlist_tail<TList>::type, TStart-1, TEnd-1> {};


/// Replace a slice in a meta-list with new elements.
/// The slice is specified like in mlist_subset.
/// If TStart==TEnd, no elements are removed; the new elements are inserted at the given position.
template <typename TList, int TStart, int TEnd, typename TInsert>
struct mlist_replace_between
   : mlist_concat<typename mlist_subset<TList, 0, TStart>::type,
                  typename mlist_concat<TInsert, typename mlist_subset<TList, TEnd>::type>::type> {};


/// Replace a single element in a meta-list with new elements.
/// When TPos==-1, nothing is changed.
template <typename TList, int TPos, typename TInsert>
struct mlist_replace_at
   : mlist_replace_between<TList, TPos, TPos+1, TInsert> {};

template <typename TList, typename TInsert>
struct mlist_replace_at<TList, -1, TInsert> {
   typedef TList type;
};


/// Remove an element from a meta-list at the given position.
/// When TPos==-1, nothing is changed.
template <typename TList, int TPos>
struct mlist_remove_at
   : mlist_replace_at<TList, TPos, void> {};


/***********************************************************************
 *
 *  Search in and comparison of meta-lists
 */

/// Find the first element of the meta-list such that it satisfies the given binary meta-function
/// in combination with a given value.  Return the position of the matching element or -1 if nothing found.
template <typename TList, typename TValue, template <typename, typename> class TCompare = std::is_same>
struct mlist_find {
   static constexpr bool head_match = TCompare<typename mlist_head<TList>::type, TValue>::value;
   static constexpr int tail_pos = mlist_find<typename mlist_tail<TList>::type, TValue, TCompare>::value;
   static constexpr int value = head_match ? 0 : tail_pos + (tail_pos >= 0);
};

template <typename TValue, template <typename, typename> class TCompare>
struct mlist_find<mlist<>, TValue, TCompare>
   : int_constant<-1> {};


/// Tell whether an element is contained in a meta-list
template <typename TList, typename TValue, template <typename, typename> class TCompare = std::is_same>
struct mlist_contains
   : bool_constant<(mlist_find<TList, TValue, TCompare>::value >= 0)> {};


/// Remove an element from a meta-list.
/// If it does not occur in the list, the result is equal the source list.
template <typename TList, typename TValue, template <typename, typename> class TCompare = std::is_same>
struct mlist_remove
   : mlist_remove_at<TList, mlist_find<TList, TValue, TCompare>::value> {};


/// Tell whether the first type occurs among the rest.
/// Convenience wrapper for SFINAE-overloaded functions.
template <typename T, typename... TExpected>
struct is_among
   : mlist_contains<typename mlist_concat<TExpected...>::type, T> {};


/** Find all pairs of elements of two meta-lists evaluating a binary meta-function to true.
 *  Every element may be used only once.
 *  The search is greedy, it does not try to maximize the mapping.
 *  Following results are defined:
 *    type  list of elements from TList1 satisfying TCompare
 *    type2 list of corresponding elements from TList2
 *    complement  list of elements from TList1 left without matching mate
 *    complement2 list of elements from TList2 left without matching mate
 */
template <typename TList1, typename TList2, template <typename, typename> class TCompare = std::is_same>
struct mlist_match {
   typedef typename mlist_head<TList2>::type head2;
   typedef mlist_find<TList1, head2, TCompare> find_head2;
   static constexpr bool head2_matched = find_head2::value >= 0;
   typedef typename mlist_at<TList1, find_head2::value+!head2_matched>::type match1;
   typedef typename mlist_remove_at<TList1, find_head2::value>::type rest1;
   typedef mlist_match<rest1, typename mlist_tail<TList2>::type, TCompare> match_rest;
   typedef typename mlist_prepend_if<head2_matched, match1, typename match_rest::type>::type type;
   typedef typename mlist_prepend_if<head2_matched, head2, typename match_rest::type2>::type type2;
   typedef typename match_rest::complement complement;
   typedef typename mlist_prepend_if<!head2_matched, head2, typename match_rest::complement2>::type complement2;
};

template <typename TList1, template <typename, typename> class TCompare>
struct mlist_match<TList1, mlist<>, TCompare> {
   typedef mlist<> type;
   typedef TList1  complement;
   typedef mlist<> type2;
   typedef mlist<> complement2;
};

template <typename TList2, template <typename, typename> class TCompare>
struct mlist_match<mlist<>, TList2, TCompare> {
   typedef mlist<> type;
   typedef mlist<> complement;
   typedef mlist<> type2;
   typedef TList2  complement2;
};

template <template <typename, typename> class TCompare>
struct mlist_match<mlist<>, mlist<>, TCompare> {
   typedef mlist<> type;
   typedef mlist<> complement;
   typedef mlist<> type2;
   typedef mlist<> complement2;
};


/// Shortcut wrappers around mlist_match

/// Tell whether meta-lists are equal regardless the element order
template <typename TList1, typename TList2, template <typename, typename> class TCompare = std::is_same>
struct mlists_are_equivalent
   : bool_constant<(mlist_length<typename mlist_match<TList1, TList2, TCompare>::complement>::value==0 &&
                    mlist_length<typename mlist_match<TList1, TList2, TCompare>::complement2>::value==0)> {};

/// Tell whether meta-lists have any elements in common
template <typename TList1, typename TList2, template <typename, typename> class TCompare = std::is_same>
struct mlists_are_intersecting
   : bool_constant<(mlist_length<typename mlist_match<TList1, TList2, TCompare>::type>::value > 0)> {};

/// Tell whether meta-lists do not have any elements in common
template <typename TList1, typename TList2, template <typename, typename> class TCompare = std::is_same>
struct mlists_are_disjoint
   : bool_constant<(mlist_length<typename mlist_match<TList1, TList2, TCompare>::type>::value == 0)> {};

/// Tell whether one meta-list is completely contained in another one regardless the element order
template <typename TList1, typename TList2, template <typename, typename> class TCompare = std::is_same>
struct mlist_is_included
   : bool_constant<(mlist_length<typename mlist_match<TList1, TList2, TCompare>::complement>::value==0)> {};


/***********************************************************************
 *
 *  Meta-list transformation
 */

/// Reverse the elements in a meta-list
template <typename TList>
struct mlist_reverse
   : mlist_concat<typename mlist_reverse<typename mlist_tail<TList>::type>::type, typename mlist_head<TList>::type> {};

template <>
struct mlist_reverse< mlist<> > {
   typedef mlist<> type;
};


template <typename T, int TDepth>
struct mlist_flatten_impl {
   typedef T type;
};

template <typename T, typename... TTail>
struct mlist_flatten_impl<mlist<T, TTail...>, 0> {
   typedef mlist<T, TTail...> type;
};

template <typename T, typename... TTail, int TDepth>
struct mlist_flatten_impl<mlist<T, TTail...>, TDepth>
   : mlist_concat< typename mlist_flatten_impl<T, TDepth-1>::type,
                   typename mlist_flatten_impl<TTail, TDepth-1>::type... > {};

/*! Create a flattened list from a list of (possibly) nested lists.
 *  Empty elements are elided.
 *  TDepth::value sets the descending depth limit:
 *  0 means no conversion at all; -1 means unlimited descending
 */
template <typename TList, typename TDepth=int_constant<-1>>
struct mlist_flatten
   : mlist_flatten_impl<TList, TDepth::value> {};


/// Apply a unary meta-function to the elements of a meta-list
template <typename T, template <typename> class TOperation>
struct mlist_transform_unary {
   typedef typename TOperation<T>::type type;
};

template <typename... T, template <typename> class TOperation>
struct mlist_transform_unary<mlist<T...>, TOperation> {
   typedef mlist<typename TOperation<T>::type...> type;
};


/// Apply a binary meta-function pairwise to the elements of two meta-lists
template <typename T1, typename T2, template <typename, typename> class TOperation>
struct mlist_transform_binary {
   static_assert(mlist_length<T1>::value==1 && mlist_length<T2>::value==1,
                 "attempt to combine a meta-list with a single element in mlist_transform_binary");
   typedef typename TOperation<T1, T2>::type type;
};

template <typename... T1, typename... T2, template <typename, typename> class TOperation>
struct mlist_transform_binary<mlist<T1...>, mlist<T2...>, TOperation> {
   typedef mlist<typename TOperation<T1, T2>::type...> type;
};

template <typename... T1, typename T2, template <typename, typename> class TOperation>
struct mlist_transform_binary<mlist<T1...>, mrepeat<T2>, TOperation> {
   typedef mlist<typename TOperation<T1, T2>::type...> type;
};

template <typename T1, typename... T2, template <typename, typename> class TOperation>
struct mlist_transform_binary<mrepeat<T1>, mlist<T2...>, TOperation> {
   typedef mlist<typename TOperation<T1, T2>::type...> type;
};


/** Split the elements of a meta-list depending on results of a unary meta-function.
 *  Following results are defined:
 *    type  list of elements evaluating TOperation to true
 *    complement  list of elements evaluating TOperation to false
 *  The order of elements is preserved.
 */
template <typename TList, template <typename> class TOperation>
struct mlist_filter_unary {
   typedef typename mlist_head<TList>::type head;
   typedef TOperation<head> eval_head;
   typedef mlist_filter_unary<typename mlist_tail<TList>::type, TOperation> eval_tail;
   typedef typename mlist_prepend_if<eval_head::value, head, typename eval_tail::type>::type type;
   typedef typename mlist_prepend_if<!eval_head::value, head, typename eval_tail::complement>::type complement;
};

template <template <typename> class TOperation>
struct mlist_filter_unary<mlist<>, TOperation> {
   typedef mlist<> type;
   typedef mlist<> complement;
};


template <typename TValue, typename TList, bool enable>
struct mlist_prepend_unless_repeated {
   typedef typename mlist_concat<TValue, TList>::type type;
};

template <typename TValue, typename TList>
struct mlist_prepend_unless_repeated<TValue, TList, false> {
   typedef TList type;
};

template <typename TValue>
struct mlist_prepend_unless_repeated<TValue, mlist<TValue>, true> {
   typedef mlist<TValue> type;
};

/** Split the elements of two meta-lists depending on results of a binary meta-function.
 *  Following results are defined:
 *    type  list of elements from TList1 evaluating TOperation to true
 *    type2 list of corresponding elements from TList2
 *    complement  list of elements from TList1 evaluating TOperation to false
 *    complement2 list of corresponding elements from TList2
 */
template <typename TList1, typename TList2, template <typename, typename> class TOperation>
struct mlist_filter_binary {
   typedef typename mlist_head<TList1>::type head1;
   typedef typename mlist_head<TList2>::type head2;
   typedef TOperation<head1, head2> eval_head;
   typedef mlist_filter_binary<typename mlist_coherent_tails<TList1, TList2>::type,
                               typename mlist_coherent_tails<TList1, TList2>::type2, TOperation> eval_tail;

   typedef typename mlist_prepend_unless_repeated<head1, typename eval_tail::type, eval_head::value>::type type;
   typedef typename mlist_prepend_unless_repeated<head2, typename eval_tail::type2, eval_head::value>::type type2;

   typedef typename std::conditional<is_instance_of<TList1, mrepeat>::value && std::is_same<mlist<head1>, type>::value,
                                     mlist<>,
                                     typename mlist_prepend_unless_repeated<head1, typename eval_tail::complement, !eval_head::value>::type>::type complement;
   typedef typename std::conditional<is_instance_of<TList2, mrepeat>::value && std::is_same<mlist<head2>, type2>::value,
                                     mlist<>,
                                     typename mlist_prepend_unless_repeated<head2, typename eval_tail::complement2, !eval_head::value>::type>::type complement2;
};

template <template <typename, typename> class TOperation>
struct mlist_filter_binary<mlist<>, mlist<>, TOperation> {
   typedef mlist<> type;
   typedef mlist<> complement;
   typedef mlist<> type2;
   typedef mlist<> complement2;
};


/** Fold a meta-list applying a binary meta-function to its elements.
 *  For an empty list, the result is void.
 *  For a single-element list, the result is the element itself.
 *  The elements of the list are passed as the left argument of TOperation,
 *  the accumulated intermediate result as the right argument.
 *  Folding starts at the tail of the list.
 *  If it should start at the head, the input list must be reversed (@see mlist_reverse).
 */
template <typename T, template <typename, typename> class TOperation>
struct mlist_fold
   : mlist_head<T> {};

template <typename T, typename T2, typename... TTail, template <typename, typename> class TOperation>
struct mlist_fold<mlist<T, T2, TTail...>, TOperation> {
   typedef typename mlist_fold<mlist<T2, TTail...>, TOperation>::type folded_tail;
   typedef typename TOperation<T, folded_tail>::type type;
};


template <typename T, typename TInit, template <typename, typename> class TOperation>
struct mlist_fold_with_init {
   typedef TInit type;
};

/// Like mlist_fold, but starting with a prescribed initial value.
template <typename T, typename... TTail, typename TInit, template <typename, typename> class TOperation>
struct mlist_fold_with_init<mlist<T, TTail...>, TInit, TOperation> {
   typedef typename mlist_fold_with_init<mlist<TTail...>, TInit, TOperation>::type folded_tail;
   typedef typename TOperation<T, folded_tail>::type type;
};


/** Transform and fold a meta-list.
 *  Every element is transformed with a given unary meta-function TTransform, the result is passed
 *  as a left argument to a binary meta-function TFold;  the right argument is the accumulated result
 *  for the processed part of the list.
 *  Folding starts at the tail of the list.
 *  If it should start at the head, the input list must be reversed (@see mlist_reverse).
 */
template <typename T, template <typename> class TTransform, template <typename, typename> class TFold>
struct mlist_fold_transform {
   typedef typename TTransform<typename mlist_head<T>::type>::type type;
};

template <template <typename> class TTransform, template <typename, typename> class TFold>
struct mlist_fold_transform<mlist<>, TTransform, TFold> {
   typedef void type;
};

template <typename T, typename T2, typename... TTail, template <typename> class TTransform, template <typename, typename> class TFold>
struct mlist_fold_transform<mlist<T, T2, TTail...>, TTransform, TFold> {
   typedef typename mlist_fold_transform<mlist<T2, TTail...>, TTransform, TFold>::type folded_tail;
   typedef typename TFold<typename TTransform<T>::type, folded_tail>::type type;
};


template <template <typename, typename> class TCompare>
struct mlist_remove_duplicates_impl {
   template <typename TElement, typename TList>
   struct TOperation
      : mlist_prepend_if<!mlist_contains<TList, TElement, TCompare>::value, TElement, TList> {};
};

/// Remove duplicate elements from a meta-list
template <typename TList, template <typename, typename> class TCompare = std::is_same>
struct mlist_remove_duplicates
   : mlist_wrap<typename mlist_fold<TList, mlist_remove_duplicates_impl<TCompare>::template TOperation>::type> {};


/// Compute a union of two meta-lists.
/// Lists are supposed to be free from duplicates.
template <typename TList1, typename TList2>
struct mlist_union
   : mlist_fold_with_init<TList1, TList2, mlist_remove_duplicates_impl<std::is_same>::template TOperation> {};

/// Compute an intersection of two meta-lists.
/// Lists are supposed to be free from duplicates.
template <typename TList1, typename TList2>
struct mlist_intersection {
   typedef typename mlist_match<TList1, TList2>::type type;
};

/// Compute a difference of two meta-lists.
/// Lists are supposed to be free from duplicates.
template <typename TList1, typename TList2>
struct mlist_difference {
   typedef typename mlist_match<TList1, TList2>::complement type;
};

/// Compute a symmetric difference of two meta-lists.
/// Lists are supposed to be free from duplicates.
template <typename TList1, typename TList2>
struct mlist_symdifference
   : mlist_concat<typename mlist_match<TList1, TList2>::complement,
                  typename mlist_match<TList1, TList2>::complement2> {};



/** Find all pairs of elements of two meta-lists evaluating a binary meta-function to true.
 *  Elements can be paired multiple times with different mates.
 *  Results are structured like in mlist_match.
 *  type and type2 may have different length, the order of their elements is unrelated to each other.
 */
template <typename TList1, typename TList2, template <typename, typename> class TCompare = std::is_same>
struct mlist_match_all {
   typedef typename mlist_head<TList2>::type head2;
   typedef mlist_filter_binary<TList1, mrepeat<head2>, TCompare> match_head;
   typedef mlist_match_all<TList1, typename mlist_tail<TList2>::type, TCompare> match_tail;
   typedef typename mlist_union<typename match_head::type, typename match_tail::type>::type type;
   typedef typename mlist_intersection<typename match_head::complement, typename match_tail::complement>::type complement;
   typedef typename mlist_concat<typename match_head::type2, typename match_tail::type2>::type type2;
   typedef typename mlist_concat<typename match_head::complement2, typename match_tail::complement2>::type complement2;
};

template <typename TList1, template <typename, typename> class TCompare>
struct mlist_match_all<TList1, mlist<>, TCompare> {
   typedef mlist<> type;
   typedef TList1 complement;
   typedef mlist<> type2;
   typedef mlist<> complement2;
};

/***********************************************************************
 *
 *  Tagged meta-lists
 */

template <typename T>
struct mis_properly_tagged
   : std::false_type {};

template <typename TValue, template <typename> class TTag>
struct mis_properly_tagged< TTag<TValue> >
   : std::true_type {};


/** Extract an element with a desired tag from a tagged meta-list.
 *  Defines following results:
 *  value = matching value or TDefault if nothing found
 *  is_specified = true If the tag occurs in the list
 *  tagged_value = TTag<TValue> or void if nothing found
 */
template <typename TList, template <typename> class TTag, typename TDefault = void>
struct mtagged_list_extract
   : mtagged_list_extract<typename mlist_tail<TList>::type, TTag, TDefault> {
   static_assert(std::is_same<TList, typename mlist_wrap<TList>::type>::value, "mtagged_list_extract applied to something which is not a meta-list");
   static_assert(mis_properly_tagged<typename mlist_head<TList>::type>::value, "untagged element encountered in a tagged meta-list");
};

template <typename TValue, typename... TTail, template <typename> class TTag, typename TDefault>
struct mtagged_list_extract<mlist<TTag<TValue>, TTail...>, TTag, TDefault> {
   typedef TValue type;
   typedef TTag<TValue> tagged_value;
   static const bool is_specified = true;
   static_assert(!mtagged_list_extract<mlist<TTail...>, TTag, TDefault>::is_specified, "a tag occurs multiple times in a meta-list");
};

template <template <typename> class TTag, typename TDefault>
struct mtagged_list_extract<mlist<>, TTag, TDefault> {
   typedef TDefault type;
   typedef void tagged_value;
   static const bool is_specified = false;
};

template <typename TList, template <typename> class TTag, typename TValue>
constexpr
TValue tagged_list_extract_integral(const TValue deflt)
{
   typedef mtagged_list_extract<TList, TTag, std::integral_constant<TValue, TValue(0)>> extractor;
   return extractor::is_specified ? extractor::type::value : deflt;
}


/** Modify a tagged meta-list: Replace the element with the given tag by the result of a binary meta-function
 *  applied to the element and a second value.
 *  If the tag does not occur in the source list, TOperation<void, TValue2>::type is appended to the result list.
 */
template <typename TList, template <typename> class TTag, template <typename, typename> class TOperation, typename TValue2>
struct mtagged_list_modify
   : mlist_concat< typename mlist_head<TList>::type,
                   typename mtagged_list_modify<typename mlist_tail<TList>::type, TTag, TOperation, TValue2>::type >
{
   static_assert(std::is_same<TList, typename mlist_wrap<TList>::type>::value, "mtagged_list_modify applied to something which is not a meta-list");
   static_assert(mis_properly_tagged<typename mlist_head<TList>::type>::value, "untagged element encountered in a tagged meta-list");
};

template <typename TValue, typename... TTail, template <typename> class TTag, template <typename, typename> class TOperation, typename TValue2>
struct mtagged_list_modify<mlist<TTag<TValue>, TTail...>, TTag, TOperation, TValue2>
   : mlist_concat< TTag<typename TOperation<TValue, TValue2>::type>, mlist<TTail...> > {
   static_assert(!mtagged_list_extract<mlist<TTail...>, TTag>::is_specified, "a tag occurs multiple times in a meta-list");
};

template <template <typename> class TTag, template <typename, typename> class TOperation, typename TValue2>
struct mtagged_list_modify<mlist<>, TTag, TOperation, TValue2> {
   typedef mlist<TTag<typename TOperation<void, TValue2>::type>> type;
};


/// Replace values of elements of a tagged meta-list.
/// If some tag does not occur in the source list, the new value is appended to the result.
template <typename TList, typename... TReplacements>
struct mtagged_list_replace;

template <typename TList>
struct mtagged_list_replace<TList> {
   typedef TList type;
};

template <typename TList, template <typename> class TTag, typename TValue, typename... TTail>
struct mtagged_list_replace<TList, TTag<TValue>, TTail...>
   : mtagged_list_modify<typename mtagged_list_replace<TList, TTail...>::type, TTag, mproject2nd, TValue> {};


/// Append elements to a tagged meta-list unless another elements with identical tags are already there.
template <typename TList, typename... TDefaults>
struct mtagged_list_add_default;

template <typename TList>
struct mtagged_list_add_default<TList> {
   typedef TList type;
};

template <typename TList, template <typename> class TTag, typename TValue, typename... TTail>
struct mtagged_list_add_default<TList, TTag<TValue>, TTail...>
   : mtagged_list_modify<typename mtagged_list_add_default<TList, TTail...>::type, TTag, mprefer1st, TValue> {};


/// Remove an element with a given tag from a tagged meta-list.
/// If the tag does not occur in the source list, the result list is identical to the source one.
template <typename TList, template <typename> class TTag>
struct mtagged_list_remove
   : mlist_concat< typename mlist_head<TList>::type,
                   typename mtagged_list_remove<typename mlist_tail<TList>::type, TTag>::type > {};

template <typename TValue, typename... TTail, template <typename> class TTag>
struct mtagged_list_remove<mlist<TTag<TValue>, TTail...>, TTag> {
   typedef mlist<TTail...> type;
   static_assert(!mtagged_list_extract<mlist<TTail...>, TTag>::is_specified, "a tag occurs multiple times in a meta-list");
};

template <template <typename> class TTag>
struct mtagged_list_remove<mlist<>, TTag> {
   typedef mlist<> type;
};


template <typename T1, typename T2>
struct has_same_tag
   : std::false_type {};

template <typename T1, typename T2, template <typename> class TTag>
struct has_same_tag<TTag<T1>, TTag<T2>>
   : std::true_type {};

/// Find all elements with desired tags in a tagged meta-list.
/// The desired tags can be specified with arbitrary contents, e.g. with void.
template <typename TList, typename TTags>
struct mtagged_list_intersect
   : mlist_match_all<TList, TTags, has_same_tag> {};


/// Combine two tagged meta-lists.
/// Elements with equal tags are merged by concatenation and elimination of duplicates.
template <typename TList1, typename TList2>
struct mtagged_list_concat
   : mlist_concat<TList1, TList2> {};

template <template <typename> class TTag, typename TValue, typename... TTail, typename TList2>
struct mtagged_list_concat<mlist<TTag<TValue>, TTail...>, TList2> {
   typedef typename mtagged_list_extract<TList2, TTag>::type value2;
   typedef typename mlist_unwrap<typename mlist_remove_duplicates<typename mlist_concat<TValue, value2>::type>::type>::type combined_value;
   typedef typename mtagged_list_concat<mlist<TTail...>, typename mtagged_list_remove<TList2, TTag>::type>::type result_tail;
   typedef typename mlist_concat<TTag<combined_value>, result_tail>::type type;
};


/// Extract the template parameter with the given ordinal number.
/// Enumeration starts with 0.  For out-of-bound numbers, the result is undefined.
template <typename T, int TPos>
struct mget_template_parameter {};

template <template <typename...> class Template, typename... TParams, int TPos>
struct mget_template_parameter<Template<TParams...>, TPos>
   : mlist_at<mlist<TParams...>, TPos> {};


template <template <typename...> class Template, typename TNewParamList>
struct mreplace_template_parameter_helper {};

template <template <typename...> class Template, typename... TNewParams>
struct mreplace_template_parameter_helper<Template, mlist<TNewParams...>> {
   typedef Template<TNewParams...> type;
};

/// Replace the template parameter with the given ordinal number with the given value.
/// Enumeration starts with 0.  For out-of-bound numbers, the result equals the input type.
template <typename T, int TPos, typename TNewValue>
struct mreplace_template_parameter {};

template <template <typename...> class Template, typename... TOldParams, int TPos, typename TNewValue>
struct mreplace_template_parameter<Template<TOldParams...>, TPos, TNewValue>
   : mreplace_template_parameter_helper<Template, typename mlist_replace_at<mlist<TOldParams...>, TPos, mlist<TNewValue>>::type> {};

}

#endif // POLYMAKE_META_LIST_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
