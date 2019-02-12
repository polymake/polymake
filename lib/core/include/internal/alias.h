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

#ifndef POLYMAKE_INTERNAL_ALIAS_H
#define POLYMAKE_INTERNAL_ALIAS_H

#include "polymake/internal/iterators.h"

#include <algorithm>
#include <memory>
#include <cstring>
#include <limits>
#include <cassert>

namespace pm {

template <typename T>
void make_mutable_alias(T&, T&);
template <typename T>
void make_mutable_alias(const T&, const T&) {}

template <typename T, bool _apply=!std::is_same<typename deref<T>::type, typename attrib<T>::minus_const_ref>::value>
struct demasq {
   typedef T type;
   static const bool apply=_apply;
};

template <typename T>
struct demasq<T,true> : T {
   typedef typename inherit_ref<typename object_traits<typename attrib<typename T::type>::minus_const_ref>::masquerade_for, T>::type
      arg_type;
   static const bool apply=true;
};

template <typename T>
using deserves_special_alias = std::is_same<decltype(make_mutable_alias(std::declval<T&>(), std::declval<T&>())), T&>;

enum class alias_kind { obj, ref, special, rvref,
                        masqueraded, delayed_masquerade
};

template <typename ObjectRef> struct alias_kind_of;
template <typename T, alias_kind kind=alias_kind_of<T>::value> class alias;

template <typename ObjectRef>
struct alias_kind_of {
   using T = typename deref<ObjectRef>::type;

   static constexpr alias_kind value=
      is_masquerade<typename demasq<ObjectRef>::type>::value
      ? (demasq<ObjectRef>::apply ? alias_kind::delayed_masquerade : alias_kind::masqueraded) :
      is_masquerade<ObjectRef>::value
      ? alias_kind::masqueraded :
      std::is_rvalue_reference<ObjectRef>::value
      ? alias_kind::rvref :
      deserves_special_alias<T>::value
      ? alias_kind::special :
      std::is_lvalue_reference<ObjectRef>::value
      ? alias_kind::ref
      : alias_kind::obj;
};

template <typename T, alias_kind kind>  // kind==alias::rvref disabled
class alias;

template <typename T>
class alias<T, alias_kind::ref> {
   // alias to a persistent object: keep a reference
public:
   typedef typename attrib<T>::minus_const_ref value_type;
   typedef typename attrib<T>::plus_ref reference;
   typedef typename attrib<T>::plus_const_ref const_reference;
   typedef typename attrib<T>::minus_ref* pointer;
   typedef const value_type* const_pointer;

   alias() : ptr(nullptr) {}
   alias(reference arg) : ptr(&arg) {}
   alias(typename assign_const<alias<typename attrib<T>::minus_const, alias_kind::ref>, !std::is_same<T, typename attrib<T>::minus_const>::value>::type& other)
      : ptr(other.ptr) {}
   alias(const alias&) = default;
   alias(alias&&) = default;
   alias& operator= (const alias&) = default;
   alias& operator= (alias&&) = default;

   reference operator* () { return *ptr; }
   const_reference operator* () const { return *ptr; }
   pointer operator-> () { return ptr; }
   const_pointer operator-> () const { return ptr; }

   reference get_object() { return *ptr; }
protected:
   pointer ptr;

   template <typename, alias_kind> friend class alias;
};

template <typename T>
class alias<T, alias_kind::special> {
   // special alias requires extra treatment in the constructor
public:
   typedef typename attrib<T>::minus_const_ref value_type;
   typedef typename attrib<T>::plus_ref reference;
   typedef typename attrib<T>::plus_const_ref const_reference;
   typedef typename attrib<T>::minus_ref* pointer;
   typedef const value_type* const_pointer;
   using arg_type = std::conditional_t<std::is_reference<T>::value, T, typename attrib<T>::plus_const_ref>;

   alias() = default;
   alias(arg_type arg) : val(arg) { make_mutable_alias(val, arg); }

   alias(typename assign_const<alias<typename attrib<T>::minus_const>, !std::is_same<T, typename attrib<T>::minus_const>::value>::type& other)
      : val(other.val) {}

   alias(const alias&) = default;

   alias& operator= (const alias& other)
   {
      val.~value_type();
      new(&val) value_type(other.val);
      return *this;
   }

   reference operator* () { return val; }
   const_reference operator* () const { return val; }
   pointer operator-> () { return &val; }
   const_pointer operator-> () const { return &val; }

   value_type& get_object() { return val; }
   const value_type& get_object() const { return val; }
protected:
   value_type val;

   template <typename, alias_kind> friend class alias;
};

template <typename TRef>
class alias<TRef, alias_kind::obj> {
public:
   using value_type = pure_type_t<TRef>;
   static constexpr bool always_const = object_traits<value_type>::is_persistent || object_traits<value_type>::is_always_const;
   using const_reference = const value_type&;
   using reference = std::conditional_t<always_const, const_reference, inherit_const_t<value_type, TRef>&>;
   using const_pointer = const value_type*;
   using pointer = std::conditional_t<always_const, const_pointer, inherit_const_t<value_type, TRef>*>;

   static_assert(std::is_move_constructible<value_type>::value, "must be moveable");

   // TODO: delete
   alias() = default;

