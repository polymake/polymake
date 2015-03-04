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

#ifndef POLYMAKE_INTERNAL_WARY_H
#define POLYMAKE_INTERNAL_WARY_H

#include "polymake/internal/iterators.h"

namespace pm {

template <typename T, typename Model=typename object_traits<T>::model>
class Wary_traits {};

template <typename T>
class Wary_traits<T, is_container> : public container_traits<T> {};

template <typename T>
class Wary :
   public Wary_traits<T>,
   public inherit_generic<Wary<T>, T>::type
{
protected:
   // never instantiate
   Wary();
   ~Wary();
public:
   using inherit_generic<Wary,T>::type::operator=;
};

template <typename T>
class Generic< Wary<T> > {
protected:
   Generic();
   ~Generic();
public:
   typedef T top_type;
   typedef T persistent_type;
   typedef Wary<T> concrete_type;

   const top_type& top() const
   {
      return reinterpret_cast<const T&>(static_cast<const Wary<T>&>(*this));
   }
   top_type& top()
   {
      return reinterpret_cast<T&>(static_cast<Wary<T>&>(*this));
   }
};

template <typename T, typename Feature>
struct check_container_feature<Wary<T>, Feature> : check_container_feature<T, Feature> {};

template <typename T>
struct redirect_object_traits< Wary<T> > : object_traits<T> {};

template <typename T> inline
Wary<T>& wary(T& x)
{
   return reinterpret_cast<Wary<T>&>(x);
}
template <typename T> inline
const Wary<T>& wary(const T& x)
{
   return reinterpret_cast<const Wary<T>&>(x);
}

template <typename T>
struct MaybeWary : assignable_to<T, Wary<T>, T> {};

template <typename T> inline
typename enable_if<Wary<T>&, MaybeWary<T>::value>::type
maybe_wary(T& x)
{
   return wary(x);
}
template <typename T> inline
typename disable_if<T&, MaybeWary<T>::value>::type
maybe_wary(T& x)
{
   return x;
}

template <typename T>
struct Unwary : True {
   typedef T type;
};

template <typename T>
struct Unwary< Wary<T> > : False {
   typedef T type;
};

template <typename T>
struct Unwary< Wary<T>& > : False {
   typedef T& type;
};

template <typename T>
struct Unwary< const Wary<T>& > : False {
   typedef const T& type;
};

template <typename T> inline
T& unwary(T& x) { return x; }

template <typename T> inline
const T& unwary(const T& x) { return x; }

template <typename T> inline
T& unwary(Wary<T>& x) { return x.top(); }

template <typename T> inline
const T& unwary(const Wary<T>& x) { return x.top(); }

}
namespace polymake {
   using pm::Wary;
   using pm::wary;
}

#endif // POLYMAKE_INTERNAL_WARY_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
