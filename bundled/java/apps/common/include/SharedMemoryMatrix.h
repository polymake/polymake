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

#ifndef POLYMAKE_COMMON_SHARED_MEMORY_MATRIX_H
#define POLYMAKE_COMMON_SHARED_MEMORY_MATRIX_H

#include "polymake/Matrix.h"
#include "polymake/Vector.h"

#include <cassert>

namespace polymake { namespace common {

class SharedMemorySegment {
protected:
   void *shmaddr;
   int shmid;

   SharedMemorySegment() : shmaddr(0) {}
   explicit SharedMemorySegment(size_t size) { resize(size); }
   ~SharedMemorySegment();

   void resize(size_t size);
private:
   // @disabled
   SharedMemorySegment(const SharedMemorySegment&);
   void operator= (const SharedMemorySegment&);
public:
   int get_shmid() const { return shmid; }
};

template <typename E>
class SharedMemoryVector
   : public SharedMemorySegment
   , public Vector<E> {
   typedef Vector<E> super;

   static size_t alloc_size(int n) { return super::shared_array_type::alloc_size(n); }
public:
   SharedMemoryVector() {}

   explicit SharedMemoryVector(int n)
      : SharedMemorySegment(alloc_size(n)),
        super(shmaddr,n) {}

   SharedMemoryVector(int n, typename pm::function_argument<E>::type init)
      : SharedMemorySegment(alloc_size(n)),
        super(shmaddr,n,constant(init).begin()) {}

   template <typename Iterator>
   SharedMemoryVector(int n, Iterator src, typename pm::enable_if<void**, !std::numeric_limits<Iterator>::is_specialized>::type=0)
      : SharedMemorySegment(alloc_size(n)),
        super(shmaddr,n,src) {}

   template <typename Vector2>
   SharedMemoryVector(const GenericVector<Vector2,E>& v)
      : SharedMemorySegment(alloc_size(v.dim())),
        super(shmaddr,v.dim(),ensure(v.top(), (pm::dense*)0).begin()) {}
   
   template <typename Vector2, typename E2>
   explicit SharedMemoryVector(const GenericVector<Vector2,E2>& v)
      : SharedMemorySegment(alloc_size(v.dim())),
        super(shmaddr,v.dim(),ensure(attach_converter<E>(v.top()), (pm::dense*)0).begin()) {}

   template <size_t n>
   SharedMemoryVector(const E (&a)[n])
      : SharedMemorySegment(alloc_size(n)),
        super(shmaddr,n,a+0) {}

   template <typename E2, size_t n>
   explicit SharedMemoryVector(const E2 (&a)[n])
      : SharedMemorySegment(alloc_size(n)),
        super(shmaddr,n,attach_converter<E>(array2container(a)).begin()) {}

   SharedMemoryVector(const SharedMemoryVector& v)
      : SharedMemorySegment(alloc_size(v.dim())),
        super(shmaddr,v.dim(),v.begin()) {}

   SharedMemoryVector& operator= (const SharedMemoryVector& other)
   {
      super::generic_type::operator=(other);
      return *this;
   }

   template <typename Vector2>
   SharedMemoryVector& operator= (const GenericVector<Vector2>& other)
   {
      super::generic_type::operator=(other);
      return *this;
   }

   using super::generic_type::swap;
protected:
   using super::generic_type::assign;
   using super::generic_type::_fill;
public:

   void clear() { this->fill(E(0)); }

   void resize(int n)
   {
      if (!shmaddr) {
         SharedMemorySegment::resize(alloc_size(n));
         super::resize(shmaddr,n);
      } else {
         assert(n==this->size());
      }
   }

   template <typename R>
   void operator|= (const R&) { throw std::runtime_error("SharedMemoryVector - concatenation not allowed"); }
};

template <typename E>
class SharedMemoryMatrix
   : public SharedMemorySegment,
     public Matrix<E> {
   typedef Matrix<E> super;
   static size_t alloc_size(int r, int c) { return super::shared_array_type::alloc_size(r*c); }
public:
   SharedMemoryMatrix() {}

   SharedMemoryMatrix(int r, int c)
      : SharedMemorySegment(alloc_size(r,c)),
        super(shmaddr,r,c) {}

   SharedMemoryMatrix(int r, int c, typename pm::function_argument<E>::type init)
      : SharedMemorySegment(alloc_size(r,c)),
        super(shmaddr,r,c,constant(init).begin()) {}

   template <typename Iterator>
   SharedMemoryMatrix(int r, int c, Iterator src)
      : SharedMemorySegment(alloc_size(r,c)),
        super(shmaddr,r,c,src) {}

   template <size_t r, size_t c>
   explicit SharedMemoryMatrix(const E (&a)[r][c])
      : SharedMemorySegment(alloc_size(r,c)),
        super(shmaddr,r,c,&a[0][0]) {}

   template <typename Matrix2, typename E2>
   SharedMemoryMatrix(const GenericMatrix<Matrix2, E2>& m,
                      typename pm::enable_if<void**, pm::convertible_to<E2, E>::value>::type=0)
      : SharedMemorySegment(alloc_size(m.rows(), m.cols())),
        super(shmaddr, m.rows(), m.cols(), ensure(concat_rows(m), (pm::dense*)0).begin()) {}

   template <typename Matrix2, typename E2>
   explicit SharedMemoryMatrix(const GenericMatrix<Matrix2, E2>& m,
                               typename pm::enable_if<void**, pm::explicitly_convertible_to<E2, E>::value>::type=0)
      : SharedMemorySegment(alloc_size(m.rows(), m.cols())),
        super(shmaddr, m.rows(), m.cols(), ensure(pm::attach_converter<E>(concat_rows(m)), (pm::dense*)0).begin()) {}

   template <typename Container>
   SharedMemoryMatrix(const Container& src,
                      typename pm::enable_if<void**, pm::isomorphic_to_container_of<Container, Vector<E> >::value>::type=0)
      : SharedMemorySegment(alloc_size(src.size(), src.empty() ? 0 : get_dim(src.front()))),
        super(shmaddr, src.size(), src.empty() ? 0 : get_dim(src.front()),src) {}

   SharedMemoryMatrix(const SharedMemoryMatrix& m)
      : SharedMemorySegment(alloc_size(m.rows(),m.cols())),
        super(shmaddr,m.rows(),m.cols(),concat_rows(m).begin()) {}

   SharedMemoryMatrix& operator= (const SharedMemoryMatrix& other)
   {
      super::generic_type::operator=(other);
      return *this;
   }

   template <typename Matrix2>
   SharedMemoryMatrix& operator= (const GenericMatrix<Matrix2>& other)
   {
      super::generic_type::operator=(other);
      return *this;
   }

   using super::generic_type::swap;
protected:
   using super::generic_type::assign;
   using super::generic_type::_fill;
public:

   void resize(int r, int c)
   {
      if (!shmaddr) {
         SharedMemorySegment::resize(alloc_size(r,c));
         super::resize(shmaddr,r,c);
      } else {
         assert(r==this->rows() && c==this->cols());
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

   using super::generic_type::operator/=;
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

#endif // POLYMAKE_COMMON_SHARED_MEMORY_MATRIX_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