   // gcc5 has a defect in std::tuple implementation prohibiting use of `explicit' here
   alias(value_type&& arg) : val(std::move(arg)) {}
   alias(const value_type& arg) : val(arg) {}

   template <typename... Args, typename=std::enable_if_t<std::is_constructible<value_type, Args...>::value>>
   explicit alias(Args&&... args) : val(std::forward<Args>(args)...) {}

   alias(alias&& other) = default;

   // TODO: =delete all these when iterators stop outliving containers
   alias(const alias& other) = default;
   using alt_arg_type = std::conditional_t<is_const<TRef>::value, const alias<typename attrib<TRef>::minus_const>, alias>;
   alias(alt_arg_type& other)
      : val(other.val) {}

   alias& operator= (alias&& other)
   {
      val.~value_type();
      new(&val) value_type(std::move(other.val));
      return *this;
   }

   // TODO: =delete when iterators stop outliving containers
   alias& operator= (const alias& other)
   {
      val.~value_type();
      new(&val) value_type(other.val);
      return *this;
   }

   reference operator* () & { return val; }
   const_reference operator* () const & { return val; }
   value_type&& operator* () && { return val; }

   pointer operator-> () { return &val; }
   const_pointer operator-> () const { return &val; }

   value_type& get_object() & { return val; }
   const value_type& get_object() const & { return val; }
   value_type&& get_object() && { return val; }

protected:
   value_type val;

   template <typename, alias_kind> friend class alias;
};

template <typename T>
class alias<T, alias_kind::masqueraded>
   : public alias<typename is_masquerade<T>::hidden_stored_type> {
   // masquerade: keep an alias to a hidden type
   typedef alias<typename is_masquerade<T>::hidden_stored_type> _super;
public:
   typedef typename attrib<T>::plus_const_ref const_reference;
   typedef typename std::conditional<object_traits<typename is_masquerade<T>::hidden_type>::is_always_const ||
                                     object_traits<typename attrib<T>::minus_const_ref>::is_always_const,
                                     const_reference, typename attrib<T>::plus_ref>::type
      reference;
   typedef const typename attrib<T>::minus_ref* const_pointer;
   typedef typename std::conditional<attrib<reference>::is_const, const_pointer, typename attrib<T>::minus_ref*>::type pointer;
   typedef reference arg_type;

   alias() = default;
   alias(arg_type arg)
      : _super(reinterpret_cast<typename is_masquerade<T>::hidden_stored_type>(arg)) {}

   typedef typename std::conditional<attrib<T>::is_const, const alias<typename attrib<T>::minus_const>, type2type<T> >::type alt_alias_arg_type;
   alias(alt_alias_arg_type& other)
      : _super(other) {}

   alias(alias&&) = default;
   alias(const alias&) = default;

   alias& operator= (const alias&) = default;
   alias& operator= (alias&&) = default;

   reference operator* () { return reinterpret_cast<reference>(_super::operator*()); }
   const_reference operator* () const { return reinterpret_cast<const_reference>(_super::operator*()); }
   pointer operator-> () { return &operator*(); }
   const_pointer operator-> () const { return &operator*(); }

   template <typename, alias_kind> friend class alias;
};

template <typename T>
class alias<T, alias_kind::delayed_masquerade>
   : public alias<typename demasq<T>::arg_type> {
   // delayed masquerade: keep an alias to a given source type
   typedef typename demasq<T>::type M;
   typedef alias<typename demasq<T>::arg_type> _super;
public:
   typedef typename attrib<M>::plus_const_ref const_reference;
   typedef typename std::conditional<object_traits<typename is_masquerade<M>::hidden_type>::is_always_const ||
                                     object_traits<typename attrib<M>::minus_const_ref>::is_always_const,
                                     const_reference, typename attrib<M>::plus_ref>::type
      reference;
   typedef const typename attrib<M>::minus_ref* const_pointer;
   typedef typename std::conditional<attrib<reference>::is_const, const_pointer, typename attrib<M>::minus_ref*>::type pointer;

   using alias<typename demasq<T>::arg_type>::alias;
   alias(_super&& arg) : _super(std::move(arg)) {}

   alias(const alias&) = default;
   alias(alias&&) = default;

   alias& operator= (const alias&) = default;
   alias& operator= (alias&&) = default;

   reference operator* () { return reinterpret_cast<reference>(_super::operator*()); }
   const_reference operator* () const { return reinterpret_cast<const_reference>(_super::operator*()); }
   pointer operator-> () { return &operator*(); }
   const_pointer operator-> () const { return &operator*(); }

   template <typename, alias_kind> friend class alias;
};

// keeps nothing
template <>
class alias<nothing, alias_kind::obj>
{
public:
   typedef const nothing& reference;
   typedef reference const_reference;
   typedef const nothing* pointer;
   typedef pointer const_pointer;
   typedef reference arg_type;

   alias() {}
   alias(arg_type) {}

   const_reference operator* () const { return std::pair<nothing,nothing>::first; }
   const_pointer operator-> () const { return &operator*(); }
};

template <typename T>
struct make_alias {
   using type = alias<T>;
};

} // end namespace pm

#endif // POLYMAKE_INTERNAL_SHARED_OBJECT_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
