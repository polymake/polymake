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

/** @file Matrix.h
    @brief Implementation of pm::Matrix class
 */

#ifndef POLYMAKE_MATRIX_H
#define POLYMAKE_MATRIX_H

#include "polymake/GenericMatrix.h"
#include "polymake/internal/shared_object.h"

namespace pm {

template <bool rowwise, typename BaseRef=void> class matrix_line_factory;

template <typename E>
class Matrix_base {
protected:
   struct dim_t {
      int dimr, dimc;
      dim_t() : dimr(0), dimc(0) {}
      dim_t(int r, int c) : dimr(c? r:0), dimc(r? c:0) {
         if (POLYMAKE_DEBUG) {
            if (dimr<0 || dimc<0)
               throw std::runtime_error("Matrix_base::dim_t out of range");
         }
      }
   };
   typedef shared_array<E, list( PrefixData<dim_t>, AliasHandler<shared_alias_handler> ) > shared_array_type;
   shared_array_type data;

   friend Matrix_base& make_mutable_alias(Matrix_base& alias, Matrix_base& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }
   template <bool,typename> friend class matrix_line_factory;

   Matrix_base() {}

   Matrix_base(int r, int c) : data(dim_t(r,c), r*c) {}

   template <typename Iterator>
   Matrix_base(int r, int c, Iterator src) : data(dim_t(r,c), r*c, src) {}

   Matrix_base(void *place, int r, int c) : data(place, dim_t(r,c), r*c) {}

   template <typename Iterator>
   Matrix_base(void *place, int r, int c, Iterator src) : data(place, dim_t(r,c), r*c, src) {}

   friend class ConcatRows<Matrix_base>;
   template <typename, int> friend class alias;
};

