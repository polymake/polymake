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

#include "polymake/meta_list.h"
#include <cstddef>
#include <string>
#include <typeinfo>
#include <tuple>
#include <array>
#include <initializer_list>
#include <functional>

namespace polymake {

// type for quantities of items of all kinds (collections sizes, indexes, offsets...)
using Int = long;

template <typename T>
struct mpure : std::remove_cv<typename std::remove_reference<T>::type> {};

template <typename T>
using pure_type_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename... T>
using mlist_pure_types = typename mlist_transform_unary<typename mlist_wrap<T...>::type, mpure>::type;

template <typename T1, typename T2>
using same_pure_type = std::is_same<pure_type_t<T1>, pure_type_t<T2>>;

template <typename T>
using is_class_or_union = bool_constant<(std::is_class<T>::value || std::is_union<T>::value) && !std::is_enum<T>::value>;

template <typename T>
using is_const = std::is_const<std::remove_reference_t<T>>;

template <typename T>
using add_const = mselect< std::enable_if< std::is_rvalue_reference<T>::value && is_class_or_union<pure_type_t<T>>::value,
                                           std::add_rvalue_reference_t<std::add_const_t<std::remove_reference_t<T>>> >,
                           std::enable_if< std::is_lvalue_reference<T>::value,
                                           std::add_lvalue_reference_t<std::add_const_t<std::remove_reference_t<T>>> >,
                           std::add_const_t<T> >;

template <typename T>
using add_const_t = typename add_const<T>::type;

template <typename Target, typename Source>
using inherit_const = std::conditional<is_const<Source>::value, std::add_const_t<Target>, Target>;

template <typename Target, typename Source>
using inherit_const_t = typename inherit_const<Target, Source>::type;

template <typename Target, typename Source>
using inherit_reference = mselect< std::enable_if< std::is_rvalue_reference<Source>::value,
                                                   std::add_rvalue_reference_t<Target> >,
                                   std::enable_if< std::is_lvalue_reference<Source>::value,
                                                   std::add_lvalue_reference_t<inherit_const_t<Target, Source>> >,
                                   Target >;

template <typename Target, typename Source>
using inherit_reference_t = typename inherit_reference<Target, Source>::type;

template <typename T>
using private_mutable_t = std::conditional_t<std::is_rvalue_reference<T&&>::value, T&&, pure_type_t<T>>;

/// enforce a local copy of an object unless it's already a prvalue
template <typename T>
private_mutable_t<T> ensure_private_mutable(T&& x)
{
   return static_cast<private_mutable_t<T>>(x);
}

template <typename T, bool applicable = std::is_unsigned<T>::value>
struct remove_unsigned {
   using type = T;
};

template <typename T>
struct remove_unsigned<T, true> : std::make_signed<T> {};

template <>
struct remove_unsigned<bool, true> : remove_unsigned<bool, false> {};

template <typename T>
using remove_unsigned_t = typename remove_unsigned<T>::type;

template <typename Target, typename Source>
using inherit_signed = mselect< std::enable_if< std::is_signed<Source>::value, std::make_signed_t<Target> >,
                                std::enable_if< std::is_unsigned<Source>::value, std::make_unsigned_t<Target> >,
                                Target >;

template <typename Target, typename Source>
using inherit_signed_t = typename inherit_signed<Target, Source>::type;


/// Calculate a pointer to an enclosing class object from a given pointer to a data member
template <typename Source, typename Member, typename Owner>
auto reverse_cast(Source* data_member, Member Owner::* member_ptr)
{
   constexpr bool const_access = std::is_const<Source>::value || std::is_const<Member>::value;
   using owner_t = std::conditional_t<const_access, std::add_const_t<Owner>, Owner>*;
   using char_t = std::conditional_t<const_access, std::add_const_t<char>, char>*;
   using member_t = std::conditional_t<const_access, std::add_const_t<Member>, Member>*;
   const ptrdiff_t fict_addr = 8;
   const ptrdiff_t offset = reinterpret_cast<char_t>(&(reinterpret_cast<owner_t>(fict_addr)->*member_ptr))
                            - reinterpret_cast<char_t>(fict_addr);
   return reinterpret_cast<owner_t>( reinterpret_cast<char_t>(static_cast<member_t>(data_member)) - offset );
}

/// Calculate a pointer to an enclosing class object from a given pointer to an element in a data member array
template <typename Member, typename Source, typename Owner, size_t size>
auto reverse_cast(Source* data_member, Int i, Member (Owner::* member_ptr)[size])
{
   constexpr bool const_access = std::is_const<Source>::value || std::is_const<Member>::value;
   using owner_t = std::conditional_t<const_access, std::add_const_t<Owner>, Owner>*;
   using char_t = std::conditional_t<const_access, std::add_const_t<char>, char>*;
   using member_t = std::conditional_t<const_access, std::add_const_t<Member>, Member>*;
   const ptrdiff_t fict_addr = 8;
   const ptrdiff_t offset = reinterpret_cast<char_t>((reinterpret_cast<owner_t>(fict_addr)->*member_ptr)+0)
                            - reinterpret_cast<char_t>(fict_addr);
   return reinterpret_cast<owner_t>( reinterpret_cast<char_t>(static_cast<member_t>(data_member)-i) - offset );
}

/******************************************************************************
 *
 *  Analysis and manipulation of parametrized types
 */

/// Extract the template parameter with the given ordinal number.
/// Enumeration starts with 0.  For out-of-bound numbers, the result is undefined.
template <typename T, int Pos>
struct mget_template_parameter {};

template <template <typename...> class Template, typename... Params, int Pos>
struct mget_template_parameter<Template<Params...>, Pos>
   : mlist_at<mlist<Params...>, Pos> {};


template <template <typename...> class Template, typename NewParamList>
struct mset_template_parameter {};

template <template <typename...> class Template, typename... NewParams>
struct mset_template_parameter<Template, mlist<NewParams...>> {
   using type = Template<NewParams...>;
};

/// Replace the template parameter with the given ordinal number with the given value.
/// Enumeration starts with 0.  For out-of-bound numbers, the result equals the input type.
template <typename T, int Pos, typename NewParam>
struct mreplace_template_parameter {};

template <template <typename...> class Template, typename... OldParams, int Pos, typename NewParam>
struct mreplace_template_parameter<Template<OldParams...>, Pos, NewParam>
   : mset_template_parameter<Template, typename mlist_replace_at<mlist<OldParams...>, Pos, mlist<NewParam>>::type> {};

/******************************************************************************
 *
 *  Checks for nested types
 */

#define DeclTypedefCHECK(TyPeNaMe)                                      \
   template <typename T, typename=void>                                 \
   struct has_##TyPeNaMe##_impl : std::false_type {};                   \
   template <typename T>                                                \
   struct has_##TyPeNaMe##_impl<T, accept_valid_type<typename T::TyPeNaMe>> : std::true_type {}; \
   template <typename T>                                                \
   struct has_##TyPeNaMe : has_##TyPeNaMe##_impl<T>::type {}

#define DeclNestedTemplateCHECK(TeMpLaTeNaMe)                           \
   template <typename T, typename=void>                                 \
   struct has_nested_##TeMpLaTeNaMe##_impl : std::false_type {};        \
   template <typename T>                                                \
   struct has_nested_##TeMpLaTeNaMe##_impl<T, accept_valid_template<T::template TeMpLaTeNaMe>> : std::true_type {}; \
   template <typename T>                                                \
   struct has_nested_##TeMpLaTeNaMe : has_nested_##TeMpLaTeNaMe##_impl<T>::type {}

/******************************************************************************
 *
 *  Analysis of relations between groups of types
 */

template <typename T, template <typename...> class... Template>
struct is_instance_of_any
   : mlist_or<is_instance_of<T, Template>...> {};

template <typename T, template <typename...> class Template, typename... TrailingParams>
class is_derived_from_instance_impl {
   struct tester {
      template <typename... Params>
      std::enable_if_t<mlist_is_tail_of<mlist<TrailingParams...>, mlist<Params...>>::value, Template<Params...>*>
      operator()(Template<Params...>*) const;

