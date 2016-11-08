/* Copyright (c) 1997-2016
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

#ifndef POLYMAKE_TYPE_UTILS_H
#define POLYMAKE_TYPE_UTILS_H

#include "polymake/meta_list.h"
#include <string>
#include <typeinfo>

namespace polymake {

template <typename T>
using pure_type_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T>
using is_class_or_union = mlist_or<std::is_class<T>, std::is_union<T>>;

template <typename Target, typename Source>
using inherit_const_t = typename std::conditional<std::is_const<std::remove_reference_t<Source>>::value,
                                                  std::add_const_t<Target>, Target>::type;
template <typename Target, typename Source>
using inherit_reference_t = typename mselect< std::enable_if< std::is_rvalue_reference<Source>::value,
                                                              std::add_rvalue_reference_t<Target> >,
                                              std::enable_if< std::is_lvalue_reference<Source>::value,
                                                              std::add_lvalue_reference_t<inherit_const_t<Target, Source>> >,
                                              Target >::type;

template <typename T>
using private_mutable_t = typename std::conditional<std::is_rvalue_reference<T&&>::value, T&&, pure_type_t<T>>::type;

/// enforce a local copy of an object unless it's already a prvalue
template <typename T> inline
private_mutable_t<T> ensure_private_mutable(T&& x)
{
   return static_cast<private_mutable_t<T>>(x);
}

/******************************************************************************
 *
 *  Analisys of relations between groups of types
 */

template <typename T, template <typename...> class Template>
class is_derived_from_instance_impl {
   struct tester {
      template <typename... TParams>
      std::true_type operator()(const Template<TParams...>*) const;

      std::false_type operator()(...) const;
   };

   static const T* instance();
public:
   typedef decltype(tester()(instance())) type;
};

/// Check whether a type is an instance of a given class template
/// or derived from an instance thereof
template <typename T, template <typename...> class Template>
struct is_derived_from_instance_of
   : is_derived_from_instance_impl<T, Template>::type {};


/// convenience shortcut
template <typename TDerived, typename TBase>
using is_derived_from = std::is_base_of<TBase, TDerived>;


template <typename TDerived, typename TBases>
struct is_derived_from_any_impl
   : mlist_at<TBases, mlist_find<TBases, TDerived, std::is_base_of>::value> {
   static const bool value=mlist_find<TBases, TDerived, std::is_base_of>::value >= 0;
};

/// Tell whether the given type is derived from any of the given base types,
/// and return the first base type from the list, if any found
template <typename TDerived, typename... TBases>
struct is_derived_from_any
   : is_derived_from_any_impl<TDerived, typename mlist_wrap<TBases...>::type> {};


template <typename T, typename TPartial=void>
struct least_derived_class_impl {};

template <typename T>
struct least_derived_class_impl<mlist<T>, void> {
   typedef T type;
};

template <typename T1, typename T2>
struct least_derived_class_impl2
   : mselect<std::enable_if<std::is_base_of<T1, T2>::value, T1>,
             std::enable_if<std::is_base_of<T2, T1>::value, T2>> {};

template <typename T, typename T2, typename... TTail>
struct least_derived_class_impl<mlist<T, T2, TTail...>,
                                typename mproject2nd<typename least_derived_class_impl<mlist<T2, TTail...>>::type, void>::type>
   : least_derived_class_impl2<T, typename least_derived_class_impl<mlist<T2, TTail...>>::type> {};


/** Find a class among all given types all others are derived from.
 *  If there is none, the result is undefined.
 *  This differs from std::common_type in two aspects:
 *   - default class constructors may be protected or even deleted;
 *     clang can only report common types for classes with public default constructors
 *   - this function does not report the common base class if that does not occur among the arguments
 */
template <typename... T>
struct least_derived_class
   : least_derived_class_impl<typename mlist_wrap<T...>::type> {};


template <typename TTarget, typename TSource>
using is_direct_constructible = std::is_constructible<TTarget, TSource>;

template <typename TSource, typename TTargets>
struct can_construct_any_impl
   : mlist_at<TTargets, mlist_find<TTargets, TSource, is_direct_constructible>::value> {
   static const bool value=mlist_find<TTargets, TSource, is_direct_constructible>::value >= 0;
};

/// Tell whether the given type can be used as a constructor argument for any of the given targets,
/// and return the first target type from the list, if any found
template <typename TSource, typename... TTargets>
struct can_construct_any
   : can_construct_any_impl<TSource, typename mlist_wrap<TTargets...>::type> {};


/// convenience shortcut swapping the order of arguments
template <typename Source, typename Target>
using can_assign_to = std::is_assignable<std::add_lvalue_reference_t<Target>, Source>;

/******************************************************************************
 *
 *  Checks for possible operations
 */

template <typename T, typename=void>
struct is_ordered_impl {
   typedef std::false_type type;
};

template <typename T>
struct is_ordered_impl<T, typename std::enable_if<std::is_same<decltype(std::declval<T>() < std::declval<T>()), bool>::value &&
                                                  std::is_same<decltype(std::declval<T>() > std::declval<T>()), bool>::value>::type> {
   typedef std::true_type type;
};

/// Check whether the "less than" and "greater than" operators are defined for a given type
template <typename T>
struct is_ordered : is_ordered_impl<T>::type {};

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

#endif // POLYMAKE_TYPE_UTILS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