   /**
     @class Matrix
     @brief \ref matrix_sec "Matrix type" class which holds the elements in a contiguous array <br>
     Additional arithmetic operations for matrices and useful constructions (<code>unit_matrix, diag, ...</code>) are listed at @ref genericMatrices "operations".
   */

template <typename E>
class Matrix :
   public GenericMatrix<Matrix<E>, E>,
   protected Matrix_base<E>
{
   typedef Matrix_base<E> base;
   typedef typename base::dim_t dim_t;

   friend Matrix& make_mutable_alias(Matrix& alias, Matrix& owner)
   {
      return static_cast<Matrix&>(make_mutable_alias(static_cast<base&>(alias), static_cast<base&>(owner)));;
   }
public:
   typedef E value_type;
   typedef E& reference;
   typedef const E& const_reference;

   /// create as empty
   Matrix() {}

   /// create matrix with r rows and c columns, initialize all elements to 0
   Matrix(int r, int c) : base(r,c) {}

   template <typename E2>
   Matrix(int r, int c, const E2& init,
          typename enable_if<void**, convertible_to<E2, E>::value>::type=0)
      : base(r, c, constant(init).begin()) {}

   template <typename E2>
   Matrix(int r, int c, const E2& init,
          typename enable_if<void**, explicitly_convertible_to<E2, E>::value>::type=0)
      : base(r, c, constant(conv<E2, E>(init)).begin()) {}

   template <typename E2, size_t r, size_t c>
   explicit Matrix(const E2 (&a)[r][c],
                   typename enable_if<void**, convertible_to<E2, E>::value>::type=0)
      : base(r, c, &a[0][0]) {}

   template <typename E2, size_t r, size_t c>
   explicit Matrix(const E2 (&a)[r][c],
                   typename enable_if<void**, explicitly_convertible_to<E2, E>::value>::type=0)
      : base(r, c, make_converting_iterator<E>(&a[0][0])) {}

   /** Create a matrix with given dimensions.  Elements are initialized from an input sequence.
       Elements are assumed to come in the row order.
   */
   template <typename Iterator>
   Matrix(int r, int c, Iterator src,
#ifdef __clang__
          // clang tries to instantiate this contructor for SharedMatrix, where the first argument is void* .
          typename disable_if<void**, std::numeric_limits<Iterator>::is_specialized>::type=0,
#endif
          typename construct_cascaded_iterator<Iterator, E, dense>::enabled=0)
      : base(r, c, construct_cascaded_iterator<Iterator, E, dense>()(src)) {}

   /// Copy of a disguised Matrix object.
   Matrix(const GenericMatrix<Matrix>& m) : base(m.top()) {}

   /// Copy of an abstract matrix of the same element type.
   template <typename Matrix2>
   Matrix(const GenericMatrix<Matrix2, E>& m)
      : base(m.rows(), m.cols(), ensure(concat_rows(m), (dense*)0).begin()) {}

   /// Copy of an abstract matrix with element conversion.
   template <typename Matrix2, typename E2>
   explicit Matrix(const GenericMatrix<Matrix2, E2>& m,
                   typename enable_if<void**, convertible_to<E2, E>::value>::type=0)
      : base(m.rows(), m.cols(), ensure(concat_rows(m), (dense*)0).begin()) {}

   template <typename Matrix2, typename E2>
   explicit Matrix(const GenericMatrix<Matrix2, E2>& m,
                   typename enable_if<void**, explicitly_convertible_to<E2, E>::value>::type=0)
      : base(m.rows(), m.cols(), ensure(attach_converter<E>(concat_rows(m)), (dense*)0).begin()) {}

   template <typename Container>
   explicit Matrix(const Container& src,
                   typename construct_cascaded_iterator<typename Container::const_iterator, E, dense>::enabled=0,
                   typename enable_if<void**, construct_cascaded_iterator<typename Container::const_iterator, E, dense>::depth==2>::type=0)
      : base(src.size(), src.empty() ? 0 : get_dim(src.front()),
             construct_cascaded_iterator<typename Container::const_iterator, E, dense>()(entire(src))) {}

protected:
   Matrix(void *place, int r, int c) : base(place,r,c) {}

   template <typename Iterator>
   Matrix(void *place, int r, int c, Iterator src,
          typename construct_cascaded_iterator<Iterator, E, dense>::enabled=0)
      : base(place, r, c, construct_cascaded_iterator<Iterator, E, dense>()(src)) {}

   template <typename Container>
   Matrix(void *place, int r, int c, const Container& src,
          typename construct_cascaded_iterator<typename Container::const_iterator, E, dense>::enabled=0,
          typename enable_if<void**, construct_cascaded_iterator<typename Container::const_iterator, E, dense>::depth==2>::type=0)
      : base(place, r, c, construct_cascaded_iterator<typename Container::const_iterator, E, dense>()(entire(src))) {}

   void resize(void *place, int r, int c)
   {
      this->data.resize(place,r*c);
      this->data.get_prefix().dimr=r;
      this->data.get_prefix().dimc=c;
   }
public:

   /// Persistent matrix objects have after the assignment the same dimensions as the right hand side operand. 
   /// Alias objects, such as matrix minor or block matrix, cannot be resized, thus must have the same dimensions as on the right hand side.
   Matrix& operator= (const Matrix& other) { assign(other); return *this; }
#ifdef __clang__
   template <typename Matrix2>
   typename Matrix::generic_type::template enable_if_assignable_from<Matrix2>::type&
   operator= (const GenericMatrix<Matrix2>& other) { return Matrix::generic_type::operator=(other); }
#else
   using Matrix::generic_type::operator=;
#endif

   /// Exchange the contents of two matrices in a most efficient way. 
   /// If at least one non-persistent object is involved, the operands must have equal dimensions. 
   void swap(Matrix& m) { this->data.swap(m.data); }

   friend void relocate(Matrix* from, Matrix* to)
   {
      relocate(&from->data, &to->data);
   }

   /// Resize to new dimensions, added elements initialized with default constructor.
   void resize(int r, int c)
   {
      const int dimc=cols(), dimr=rows();
      if (c==dimc) {
         this->data.resize(r*c);
         this->data.get_prefix().dimr=r;
      } else if (c<dimc && r<=dimr) {
         *this=this->minor(sequence(0,r),sequence(0,c));
      } else {
         Matrix M(r,c);
         if (c<dimc)
            M.minor(sequence(0,dimr),All)=this->minor(All,sequence(0,c));
         else
            M.minor(sequence(0,std::min(dimr,r)), sequence(0,dimc))=this->minor(sequence(0,std::min(dimr,r)),All);
         *this=M;
      }
   }

   template <typename E2>
   typename enable_if<void, convertible_to<E2, E>::value>::type
   assign(int r, int c, const E2& x)
   {
      this->data.assign(r*c, constant(x).begin());
      this->data.get_prefix()=dim_t(r,c);
   }

   /// Truncate to 0x0 matrix.
   void clear() { this->data.clear(); }
   void clear(int r, int c)
   {
      this->data.resize(r*c);
      this->data.get_prefix()=dim_t(r,c);
   }

   /// the number of rows of the matrix
   int rows() const { return this->data.get_prefix().dimr; }

   /// the number of columns of the matrix
   int cols() const { return this->data.get_prefix().dimc; }

   reference operator() (int i, int j)
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=this->rows() || j<0 || j>=this->cols())
            throw std::runtime_error("Matrix::operator() - index out of range");
      }
      return (*this->data)[i*cols()+j];
   }
   const_reference operator() (int i, int j) const
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=this->rows() || j<0 || j>=this->cols())
            throw std::runtime_error("Matrix::operator() - index out of range");
      }
      return (*this->data)[i*cols()+j];
   }

