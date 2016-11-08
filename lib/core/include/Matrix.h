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

      dim_t(int r, int c) : dimr(r), dimc(c) {
         if (POLYMAKE_DEBUG) {
            if (dimr<0 || dimc<0)
               throw std::runtime_error("Matrix_base::dim_t out of range");
         }
      }
   };

   typedef shared_array<E, PrefixDataTag<dim_t>, AliasHandlerTag<shared_alias_handler>> shared_array_type;
   shared_array_type data;

   friend Matrix_base& make_mutable_alias(Matrix_base& alias, Matrix_base& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }
   template <bool, typename> friend class matrix_line_factory;

   Matrix_base() {}

   Matrix_base(int r, int c)
      : data(dim_t(r,c), r*c) {}

   template <typename... TArgs>
   Matrix_base(int r, int c, TArgs&&... args)
      : data(dim_t(r,c), r*c, std::forward<TArgs>(args)...) {}

   Matrix_base(const shared_array_placement& place, int r, int c)
      : data(place, dim_t(r,c), r*c) {}

   template <typename... TArgs>
   Matrix_base(const shared_array_placement& place, int r, int c, TArgs&&... args)
      : data(place, dim_t(r,c), r*c, std::forward<TArgs>(args)...) {}

   friend class ConcatRows<Matrix_base>;
   template <typename, int> friend class alias;
};

/**
   @class Matrix
   @brief \ref matrix_sec "Matrix type" class which holds the elements in a contiguous array <br>
   Additional arithmetic operations for matrices and useful constructions (<code>unit_matrix, diag, ...</code>) are listed at @ref genericMatrices "operations".
*/

