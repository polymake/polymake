/* Copyright (c) 1997-2022
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

#include "polymake/Matrix.h"
#include "polymake/Vector.h"

#include <cassert>

namespace polymake { namespace common {

class SharedMemorySegment {
protected:
   pm::shared_array_placement shmaddr;
   int shmid;

   SharedMemorySegment() {}
   explicit SharedMemorySegment(size_t size) { resize(size); }
   ~SharedMemorySegment();

   void resize(size_t size);
private:
   SharedMemorySegment(const SharedMemorySegment&) = delete;
   void operator= (const SharedMemorySegment&) = delete;
public:
   int get_shmid() const { return shmid; }
};

template <typename E>
class SharedMemoryVector
   : public SharedMemorySegment
   , public Vector<E> {
   typedef Vector<E> base_t;

   static size_t alloc_size(Int n) { return base_t::shared_array_type::alloc_size(n); }
public:
   SharedMemoryVector() {}

   explicit SharedMemoryVector(Int n)
      : SharedMemorySegment(alloc_size(n))
      , base_t(shmaddr, n) {}

   template <typename E2>
   SharedMemoryVector(Int n, const E2& init,
                      std::enable_if_t<pm::can_initialize<E2, E>::value, std::nullptr_t> = nullptr)
      : SharedMemorySegment(alloc_size(n))
      , base_t(shmaddr, n, init) {}

   template <typename Iterator>
   SharedMemoryVector(Int n, Iterator&& src,
                      std::enable_if_t<pm::assess_iterator_value<Iterator, pm::can_initialize, E>::value, std::nullptr_t> = nullptr)
      : SharedMemorySegment(alloc_size(n))
      , base_t(shmaddr, n, std::forward<Iterator>(src)) {}

   template <typename Vector2>
   SharedMemoryVector(const GenericVector<Vector2, E>& v)
      : SharedMemorySegment(alloc_size(v.dim()))
      , base_t(shmaddr, v.dim(), ensure(v.top(), pm::dense()).begin()) {}
   
   template <typename Vector2, typename E2>
   explicit SharedMemoryVector(const GenericVector<Vector2, E2>& v,
                               std::enable_if_t<pm::can_initialize<E2, E>::value, std::nullptr_t> = nullptr)
      : SharedMemorySegment(alloc_size(v.dim()))
      , base_t(shmaddr, v.dim(), ensure(v.top(), pm::dense()).begin()) {}

   template <typename E2,
             typename = std::enable_if_t<pm::can_initialize<E2, E>::value>>
   SharedMemoryVector(std::initializer_list<E2> l)
      : SharedMemorySegment(alloc_size(l.size()))
      , base_t(shmaddr, l.size(), l.begin()) {}

   SharedMemoryVector(const SharedMemoryVector& v)
      : SharedMemorySegment(alloc_size(v.dim()))
      , base_t(shmaddr, v.dim(), v.begin()) {}

   SharedMemoryVector& operator= (const SharedMemoryVector& other)
   {
      base_t::generic_type::operator=(other);
      return *this;
   }
   using base_t::generic_type::operator=;

   template <typename Vector2>
   SharedMemoryVector& operator= (const GenericVector<Vector2>& other)
   {
      base_t::generic_type::operator=(other);
      return *this;
   }

   using base_t::generic_type::swap;
protected:
   using base_t::generic_type::assign;
   using base_t::generic_type::fill_impl;
public:

   void clear() { this->fill(zero_value<E>()); }

   void resize(Int n)
   {
      if (!shmaddr.get()) {
         SharedMemorySegment::resize(alloc_size(n));
         base_t::resize(shmaddr, n);
      } else {
         assert(n == this->size());
      }
   }

   template <typename R>
   void operator|= (const R&) { throw std::runtime_error("SharedMemoryVector - concatenation not allowed"); }
};

template <typename E>
class SharedMemoryMatrix
   : public SharedMemorySegment
   , public Matrix<E> {
   using base_t = Matrix<E>;

   template <typename Iterator>
   using fits_as_input_iterator = typename base_t::template fits_as_input_iterator<Iterator>;

   static size_t alloc_size(Int r, Int c)
   {
      return base_t::shared_array_type::alloc_size(r * c);
   }
public:
   SharedMemoryMatrix() {}

   SharedMemoryMatrix(Int r, Int c)
      : SharedMemorySegment(alloc_size(r, c))
      , base_t(shmaddr, r, c) {}

   SharedMemoryMatrix(Int r, Int c, const E& init)
      : SharedMemorySegment(alloc_size(r, c))
      , base_t(shmaddr, r, c, init) {}

   template <typename Iterator, typename = std::enable_if_t<fits_as_input_iterator<Iterator>::value>>
   SharedMemoryMatrix(Int r, Int c, Iterator&& src)
      : SharedMemorySegment(alloc_size(r, c))
      , base_t(shmaddr, r, c, std::forward<Iterator>(src)) {}

   template <typename Matrix2, typename E2>
   SharedMemoryMatrix(const GenericMatrix<Matrix2, E2>& m,
                      std::enable_if_t<pm::can_initialize<E2, E>::value, std::nullptr_t> = nullptr)
      : SharedMemorySegment(alloc_size(m.rows(), m.cols()))
      , base_t(shmaddr, m.rows(), m.cols(), ensure(concat_rows(m), pm::dense()).begin()) {}

   template <typename Container>
   SharedMemoryMatrix(const Container& src,
                      std::enable_if_t<pm::isomorphic_to_container_of<Container, Vector<E>>::value, std::nullptr_t> = nullptr)
      : SharedMemorySegment(alloc_size(src.size(), src.empty() ? 0 : get_dim(src.front())))
      , base_t(shmaddr, src.size(), src.empty() ? 0 : get_dim(src.front()), src.begin()) {}

   SharedMemoryMatrix(const SharedMemoryMatrix& m)
      : SharedMemorySegment(alloc_size(m.rows(), m.cols()))
      , base_t(shmaddr, m.rows(), m.cols(), concat_rows(m).begin()) {}

   SharedMemoryMatrix& operator= (const SharedMemoryMatrix& other)
   {
      base_t::generic_type::operator=(other);
      return *this;
   }

   using base_t::generic_type::operator=;

   using base_t::generic_type::swap;
protected:
   using base_t::generic_type::assign;
   using base_t::generic_type::fill_impl;
public:

   void resize(Int r, Int c)
   {
      if (!shmaddr.get()) {
         SharedMemorySegment::resize(alloc_size(r, c));
         base_t::resize(shmaddr, r, c);
      } else {
         assert(r == this->rows() && c == this->cols());
      }
   }

   void clear() { this->fill(E(0)); }

   template <typename Matrix2>
   void operator/= (const GenericMatrix<Matrix2,E>&) { throw std::runtime_error("SharedMemoryMatrix - concatenation not allowed"); }
   template <typename Vector2>
   void operator/= (const GenericVector<Vector2,E>&) { throw std::runtime_error("SharedMemoryMatrix - concatenation not allowed"); }
   template <typename Matrix2>
   void operator|= (const GenericMatrix<Matrix2,E>&) { throw std::runtime_error("SharedMemoryMatrix - concatenation not allowed"); }
   template <typename Vector2>
   void operator|= (const GenericVector<Vector2,E>&) { throw std::runtime_error("SharedMemoryMatrix - concatenation not allowed"); }
};

template <typename E> inline
Rows< Matrix<E> >& rows(SharedMemoryMatrix<E>& m) { return rows(static_cast<Matrix<E>&>(m)); }
template <typename E> inline
const Rows< Matrix<E> >& rows(const SharedMemoryMatrix<E>& m) { return rows(static_cast<const Matrix<E>&>(m)); }
template <typename E> inline
Cols< Matrix<E> >& cols(SharedMemoryMatrix<E>& m) { return cols(static_cast<Matrix<E>&>(m)); }
template <typename E> inline
const Cols< Matrix<E> >& cols(const SharedMemoryMatrix<E>& m) { return cols(static_cast<const Matrix<E>&>(m)); }
template <typename E> inline
ConcatRows< Matrix<E> >& concat_rows(SharedMemoryMatrix<E>& m) { return concat_rows(static_cast<Matrix<E>&>(m)); }
template <typename E> inline
const ConcatRows< Matrix<E> >& concat_rows(const SharedMemoryMatrix<E>& m) { return concat_rows(static_cast<const Matrix<E>&>(m)); }

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