protected:
   void assign(const GenericMatrix<Matrix>& m) { this->data=m.top().data; }

   template <typename Matrix2>
   void assign(const GenericMatrix<Matrix2>& m)
   {
      const int r=m.rows(), c=m.cols();
      this->data.assign(r*c, ensure(concat_rows(convert_to<E>(m)), (dense*)0).begin());
      this->data.get_prefix().dimr=r;
      this->data.get_prefix().dimc=c;
   }

   template <typename Operation>
   void assign_op(const Operation& op)
   {
      this->data.assign_op(op);
   }

   template <typename Source2, typename Operation>
   void assign_op(const Source2& src2, const Operation& op)
   {
      this->data.assign_op(ensure(concat_rows(src2), (dense*)0).begin(), op);
   }

   template <typename Matrix2, typename E2>
   void append_rows(const GenericMatrix<Matrix2, E2>& m)
   {
      this->data.append(concat_rows(m).dim(), ensure(concat_rows(convert_to<E>(m)), (dense*)0).begin());
      this->data.get_prefix().dimr+=m.rows();
   }

   template <typename Vector2>
   void append_row(const GenericVector<Vector2>& v)
   {
      this->data.append(v.dim(), ensure(convert_to<E>(v), (dense*)0).begin());
      this->data.get_prefix().dimr++;
   }

   template <typename Matrix2>
   void append_cols(const GenericMatrix<Matrix2>& m)
   {
      this->data.weave(m.rows()*m.cols(), this->cols(), pm::rows(convert_to<E>(m)).begin());
      this->data.get_prefix().dimc+=m.cols();
   }

   template <typename Vector2>
   void append_col(const GenericVector<Vector2>& v)
   {
      this->data.weave(v.dim(), this->cols(), ensure(convert_to<E>(v), (dense*)0).begin());
      this->data.get_prefix().dimc++;
   }

   template <typename E2>
   typename enable_if<void, convertible_to<E2, E>::value>::type
   _fill(const E2& x, True)
   {
      this->data.assign(this->data.size(), constant(x).begin());
   }

   void stretch_rows(int r)
   {
      this->data.enforce_unshared().get_prefix().dimr=r;
   }

   void stretch_cols(int c)
   {
      this->data.enforce_unshared().get_prefix().dimc=c;
   }

   friend class ConcatRows<Matrix>;
   template <typename,typename> friend class GenericMatrix;
   friend class Rows<Matrix>;
   friend class Cols<Matrix>;
   template <typename, typename> friend class RowChain;
   template <typename, typename> friend class ColChain;
};

template <typename E>
struct check_container_feature<Matrix<E>, FlatStorage> : True {};

template <typename E>
class ConcatRows< Matrix_base<E> >
   : public plain_array< ConcatRows< Matrix_base<E> >, E >
   , public GenericVector< ConcatRows< Matrix_base<E> >, E> {
protected:
   ~ConcatRows();
   Matrix_base<E>& hidden() { return *reinterpret_cast<Matrix_base<E>*>(this); }
   const Matrix_base<E>& hidden() const { return *reinterpret_cast<const Matrix_base<E>*>(this); }
public:
   int size() const { return hidden().data.size(); }
   E& front() { return **hidden().data; }
   const E& front() const { return **hidden().data; }

   ConcatRows& operator= (const ConcatRows& other) { return ConcatRows::generic_type::operator=(other); }
   using ConcatRows::generic_type::operator=;
};

template <typename E>
class ConcatRows< Matrix<E> >
   : public ConcatRows< Matrix_base<E> > {
public:
   using ConcatRows::generic_type::operator=;
};

template <bool rowwise, typename BaseRef>
class matrix_line_factory {
public:
   typedef BaseRef first_argument_type;
   typedef int second_argument_type;
   typedef IndexedSlice<masquerade<ConcatRows, BaseRef>, Series<int,rowwise> > result_type;

   result_type operator() (BaseRef matrix, int start) const
   {
      const typename deref<BaseRef>::type::dim_t& dims=matrix.data.get_prefix();
      return result_type(matrix, Series<int,rowwise>(start, rowwise ? dims.dimc : dims.dimr, rowwise ? 1 : dims.dimc));
   }
};

template <bool rowwise>
class matrix_line_factory<rowwise, void> : public operations::incomplete {};