template <typename E>
class Matrix
   : public GenericMatrix<Matrix<E>, E>
   , protected Matrix_base<E>
{
   typedef Matrix_base<E> base_t;
   using typename base_t::dim_t;

   friend Matrix& make_mutable_alias(Matrix& alias, Matrix& owner)
   {
      return static_cast<Matrix&>(make_mutable_alias(static_cast<base_t&>(alias), static_cast<base_t&>(owner)));;
   }

   template <typename Iterator>
   struct fits_as_input_iterator
      : bool_constant<construct_cascaded_iterator<Iterator, E, dense>::depth != 0> {};

public:
   using typename GenericMatrix<Matrix>::generic_type;

   typedef E value_type;
   typedef E& reference;
   typedef const E& const_reference;

   /// create as empty
   Matrix() {}

   /// create matrix with r rows and c columns, initialize all elements to 0
   Matrix(int r, int c)
      : base_t(r,c) {}

   template <typename E2,
             typename=typename std::enable_if<can_initialize<E2, E>::value>::type>
   Matrix(std::initializer_list<std::initializer_list<E2>> l)
      : base_t(l.size(), count_columns(l), entire(cascade(l))) {}

   /// Create a matrix with given dimensions.  Elements are initialized from one or more input sequences.
   /// Elements are assumed to come in the row order.
   template <typename... Iterator, typename=typename std::enable_if<mlist_and_nonempty<fits_as_input_iterator<Iterator>...>::value>::type>
   Matrix(int r, int c, Iterator&&... src)
      : base_t(r, c, ensure_private_mutable(construct_cascaded_iterator<Iterator, E, dense>()(std::forward<Iterator>(src)))...) {}

   /// Create a matrix with given dimensions.  Elements are moved from one or more input sequences.
   template <typename... Iterator, typename=typename std::enable_if<mlist_and_nonempty<fits_as_input_iterator<Iterator>...>::value>::type>
   Matrix(int r, int c, polymake::operations::move, Iterator&&... src)
      : base_t(r, c, enforce_movable_values(construct_cascaded_iterator<Iterator, E, dense>()(std::forward<Iterator>(src)))...) {}

   /// Copy of a disguised Matrix object.
   Matrix(const GenericMatrix<Matrix>& m)
      : base_t(m.top()) {}

   /// Copy of an abstract matrix of the same element type.
   template <typename Matrix2>
   Matrix(const GenericMatrix<Matrix2, E>& m)
      : base_t(m.rows(), m.cols(), ensure(concat_rows(m), (dense*)0).begin()) {}

   /// Copy of an abstract matrix with element conversion.
   template <typename Matrix2, typename E2>
   explicit Matrix(const GenericMatrix<Matrix2, E2>& m,
                   typename std::enable_if<can_initialize<E2, E>::value, void**>::type=nullptr)
      : base_t(m.rows(), m.cols(), ensure(concat_rows(m), (dense*)0).begin()) {}

   template <typename Container>
   explicit Matrix(const Container& src,
                   typename std::enable_if<construct_cascaded_iterator<typename Container::const_iterator, E, dense>::depth==2, void**>::type=nullptr)
      : base_t(src.size(), src.empty() ? 0 : get_dim(src.front()),
               construct_cascaded_iterator<typename Container::const_iterator, E, dense>()(entire(src))) {}

protected:
   Matrix(const shared_array_placement& place, int r, int c)
      : base_t(place,r,c) {}

   template <typename Iterator>
   Matrix(const shared_array_placement& place, int r, int c, Iterator&& src)
      : base_t(place, r, c, construct_cascaded_iterator<Iterator, E, dense>()(std::forward<Iterator>(src))) {}

   void resize(const shared_array_placement& place, int r, int c)
   {
      this->data.resize(place, r*c);
      this->data.get_prefix().dimr=r;
      this->data.get_prefix().dimc=c;
   }
public:

   Matrix& operator= (const Matrix& other) { assign(other); return *this; }
   using generic_type::operator=;

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
   typename std::enable_if<can_initialize<E2, E>::value, void>::type
   assign(int r, int c, const E2& x)
   {
      this->data.assign(r*c, x);
      this->data.get_prefix()=dim_t(r,c);
   }

   /// Truncate to 0x0 matrix.
   void clear() { this->data.clear(); }

   void clear(int r, int c)
   {
      this->data.resize(r*c);
      this->data.enforce_unshared().get_prefix()=dim_t(r,c);
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
      this->data.assign(r*c, ensure(concat_rows(m), (dense*)0).begin());
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

   // TODO: provide moving version of the following 4 methods
   template <typename Matrix2, typename E2>
   void append_rows(const GenericMatrix<Matrix2, E2>& m)
   {
      this->data.append(concat_rows(m).dim(), ensure(concat_rows(m), (dense*)0).begin());
      this->data.get_prefix().dimr+=m.rows();
   }

   template <typename Vector2>
   void append_row(const GenericVector<Vector2>& v)
   {
      this->data.append(v.dim(), ensure(v.top(), (dense*)0).begin());
      this->data.get_prefix().dimr++;
   }

   template <typename Matrix2>
   void append_cols(const GenericMatrix<Matrix2>& m)
   {
      this->data.weave(m.rows()*m.cols(), this->cols(), pm::rows(m).begin());
      this->data.get_prefix().dimc+=m.cols();
   }

   template <typename Vector2>
   void append_col(const GenericVector<Vector2>& v)
   {
      append_cols(vector2col(v));
   }

   template <typename E2>
   void fill_impl(const E2& x, std::true_type)
   {
      this->data.assign(this->data.size(), x);
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
struct check_container_feature<Matrix<E>, FlatStorage> : std::true_type {};

template <typename E>
class ConcatRows< Matrix_base<E> >
   : public plain_array< ConcatRows< Matrix_base<E> >, E >
   , public GenericVector< ConcatRows< Matrix_base<E> >, E> {
protected:
   ~ConcatRows();
   Matrix_base<E>& hidden() { return *reinterpret_cast<Matrix_base<E>*>(this); }
   const Matrix_base<E>& hidden() const { return *reinterpret_cast<const Matrix_base<E>*>(this); }

   friend class plain_array< ConcatRows< Matrix_base<E> >, E >;

   E* get_data() { return *hidden().data; }
   const E* get_data() const { return *hidden().data; }
public:
   int size() const { return hidden().data.size(); }

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
                                          mlist< Container1Tag< constant_value_container< Matrix_base<E>& > >,
                                                 Container2Tag< series >,
                                                 OperationTag< matrix_line_factory<true> >,
                                                 MasqueradedTop > > {
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
                                          mlist< Container1Tag< constant_value_container< Matrix_base<E>& > >,
                                                 Container2Tag< sequence >,
                                                 OperationTag< matrix_line_factory<false> >,
                                                 MasqueradedTop > > {
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

template <typename TMatrix, typename E, typename Permutation> inline
typename std::enable_if<TMatrix::is_nonsymmetric && !TMatrix::is_sparse, Matrix<E>>::type
permuted_rows(const GenericMatrix<TMatrix, E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<TMatrix>::value) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_rows - dimension mismatch");
   }
   return Matrix<E>(m.rows(), m.cols(), select(rows(m),perm).begin());
}

template <typename TMatrix, typename E, typename Permutation> inline
typename std::enable_if<TMatrix::is_nonsymmetric && !TMatrix::is_sparse, Matrix<E>>::type
permuted_cols(const GenericMatrix<TMatrix, E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<TMatrix>::value) {
      if (m.cols() != perm.size())
         throw std::runtime_error("permuted_cols - dimension mismatch");
   }
   Matrix<E> result(m.rows(), m.cols());
   copy_range(entire(select(cols(m),perm)), cols(result).begin());
   return result;
}

template <typename TMatrix, typename E, typename Permutation> inline
typename std::enable_if<TMatrix::is_nonsymmetric && !TMatrix::is_sparse, Matrix<E>>::type
permuted_inv_rows(const GenericMatrix<TMatrix, E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<TMatrix>::value) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_inv_rows - dimension mismatch");
   }
   Matrix<E> result(m.rows(), m.cols());
   copy_range(entire(rows(m)), select(rows(result), perm).begin());
   return result;
}

template <typename TMatrix, typename E, typename Permutation> inline
typename std::enable_if<TMatrix::is_nonsymmetric && !TMatrix::is_sparse, Matrix<E>>::type
permuted_inv_cols(const GenericMatrix<TMatrix, E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<TMatrix>::value) {
      if (m.cols() != perm.size())
         throw std::runtime_error("permuted_inv_cols - dimension mismatch");
   }
   Matrix<E> result(m.rows(), m.cols());
   copy_range(entire(cols(m)), select(cols(result), perm).begin());
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
