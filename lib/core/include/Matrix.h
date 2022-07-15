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
/** @file Matrix.h
    @brief Implementation of pm::Matrix class
 */


#include "polymake/GenericMatrix.h"
#include "polymake/internal/shared_object.h"

namespace pm {

template <bool rowwise, typename BaseRef=void> class matrix_line_factory;

template <typename E>
class Matrix_base {
protected:
   struct dim_t {
      Int dimr, dimc;

      dim_t() : dimr(0), dimc(0) {}

      dim_t(Int r, Int c) : dimr(r), dimc(c) {
         if (POLYMAKE_DEBUG) {
            if (dimr < 0 || dimc < 0)
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

   Matrix_base(Int r, Int c)
      : data(dim_t(r, c), r*c) {}

   template <typename... TArgs>
   Matrix_base(Int r, Int c, TArgs&&... args)
      : data(dim_t(r, c), r*c, std::forward<TArgs>(args)...) {}

   Matrix_base(const shared_array_placement& place, Int r, Int c)
      : data(place, dim_t(r, c), r*c) {}

   template <typename... TArgs>
   Matrix_base(const shared_array_placement& place, Int r, Int c, TArgs&&... args)
      : data(place, dim_t(r, c), r*c, std::forward<TArgs>(args)...) {}

   friend class ConcatRows<Matrix_base>;
   template <typename, alias_kind> friend class alias;
};

/**
   @class Matrix
   @brief \ref matrix_sec "Matrix type" class which holds the elements in a contiguous array <br>
   Additional arithmetic operations for matrices and useful constructions (<code>unit_matrix, diag, ...</code>) are listed at @ref genericMatrices "operations".
*/

template <typename E>
class Matrix
   : public Matrix_base<E>
   , public GenericMatrix<Matrix<E>, E> {
   using base_t = Matrix_base<E>;
   using typename base_t::dim_t;

   friend Matrix& make_mutable_alias(Matrix& alias, Matrix& owner)
   {
      return static_cast<Matrix&>(make_mutable_alias(static_cast<base_t&>(alias), static_cast<base_t&>(owner)));;
   }

protected:
   template <typename Iterator>
   struct fits_as_input_iterator
      : bool_constant<(assess_iterator_value<Iterator, can_initialize, E>::value ||
                       assess_iterator_value<Iterator, can_initialize, Vector<E>>::value)> {};

   template <typename Matrix2>
   auto make_src_iterator(const Matrix2& m, std::enable_if_t<Matrix2::is_flat, void**> =nullptr)
   {
      return ensure(concat_rows(m), dense()).begin();
   }

   template <typename Matrix2>
   auto make_src_iterator(const Matrix2& m, std::enable_if_t<!Matrix2::is_flat, void**> =nullptr)
   {
      return ensure(pm::rows(m), dense()).begin();
   }

public:
   using typename GenericMatrix<Matrix>::generic_type;

   using value_type = E;
   using reference = E&;
   using const_reference = const E&;

   /// create as empty
   Matrix() {}

   /// create matrix with r rows and c columns, initialize all elements to 0
   Matrix(Int r, Int c)
      : base_t(r, c) {}

   template <typename E2,
             typename=std::enable_if_t<can_initialize<E2, E>::value>>
   Matrix(std::initializer_list<std::initializer_list<E2>> l)
      : base_t(l.size(), count_columns(l), l.begin()) {}

   /// Create a matrix with given dimensions.  Elements are initialized from one or more input sequences.
   /// Elements are assumed to come in the row order.
   template <typename... Iterator, typename=std::enable_if_t<mlist_and_nonempty<fits_as_input_iterator<Iterator>...>::value>>
   Matrix(Int r, Int c, Iterator&&... src)
      : base_t(r, c, ensure_private_mutable(std::forward<Iterator>(src))...) {}

   /// Create a matrix with given dimensions.  Elements are moved from one or more input sequences.
   template <typename... Iterator, typename=std::enable_if_t<mlist_and_nonempty<fits_as_input_iterator<Iterator>...>::value>>
   Matrix(Int r, Int c, polymake::operations::move, Iterator&&... src)
      : base_t(r, c, polymake::operations::move(), std::forward<Iterator>(src)...) {}

   /// Copy of a disguised Matrix object.
   Matrix(const GenericMatrix<Matrix>& m)
      : base_t(m.top()) {}

   /// Copy of an abstract matrix of the same element type.
   template <typename Matrix2>
   Matrix(const GenericMatrix<Matrix2, E>& m)
      : base_t(m.rows(), m.cols(), make_src_iterator(m.top())) {}

   /// Copy of an abstract matrix with element conversion.
   template <typename Matrix2, typename E2>
   explicit Matrix(const GenericMatrix<Matrix2, E2>& m,
                   std::enable_if_t<can_initialize<E2, E>::value, void**> =nullptr)
      : base_t(m.rows(), m.cols(), make_src_iterator(m.top())) {}

   template <typename Container>
   explicit Matrix(const Container& src,
                   std::enable_if_t<isomorphic_to_container_of<Container, Vector<E>>::value, void**> =nullptr)
      : base_t(src.size(), src.empty() ? 0 : get_dim(src.front()), src.begin()) {}

protected:
   Matrix(const shared_array_placement& place, Int r, Int c)
      : base_t(place, r, c) {}

   template <typename Iterator>
   Matrix(const shared_array_placement& place, Int r, Int c, Iterator&& src)
      : base_t(place, r, c, std::forward<Iterator>(src)) {}

   void resize(const shared_array_placement& place, Int r, Int c)
   {
      this->data.resize(place, r*c);
      this->data.get_prefix().dimr = r;
      this->data.get_prefix().dimc = c;
   }
public:

   Matrix& operator= (const Matrix& other) { assign(other); return *this; }
   using generic_type::operator=;

   /// Exchange the contents of two matrices in a most efficient way.
   void swap(Matrix& m) { this->data.swap(m.data); }

   friend void relocate(Matrix* from, Matrix* to)
   {
      relocate(&from->data, &to->data);
   }

   /// Resize to new dimensions, added elements initialized with default constructor.
   void resize(Int r, Int c)
   {
      const Int dimc = cols(), dimr = rows();
      if (c == dimc) {
         this->data.resize(r*c);
         this->data.get_prefix().dimr = r;
      } else if (c < dimc && r <= dimr) {
         *this = this->minor(sequence(0, r), sequence(0, c));
      } else {
         Matrix M(r, c);
         if (c < dimc)
            M.minor(sequence(0, dimr), All) = this->minor(All, sequence(0, c));
         else
            M.minor(sequence(0, std::min(dimr, r)), sequence(0, dimc)) = this->minor(sequence(0, std::min(dimr, r)), All);
         *this = M;
      }
   }

   template <typename E2>
   std::enable_if_t<can_initialize<E2, E>::value>
   assign(Int r, Int c, const E2& x)
   {
      this->data.assign(r*c, x);
      this->data.get_prefix() = dim_t(r, c);
   }

   /// Truncate to 0x0 matrix.
   void clear() { this->data.clear(); }

   void clear(Int r, Int c)
   {
      this->data.resize(r*c);
      this->data.enforce_unshared().get_prefix() = dim_t(r, c);
   }

   /// the number of rows of the matrix
   Int rows() const { return this->data.get_prefix().dimr; }

   /// the number of columns of the matrix
   Int cols() const { return this->data.get_prefix().dimc; }

   reference operator() (Int i, Int j)
   {
      if (POLYMAKE_DEBUG) {
         if (i < 0 || i >= rows() || j < 0 || j >= cols())
            throw std::runtime_error("Matrix::operator() - index out of range");
      }
      return (*this->data)[i*cols()+j];
   }
   const_reference operator() (Int i, Int j) const
   {
      if (POLYMAKE_DEBUG) {
         if (i < 0 || i >= rows() || j < 0 || j >= cols())
            throw std::runtime_error("Matrix::operator() - index out of range");
      }
      return (*this->data)[i*cols()+j];
   }

protected:
   void assign(const GenericMatrix<Matrix>& m) { this->data=m.top().data; }

   template <typename Matrix2>
   void assign(const GenericMatrix<Matrix2>& m)
   {
      const Int r = m.rows(), c = m.cols();
      this->data.assign(r*c, make_src_iterator(m.top()));
      this->data.get_prefix().dimr = r;
      this->data.get_prefix().dimc = c;
   }

   template <typename Operation>
   void assign_op(const Operation& op)
   {
      this->data.assign_op(op);
   }

   template <typename Source2, typename Operation>
   void assign_op(const Source2& src2, const Operation& op)
   {
      this->data.assign_op(make_src_iterator(src2), op);
   }

   // TODO: provide moving version of the following 4 methods
   template <typename Matrix2, typename E2>
   void append_rows(const GenericMatrix<Matrix2, E2>& m)
   {
      this->data.append(m.rows()*m.cols(), make_src_iterator(m.top()));
      this->data.get_prefix().dimr+=m.rows();
   }

   template <typename Vector2>
   void append_row(const GenericVector<Vector2>& v)
   {
      this->data.append(v.dim(), ensure(v.top(), dense()).begin());
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

   friend class ConcatRows<Matrix>;
   template <typename,typename> friend class GenericMatrix;
   friend class Rows<Matrix>;
   friend class Cols<Matrix>;
   template <typename, typename> friend class BlockMatrix;
};

template <typename E>
struct check_container_feature<Matrix<E>, FlatStorage> : std::true_type {};

template <typename E>
class ConcatRows< Matrix_base<E> >
   : public plain_array< ConcatRows< Matrix_base<E> >, E >
   , public GenericVector< ConcatRows< Matrix_base<E> >, E> {
protected:
   ~ConcatRows() = delete;
   Matrix_base<E>& hidden() { return *reinterpret_cast<Matrix_base<E>*>(this); }
   const Matrix_base<E>& hidden() const { return *reinterpret_cast<const Matrix_base<E>*>(this); }

   friend class plain_array< ConcatRows< Matrix_base<E> >, E >;

   E* get_data() { return *hidden().data; }
   const E* get_data() const { return *hidden().data; }
public:
   Int size() const { return hidden().data.size(); }

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
   typedef Int second_argument_type;
   typedef IndexedSlice<masquerade<ConcatRows, BaseRef>, const Series<Int, rowwise>> result_type;

   result_type operator() (BaseRef matrix, Int start) const
   {
      const auto& dims = matrix.data.get_prefix();
      return result_type(matrix, Series<Int, rowwise>(start, rowwise ? dims.dimc : dims.dimr, rowwise ? 1 : dims.dimc));
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
                                          mlist< Container1Tag< same_value_container< Matrix_base<E>& > >,
                                                 Container2Tag< series >,
                                                 OperationTag< matrix_line_factory<true> >,
                                                 MasqueradedTop > > {
protected:
   ~Rows();
public:
   auto get_container1()
   {
      return same_value_container< Matrix_base<E>& >(static_cast<Matrix_base<E>&>(this->hidden()));
   }
   auto get_container1() const
   {
      return same_value_container< const Matrix_base<E>& >(static_cast<const Matrix_base<E>&>(this->hidden()));
   }
   series get_container2() const
   {
      const Matrix<E>& me=this->hidden();
      return series(0, me.rows(), std::max(me.cols(), 1L));  // Matrix (R x 0) which can occur in a block matrix should not look like an empty container
   }
   void resize(Int n)
   {
      Matrix<E>& me = this->hidden();
      me.resize(n, me.cols());
   }
};

template <typename E>
class Cols< Matrix<E> >
   : public modified_container_pair_impl< Cols< Matrix<E> >,
                                          mlist< Container1Tag< same_value_container< Matrix_base<E>& > >,
                                                 Container2Tag< sequence >,
                                                 OperationTag< matrix_line_factory<false> >,
                                                 MasqueradedTop > > {
protected:
   ~Cols();
public:
   auto get_container1()
   {
      return same_value_container< Matrix_base<E>& >(static_cast<Matrix_base<E>&>(this->hidden()));
   }
   auto get_container1() const
   {
      return same_value_container< const Matrix_base<E>& >(static_cast<const Matrix_base<E>&>(this->hidden()));
   }
   sequence get_container2() const
   {
      return sequence(0, this->hidden().cols());
   }
   void resize(Int n)
   {
      Matrix<E>& me = this->hidden();
      me.resize(me.rows(), n);
   }
};

template <typename TMatrix, typename E, typename Permutation>
std::enable_if_t<TMatrix::is_nonsymmetric && !TMatrix::is_sparse, Matrix<E>>
permuted_rows(const GenericMatrix<TMatrix, E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_rows - dimension mismatch");
   }
   return Matrix<E>(m.rows(), m.cols(), select(rows(m),perm).begin());
}

template <typename TMatrix, typename E, typename Permutation>
std::enable_if_t<TMatrix::is_nonsymmetric && !TMatrix::is_sparse, Matrix<E>>
permuted_cols(const GenericMatrix<TMatrix, E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (m.cols() != perm.size())
         throw std::runtime_error("permuted_cols - dimension mismatch");
   }
   Matrix<E> result(m.rows(), m.cols());
   copy_range(entire(select(cols(m),perm)), cols(result).begin());
   return result;
}

template <typename TMatrix, typename E, typename Permutation>
std::enable_if_t<TMatrix::is_nonsymmetric && !TMatrix::is_sparse, Matrix<E>>
permuted_inv_rows(const GenericMatrix<TMatrix, E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_inv_rows - dimension mismatch");
   }
   Matrix<E> result(m.rows(), m.cols());
   copy_range(entire(rows(m)), select(rows(result), perm).begin());
   return result;
}

template <typename TMatrix, typename E, typename Permutation>
std::enable_if_t<TMatrix::is_nonsymmetric && !TMatrix::is_sparse, Matrix<E>>
permuted_inv_cols(const GenericMatrix<TMatrix, E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
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

template <typename E>
void swap(pm::Matrix<E>& m1, pm::Matrix<E>& m2) { m1.swap(m2); }

}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
