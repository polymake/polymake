/* Copyright (c) 1997-2014
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

/** @file Vector.h
    @brief Implementation of pm::Vector class
 */

#ifndef POLYMAKE_VECTOR_H
#define POLYMAKE_VECTOR_H

#include "polymake/GenericVector.h"
#include "polymake/internal/shared_object.h"

namespace pm {

/// @ref vector_sec "Vector type" class which holds the elements in a contiguous array
template <typename E>
class Vector :
   public GenericVector<Vector<E>, E> {
protected:
   typedef shared_array<E, AliasHandler<shared_alias_handler> > shared_array_type;
   shared_array_type data;

   friend Vector& make_mutable_alias(Vector& alias, Vector& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }
public:
   /// element type
   typedef E value_type;
   /// reference to element type
   typedef E& reference;
   /// constant reference to element type
   typedef const E& const_reference;

   /// create as empty
   Vector() {}

   /// create vector of length n
   explicit Vector(int n) : data(n) {}

   template <typename E2>
   Vector(int n, const E2& init,
          typename enable_if<void**, convertible_to<E2, E>::value>::type=0) :
      data(n, constant(init).begin()) {}

   template <typename E2>
   Vector(int n, const E2& init,
          typename enable_if<void**, explicitly_convertible_to<E2, E>::value>::type=0) :
      data(n, constant(conv<E2, E>()(init)).begin()) {}

   template <typename Iterator>
   Vector(int n, Iterator src,
          typename construct_matching_iterator<Iterator, E>::enabled=0)
      : data(n, construct_matching_iterator<Iterator, E>()(src)) {}

   template <typename Iterator>
   Vector(Iterator first, Iterator last,
          typename construct_matching_iterator<Iterator, E>::enabled=0)
      : data(std::distance(first, last), construct_matching_iterator<Iterator, E>()(first)) {}

   template <typename Container>
   explicit Vector(const Container& src,
                   typename enable_if<void**, (isomorphic_to_container_of<Container, E, is_vector>::value &&
                                               convertible_to<typename Container::value_type, E>::value)>::type=0)
      : data(src.size(), ensure(src, (dense*)0).begin()) {}

   template <typename Container>
   explicit Vector(const Container& src,
                   typename enable_if<void**, (isomorphic_to_container_of<Container, E, is_vector>::value &&
                                               explicitly_convertible_to<typename Container::value_type, E>::value)>::type=0)
      : data(src.size(), make_converting_iterator<E>(ensure(src, (dense*)0).begin())) {}

   Vector(const GenericVector<Vector>& v) : data(v.top().data) {}

   template <typename Vector2>
   Vector(const GenericVector<Vector2, E>& v)
      : data(v.dim(), ensure(v.top(), (dense*)0).begin()) {}

   template <typename Vector2, typename E2>
   explicit Vector(const GenericVector<Vector2, E2>& v,
                   typename enable_if<void**, convertible_to<E2, E>::value>::type=0)
      : data(v.dim(), ensure(v.top(), (dense*)0).begin()) {}

   template <typename Vector2, typename E2>
   explicit Vector(const GenericVector<Vector2, E2>& v,
                   typename enable_if<void**, explicitly_convertible_to<E2, E>::value>::type=0)
      : data(v.dim(), make_converting_iterator<E>(ensure(v.top(), (dense*)0).begin())) {}

   template <typename E2, size_t n>
   explicit Vector(const E2 (&a)[n],
                   typename enable_if<void**, convertible_to<E2, E>::value>::type=0)
      : data(n, a+0) {}

   template <typename E2, size_t n>
   explicit Vector(const E2 (&a)[n],
                   typename enable_if<void**, explicitly_convertible_to<E2, E>::value>::type=0)
      : data(n, attach_converter<E>(array2container(a)).begin()) {}

protected:
   Vector(void *place, int n) : data(place,n) {}
   template <typename Iterator>
   Vector(void *place, int n, Iterator src) : data(place,n,src) {}
   void resize(void *place, int n) { data.resize(place,n); }

public:
   /// number of elements
   int size() const { return data.size(); }

   /// true if Vector does not contain any elements
   bool empty() const { return size()==0; }

   /// truncate to zero size
   void clear() { data.clear(); }

   /// change the size, initialize appended elements with default constructor
   void resize(int n) { data.resize(n); }

   template <typename E2>
   typename enable_if<void, convertible_to<E2, E>::value>::type
   assign(int n, const E2& x)
   {
      data.assign(n, constant(x).begin());
   }

   Vector& operator= (const Vector& other) { assign(other); return *this; }
   using Vector::generic_type::operator=;

   void swap(Vector& v) { data.swap(v.data); }

   friend void relocate(Vector* from, Vector* to)
   {
      relocate(&from->data, &to->data);
   }

   /// random access
   E& operator[] (int i)
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=this->size())
            throw std::runtime_error("Vector::operator[] - index out of range");
      }
      return (*data)[i];
   }

   /// constant random access
   const E& operator[] (int i) const
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=this->size())
            throw std::runtime_error("Vector::operator[] - index out of range");
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

   reference front() { return begin()[0]; }
   const_reference front() const { return begin()[0]; }
   reference back() { return end()[-1]; }
   const_reference back() const { return end()[-1]; }

   /// appending a GenericVector
   template <typename Vector2, typename E2> inline
   typename enable_if<Vector, convertible_to<E2, E>::value>::type&
   operator|= (const GenericVector<Vector2, E2>& v)
   {
      data.append(v.dim(), ensure(v.top(), (dense*)0).begin());
      return *this;
   }

   template <typename Vector2, typename E2> inline
   typename enable_if<Vector, explicitly_convertible_to<E2, E>::value>::type&
   operator|= (const GenericVector<Vector2, E2>& v)
   {
      data.append(v.dim(), ensure(attach_converter<E>(v.top()), (dense*)0).begin());
      return *this;
   }

   /// appending an element
   template <typename E2>
   typename enable_if<Vector, (isomorphic_types<E, E2>::value && convertible_to<E2, E>::value)>::type&
   operator|= (const E2& r)
   {
      data.append(1, &r);
      return *this;
   }

   template <typename E2>
   typename enable_if<Vector, (isomorphic_types<E, E2>::value && explicitly_convertible_to<E2, E>::value)>::type&
   operator|= (const E2& r)
   {
      data.append(1, make_converting_iterator<E>(&r));
      return *this;
   }

