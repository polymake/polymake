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
/** @file Array.h
    @brief Implementation of pm::Array class
*/


#include "polymake/internal/shared_object.h"
#include "polymake/internal/converters.h"
#include "polymake/GenericIO.h"
#include "polymake/internal/comparators.h"
#include "polymake/internal/Array.h"

#include <stdexcept>
#include <limits>

namespace pm {

/** @class Array
    @brief Container class with constant time random access.

    Offers practically the same as @c std::vector.  The only significant
    differences are that the data array is attached via a smart pointer with
    @ref refcounting "reference counting", and the set of operations changing
    the size of the array is reduced to the minimum.

    Array inplements STL's "Random Access Container" interface.  The element
    index in the random access method should lie in the valid range, the array
    does not grow implicitly.
*/

template <typename E, typename... SharedParams>
class Array
   : public plain_array< Array<E, SharedParams...>, E> {

   static_assert(!std::is_same<E, int>::value, "use Int instead");
protected:
   /// data array
   shared_array<E, typename mtagged_list_add_default<typename mlist_wrap<SharedParams...>::type, AliasHandlerTag<shared_alias_handler>>::type> data;

   friend Array& make_mutable_alias(Array& alias, Array& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }

   friend class plain_array< Array<E, SharedParams...>, E>;

   E* get_data() { return *data; }
   const E* get_data() const { return *data; }

public:
   /// Create an empty array.
   Array() {}

   /// Create an array with @a n elements, initialized with the default constructor.
   explicit Array(Int n)
      : data(n) {}

   /// Create an array with @a n elements, initialized with the same value.
   template <typename E2>
   Array(Int n, const E2& init,
         std::enable_if_t<can_initialize<E2, E>::value, std::nullptr_t> = nullptr)
      : data(n, init) {}

   /// Create an array with @a n, initialized from a data sequence.
   template <typename Iterator>
   Array(Int n, Iterator&& src,
         std::enable_if_t<assess_iterator_value<Iterator, can_initialize, E>::value, std::nullptr_t> = nullptr)
      : data(n, ensure_private_mutable(make_converting_iterator<E>(std::forward<Iterator>(src)))) {}

   template <typename Iterator>
   Array(Iterator&& src, Iterator&& src_end,
         std::enable_if_t<assess_iterator_value<Iterator, can_initialize, E>::value, std::nullptr_t> = nullptr)
      : data(std::distance(src, src_end), ensure_private_mutable(make_converting_iterator<E>(std::forward<Iterator>(src)))) {}

   template <typename E2, typename = std::enable_if_t<can_initialize<E2, E>::value>>
   Array(std::initializer_list<E2> l)
      : data(l.size(), make_converting_iterator<E>(l.begin())) {}

   template <typename E2, typename = std::enable_if_t<can_initialize<std::initializer_list<E2>, E>::value>>
   Array(std::initializer_list< std::initializer_list<E2> > l)
      : data(l.size(), l.begin()) {}

   template <typename... Containers,
             typename = std::enable_if_t<isomorphic_to_container_of<mlist<Containers...>, E, allow_conversion>::value>>
   explicit Array(const Containers&... src)
      : data(total_size(src...),
             make_converting_iterator<E>(ensure(src, std::conditional_t<sizeof...(Containers)==1, dense, mlist<dense, end_sensitive>>()).begin())...) {}

   /// number of elements
   Int size() const { return data.size(); }

   /// truncate to zero size
   void clear() { data.clear(); }

   /// If @a n is less than the current array length, delete the trailing
   /// elements. If greater, add new elements initialized with the default
   /// constructor resp. the copies of the given value @a x.
   ///
   /// Unlike @c std::vector, @c Array never allocates extra stock
   /// storage. Each resize operation causes the data area reallocation.
   void resize(Int n) { data.resize(n); }

   /// Same as above, with explicit element construction.
   void resize(Int n, const E& x)
   {
      data.append(n-size(), x);
   }

   void assign(Int n, const E& x)
   {
      data.assign(n, x);
   }

   template <typename E2, typename = std::enable_if_t<can_assign_to<E2, E>::value>>
   Array& operator= (std::initializer_list<E2> l)
   {
      data.assign(l.size(), make_converting_iterator<E>(l.begin()));
      return *this;
   }

   template <typename E2, typename = std::enable_if_t<can_initialize<std::initializer_list<E2>, E>::value>>
   Array& operator= (std::initializer_list< std::initializer_list<E2> > l)
   {
      data.assign(l.size(), l.begin());
      return *this;
   }

   // TODO: introduce 'allow_implicit_conversion' and restrict to it here and elsewhere
   template <typename Container, typename = std::enable_if<isomorphic_to_container_of<Container, E, allow_conversion>::value>>
   Array& operator= (const Container& src)
   {
      data.assign(src.size(), make_converting_iterator<E>(ensure(src, dense()).begin()));
      return *this;
   }

   /// Keep the old elements, add @a n new elements at the tail, assign them values from the data sequence.
   template <typename Iterator,
             typename = std::enable_if_t<assess_iterator_value<Iterator, can_initialize, E>::value>>
   Array& append(Int n, Iterator&& src)
   {
      data.append(n, ensure_private_mutable(make_converting_iterator<E>(std::forward<Iterator>(src))));
      return *this;
   }

   /// Add new elements at the tail
   template <typename... Containers,
             typename = std::enable_if_t<isomorphic_to_container_of<mlist<Containers...>, E, allow_conversion>::value, void>>
   Array& append(const Containers&... src)
   {
      data.append(total_size(src...),
                  make_converting_iterator<E>(ensure(src, std::conditional_t<sizeof...(Containers)==1, dense, mlist<dense, end_sensitive>>()).begin())...);
      return *this;
   }

   template <typename E2, typename = std::enable_if_t<can_initialize<E2, E>::value>>
   Array& append(std::initializer_list<E2> l)
   {
      data.append(l.size(), make_converting_iterator<E>(l.begin()));
      return *this;
   }

   template <typename E2, typename = std::enable_if_t<can_initialize<std::initializer_list<E2>, E>::value>>
   Array& append(std::initializer_list< std::initializer_list<E2> > l)
   {
      data.append(l.size(), l.begin());
      return *this;
   }

   /// Assign @a x to all elements.
   template <typename E2>
   std::enable_if_t<std::is_convertible<E2, E>::value>
   fill(const E2& x)
   {
      data.assign(size(), x);
   }

   /// Swap the contents of two arrays in a most efficient way.
   void swap(Array& a) { data.swap(a.data); }

   friend void relocate(Array* from, Array* to)
   {
      relocate(&from->data, &to->data);
   }
      
   template <typename... Params2> friend
   bool operator== (const Array& l, const Array<E, Params2...>& r)
   {
      return r.size()==l.size() && equal_ranges(entire(l), r.begin());
   }

   template <typename... Params2> friend
   bool operator!= (const Array& l, const Array<E, Params2...>& r)
   {
      return !(l==r);
   }

#if POLYMAKE_DEBUG
private:
   void dump(std::false_type) const { cerr << "elements are not printable" << std::flush; }
   void dump(std::true_type) const { cerr << *this << std::flush; }
public:
   void dump() const __attribute__((used)) { dump(bool_constant<is_printable<E>::value>()); }
#endif
};

template <typename E, typename... SharedParams>
struct spec_object_traits<Array<E, SharedParams...>>
   : spec_object_traits<is_container> {};

} // end namespace pm

namespace polymake {
using pm::Array;
}

namespace std {
template <typename E> inline
void swap(pm::Array<E>& a1, pm::Array<E>& a2) { a1.swap(a2); }
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