      void operator()(...) const;
   };
public:
   using instance_type = std::remove_pointer_t< decltype(std::declval<tester>()(std::declval<T*>())) >;
   using type = bool_constant<!std::is_same<instance_type, void>::value>;
};

/// Check whether a type is an instance of a given class template
/// or derived from an instance thereof
template <typename T, template <typename...> class Template, typename... TrailingParams>
struct is_derived_from_instance_of
   : is_derived_from_instance_impl<T, Template, TrailingParams...>::type {
   using instance_type = typename is_derived_from_instance_impl<T, Template, TrailingParams...>::instance_type;
};


/// convenience shortcut
template <typename Derived, typename Base>
using is_derived_from = std::is_base_of<Base, Derived>;


/// Tell whether the given type is derived from any of the given base types
/// @return match first base type from the list or void
template <typename Derived, typename... Bases>
using is_derived_from_any = mlist_find_if<typename mlist_wrap<Bases...>::type, std::is_base_of, Derived>;


template <typename T1, typename T2>
using least_derived_helper
   = mselect<std::enable_if<std::is_base_of<T1, T2>::value, T1>,
             std::enable_if<std::is_base_of<T2, T1>::value, T2>,
             void>;

template <typename T1, typename T2>
using most_derived_helper
   = mselect<std::enable_if<std::is_base_of<T1, T2>::value, T2>,
             std::enable_if<std::is_base_of<T2, T1>::value, T1>,
             void>;


/** Find a class among all given types all others are derived from.
 *  If there is none, the result is void.
 *  This differs from std::common_type in two aspects:
 *   - default class constructors may be protected or even deleted;
 *     clang can only report common types for classes with public default constructors
 *   - this function does not report the common base class if that does not occur among the arguments
 */
template <typename... T>
using least_derived_class
   = mlist_fold<typename mlist_wrap<T...>::type, least_derived_helper>;

/// Find a class derived from all others in the list.
/// If there is none, the result is void.
template <typename... T>
using most_derived_class
   = mlist_fold<typename mlist_wrap<T...>::type, most_derived_helper>;

/// alias wrapping variadic std::is_constructible into a binary meta-function
template <typename Target, typename Source>
using is_direct_constructible = std::is_constructible<Target, Source>;

/// Tell whether the given type can be used as a constructor argument for any of the given targets.
/// @return match first target type from the list or void
template <typename Source, typename... Targets>
using can_construct_any = mlist_find_if<typename mlist_wrap<Targets...>::type, is_direct_constructible, Source>;

/// convenience shortcut swapping the order of arguments
template <typename Source, typename Target>
using can_assign_to = std::is_assignable<std::add_lvalue_reference_t<Target>, Source>;

/// Tell whether the Source type can be converted to Target without precision loss
/// i.e. the constructor Target(Source) is not explicit, and Source is not equivalent to an element of Target
/// if the latter is a container
template <typename Source, typename Target, typename=void>
struct is_lossless_convertible_impl2 : std::true_type {};

template <typename Source, typename Target>
struct is_lossless_convertible_impl2<Source, Target, accept_valid_type<decltype(Target(std::declval<std::initializer_list<Source>>()))>> : std::false_type {};

template <typename Source, typename Target, typename=void>
struct is_lossless_convertible_impl : std::false_type {};

template <typename Source, typename Target>
struct is_lossless_convertible_impl<Source, Target, accept_valid_type<decltype(Target{std::declval<Source>()})>> : is_lossless_convertible_impl2<Source, Target> {};

template <typename Source, typename Target>
using is_lossless_convertible = typename is_lossless_convertible_impl<Source, Target>::type;

/******************************************************************************
 *
 *  Checks for possible operations
 */

template <typename T1, typename T2, typename=void>
struct are_comparable_impl : std::false_type {};

template <typename T1, typename T2>
struct are_comparable_impl<T1, T2, std::enable_if_t<std::is_same<decltype((std::declval<T1>() == std::declval<T2>()) ||
                                                                          (std::declval<T1>() != std::declval<T2>())), bool>::value>>
  : std::true_type { };

template <typename T1, typename T2, typename=void>
struct are_less_greater_comparable_impl : std::false_type {};

template <typename T1, typename T2>
struct are_less_greater_comparable_impl<T1, T2, std::enable_if_t<std::is_same<decltype((std::declval<T1>() < std::declval<T2>()) ||
                                                                                       (std::declval<T1>() > std::declval<T2>())), bool>::value>>
  : std::true_type { };

template <typename T1, typename T2>
using are_comparable = are_comparable_impl<T1, T2>;

template <typename T1, typename T2>
using are_less_greater_comparable = are_less_greater_comparable_impl<T1, T2>;

/// Check whether the "equal" and "not equal" operators are defined for a given type
template <typename T>
using is_comparable = are_comparable<T, T>;

/// Check whether the "less than" and "greater than" operators are defined for a given type
template <typename T>
using is_less_greater_comparable = are_less_greater_comparable<T, T>;

/******************************************************************************
 *
 *  Conversion between meta-lists and tuples
 */

template <typename T>
struct mlist2tuple {
   using type = std::tuple<T>;
};

template <typename... T>
struct mlist2tuple<mlist<T...>> {
   using type = std::tuple<T...>;
};

template <typename T>
struct tuple2mlist
   : mlist_wrap<T> {};

template <typename... T>
struct tuple2mlist<std::tuple<T...>> {
   using type = mlist<T...>;
};


/// Convert a meta-list of encapsulated integer constants into an index sequence
template <typename T>
struct munwrap_index_sequence;

template <size_t... I>
struct munwrap_index_sequence<mlist<std::integral_constant<size_t, I>...>> {
   using type = std::index_sequence<I...>;
};


/// Construct a falling index sequence N-1, N-2, ..., 1, 0
template <size_t N>
struct reverse_index_sequence;

template <>
struct reverse_index_sequence<1> {
   using type = std::index_sequence<0>;
};

template <>
struct reverse_index_sequence<0> {
   using type = std::index_sequence<>;
};

template <size_t N, typename S>
struct prepend_index_sequence;

template <size_t N, size_t... M>
struct prepend_index_sequence<N, std::index_sequence<M...>> {
   using type = std::index_sequence<N, M...>;
};

template <size_t N>
struct reverse_index_sequence
   : prepend_index_sequence<N-1, typename reverse_index_sequence<N-1>::type> {};

template <size_t N>
using make_reverse_index_sequence = typename reverse_index_sequence<N>::type;


template <typename T>
struct index_sequence_for;

template <typename... T>
struct index_sequence_for<mlist<T...>> {
   using type = std::make_index_sequence<sizeof...(T)>;
};

template <typename... T>
struct index_sequence_for<std::tuple<T...>> {
   using type = std::make_index_sequence<sizeof...(T)>;
};

template <typename T1, typename T2>
struct index_sequence_for<std::pair<T1, T2>> {
   using type = std::make_index_sequence<2>;
};

template <typename T, size_t N>
struct index_sequence_for<std::array<T, N>> {
   using type = std::make_index_sequence<N>;
};

template <typename T>
struct index_sequence_for<mrepeat<T>> {
   using type = void;
};

template <typename T>
constexpr auto make_index_sequence_for(const T&)
{
   return typename index_sequence_for<T>::type();
}

template <typename Tuple, typename Func, size_t... Index>
void foreach_in_tuple(Tuple& t, Func& func, std::index_sequence<Index...>)
{
   (void)std::initializer_list<bool>{ (func(std::get<Index>(t)), false)... };
}

template <typename Tuple, typename Func>
void foreach_in_tuple(Tuple&& t, Func&& func)
{
   foreach_in_tuple(t, func, make_index_sequence_for(t));
}


template <typename T, template <typename> class UnaryFunc, typename Indexes>
struct index_sequence_for_filter_unary_impl;

template <typename... T, template <typename> class UnaryFunc, size_t... I>
struct index_sequence_for_filter_unary_impl<mlist<T...>, UnaryFunc, std::index_sequence<I...> > {
   using matching = typename mlist_remove_void<std::conditional_t<UnaryFunc<T>::value, std::integral_constant<size_t, I>, void>...>::type;
   using type = typename munwrap_index_sequence<matching>::type;
};

template <typename T, template <typename> class UnaryFunc>
struct index_sequence_for<mlist_filter_unary<T, UnaryFunc>>
   : index_sequence_for_filter_unary_impl<typename mlist_wrap<T>::type, UnaryFunc,
                                          typename index_sequence_for<typename mlist_wrap<T>::type>::type> {};


template <typename T1, typename T2, template <typename, typename> class BinaryFunc, typename Indexes>
struct index_sequence_for_filter_binary_impl;

template <typename... T1, typename... T2, template <typename, typename> class BinaryFunc, size_t... I>
struct index_sequence_for_filter_binary_impl<mlist<T1...>, mlist<T2...>, BinaryFunc, std::index_sequence<I...> > {
   static_assert(sizeof...(T1) == sizeof...(T2), "mlist_filter_binary - meta-lists size mismatch");
   using matching = typename mlist_remove_void<std::conditional_t<BinaryFunc<T1, T2>::value, std::integral_constant<size_t, I>, void>...>::type;
   using type = typename munwrap_index_sequence<matching>::type;
};

template <typename... T1, typename T2, template <typename, typename> class BinaryFunc, size_t... I>
struct index_sequence_for_filter_binary_impl<mlist<T1...>, mrepeat<T2>, BinaryFunc, std::index_sequence<I...> > {
   using matching = typename mlist_remove_void<std::conditional_t<BinaryFunc<T1, T2>::value, std::integral_constant<size_t, I>, void>...>::type;
   using type = typename munwrap_index_sequence<matching>::type;
};

template <typename T1, typename... T2, template <typename, typename> class BinaryFunc, size_t... I>
struct index_sequence_for_filter_binary_impl<mrepeat<T1>, mlist<T2...>, BinaryFunc, std::index_sequence<I...> > {
   using matching = typename mlist_remove_void<std::conditional_t<BinaryFunc<T1, T2>::value, std::integral_constant<size_t, I>, void>...>::type;
   using type = typename munwrap_index_sequence<matching>::type;
};

template <typename T1, typename T2, template <typename, typename> class BinaryFunc>
struct index_sequence_for_filter_binary_impl<mrepeat<T1>, mrepeat<T2>, BinaryFunc, void> {
   static constexpr size_t matching = BinaryFunc<T1, T2>::value ? 1 : 0;
   using type = std::make_index_sequence<matching>;
};

template <typename T1, typename T2, template <typename, typename> class BinaryFunc>
struct index_sequence_for<mlist_filter_binary<T1, T2, BinaryFunc>>
   : index_sequence_for_filter_binary_impl<typename mlist_wrap<T1>::type, typename mlist_wrap<T2>::type, BinaryFunc,
                                           typename mprefer1st<typename index_sequence_for<typename mlist_wrap<T1>::type>::type,
                                                               typename index_sequence_for<typename mlist_wrap<T2>::type>::type>::type> {};


/******************************************************************************
 *
 *  Variadic argument list analysis
 */

template <typename Indexes>
struct forward_single_arg {
   template <typename Tuple, typename Default>
   static void extract(Tuple&& args, Default&&)
   {
      static_assert(std::is_same<Default, void>::value,
                    "more than one argument satisfies the selection criteria");
   }
};

template <size_t I>
struct forward_single_arg<std::index_sequence<I>> {
   template <typename Tuple, typename Default>
   static decltype(auto) extract(Tuple&& args, Default&&)
   {
      using type = typename std::tuple_element<I, pure_type_t<Tuple>>::type;
      return std::forward<type>(std::get<I>(std::forward<Tuple>(args)));
   }
};

template <>
struct forward_single_arg<std::index_sequence<>> {
   template <typename Tuple, typename Default>
   static Default extract(Tuple&&, Default&& deflt)
   {
      return std::forward<Default>(deflt);
   }
};

template <template <typename> class Func>
struct forward_matching_arg {
   template <typename Tuple, typename Default>
   static decltype(auto) extract(Tuple&& args, Default&& deflt)
   {
      return forward_single_arg<typename index_sequence_for<mlist_filter_unary<mlist_pure_types<typename tuple2mlist<pure_type_t<Tuple>>::type>, Func>>::type>
         ::extract(std::forward<Tuple>(args), std::forward<Default>(deflt));
   }
};

template <typename Indexes>
struct forward_multiple_args;

template <size_t... I>
struct forward_multiple_args<std::index_sequence<I...>> {
   template <typename Tuple>
   static decltype(auto) extract(Tuple&& args)
   {
      return std::forward_as_tuple(std::forward<typename std::tuple_element<I, pure_type_t<Tuple>>::type>(std::get<I>(std::forward<Tuple>(args)))...);
   }
};

template <template <typename> class Func>
struct forward_all_matching_args {
   template <typename Tuple>
   static decltype(auto) extract(Tuple&& args)
   {
      return forward_multiple_args<typename index_sequence_for<mlist_filter_unary<mlist_pure_types<typename tuple2mlist<pure_type_t<Tuple>>::type>, Func>>::type>
         ::extract(std::forward<Tuple>(args));
   }
};

template <typename DesiredTypes, typename Tuple, typename Default>
decltype(auto)
forward_arg_of_type(Tuple&& args, Default&& deflt)
{
   return forward_matching_arg<mbind1st<mlist_find, DesiredTypes>::template func>::extract(std::forward<Tuple>(args), std::forward<Default>(deflt));
}

template <typename DesiredTypes, typename Tuple>
decltype(auto)
forward_arg_of_type(Tuple&& args)
{
   return forward_arg_of_type<DesiredTypes>(std::forward<Tuple>(args), nullptr);
}

template <typename DesiredType, typename Tuple, typename Default>
decltype(auto)
forward_arg_derived_from(Tuple&& args, Default&& deflt)
{
   return forward_matching_arg<mbind1st<std::is_base_of, DesiredType>::template func>::extract(std::forward<Tuple>(args), std::forward<Default>(deflt));
}

template <typename DesiredType, typename Tuple>
decltype(auto)
forward_arg_derived_from(Tuple&& args)
{
   return forward_arg_derived_from(std::forward<Tuple>(args), nullptr);
}

template <template <typename> class Func, typename Tuple, typename Default>
decltype(auto)
forward_arg_satisfying(Tuple&& args, Default&& deflt)
{
   return forward_matching_arg<Func>::extract(std::forward<Tuple>(args), std::forward<Default>(deflt));
}

template <template <typename> class Func, typename Tuple>
decltype(auto)
forward_arg_satisfying(Tuple&& args)
{
   return forward_arg_satisfying(std::forward<Tuple>(args), nullptr);
}

template <typename DesiredTypes, typename Tuple>
decltype(auto)
forward_all_args_of_type(Tuple&& args)
{
   return forward_all_matching_args<mbind1st<mlist_find, DesiredTypes>::template func>::extract(std::forward<Tuple>(args));
}

template <typename DesiredType, typename Tuple>
decltype(auto)
forward_all_args_derived_from(Tuple&& args)
{
   return forward_all_matching_args<mbind1st<std::is_base_of, DesiredType>::template func>::extract(std::forward<Tuple>(args));
}

template <template <typename> class Func, typename Tuple>
decltype(auto)
forward_all_args_satisfying(Tuple&& args)
{
   return forward_all_matching_args<Func>::extract(std::forward<Tuple>(args));
}

/******************************************************************************
 *
 *  Arguments and return types of functions and members
 */

template <typename Func>
struct mget_func_signature;

template <typename Ret, typename... Args>
struct mget_func_signature<Ret(Args...)> {
   using arg_list = mlist<Args...>;
   using return_type = Ret;
};

template <typename Ret, typename... Args>
struct mget_func_signature<Ret(*)(Args...)> {
   using arg_list = mlist<Args...>;
   using return_type = Ret;
};

template <typename Owner, typename Ret, typename... Args>
struct mget_func_signature<Ret(Owner::*)(Args...)> {
   using arg_list = mlist<Args...>;
   using return_type = Ret;
};

template <typename Owner, typename Ret, typename... Args>
struct mget_func_signature<Ret(Owner::*)(Args...) const> {
   using arg_list = mlist<Args...>;
   using return_type = Ret;
};

template <typename Owner, typename Ret, typename... Args>
struct mget_func_signature<Ret(Owner::*)(Args...) &> {
   using arg_list = mlist<Args...>;
   using return_type = Ret;
};

template <typename Owner, typename Ret, typename... Args>
struct mget_func_signature<Ret(Owner::*)(Args...) const &> {
   using arg_list = mlist<Args...>;
   using return_type = Ret;
};

template <typename Owner, typename Ret, typename... Args>
struct mget_func_signature<Ret(Owner::*)(Args...) &&> {
   using arg_list = mlist<Args...>;
   using return_type = Ret;
};

template <typename Owner, typename Ret, typename... Args>
struct mget_func_signature<Ret(Owner::*)(Args...) const &&> {
   using arg_list = mlist<Args...>;
   using return_type = Ret;
};

template <typename Func>
using func_return_t = typename mget_func_signature<Func>::return_type;

template <typename Func>
using func_args_t = typename mget_func_signature<Func>::arg_list;

/******************************************************************************
 *
 *  Pretty printing
 */

/// Produce a type name as a valid C++ source code type id.
/// Namespace prefixes polymake:: are removed for better legibility.
std::string legible_typename(const std::type_info& ti);

std::string legible_typename(const char* typeid_name);

/// convenience wrapper
template <typename T> inline
std::string legible_typename()
{
   return legible_typename(typeid(T));
}

}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