template <bool rowwise, typename BaseRef>
struct operation_cross_const_helper< matrix_line_factory<rowwise, BaseRef> > {
   typedef matrix_line_factory<rowwise, typename attrib<BaseRef>::minus_const> operation;
   typedef matrix_line_factory<rowwise, typename attrib<BaseRef>::plus_const> const_operation;
};

template <bool rowwise, typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder< matrix_line_factory<rowwise>, Iterator1, Iterator2, Reference1, Reference2>
   : empty_op_builder< matrix_line_factory<rowwise, Reference1> > {};

template <typename E>
class Rows< Matrix<E> >
   : public modified_container_pair_impl< Rows< Matrix<E> >,
                                          list( Container1< constant_value_container< Matrix_base<E>& > >,
                                                Container2< series >,
                                                Operation< matrix_line_factory<true> >,
                                                MasqueradedTop ) > {
protected:
   ~Rows();
public:
   constant_value_container< Matrix_base<E>& > get_container1()
   {
      return this->hidden();
   }
   const constant_value_container< const Matrix_base<E>& > get_container1() const
   {
      return this->hidden();
   }
   series get_container2() const
   {
      const Matrix<E>& me=this->hidden();
      return series(0, me.rows(), std::max(me.cols(), 1));  // Matrix (R x 0) which can occur in a block matrix should not look like an empty container
   }
   void resize(int n)
   {
      Matrix<E>& me=this->hidden();
      me.resize(n, me.cols());
   }
};

template <typename E>
class Cols< Matrix<E> >
   : public modified_container_pair_impl< Cols< Matrix<E> >,
                                          list( Container1< constant_value_container< Matrix_base<E>& > >,
                                                Container2< sequence >,
                                                Operation< matrix_line_factory<false> >,
                                                MasqueradedTop ) > {
protected:
   ~Cols();
public:
   constant_value_container< Matrix_base<E>& > get_container1()
   {
      return this->hidden();
   }
   const constant_value_container< const Matrix_base<E>& > get_container1() const
   {
      return this->hidden();
   }
   sequence get_container2() const
   {
      return sequence(0, this->hidden().cols());
   }
   void resize(int n)
   {
      Matrix<E>& me=this->hidden();
      me.resize(me.rows(), n);
   }
};

template <typename MatrixTop, typename E, typename Permutation> inline
typename enable_if<Matrix<E>, (MatrixTop::is_nonsymmetric && !MatrixTop::is_sparse)>::type
permuted_rows(const GenericMatrix<MatrixTop,E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_rows - dimension mismatch");
   }
   return Matrix<E>(m.rows(), m.cols(), select(rows(m),perm).begin());
}

template <typename MatrixTop, typename E, typename Permutation> inline
typename enable_if<Matrix<E>, (MatrixTop::is_nonsymmetric && !MatrixTop::is_sparse)>::type
permuted_cols(const GenericMatrix<MatrixTop,E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value) {
      if (m.cols() != perm.size())
         throw std::runtime_error("permuted_cols - dimension mismatch");
   }
   Matrix<E> result(m.rows(), m.cols());
   copy(entire(select(cols(m),perm)), cols(result).begin());
   return result;
}

template <typename MatrixTop, typename E, typename Permutation> inline
typename enable_if<Matrix<E>, (MatrixTop::is_nonsymmetric && !MatrixTop::is_sparse)>::type
permuted_inv_rows(const GenericMatrix<MatrixTop,E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_inv_rows - dimension mismatch");
   }
   Matrix<E> result(m.rows(), m.cols());
   copy(entire(rows(m)), select(rows(result), perm).begin());
   return result;
}

template <typename MatrixTop, typename E, typename Permutation> inline
typename enable_if<Matrix<E>, (MatrixTop::is_nonsymmetric && !MatrixTop::is_sparse)>::type
permuted_inv_cols(const GenericMatrix<MatrixTop,E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value) {
      if (m.cols() != perm.size())
         throw std::runtime_error("permuted_inv_cols - dimension mismatch");
   }
   Matrix<E> result(m.rows(), m.cols());
   copy(entire(cols(m)), select(cols(result), perm).begin());
   return result;
}

} // end namespace pm

namespace polymake {
   using pm::Matrix;
}

namespace std {

   /// Exchange the contents of two matrices in a most efficient way. 
   /// If at least one non-persistent object is involved, the operands must have equal dimensions. 
   template <typename E> inline
   void swap(pm::Matrix<E>& m1, pm::Matrix<E>& m2) { m1.swap(m2); }
}

#endif // POLYMAKE_MATRIX_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