protected:
   template <typename, typename> friend class GenericVector;

   void assign(const Vector& v) { data=v.data; }

   template <typename Container>
   typename enable_if<void, convertible_to<typename Container::value_type, E>::value>::type
   assign(const Container& c)
   {
      data.assign(c.dim(), ensure(c, (dense*)0).begin());
   }

   template <typename Container>
   typename enable_if<void, explicitly_convertible_to<typename Container::value_type, E>::value>::type
   assign(const Container& c)
   {
      data.assign(c.dim(), ensure(attach_converter<E>(c), (dense*)0).begin());
   }

   template <typename Operation>
   void assign_op(const Operation& op)
   {
      data.assign_op(op);
   }

   template <typename Container, typename Operation>
   void assign_op(const Container& c, const Operation& op)
   {
      data.assign_op(ensure(c, (dense*)0).begin(), op);
   }

   template <typename E2>
   typename enable_if<void, convertible_to<E2, E>::value>::type
   _fill(const E2& x, dense)
   {
      data.assign(data.size(), constant(x).begin());
   }
};

template <typename VectorTop, typename E, typename Permutation> inline
typename enable_if<Vector<E>, !VectorTop::is_sparse>::type
permuted(const GenericVector<VectorTop,E>& v, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<VectorTop>::value) {
      if (v.dim() != perm.size())
         throw std::runtime_error("permuted - dimension mismatch");
   }
   return Vector<E>(v.dim(), select(v.top(),perm).begin());
}

template <typename VectorTop, typename E, typename Permutation> inline
typename enable_if<Vector<E>, !VectorTop::is_sparse>::type
permuted_inv(const GenericVector<VectorTop,E>& v, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<VectorTop>::value) {
      if (v.dim() != perm.size())
         throw std::runtime_error("permuted_inv - dimension mismatch");
   }
   Vector<E> result(v.dim());
   copy(entire(v.top()), select(result,perm).begin());
   return result;
}

} // end namespace pm

namespace polymake {
   using pm::Vector;
}

namespace std {
   template <typename E> inline
   void swap(pm::Vector<E>& v1, pm::Vector<E>& v2) { v1.swap(v2); }
}

#endif // POLYMAKE_VECTOR_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
