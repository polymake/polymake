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

/** @file Array.h
    @brief Implementation of pm::Array class
*/

#ifndef POLYMAKE_ARRAY_H
#define POLYMAKE_ARRAY_H

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

template <typename E, typename SharedParams>
class Array : public operators::base {
protected:
   /// data array
   shared_array<E, typename concat_list<SharedParams, AliasHandler<shared_alias_handler> >::type> data;

   friend Array& make_mutable_alias(Array& alias, Array& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }
public:
   /// element type
   typedef E value_type;
   /// element reference type
   typedef E& reference;
   /// ... constant version
   typedef const E& const_reference;

   /// Create an empty array.
   Array() {}

#if POLYMAKE_DEBUG
   ~Array() { POLYMAKE_DEBUG_METHOD(Array,dump); }
   void dump(False) const { cerr << "elements are not printable" << std::flush; }
   void dump(True) const { cerr << *this << std::flush; }
   void dump() const { dump(bool2type<is_printable<E>::value>()); }
#endif

   /// Create an array with @a n elements, initialized with the default constructor.
   explicit Array(int n) : data(n) {}

   /// Create an array with @a n elements, initialized with the same value.
   Array(int n, const E& init) : data(n, constant(init).begin()) {}

   /// Create an array with @a n, initialized from a data sequence.
   template <typename Iterator>
   Array(int n, Iterator src,
         typename construct_matching_iterator<Iterator, E>::enabled=0) :
      data(n, construct_matching_iterator<Iterator, E>()(src)) {}

   template <typename Iterator>
   Array(Iterator first, Iterator last,
         typename construct_matching_iterator<Iterator, E>::enabled=0) :
      data(std::distance(first, last), construct_matching_iterator<Iterator, E>()(first)) {}

   /// Create an array with @a n elements, initialized from a built-in array.
   template <typename E2, size_t n>
   explicit Array(const E2 (&a)[n],
                  typename enable_if<void**, convertible_to<E2, E>::value>::type=0) :
      data(n, a+0) {}

   /// ... with explicit element conversion.
   template <typename E2, size_t n>
   explicit Array(const E2 (&a)[n],
                  typename enable_if<void**, explicitly_convertible_to<E2, E>::value>::type=0) :
      data(n, attach_converter<E>(array2container(a)).begin()) {}

   template <typename E2, size_t m, size_t n>
   explicit Array(const E2 (&a)[m][n],
                  typename enable_if<void**, isomorphic_to_container_of<E, E2, allow_conversion>::value>::type=0) :
      data(m, a+0) {}

   template <typename Container>
   Array(const Container& src,
         typename enable_if<void**, isomorphic_to_container_of<Container, E, allow_conversion>::value>::type=0)
      : data(src.size(), ensure(src, (dense*)0).begin()) {}

   /// number of elements
   int size() const { return data.size(); }

   /// true if empty
   bool empty() const { return size()==0; }

   /// truncate to zero size
   void clear() { data.clear(); }

   /// If @a n is less than the current array length, delete the trailing
   /// elements. If greater, add new elements initialized with the default
   /// constructor resp. the copies of the given value @a x.
   ///
   /// Unlike @c std::vector, @c Array never allocates extra stock
   /// storage. Each resize operation causes the data area reallocation.
   void resize(int n) { data.resize(n); }

   /// Same as above, with explicit element construction.
   void resize(int n, const E& x)
   {
      data.append(n-size(), constant(x).begin());
   }

   void assign(int n, const E& x)
   {
      data.assign(n, constant(x).begin());
   }

   /// Keep the old elements, add @a n new elements to the tail, assign them values from the data sequence.
   template <typename Iterator>
   void append(int n, Iterator src)
   {
      data.append(n, src);
   }

   /// Assign @a x to all elements.
   void fill(const E& x)
   {
      data.assign(size(), constant(x).begin());
   }

   /// Resize to @a n elements, assign values from the built-in array.
   template <int n>
   Array& operator= (const E (&a)[n])
   {
      data.assign(n,a+0);
      return *this;
   }

   /// Swap the contents of two arrays in a most efficient way.
   void swap(Array& a) { data.swap(a.data); }

   friend void relocate(Array* from, Array* to)
   {
      relocate(&from->data, &to->data);
   }

   /// random access
   E& operator[] (int i)
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=size())
            throw std::runtime_error("Array::operator[] - index out of range");
      }
      return (*data)[i];
   }

   /// ... constant version
   const E& operator[] (int i) const
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=size())
            throw std::runtime_error("Array::operator[] - index out of range");
      }
      return (*data)[i];
   }

   typedef E* iterator;
   typedef const E* const_iterator;

   iterator begin() { return *data; }
   iterator end() { return *data+size(); }
   const_iterator begin() const { return *data; }
   const_iterator end() const { return *data+size(); }

   typedef std::reverse_iterator<iterator> reverse_iterator;
   typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

   reverse_iterator rbegin() { return reverse_iterator(end()); }
   reverse_iterator rend() { return reverse_iterator(begin()); }
   const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
   const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

   reference front() { return (*data)[0]; }
   const_reference front() const { return (*data)[0]; }
   reference back() { return (*data)[size()-1]; }
   const_reference back() const { return (*data)[size()-1]; }
};

template <typename E>
struct spec_object_traits< Array<E> >
   : spec_object_traits<is_container> {};

   
namespace operators {
      
   /// equality of Array objects
   template <typename E> inline
   bool operator== (const Array<E>& l, const Array<E>& r) {
      if ( r.size() != l.size() ) { return false; }
      operations::eq<const Array<E>&, const Array<E>&> cmp_op;
      return cmp_op(l, r);
   }

}
   
} // end namespace pm

namespace polymake {
using pm::Array;
}

namespace std {
template <typename E> inline
void swap(pm::Array<E>& a1, pm::Array<E>& a2) { a1.swap(a2); }
}

#endif // POLYMAKE_ARRAY_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
