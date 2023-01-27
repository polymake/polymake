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

#include "polymake/internal/iterators.h"

namespace pm {

template <typename T, typename Model=typename object_traits<T>::model>
class Wary_traits {};

template <typename T>
class Wary_traits<T, is_container> : public container_traits<T> {};

template <typename T>
class Wary
   : public Wary_traits<T>
   , public inherit_generic<Wary<T>, T>::type {
   Wary() = delete;
   ~Wary() = delete;
public:
   using inherit_generic<Wary,T>::type::operator=;
};

template <typename T>
class Generic< Wary<T> > {
   Generic() = delete;
   ~Generic() = delete;
public:
   using top_type = T;
   using persistent_type = T;
   using concrete_type = Wary<T>;

   const top_type& top() const &
   {
      return reinterpret_cast<const T&>(static_cast<const Wary<T>&>(*this));
   }
   top_type& top() &
   {
      return reinterpret_cast<T&>(static_cast<Wary<T>&>(*this));
   }
   top_type&& top() &&
   {
      return reinterpret_cast<T&&>(static_cast<Wary<T>&&>(*this));
   }
};

template <typename T, typename Feature>
struct check_container_feature<Wary<T>, Feature> : check_container_feature<T, Feature> {};

template <typename T>
struct redirect_object_traits< Wary<T> > : object_traits<T> {};

template <typename T>
decltype(auto) wary(T&& x)
{
   return reinterpret_cast<inherit_reference_t<Wary<pure_type_t<T>>, T&&>>(x);
}

template <typename T>
using MaybeWary = can_assign_to<T, Wary<T>>;

template <typename T>
typename std::enable_if<MaybeWary<T>::value, Wary<T>&>::type
maybe_wary(T& x)
{
   return wary(x);
}

template <typename T>
typename std::enable_if<!MaybeWary<T>::value, T&>::type
maybe_wary(T& x)
{
   return x;
}

template <typename T>
struct Unwary {
   using type = T;
};

template <typename T>
struct Unwary< Wary<T> > {
   using type = T;
};

template <typename T>
using pure_unwary_t = typename Unwary<typename Concrete<pure_type_t<T>>::type>::type;

template <typename T>
using unwary_t = inherit_reference_t<pure_unwary_t<T>, T>;

template <typename T>
constexpr bool is_wary()
{
   using concrete_type = typename Concrete<pure_type_t<T>>::type;
   return !std::is_same<concrete_type, typename Unwary<concrete_type>::type>::value;
}

template <typename T>
decltype(auto) unwary(T&& x, std::enable_if_t<!is_wary<T>(), void**> =nullptr)
{
   return concrete(std::forward<T>(x));
}

template <typename T>
decltype(auto) unwary(T&& x, std::enable_if_t<is_wary<T>(), void**> =nullptr)
{
   return static_cast<unwary_t<T&&>>(x.top());
}

}
namespace polymake {
   using pm::Wary;
   using pm::wary;
   using pm::is_wary;
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
