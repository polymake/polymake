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

#ifndef POLYMAKE_GENERIC_MATRIX_H
#define POLYMAKE_GENERIC_MATRIX_H

#include "polymake/Vector.h"
#include "polymake/CascadedContainer.h"
#include "polymake/internal/matrix_methods.h"

namespace pm {

/* ------------------------------------------
 *  Matrix masquerade: ConcatRows, ConcatCols
 * ------------------------------------------ */

template <typename Matrix>
class ConcatRows_default
   : public cascade_impl< ConcatRows_default<Matrix>,
                          list( Container< Rows<Matrix> >,
                                CascadeDepth< int2type<2> >,
                                MasqueradedTop ) > {
   typedef cascade_impl<ConcatRows_default> _super;

   int _size(False) const { return dim(); }
   int _size(True) const { return _super::size(); }
public:
   int dim() const
   {
      return this->hidden().rows() * this->hidden().cols();
   }
   int size() const
   {
      return _size(bool2type<check_container_feature<Matrix, sparse>::value>());
   }
};

template <typename Matrix>
class ConcatRows
   : public ConcatRows_default<Matrix>,
     public GenericVector< ConcatRows<Matrix>,
                           typename object_traits<typename Matrix::value_type>::persistent_type > {
protected:
   ~ConcatRows();
public:
   ConcatRows& operator= (const ConcatRows& other) { return ConcatRows::generic_type::operator=(other); }
   using ConcatRows::generic_type::operator=;
   using ConcatRows_default<Matrix>::dim;
};

template <typename Matrix>
class ConcatCols
   : public ConcatRows< Transposed<Matrix> > {};

template <typename Matrix>
struct spec_object_traits< ConcatRows<Matrix> >
   : spec_object_traits<is_container> {
   static const bool is_always_const=object_traits<Matrix>::is_always_const;
   static const int is_resizeable=0;
   typedef Matrix masquerade_for;
};

template <typename Matrix>
struct spec_object_traits< ConcatCols<Matrix> >
   : spec_object_traits< ConcatRows<Matrix> > {};

template <typename Matrix>
struct check_container_feature<ConcatRows<Matrix>, sparse>
   : check_container_feature<Matrix, sparse> {};

template <typename Matrix>
struct check_container_feature<ConcatRows<Matrix>, pure_sparse>
   : check_container_feature<Matrix, pure_sparse> {};

template <typename Matrix>
struct check_container_feature<ConcatCols<Matrix>, sparse>
   : check_container_feature<Matrix, sparse> {};

template <typename Matrix>
struct check_container_feature<ConcatCols<Matrix>, pure_sparse>
   : check_container_feature<Matrix, pure_sparse> {};

/* ----------------
 *  Generic matrix
 * ---------------- */

/** @defgroup genericMatrices Generic Matrices
 *  @{
 */
template <typename E> class Matrix;
template <typename E, typename _symmetric=NonSymmetric> class SparseMatrix;
template <typename E> class SparseMatrix2x2;
template <typename E> class constant_value_matrix;

template <typename Matrix, typename Other>
struct expandable_matrix
   : public enable_if<typename Unwary<Matrix>::type, (Matrix::is_nonsymmetric && object_traits<Matrix>::is_resizeable!=0 &&
                                                      (convertible_to<typename Other::element_type, typename Matrix::element_type>::value ||
                                                       explicitly_convertible_to<typename Other::element_type, typename Matrix::element_type>::value))> {};


/** @file GenericMatrix.h
    @class GenericMatrix
    @brief @ref generic "Generic type" for @ref matrix_sec "matrices"
 */
template <typename MatrixTop, typename E=typename MatrixTop::element_type>
class GenericMatrix
   : public Generic<MatrixTop>,
     public matrix_methods<MatrixTop, E>,
     public operators::base {
   template <typename, typename> friend class GenericMatrix;

protected:
   GenericMatrix() {}
   GenericMatrix(const GenericMatrix&) {}
#if POLYMAKE_DEBUG
   ~GenericMatrix() { POLYMAKE_DEBUG_METHOD(GenericMatrix,dump); }
   void dump() const { cerr << this->top() << std::flush; }
#endif

public:
   static const bool
      is_sparse= check_container_feature<MatrixTop, sparse>::value,
      is_symmetric=check_container_feature<MatrixTop, Symmetric>::value,
      is_skew=check_container_feature<MatrixTop, SkewSymmetric>::value,
      is_nonsymmetric=!is_symmetric && !is_skew,
      is_flat=check_container_feature<MatrixTop, FlatStorage>::value;

   typedef typename matrix_symmetry_type<MatrixTop>::type sym_discr;
   typedef typename Generic<MatrixTop>::top_type top_type;
   typedef typename if_else< is_sparse,
                             SparseMatrix<E,sym_discr>,
                             Matrix<E> >::type
      persistent_type;
   typedef typename if_else< is_sparse,
                             SparseMatrix<E>,
                             Matrix<E> >::type
      persistent_nonsymmetric_type;
   typedef GenericMatrix generic_type;
protected:
   template <typename Matrix2>
   typename enable_if<void, convertible_to<typename Matrix2::element_type, E>::value>::type
   _assign(const GenericMatrix<Matrix2>& m, True, NonSymmetric)
   {
      concat_rows(*this) = concat_rows(m);
   }

   template <typename Matrix2>
   typename enable_if<void, explicitly_convertible_to<typename Matrix2::element_type, E>::value>::type
   _assign(const GenericMatrix<Matrix2>& m, True, NonSymmetric)
   {
      concat_rows(*this) = concat_rows(convert_to<E>(m));
   }

   template <typename Matrix2>
   void _assign(const GenericMatrix<Matrix2>& m, False, NonSymmetric)
   {
      copy(pm::rows(m).begin(), entire(pm::rows(*this)));
   }

   template <typename Matrix2>
   void _assign(const GenericMatrix<Matrix2>& m, False, Symmetric)
   {
      typename Rows<Matrix2>::const_iterator src=pm::rows(m).begin();
      for (typename Entire< Rows<MatrixTop> >::iterator r_i=entire(pm::rows(*this)); !r_i.at_end(); ++r_i, ++src)
         r_i->assign(*src, sym_discr());
   }

   template <typename Matrix2>
   void assign(const GenericMatrix<Matrix2>& m)
   {
      _assign(m, bool2type<is_flat && check_container_feature<Matrix2, FlatStorage>::value>(), sym_discr());
   }

   template <typename Operation>
   void _assign_op(const Operation& op, True, NonSymmetric)
   {
      concat_rows(*this).assign_op(op);
   }

   template <typename Operation>
   void _assign_op(const Operation& op, False, NonSymmetric)
   {
      for (typename Entire< Rows<MatrixTop> >::iterator r_i=entire(pm::rows(*this)); !r_i.at_end(); ++r_i)
         r_i->assign_op(op);
   }

   template <typename Operation>
   void _assign_op(const Operation& op, False, Symmetric)
   {
      for (typename Entire< Rows<MatrixTop> >::iterator r_i=entire(pm::rows(*this)); !r_i.at_end(); ++r_i)
         r_i->assign_op(op,sym_discr());
   }

   template <typename Matrix2, typename Operation>
   void _assign_op(const GenericMatrix<Matrix2>& m, const Operation& op, True, NonSymmetric)
   {
      concat_rows(*this).assign_op(concat_rows(m), op);
   }

   template <typename Matrix2, typename Operation>
   void _assign_op(const GenericMatrix<Matrix2>& m, const Operation& op, False, NonSymmetric)
   {
      typename Rows<Matrix2>::const_iterator src=pm::rows(m).begin();
      for (typename Entire< Rows<MatrixTop> >::iterator r_i=entire(pm::rows(*this)); !r_i.at_end(); ++r_i, ++src)
         r_i->assign_op(*src,op);
   }

   template <typename Matrix2, typename Operation>
   void _assign_op(const GenericMatrix<Matrix2>& m, const Operation& op, False, Symmetric)
   {
      typename Rows<Matrix2>::const_iterator src=pm::rows(m).begin();
      for (typename Entire< Rows<MatrixTop> >::iterator r_i=entire(pm::rows(*this)); !r_i.at_end(); ++r_i, ++src)
         r_i->assign_op(*src,op,sym_discr());
   }

   template <typename E2>
   void _fill(const E2& x, True)
   {
      concat_rows(*this).fill(x);
   }

   template <typename E2>
   void _fill(const E2& x, False)
   {
      for (typename Entire< Rows<MatrixTop> >::iterator r_i=entire(pm::rows(*this)); !r_i.at_end(); ++r_i)
         r_i->fill(x);
   }

   bool non_zero(True) const
   {
      return !is_zero(concat_rows(*this));
   }

   bool non_zero(False) const
   {
      return !entire(attach_selector(pm::rows(*this), BuildUnary<operations::non_zero>())).at_end();
   }

   template <typename Matrix2>
   bool trivial_assignment(const GenericMatrix<Matrix2,E>&) const { return false; }

   bool trivial_assignment(const GenericMatrix& other) const { return this==&other; }

   template <typename Matrix2>
   struct enable_if_assignable_from
      : enable_if<top_type, (!spec_object_traits<top_type>::is_always_const &&
                             (is_nonsymmetric || identical<sym_discr, typename Matrix2::sym_discr>::value) &&
                             (convertible_to<typename Matrix2::element_type, E>::value ||
                              explicitly_convertible_to<typename Matrix2::element_type, E>::value))> {};
public:
   /**
      Persistent matrix objects have after the assignment the same dimensions as 
      the right hand side operand. Alias objects, such as matrix minor or block matrix, 
      cannot be resized, thus must have the same dimensions as on the right hand side. 
    */
   top_type& operator= (const GenericMatrix& m)
   {
      if (!trivial_assignment(m)) {
         if (!object_traits<MatrixTop>::is_resizeable && (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value)) {
            if (this->rows() != m.rows() || this->cols() != m.cols())
               throw std::runtime_error("GenericMatrix::operator= - dimension mismatch");
         }
         this->top().assign(m.top());
      }
      return this->top();
   }

   template <typename Matrix2>
   typename enable_if_assignable_from<Matrix2>::type&
   operator= (const GenericMatrix<Matrix2>& m)
   {
      if (!object_traits<MatrixTop>::is_resizeable && (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value)) {
         if (this->rows() != m.rows() || this->cols() != m.cols())
            throw std::runtime_error("GenericMatrix::operator= - dimension mismatch");
      }
      this->top().assign(m.top());
      return this->top();
   }

   /**
      Exchange the contents of two matrices in a most efficient way. 
      If at least one non-persistent object is involved, the operands must 
      have equal dimensions. 
    */
   template <typename Matrix2>
   void swap(GenericMatrix<Matrix2, E>& m)
   {
      if (trivial_assignment(m)) return;

      if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value || !Unwary<Matrix2>::value) {
         if (this->rows() != m.rows() || this->cols() != m.cols())
            throw std::runtime_error("GenericMatrix::swap - dimension mismatch");
      }
      swap_ranges(entire(pm::rows(*this)), pm::rows(m).begin());
   }

   template <typename Operation>
   void assign_op(const Operation& op)
   {
      _assign_op(op, bool2type<is_flat>(), sym_discr());
   }

   template <typename Matrix2, typename Operation>
   void assign_op(const GenericMatrix<Matrix2>& m, const Operation& op)
   {
      _assign_op(m.top(), op, bool2type<is_flat && check_container_feature<Matrix2, FlatStorage>::value>(), sym_discr());
   }

   /**
      Fill with given value without changing the dimensions. 
      x can be of arbitrary type assignable to the type E2. 
    */
   template <typename E2>
   void fill(const E2& x)
   {
      this->top()._fill(x, bool2type<is_flat>());
   }

   top_type& negate()
   {
      this->top().assign_op(BuildUnary<operations::neg>());
      return this->top();
   }

   /// adding a GenericMatrix
   template <typename Matrix2>
   typename enable_if<top_type, (is_nonsymmetric || identical<sym_discr, typename Matrix2::sym_discr>::value)>::type&
   operator+= (const GenericMatrix<Matrix2>& m)
   {
      if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value || !Unwary<Matrix2>::value) {
         if (this->rows() != m.rows() || this->cols() != m.cols())
            throw std::runtime_error("GenericMatrix::operator+= - dimension mismatch");
      }
      this->top().assign_op(m.top(), BuildBinary<operations::add>());
      return this->top();
   }

   /// subtracting a GenericMatrix
   template <typename Matrix2>
   typename enable_if<top_type, (is_nonsymmetric || identical<sym_discr, typename Matrix2::sym_discr>::value)>::type&
   operator-= (const GenericMatrix<Matrix2,E>& m)
   {
      if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value || !Unwary<Matrix2>::value) {
         if (this->rows() != m.rows() || this->cols() != m.cols())
            throw std::runtime_error("GenericMatrix::operator-= - dimension mismatch");
      }
      this->top().assign_op(m.top(), BuildBinary<operations::sub>());
      return this->top();
   }

   /// multiply with an element
   template <typename Right>
   top_type& operator*= (const Right& r)
   {
      if (!is_zero(r))
         this->top().assign_op(constant_value_matrix<const Right&>(r), BuildBinary<operations::mul>());
      else
         fill(r);
      return this->top();
   }

   template <typename Right>
   typename enable_if<top_type, isomorphic_types<E,Right>::value>::type&
   operator/= (const Right& r)
   {
      this->top().assign_op(constant_value_matrix<const Right&>(r), BuildBinary<operations::div>());
      return this->top();
   }

   /// divide by an element 
   template <typename Right>
   typename enable_if<top_type, isomorphic_types<E,Right>::value>::type&
   div_exact(const Right& r)
   {
      this->top().assign_op(constant_value_matrix<const Right&>(r), BuildBinary<operations::divexact>());
      return this->top();
   }

   /// append the rows of a GenericMatrix
   template <typename Matrix2>
   typename expandable_matrix<MatrixTop, Matrix2>::type&
   operator/= (const GenericMatrix<Matrix2>& m)
   {
      if (m.rows()) {
         if (this->rows()) {
            if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value || !Unwary<Matrix2>::value) {
               if (this->cols() != m.cols())
                  throw std::runtime_error("GenericMatrix::operator/= - dimension mismatch");
            }
            this->top().append_rows(m.top());
         } else {
            this->top().assign(m.top());
         }
      }
      return this->top();
   }

   /// append a GenericVector as a row
   template <typename Vector2>
   typename expandable_matrix<MatrixTop, Vector2>::type&
   operator/= (const GenericVector<Vector2>& v)
   {
      if (this->rows()) {
         if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value || !Unwary<Vector2>::value) {
            if (this->cols() != v.dim())
               throw std::runtime_error("GenericMatrix::operator/= - dimension mismatch");
         }
         this->top().append_row(v.top());
      } else {
         this->top().assign(vector2row(v));
      }
      return this->top();
   }

   /// append the columns of a GenericMatrix
   template <typename Matrix2>
   typename expandable_matrix<MatrixTop, Matrix2>::type&
   operator|= (const GenericMatrix<Matrix2>& m)
   {
      if (m.cols()) {
         if (this->cols()) {
            if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value || !Unwary<Matrix2>::value) {
               if (this->rows() != m.rows())
                  throw std::runtime_error("GenericMatrix::operator|= - dimension mismatch");
            }
            this->top().append_cols(m.top());
         } else {
            this->top().assign(m.top());
         }
      }
      return this->top();
   }

   /// append a GenericVector as a column
   template <typename Vector2>
   typename expandable_matrix<MatrixTop, Vector2>::type&
   operator|= (const GenericVector<Vector2>& v)
   {
      if (this->cols()) {
         if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value || !Unwary<Vector2>::value) {
            if (this->rows() != v.dim())
               throw std::runtime_error("GenericMatrix::operator|= - dimension mismatch");
         }
         this->top().append_col(v.top());
      } else {
         this->top().assign(vector2col(v));
      }
      return this->top();
   }

   bool non_zero() const
   {
      return non_zero(bool2type<is_flat>());
   }

   /// @param i==0: main diagonal; i>0: i-th diagonal below the main; i<0: (-i)-th above the main
   IndexedSlice<ConcatRows<typename Unwary<MatrixTop>::type>&, series>
   diagonal(int i=0)
   {
      const int r=this->rows(), c=this->cols();
      if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value) {
         if (i>0 ? i>=r : -i >= c)
            throw std::runtime_error("GenericMatrix::diagonal - index out of range");
      }
      const int start= i>0 ? i*c : -i,
                 size= i>0 ? std::min(r-i, c) : std::min(r, c+i);
      return IndexedSlice<ConcatRows<typename Unwary<MatrixTop>::type>&, series> (concat_rows(*this), series(start,size,c+1));
   }

   const IndexedSlice<const ConcatRows<typename Unwary<MatrixTop>::type>&, series>
   diagonal(int i=0) const
   {
      const int r=this->rows(), c=this->cols();
      if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value) {
         if (i>0 ? i>=r : -i >= c)
            throw std::runtime_error("GenericMatrix::diagonal - index out of range");
      }
      const int start= i>0 ? i*c : -i,
                 size= i>0 ? std::min(r-i, c) : std::min(r, c+i);
      return IndexedSlice<const ConcatRows<typename Unwary<MatrixTop>::type>&, series> (concat_rows(*this), series(start,size,c+1));
   }

   /// @param i==0: main anti-diagonal; i>0: i-th diagonal below the main; i<0: (-i)-th above the main
   IndexedSlice<ConcatRows<typename Unwary<MatrixTop>::type>&, series>
   anti_diagonal(int i=0)
   {
      const int r=this->rows(), c=this->cols();
      if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value) {
         if (i>0 ? i>=r : -i >= c)
            throw std::runtime_error("GenericMatrix::anti_diagonal - index out of range");
      }
      const int start= i>0 ? (i+1)*c-1 : c+i-1,
                 size= i>0 ? std::min(r-i, c) : std::min(r, c+i);
      return IndexedSlice<ConcatRows<typename Unwary<MatrixTop>::type>&, series> (concat_rows(*this), series(start,size,c-1));
   }

   const IndexedSlice<const ConcatRows<typename Unwary<MatrixTop>::type>&, series>
   anti_diagonal(int i=0) const
   {
      const int r=this->rows(), c=this->cols();
      if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value) {
         if (i>0 ? i>=r : -i >= c)
            throw std::runtime_error("GenericMatrix::anti_diagonal - index out of range");
      }
      const int start= i>0 ? (i+1)*c-1 : c+i-1,
                 size= i>0 ? std::min(r-i, c) : std::min(r, c+i);
      return IndexedSlice<const ConcatRows<typename Unwary<MatrixTop>::type>&, series> (concat_rows(*this), series(start,size,c-1));
   }

protected:
   template <typename Line, typename E2>
   static void _multiply(Line& l_i, Line& l_j,
                         const E2& a_ii, const E2& a_ij, const E2& a_ji, const E2& a_jj, False)
   {
      typename Line::iterator e_j=l_j.begin();
      for (typename Entire<Line>::iterator e_i=entire(l_i); !e_i.at_end(); ++e_i, ++e_j) {
         const E x_i= (*e_i)*a_ii + (*e_j)*a_ij;
         *e_j =       (*e_i)*a_ji + (*e_j)*a_jj;
         *e_i =  x_i;
      }
   }

   template <typename Line, typename E2>
   static void _multiply(Line& l_i, Line& l_j,
                         const E2& a_ii, const E2& a_ij, const E2& a_ji, const E2& a_jj, True)
   {
      typename Line::iterator e_i=l_i.begin(), e_j=l_j.begin();
      int state=zipper_both;
      if (e_i.at_end()) state>>=3;
      if (e_j.at_end()) state>>=6;
      while (state) {
         if (state >= zipper_both) {
            state &= ~zipper_cmp;
            state += 1 << sign(e_i.index() - e_j.index())+1;
         }
         if (state & zipper_lt) {
            if (!is_zero(a_ji))
               l_j.insert(e_j, e_i.index(), (*e_i)*a_ji);
            if (!is_zero(a_ii))
               *e_i++ *= a_ii;
            else
               l_i.erase(e_i++);
            if (e_i.at_end())
               state>>=3;
         } else if (state & zipper_gt) {
            if (!is_zero(a_ij))
               l_i.insert(e_i,e_j.index(),(*e_j)*a_ij);
            if (!is_zero(a_jj))
               *e_j++ *= a_jj;
            else
               l_j.erase(e_j++);
            if (e_j.at_end())
               state>>=6;
         } else {
            const E x_i= (*e_i)*a_ii + (*e_j)*a_ij;
                  *e_j = (*e_i)*a_ji + (*e_j)*a_jj;
            if (!is_zero(x_i))
               *e_i++ = x_i;
            else
               l_i.erase(e_i++);
            if (e_i.at_end())
               state>>=3;
            if (!is_zero(*e_j))
               ++e_j;
            else
               l_j.erase(e_j++);
            if (e_j.at_end())
               state>>=6;
         }
      }
   }

public:
   void multiply_from_left(const SparseMatrix2x2<E>& U)
   {
      if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value) {
         if (U.i<0 || U.j<0 || U.i >= this->rows() || U.j >= this->rows())
            throw std::runtime_error("GenericMatrix::multiply_from_left - dimension mismatch");
      }
      _multiply(this->row(U.i).top(), this->row(U.j).top(),
                U.a_ii, U.a_ij, U.a_ji, U.a_jj, bool2type<is_sparse>());
   }

   void multiply_from_left(const Transposed< SparseMatrix2x2<E> >& U)
   {
      if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value) {
         if (U.i<0 || U.j<0 || U.i >= this->rows() || U.j >= this->rows())
            throw std::runtime_error("GenericMatrix::multiply_from_left - dimension mismatch");
      }
      _multiply(this->row(U.i).top(), this->row(U.j).top(),
                U.a_ii, U.a_ji, U.a_ij, U.a_jj, bool2type<is_sparse>());
   }

   void multiply_from_right(const SparseMatrix2x2<E>& U)
   {
      if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value) {
         if (U.i<0 || U.j<0 || U.i >= this->cols() || U.j >= this->cols())
            throw std::runtime_error("GenericMatrix::multiply_from_right - dimension mismatch");
      }
      _multiply(this->col(U.i).top(), this->col(U.j).top(),
                U.a_ii, U.a_ji, U.a_ij, U.a_jj, bool2type<is_sparse>());
   }

   void multiply_from_right(const Transposed< SparseMatrix2x2<E> >& U)
   {
      if (POLYMAKE_DEBUG || !Unwary<MatrixTop>::value) {
         if (U.i<0 || U.j<0 || U.i >= this->cols() || U.j >= this->cols())
            throw std::runtime_error("GenericMatrix::multiply_from_right - dimension mismatch");
      }
      _multiply(this->col(U.i).top(), this->col(U.j).top(),
                U.a_ii, U.a_ij, U.a_ji, U.a_jj, bool2type<is_sparse>());
   }

   template <typename Result>
   struct rebind_generic {
      typedef GenericMatrix<Result, E> type;
   };
};

struct is_matrix;

template <typename Matrix, typename E>
struct spec_object_traits< GenericMatrix<Matrix, E> >
   : spec_or_model_traits<Matrix,is_container> {
   static const int dimension=2, is_resizeable= spec_or_model_traits<Matrix,is_container>::is_resizeable ? 2 : 0;
   static const bool allow_sparse=true;
   typedef is_matrix generic_tag;

   static bool is_zero(const Matrix& m) { return !m.non_zero(); }
};

template <typename Matrix> inline
ConcatRows<typename Unwary<Matrix>::type>&
concat_rows(GenericMatrix<Matrix>& m)
{
   return reinterpret_cast<ConcatRows<typename Unwary<Matrix>::type>&>(m.top());
}

template <typename Matrix> inline
const ConcatRows<typename Unwary<Matrix>::type>&
concat_rows(const GenericMatrix<Matrix>& m)
{
   return reinterpret_cast<const ConcatRows<typename Unwary<Matrix>::type>&>(m.top());
}

template <typename Matrix> inline
ConcatCols<typename Unwary<Matrix>::type>&
concat_cols(GenericMatrix<Matrix>& m)
{
   return reinterpret_cast<ConcatCols<typename Unwary<Matrix>::type>&>(m.top());
}

template <typename Matrix> inline
const ConcatCols<typename Unwary<Matrix>::type>&
concat_cols(const GenericMatrix<Matrix>& m)
{
   return reinterpret_cast<const ConcatCols<typename Unwary<Matrix>::type>&>(m.top());
}

/* --------------------------------------------
 *  LazyMatrix1
 * lazy evaluation of an unary matrix operator
 * -------------------------------------------- */

template <typename MatrixRef, typename Operation>
struct lazy1_traits {
   typedef unary_op_builder<Operation, void, typename deref<MatrixRef>::type::const_reference>
      op_builder;
   typedef typename op_builder::operation::result_type reference;
   typedef typename deref<reference>::type value_type;
   typedef typename object_traits<value_type>::persistent_type element_type;
};

template <typename MatrixRef, typename Operation>
class LazyMatrix1
   : public modified_container_base<MatrixRef, Operation>,
     public GenericMatrix< LazyMatrix1<MatrixRef,Operation>,
                           typename lazy1_traits<MatrixRef,Operation>::element_type > {
   typedef modified_container_base<MatrixRef, Operation> _base;
public:
   typedef typename lazy1_traits<MatrixRef,Operation>::value_type value_type;
   typedef typename lazy1_traits<MatrixRef,Operation>::reference reference;
   typedef reference const_reference;
   typedef typename deref<MatrixRef>::type matrix_type;

   LazyMatrix1(typename _base::arg_type src_arg, const Operation& op_arg=Operation())
      : _base(src_arg, op_arg) {}
};

template <typename MatrixRef, typename Operation>
struct spec_object_traits< LazyMatrix1<MatrixRef, Operation> >
   : spec_object_traits<is_container> {
   static const bool is_lazy=true, is_temporary=true, is_always_const=true;
};

template <typename MatrixRef, typename Operation>
struct check_container_feature<LazyMatrix1<MatrixRef, Operation>, sparse>
   : check_container_ref_feature<MatrixRef, sparse> {};

template <typename MatrixRef, typename Operation>
struct check_container_feature<LazyMatrix1<MatrixRef, Operation>, pure_sparse>
   : check_container_ref_feature<MatrixRef, pure_sparse> {};

template <typename MatrixRef, typename Operation>
struct check_container_feature<LazyMatrix1<MatrixRef, Operation>, Symmetric>
   : check_container_ref_feature<MatrixRef, Symmetric> {};

template <typename MatrixRef, typename Operation>
struct check_container_feature<LazyMatrix1<MatrixRef, Operation>, SkewSymmetric>
   : check_container_ref_feature<MatrixRef, SkewSymmetric> {};

template <typename MatrixRef, typename Operation>
struct check_container_feature<LazyMatrix1<MatrixRef, Operation>, FlatStorage>
   : check_container_ref_feature<MatrixRef, FlatStorage> {};

template <typename MatrixRef, typename Operation>
class matrix_random_access_methods< LazyMatrix1<MatrixRef, Operation> > {
   typedef LazyMatrix1<MatrixRef,Operation> master;
public:
   typename lazy1_traits<MatrixRef,Operation>::reference
   operator() (int i, int j) const
   {
      const master& me=static_cast<const master&>(*this);
      typedef typename lazy1_traits<MatrixRef,Operation>::op_builder opb;
      return opb::create(me.get_operation()) (me.get_container()(i,j));
   }
};

template <typename MatrixRef, typename Operation>
class ConcatRows< LazyMatrix1<MatrixRef, Operation> >
   : public LazyVector1<masquerade<ConcatRows,MatrixRef>, Operation> {
protected:
   ConcatRows();
   ~ConcatRows();
};

template <typename MatrixRef, typename OperationVal>
class Rows< LazyMatrix1<MatrixRef, OperationVal> >
   : public modified_container_impl< Rows< LazyMatrix1<MatrixRef, OperationVal> >,
                                     list( Container< masquerade<pm::Rows,MatrixRef> >,
                                           Operation< operations::construct_unary2_with_arg<LazyVector1, OperationVal> >,
                                           MasqueradedTop ) > {
   typedef modified_container_impl<Rows> _super;
protected:
   ~Rows();
public:
   const typename _super::container& get_container() const
   {
      return rows(this->hidden().get_container());
   }
   typename _super::operation get_operation() const
   {
      return this->hidden().get_operation();
   }
};

template <typename MatrixRef, typename OperationVal>
class Cols< LazyMatrix1<MatrixRef, OperationVal> >
   : public modified_container_impl< Cols< LazyMatrix1<MatrixRef, OperationVal> >,
                                     list( Container< masquerade<pm::Cols,MatrixRef> >,
                                           Operation< operations::construct_unary2_with_arg<LazyVector1, OperationVal> >,
                                           MasqueradedTop ) > {
   typedef modified_container_impl<Cols> _super;
protected:
   ~Cols();
public:
   const typename _super::container& get_container() const
   {
      return cols(this->hidden().get_container());
   }
   typename _super::operation get_operation() const
   {
      return this->hidden().get_operation();
   }
};

/* --------------------------------------------
 *  LazyMatrix2
 * lazy evaluation of a binary matrix operator
 * -------------------------------------------- */

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
struct lazy2_traits {
   typedef binary_op_builder<Operation, void, void,
                             typename deref<MatrixRef1>::type::const_reference,
                             typename deref<MatrixRef2>::type::const_reference>
      op_builder;
   typedef typename op_builder::operation::result_type reference;
   typedef typename deref<reference>::type value_type;
   typedef typename object_traits<value_type>::persistent_type element_type;
};

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
class LazyMatrix2
   : public modified_container_pair_base<MatrixRef1, MatrixRef2, Operation>,
     public GenericMatrix< LazyMatrix2<MatrixRef1,MatrixRef2,Operation>,
                           typename lazy2_traits<MatrixRef1,MatrixRef2,Operation>::element_type > {
   typedef modified_container_pair_base<MatrixRef1, MatrixRef2, Operation> _base;
public:
   typedef typename lazy2_traits<MatrixRef1,MatrixRef2,Operation>::value_type value_type;
   typedef typename lazy2_traits<MatrixRef1,MatrixRef2,Operation>::reference reference;
   typedef reference const_reference;

   LazyMatrix2(typename _base::first_arg_type src1_arg, typename _base::second_arg_type src2_arg, const Operation& op_arg=Operation())
      : _base(src1_arg, src2_arg, op_arg) {}
};

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
struct spec_object_traits< LazyMatrix2<MatrixRef1, MatrixRef2, Operation> >
   : spec_object_traits<is_container> {
   static const bool is_lazy=true, is_temporary=true, is_always_const=true;
};

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
struct check_container_feature<LazyMatrix2<MatrixRef1, MatrixRef2, Operation>, sparse>
   : bool2type< TransformedContainerPair_helper1<MatrixRef1, MatrixRef2, Operation>::sparse_result > {};

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
struct check_container_feature<LazyMatrix2<MatrixRef1, MatrixRef2, Operation>, Symmetric> {
   static const bool value=check_container_ref_feature<MatrixRef1,Symmetric>::value &&
                           check_container_ref_feature<MatrixRef2,Symmetric>::value;
};

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
struct check_container_feature<LazyMatrix2<MatrixRef1, MatrixRef2, Operation>, SkewSymmetric> {
   static const bool value=check_container_ref_feature<MatrixRef1,SkewSymmetric>::value &&
                           check_container_ref_feature<MatrixRef2,SkewSymmetric>::value;
};

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
struct check_container_feature<LazyMatrix2<MatrixRef1, MatrixRef2, Operation>, FlatStorage> {
   static const bool value=check_container_ref_feature<MatrixRef1,FlatStorage>::value &&
                           check_container_ref_feature<MatrixRef2,FlatStorage>::value;
};

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
class matrix_random_access_methods< LazyMatrix2<MatrixRef1, MatrixRef2, Operation> > {
   typedef LazyMatrix2<MatrixRef1,MatrixRef2,Operation> master;
public:
   typename lazy2_traits<MatrixRef1,MatrixRef2,Operation>::reference
   operator() (int i, int j) const
   {
      const master& me=static_cast<const master&>(*this);
      typedef typename lazy2_traits<MatrixRef1,MatrixRef2,Operation>::op_builder opb;
      return opb::create(me.get_operation()) (me.get_container1()(i,j), me.get_container2()(i,j));
   }
};

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
class ConcatRows< LazyMatrix2<MatrixRef1, MatrixRef2, Operation> >
   : public LazyVector2< masquerade<ConcatRows,MatrixRef1>,
                         masquerade<ConcatRows,MatrixRef2>, Operation > {
protected:
   ConcatRows();
   ~ConcatRows();
};

template <typename MatrixRef1, typename MatrixRef2, typename OperationVal>
class Rows< LazyMatrix2<MatrixRef1, MatrixRef2, OperationVal> >
   : public modified_container_pair_impl< Rows< LazyMatrix2<MatrixRef1, MatrixRef2, OperationVal> >,
                                          list( Container1< masquerade<pm::Rows, MatrixRef1> >,
                                                Container2< masquerade<pm::Rows, MatrixRef2> >,
                                                Operation< operations::construct_binary2_with_arg<LazyVector2, OperationVal> >,
                                                MasqueradedTop ) > {
   typedef modified_container_pair_impl<Rows> _super;
protected:
   ~Rows();
public:
   const typename _super::container1& get_container1() const
   {
      return rows(this->hidden().get_container1());
   }
   const typename _super::container2& get_container2() const
   {
      return rows(this->hidden().get_container2());
   }
   typename _super::operation get_operation() const
   {
      return this->hidden().get_operation();
   }
};

template <typename MatrixRef1, typename MatrixRef2, typename OperationVal>
class Cols< LazyMatrix2<MatrixRef1, MatrixRef2, OperationVal> >
   : public modified_container_pair_impl< Cols< LazyMatrix2<MatrixRef1, MatrixRef2, OperationVal> >,
                                          list( Container1< masquerade<pm::Cols, MatrixRef1> >,
                                                Container2< masquerade<pm::Cols, MatrixRef2> >,
                                                Operation< operations::construct_binary2_with_arg<LazyVector2, OperationVal> >,
                                                MasqueradedTop ) > {
   typedef modified_container_pair_impl<Cols> _super;
protected:
   ~Cols();
public:
   const typename _super::container1& get_container1() const
   {
      return cols(this->hidden().get_container1());
   }
   const typename _super::container2& get_container2() const
   {
      return cols(this->hidden().get_container2());
   }
   typename _super::operation get_operation() const
   {
      return this->hidden().get_operation();
   }
};

template <typename MatrixRef, typename Operation>
class ConcatRows< Transposed< LazyMatrix1<MatrixRef, Operation> > >
   : public ConcatRows< LazyMatrix1<masquerade<Transposed,MatrixRef>, Operation> > {};

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
class ConcatRows< Transposed< LazyMatrix2<MatrixRef1, MatrixRef2, Operation> > >
   : public ConcatRows< LazyMatrix2<masquerade<Transposed,MatrixRef1>,
                                    masquerade<Transposed,MatrixRef2>, Operation> > {};

/// explicit conversion of matrix elements to another type
template <typename TargetType, typename Matrix> inline
const Matrix&
convert_to(const GenericMatrix<Matrix, TargetType>& m)
{
   return m.top();
}

template <typename TargetType, typename Matrix, typename E> inline
const LazyMatrix1<const Matrix&, conv_by_cast<E, TargetType> >
convert_to(const GenericMatrix<Matrix, E>& m,
           typename enable_if<void**, (convertible_to<E, TargetType>::value && !identical<E, TargetType>::value)>::type=0)
{
   return m.top();
}

template <typename TargetType, typename Matrix, typename E> inline
const LazyMatrix1<const Matrix&, conv<E, TargetType> >
convert_to(const GenericMatrix<Matrix, E>& m,
           typename enable_if<void**, explicitly_convertible_to<E, TargetType>::value>::type=0)
{
   return m.top();
}

template <typename Matrix, typename Operation> inline
const LazyMatrix1<const Matrix&, Operation>
apply_operation(const GenericMatrix<Matrix>& m, const Operation& op)
{
   return LazyMatrix1<const Matrix&, Operation>(m.top(), op);
}

/* --------------------------
 *  MatrixMinor concatenated
 * -------------------------- */

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef,
          bool contiguous=derived_from<typename deref<RowIndexSetRef>::type, sequence>::value>
class MatrixMinorConcatRows
   : public ConcatRows_default< MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef> > {};

template <typename MatrixRef, typename RowIndexSetRef>
class MatrixMinorConcatRows<MatrixRef, RowIndexSetRef, const all_selector&, true>
   : public indexed_subset_impl< MatrixMinorConcatRows<MatrixRef, RowIndexSetRef, const all_selector&, true>,
                                 list( Container1< masquerade<ConcatRows,MatrixRef> >,
                                       Container2< sequence >,
                                       Hidden< MatrixMinor<MatrixRef, RowIndexSetRef, const all_selector&> > ) > {
   typedef indexed_subset_impl<MatrixMinorConcatRows> _super;
public:
   typename _super::container1& get_container1()
   {
      return concat_rows(this->hidden().get_matrix());
   }
   const typename _super::container1& get_container1() const
   {
      return concat_rows(this->hidden().get_matrix());
   }
   sequence get_container2() const
   {
      return sequence(this->hidden().get_subset(int2type<1>()).front() * this->hidden().cols(), dim());
   }

   int dim() const { return this->hidden().rows() * this->hidden().cols(); }
};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
class ConcatRows< MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef> >
   : public MatrixMinorConcatRows<MatrixRef,RowIndexSetRef,ColIndexSetRef>,
     public GenericVector< ConcatRows< MatrixMinor<MatrixRef,RowIndexSetRef,ColIndexSetRef> >,
                           typename deref<MatrixRef>::type::element_type > {
protected:
   ~ConcatRows();
public:
   int dim() const
   {
      return this->hidden().rows() * this->hidden().cols();
   }

   ConcatRows& operator= (const ConcatRows& other) { return ConcatRows::generic_type::operator=(other); }
   using ConcatRows::generic_type::operator=;
};

/* -----------------------------
 *  Block matrices concatenated
 * ----------------------------- */

template <typename MatrixRef1, typename MatrixRef2>
class ConcatRows< RowChain<MatrixRef1, MatrixRef2> >
   : public container_chain_impl< ConcatRows< RowChain<MatrixRef1,MatrixRef2> >,
                                  list( Container1< masquerade<pm::ConcatRows,MatrixRef1> >,
                                        Container2< masquerade<pm::ConcatRows,MatrixRef2> >,
                                        MasqueradedTop ) >,
     public GenericVector< ConcatRows< RowChain<MatrixRef1,MatrixRef2> >,
                                       typename deref<MatrixRef1>::type::element_type> {
   typedef container_chain_impl<ConcatRows> _super;
protected:
   ~ConcatRows();
public:
   typename ConcatRows::container1& get_container1()
   {
      return concat_rows(this->hidden().get_container1());
   }
   typename ConcatRows::container2& get_container2()
   {
      return concat_rows(this->hidden().get_container2());
   }
   const typename ConcatRows::container1& get_container1() const
   {
      return concat_rows(this->hidden().get_container1());
   }
   const typename ConcatRows::container2& get_container2() const
   {
      return concat_rows(this->hidden().get_container2());
   }

   ConcatRows& operator= (const ConcatRows& other) { return ConcatRows::generic_type::operator=(other); }
   using ConcatRows::generic_type::operator=;
   using _super::dim;
};

template <typename MatrixRef1, typename MatrixRef2>
class ConcatRows< Transposed< RowChain<MatrixRef1,MatrixRef2> > >
   : public ConcatRows< ColChain< masquerade<Transposed,MatrixRef1>, masquerade<Transposed,MatrixRef2> > > {};

template <typename MatrixRef1, typename MatrixRef2>
class ConcatRows< Transposed< ColChain<MatrixRef1,MatrixRef2> > >
   : public ConcatRows< RowChain< masquerade<Transposed,MatrixRef1>, masquerade<Transposed,MatrixRef2> > > {};

/* ----------------------
 *  SingleRow, SingleCol
 * ---------------------- */
template <typename VectorRef>
class SingleRow
   : public single_line_matrix<VectorRef>,
     public GenericMatrix< SingleRow<VectorRef>, typename deref<VectorRef>::type::element_type> {
   typedef single_line_matrix<VectorRef> _super;
public:
   typedef typename deref<VectorRef>::type::value_type value_type;
   typedef typename deref<VectorRef>::type::const_reference const_reference;
   typedef typename if_else<_super::is_always_const, const_reference, typename deref<VectorRef>::type::reference>::type
      reference;

   SingleRow(typename _super::arg_type arg) : _super(arg) {}

protected:
   void stretch_cols(int c)
   {
      this->_line.get_object().stretch_dim(c);
   }

   friend class Rows<SingleRow>;
   template <typename, typename> friend class RowChain;
};

template <typename VectorRef>
class SingleCol
   : public single_line_matrix<VectorRef>,
     public GenericMatrix< SingleCol<VectorRef>, typename deref<VectorRef>::type::element_type> {
   typedef single_line_matrix<VectorRef> _super;
public:
   typedef typename deref<VectorRef>::type::value_type value_type;
   typedef typename deref<VectorRef>::type::const_reference const_reference;
   typedef typename if_else<_super::is_always_const, const_reference, typename deref<VectorRef>::type::reference>::type
      reference;

   SingleCol(typename _super::arg_type arg) : _super(arg) {}

protected:
   void stretch_rows(int r)
   {
      this->_line.get_object().stretch_dim(r);
   }

   template <typename, typename> friend class ColChain;
};

template <typename VectorRef>
class matrix_random_access_methods< SingleRow<VectorRef> > {
public:
   typename container_traits<VectorRef>::reference
   operator() (int i, int j)
   {
      SingleRow<VectorRef>& me=static_cast<SingleRow<VectorRef>&>(*this);
      return me.get_line()[j];
   }
   typename container_traits<VectorRef>::const_reference
   operator() (int i, int j) const
   {
      const SingleRow<VectorRef>& me=static_cast<const SingleRow<VectorRef>&>(*this);
      return me.get_line()[j];
   }
};

template <typename VectorRef>
class matrix_random_access_methods< SingleCol<VectorRef> > {
public:
   typename container_traits<VectorRef>::reference
   operator() (int i, int j)
   {
      SingleCol<VectorRef>& me=static_cast<SingleCol<VectorRef>&>(*this);
      return me.get_line()[i];
   }

   typename container_traits<VectorRef>::const_reference
   operator() (int i, int j) const
   {
      const SingleCol<VectorRef>& me=static_cast<const SingleCol<VectorRef>&>(*this);
      return me.get_line()[i];
   }
};

template <typename VectorRef>
struct spec_object_traits< SingleRow<VectorRef> >
   : spec_object_traits< single_value_container<VectorRef> > {};

template <typename VectorRef>
struct spec_object_traits< SingleCol<VectorRef> >
   : spec_object_traits< single_value_container<VectorRef> > {};

template <typename VectorRef>
struct check_container_feature<SingleRow<VectorRef>, FlatStorage> : True {};

template <typename VectorRef>
struct check_container_feature<SingleCol<VectorRef>, FlatStorage> : True {};

template <typename VectorRef>
class ConcatRows< SingleRow<VectorRef> >
   : public redirected_container< ConcatRows< SingleRow<VectorRef> >,
                                  list( Container< VectorRef >,
                                        MasqueradedTop ) >,
     public GenericVector< ConcatRows< SingleRow<VectorRef> >, typename deref<VectorRef>::type::element_type > {
protected:
   ~ConcatRows();
public:
   typename single_value_container<VectorRef>::reference get_container() { return this->hidden().get_line(); }
   typename single_value_container<VectorRef>::const_reference get_container() const { return this->hidden().get_line(); }
   int dim() const { return get_container().dim(); }
};

template <typename VectorRef>
class ConcatRows< SingleCol<VectorRef> > : public ConcatRows< SingleRow<VectorRef> > {
protected:
   ~ConcatRows();
};

template <typename VectorRef>
struct check_container_feature< SingleRow<VectorRef>, sparse>
   : check_container_ref_feature<VectorRef, sparse> {};

template <typename VectorRef>
struct check_container_feature< SingleRow<VectorRef>, pure_sparse>
   : check_container_ref_feature<VectorRef, pure_sparse> {};

template <typename VectorRef>
struct check_container_feature< SingleCol<VectorRef>, sparse>
   : check_container_ref_feature<VectorRef, sparse> {};

template <typename VectorRef>
struct check_container_feature< SingleCol<VectorRef>, pure_sparse>
   : check_container_ref_feature<VectorRef, pure_sparse> {};

template <typename VectorRef, bool _sparse=check_container_ref_feature<VectorRef,sparse>::value>
class single_line_across
   : public modified_container_impl< single_line_across<VectorRef,_sparse>,
                                     list( Container< VectorRef >,
                                           Operation< operations::construct_unary<SingleElementVector> >,
                                           Hidden< single_line_matrix<VectorRef> > ) > {
   typedef modified_container_impl<single_line_across> _super;
public:
   typename single_value_container<VectorRef>::reference get_container() { return this->hidden().get_line(); }
   typename single_value_container<VectorRef>::const_reference get_container() const { return this->hidden().get_line(); }
};

template <typename VectorRef>
class single_line_across<VectorRef, true>
   : public modified_container_pair_impl< single_line_across<VectorRef,true>,
                                          list( Container1< VectorRef >,
                                                Container2< sequence >,
                                                IteratorCoupler< zipping_coupler< operations::cmp, set_union_zipper, true, false> >,
                                                Operation< BuildBinary<SingleElementSparseVector_factory> >,
                                                Hidden< single_line_matrix<VectorRef> > ) > {
   typedef modified_container_pair_impl<single_line_across> _super;
public:
   typename single_value_container<VectorRef>::reference get_container1() { return this->hidden().get_line(); }
   typename single_value_container<VectorRef>::const_reference get_container1() const { return this->hidden().get_line(); }
   sequence get_container2() const { return sequence(0, size()); }
   int size() const { return this->hidden().get_line().dim(); }
};

template <typename VectorRef>
class Rows< SingleRow<VectorRef> >
   : public redirected_container< Rows< SingleRow<VectorRef> >,
                                  list( Container< single_value_container<VectorRef> >,
                                        MasqueradedTop ) > {
protected:
   ~Rows();
public:
   single_value_container<VectorRef>& get_container() { return this->hidden()._line; }
   const single_value_container<VectorRef>& get_container() const { return this->hidden()._line; }
};

template <typename VectorRef>
class Cols< SingleRow<VectorRef> > : public single_line_across<VectorRef> {
protected:
   ~Cols();
};

template <typename VectorRef>
class Rows< SingleCol<VectorRef> > : public single_line_across<VectorRef> {
protected:
   ~Rows();
};

template <typename VectorRef>
class Cols< SingleCol<VectorRef> > : public Rows< SingleRow<VectorRef> > {
protected:
   ~Cols();
};

template <typename VectorRef>
class ConcatRows< Transposed< SingleRow<VectorRef> > > : public ConcatRows< SingleRow<VectorRef> > {
protected:
   ~ConcatRows();
};

template <typename VectorRef>
class ConcatRows< Transposed< SingleCol<VectorRef> > > : public ConcatRows< SingleRow<VectorRef> > {
protected:
   ~ConcatRows();
};

/// disguise a GenericVector as a matrix with 1 row 
template <typename Vector> inline
SingleRow<typename Unwary<Vector>::type&>
vector2row(GenericVector<Vector>& v)
{
   return v.top();
}

template <typename Vector> inline
const SingleRow<const typename Unwary<Vector>::type&>
vector2row(const GenericVector<Vector>& v)
{
   return v.top();
}

/// disguise a GenericVector as a matrix with 1 column 
template <typename Vector> inline
SingleCol<typename Unwary<Vector>::type&>
vector2col(GenericVector<Vector>& v)
{
   return v.top();
}

template <typename Vector> inline
const SingleCol<const typename Unwary<Vector>::type&>
vector2col(const GenericVector<Vector>& v)
{
   return v.top();
}

/* --------------------------
 *  RepeatedRow, RepeatedCol
 * -------------------------- */

template <typename VectorRef>
class repeated_line_matrix {
public:
   static const bool same_elem=derived_from_instance<typename deref<VectorRef>::type, SameElementVector>::value;
   typedef typename deref<VectorRef>::type::value_type value_type;
   typedef typename deref<VectorRef>::type::const_reference const_reference;
   typedef const_reference reference;

   typedef typename repeated_value_container<VectorRef>::arg_type arg_type;

   repeated_line_matrix(arg_type vector_arg, int cnt_arg)
      : lines(vector_arg,cnt_arg) {}

   typename alias<VectorRef>::const_reference get_vector() const { return lines.front(); }
   int get_count() const { return lines.size(); }
protected:
   repeated_value_container<VectorRef> lines;
};

template <typename VectorRef>
class RepeatedRow
   : public repeated_line_matrix<VectorRef>,
     public GenericMatrix< RepeatedRow<VectorRef>,
                           typename deref<VectorRef>::type::element_type> {
   typedef repeated_line_matrix<VectorRef> _super;
public:
   RepeatedRow(typename _super::arg_type vector_arg, int cnt_arg)
      : _super(vector_arg, cnt_arg) {}

protected:
   void stretch_rows(int r)
   {
      this->lines.stretch_dim(r);
   }

   void stretch_cols(int c)
   {
      this->lines.get_elem_alias().get_object().stretch_dim(c);
   }

   friend class Rows<RepeatedRow>;
   template <typename, typename> friend class RowChain;
   template <typename, typename> friend class ColChain;
};

template <typename VectorRef>
class RepeatedCol
   : public repeated_line_matrix<VectorRef>,
     public GenericMatrix< RepeatedCol<VectorRef>,
                           typename deref<VectorRef>::type::element_type> {
   typedef repeated_line_matrix<VectorRef> _super;
public:
   RepeatedCol(typename _super::arg_type vector_arg, int cnt_arg)
      : _super(vector_arg, cnt_arg) {}

protected:
   void stretch_rows(int r)
   {
      this->lines.get_elem_alias().get_object().stretch_dim(r);
   }

   void stretch_cols(int c)
   {
      this->lines.stretch_dim(c);
   }

   template <typename, typename> friend class RowChain;
   template <typename, typename> friend class ColChain;
};

template <typename VectorRef>
struct spec_object_traits< RepeatedRow<VectorRef> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true, is_always_const=true;
};

template <typename VectorRef>
struct spec_object_traits< RepeatedCol<VectorRef> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true, is_always_const=true;
};

template <typename VectorRef>
struct check_container_feature< RepeatedRow<VectorRef>, sparse>
   : check_container_ref_feature<VectorRef, sparse> {};

template <typename VectorRef>
struct check_container_feature< RepeatedRow<VectorRef>, pure_sparse>
   : check_container_ref_feature<VectorRef, pure_sparse> {};

template <typename VectorRef>
struct check_container_feature< RepeatedCol<VectorRef>, sparse>
   : check_container_ref_feature<VectorRef, sparse> {};

template <typename VectorRef>
struct check_container_feature< RepeatedCol<VectorRef>, pure_sparse>
   : check_container_ref_feature<VectorRef, pure_sparse> {};

template <typename VectorRef>
struct check_container_feature<RepeatedRow<VectorRef>, FlatStorage>
{
   static const bool value=repeated_line_matrix<VectorRef>::same_elem;
};

template <typename VectorRef>
struct check_container_feature<RepeatedCol<VectorRef>, FlatStorage>
   : check_container_feature< RepeatedRow<VectorRef>, FlatStorage> {};

template <typename VectorRef>
class matrix_random_access_methods< RepeatedRow<VectorRef> > {
   typedef RepeatedRow<VectorRef> master;
public:
   typename container_traits<VectorRef>::const_reference
   operator() (int i, int j) const
   {
      const master& me=static_cast<const master&>(*this);
      return me.get_vector()[j];
   }
};

template <typename VectorRef>
class matrix_random_access_methods< RepeatedCol<VectorRef> > {
   typedef RepeatedCol<VectorRef> master;
public:
   typename container_traits<VectorRef>::const_reference
   operator() (int i, int j) const
   {
      const master& me=static_cast<const master&>(*this);
      return me.get_vector()[i];
   }
};

template <typename VectorRef,
          bool _same_elem=repeated_line_matrix<VectorRef>::same_elem>
class ConcatRepeatedRow_impl
   : public container_product_impl< ConcatRepeatedRow_impl<VectorRef,_same_elem>,
                                    typename concat_list< typename list2cons<typename repeated_container<VectorRef>::params>::type,
                                                          Hidden< repeated_line_matrix<VectorRef> > >::type > {
public:
   count_down get_container1() const { return count_down(this->hidden().get_count()); }
   typename alias<VectorRef>::const_reference get_container2() const { return this->hidden().get_vector(); }

   int dim() const
   {
      return this->hidden().get_count() * get_container2().dim();
   }
};

template <typename VectorRef>
class ConcatRepeatedRow_impl<VectorRef, true>
   : public repeated_value_container_impl< ConcatRepeatedRow_impl<VectorRef,true>,
                                           typename deref<VectorRef>::type::element_reference,
                                           Hidden< repeated_line_matrix<VectorRef> > > {
public:
   const alias<typename deref<VectorRef>::type::element_reference>& get_elem_alias() const
   {
      return this->hidden().get_vector().get_elem_alias();
   }
   int size() const
   {
      return this->hidden().get_count() * this->hidden().get_vector().size();
   }
   int dim() const
   {
      return this->hidden().get_count() * this->hidden().get_vector().dim();
   }
};

template <typename VectorRef,
          bool _same_elem=repeated_line_matrix<VectorRef>::same_elem>
class ConcatRepeatedCol_impl
   : public container_product_impl< ConcatRepeatedCol_impl<VectorRef,_same_elem>,
                                    list( Container1<VectorRef>,
                                          Container2<count_down>,
                                          Hidden< repeated_line_matrix<VectorRef> > ) > {
public:
   typename alias<VectorRef>::const_reference get_container1() const { return this->hidden().get_vector(); }
   count_down get_container2() const { return count_down(this->hidden().get_count()); }

   int dim() const
   {
      return this->hidden().get_count() * get_container1().dim();
   }
};

template <typename VectorRef>
class ConcatRepeatedCol_impl<VectorRef, true>
   : ConcatRepeatedRow_impl<VectorRef, true> {};

template <typename VectorRef>
class ConcatRows< RepeatedRow<VectorRef> >
   : public ConcatRepeatedRow_impl<VectorRef>,
     public GenericVector< ConcatRows< RepeatedRow<VectorRef> >,
                           typename deref<VectorRef>::type::element_type > {
protected:
   ~ConcatRows();
public:
   using ConcatRepeatedRow_impl<VectorRef>::dim;
};

template <typename VectorRef>
class ConcatRows< RepeatedCol<VectorRef> >
   : public ConcatRepeatedCol_impl<VectorRef>,
     public GenericVector< ConcatRows< RepeatedCol<VectorRef> >,
                           typename deref<VectorRef>::type::element_type > {
protected:
   ~ConcatRows();
public:
   using ConcatRepeatedCol_impl<VectorRef>::dim;
};

template <typename VectorRef, bool _sparse=check_container_ref_feature<VectorRef,sparse>::value>
class repeated_line_across
   : public modified_container_impl< repeated_line_across<VectorRef, _sparse>,
                                     list( Container< typename attrib<VectorRef>::plus_const >,
                                           Operation< operations::construct_unary_with_arg<SameElementVector,int> >,
                                           Hidden< repeated_line_matrix<VectorRef> > ) > {
   typedef modified_container_impl<repeated_line_across> _super;
public:
   const typename _super::container& get_container() const { return this->hidden().get_vector(); }
   typename _super::operation get_operation() const { return this->hidden().get_count(); }
};

template <typename VectorRef>
class repeated_line_across<VectorRef, true>
   : public modified_container_pair_impl< repeated_line_across<VectorRef,true>,
                                          list( Container1< sequence >,
                                                Container2< typename attrib<VectorRef>::plus_const >,
                                                IteratorCoupler< zipping_coupler< operations::cmp, set_union_zipper, false, true> >,
                                                Operation< SameElementSparseVector_factory<1> >,
                                                Hidden< repeated_line_matrix<VectorRef> > ) > {
   typedef modified_container_pair_impl<repeated_line_across> _super;
public:
   sequence get_container1() const { return sequence(0, size()); }
   const typename _super::container2& get_container2() const { return this->hidden().get_vector(); }
   typename _super::operation get_operation() const { return this->hidden().get_count(); }
   int size() const { return get_container2().dim(); }
};

template <typename VectorRef>
class Rows< RepeatedRow<VectorRef> >
   : public redirected_container< Rows< RepeatedRow<VectorRef> >,
                                  list( Container< repeated_value_container<VectorRef> >,
                                        MasqueradedTop ) > {
protected:
   ~Rows();
public:
   const repeated_value_container<VectorRef>& get_container() const { return this->hidden().lines; }
};

template <typename VectorRef>
class Rows< RepeatedCol<VectorRef> >
   : public repeated_line_across<VectorRef> {
protected:
   ~Rows();
};

template <typename VectorRef>
class Cols< RepeatedRow<VectorRef> >
   : public repeated_line_across<VectorRef> {
protected:
   ~Cols();
};

template <typename VectorRef>
class Cols< RepeatedCol<VectorRef> >
   : public Rows< RepeatedRow<VectorRef> > {};

template <typename VectorRef>
class Transposed< RepeatedRow<VectorRef> >
   : public RepeatedCol<VectorRef> {};

template <typename VectorRef>
class Transposed< RepeatedCol<VectorRef> >
   : public RepeatedRow<VectorRef> {};

template <typename VectorRef>
class ConcatRows< Transposed< RepeatedRow<VectorRef> > >
   : public ConcatRows< RepeatedCol<VectorRef> > {};

template <typename VectorRef>
class ConcatRows< Transposed< RepeatedCol<VectorRef> > >
   : public ConcatRows< RepeatedRow<VectorRef> > {};

/// Create a matrix with n rows, each equal to v. 
template <typename Vector> inline
const RepeatedRow<typename Diligent<const typename Unwary<Vector>::type&>::type>
repeat_row(const GenericVector<Vector>& v, int n=0)
{
   return RepeatedRow<typename Diligent<const typename Unwary<Vector>::type&>::type> (v.top(), n);
}

/// Create a matrix with n columns, each equal to v. 
template <typename Vector> inline
const RepeatedCol<typename Diligent<const typename Unwary<Vector>::type&>::type>
repeat_col(const GenericVector<Vector>& v, int n=0)
{
   return RepeatedCol<typename Diligent<const typename Unwary<Vector>::type&>::type> (v.top(), n);
}

/// Create a matrix with m rows and n columns whose entries
/// are all equal to the given element x.
template <typename E> inline
const RepeatedRow< SameElementVector<const E&> >
same_element_matrix(const E& x, int m, int n)
{
   return RepeatedRow< SameElementVector<const E&> > (same_element_vector(x,n), m);
}

/// Create a matrix with m rows and n columns whose entries
/// are all equal to 1.
template <typename E> inline
const RepeatedRow< SameElementVector<const E&> >
ones_matrix(int m, int n)
{
   return RepeatedRow< SameElementVector<const E&> > (ones_vector<E>(n), m);
}

/// Create a matrix with m rows and n columns whose entries
/// are all equal to 0.
template <typename E> inline
const RepeatedRow< SameElementVector<const E&> >
zero_matrix(int m, int n)
{
   return RepeatedRow< SameElementVector<const E&> > (zero_vector<E>(n), m);
}

template <typename E>
class constant_value_matrix
   : public constant_value_container<E>,
     public GenericMatrix<constant_value_matrix<E>, typename deref<E>::type> {
   typedef constant_value_container<E> _super;
public:
   constant_value_matrix(typename _super::arg_type arg) : _super(arg) {}

   typename _super::const_reference operator() (int, int) const { return this->front(); }
};

template <typename E>
struct spec_object_traits< constant_value_matrix<E> > : spec_object_traits< constant_value_container<E> > {};

template <typename E>
struct check_container_feature<constant_value_matrix<E>, FlatStorage> : True {};

template <typename E>
class ConcatRows< constant_value_matrix<E> >
   : public constant_value_container<E> {
protected:
   ConcatRows();
   ~ConcatRows();
};

template <typename E>
class Rows< constant_value_matrix<E> >
   : public constant_masquerade_container< constant_value_container<E> > {
protected:
   ~Rows();
};

template <typename E>
class Cols< constant_value_matrix<E> >
   : public constant_masquerade_container< constant_value_container<E> > {
protected:
   ~Cols();
};

/* --------------------------------------
 *  diagonal and block-diagonal matrices
 * -------------------------------------- */

// _main==true: the vector is laid out along the main diagonal
// _main==false: along the anti-diagonal (starting in the lower left corner)

template <typename VectorRef, bool _main=true>
class DiagMatrix
   : public GenericMatrix< DiagMatrix<VectorRef, _main>, typename deref<VectorRef>::type::element_type > {
protected:
   alias<VectorRef> vector;
public:
   typedef typename deref<VectorRef>::type::value_type value_type;
   typedef typename deref<VectorRef>::type::const_reference const_reference;
   typedef const_reference reference;

   typedef typename alias<VectorRef>::arg_type arg_type;
   DiagMatrix(arg_type vector_arg) : vector(vector_arg) {}

   typename alias<VectorRef>::const_reference get_vector() const { return *vector; }
protected:
   void stretch_rows(int r)
   {
      vector.get_object().stretch_dim(r);
   }
   void stretch_cols(int c)
   {
      vector.get_object().stretch_dim(c);
   }

   template <typename, typename> friend class RowChain;
   template <typename, typename> friend class ColChain;
};

template <typename VectorRef, bool _main>
struct spec_object_traits< DiagMatrix<VectorRef, _main> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true, is_always_const=true;
};

template <typename VectorRef, bool _main>
class matrix_random_access_methods< DiagMatrix<VectorRef, _main> > {
   typedef DiagMatrix<VectorRef, _main> master;
public:
   typename container_traits<VectorRef>::const_reference
   operator() (int i, int j) const
   {
      const master& me=static_cast<const master&>(*this);
      if (_main ? i==j : i+j==me.get_vector().dim()-1)
         return me.get_vector()[j];
      return zero_value<typename deref<VectorRef>::type::element_type>();
   }
};

template <typename VectorRef, bool _main>
class ConcatRows< DiagMatrix<VectorRef, _main> >
   : public modified_container_pair_impl< ConcatRows< DiagMatrix<VectorRef, _main> >,
                                          list( Container1< masquerade_add_features<VectorRef,
                                                               typename concat_list< cons<indexed, end_sensitive>,
                                                                  typename if_else<_main, void, _reversed>::type >::type > >,
                                                Container2< series >,
                                                Operation< pair<nothing, BuildBinaryIt<operations::dereference2> > >,
                                                MasqueradedTop ) >,
     public GenericVector< ConcatRows< DiagMatrix<VectorRef, _main> >,
                           typename deref<VectorRef>::type::element_type > {
   typedef modified_container_pair_impl<ConcatRows> _super;
public:
   const typename _super::container1& get_container1() const
   {
      return reinterpret_cast<const typename _super::container1&>(this->hidden().get_vector());
   }

   series get_container2() const
   {
      const int d=this->hidden().get_vector().dim();
      return series(_main ? 0 : d-1, d, _main ? d+1 : d-1);
   }

   int dim() const
   {
      const int d=this->hidden().get_vector().dim();
      return d*d;
   }
};

template <typename VectorRef, bool _main>
struct check_container_feature< DiagMatrix<VectorRef, _main>, pure_sparse> : True {};

template <typename VectorRef, bool _main>
struct check_container_feature< DiagMatrix<VectorRef, _main>, Symmetric> : bool2type<_main> {};

template <typename VectorRef, bool _main, typename reversed,
          bool _simple=derived_from_instance<typename deref<VectorRef>::type, SameElementVector>::value>
struct DiagRowsCols_helper {
   static const bool use_sequence_index= !_main && identical<reversed, void>::value;
   typedef zipping_coupler< operations::cmp, set_union_zipper, use_sequence_index, true > zipper;
   typedef list (params)( Container1< Series<int, _main> >,
                          Container2< masquerade_add_features<VectorRef, typename concat_list<pure_sparse, reversed>::type> >,
                          IteratorCoupler< typename reverse_coupler_helper<zipper, reversed>::type >,
                          Operation< SameElementSparseVector_factory<3> >,
                          Hidden< DiagMatrix<VectorRef,_main> > );
};

template <typename VectorRef, bool _main, typename reversed>
struct DiagRowsCols_helper<VectorRef, _main, reversed, true> {
   typedef list (params)( Container1< Series<int, _main> >,
                          Container2< VectorRef >,
                          Operation< SameElementSparseVector_factory<2> >,
                          Hidden< DiagMatrix<VectorRef,_main> > );
};


template <typename VectorRef, bool _main, typename reversed=void>
class DiagRowsCols
   : public modified_container_pair_impl< DiagRowsCols<VectorRef, _main, reversed>,
                                          typename DiagRowsCols_helper<VectorRef, _main, reversed>::params> {
   typedef modified_container_pair_impl<DiagRowsCols> _super;
public:
   Series<int, _main> get_container1() const
   {
      const int n=size();
      return Series<int, _main>(_main ? 0 : n-1, n, _main ? 1 : -1);
   }
   const typename _super::container2& get_container2() const
   {
      return reinterpret_cast<const typename _super::container2&>(this->hidden().get_vector());
   }

   typename _super::operation get_operation() const { return size(); }
   int size() const { return this->hidden().get_vector().dim(); }
};

template <typename VectorRef>
class Rows< DiagMatrix<VectorRef, true> > : public DiagRowsCols<VectorRef, true> {
protected:
   ~Rows();
};

template <typename VectorRef>
class Cols< DiagMatrix<VectorRef, true> > : public Rows< DiagMatrix<VectorRef, true> > {};

template <typename VectorRef>
class Rows< DiagMatrix<VectorRef, false> > : public DiagRowsCols<VectorRef, false, _reversed> {
protected:
   ~Rows();
};

template <typename VectorRef>
class Cols< DiagMatrix<VectorRef, false> > : public DiagRowsCols<VectorRef, false> {
protected:
   ~Cols();
};

/// Create a square diagonal matrix from a GenericVector. 
template <typename Vector> inline
const DiagMatrix<const typename Unwary<Vector>::type&>
diag(const GenericVector<Vector>& v)
{
   return v.top();
}

template <typename E, int size>
const DiagMatrix<const FixedVector<E,size>&>
diag(const E (&a)[size])
{
   return array2vector(a);
}

/// Create a anti-diagonal matrix. 
template <typename Vector> inline
const DiagMatrix<const typename Unwary<Vector>::type&, false>
anti_diag(const GenericVector<Vector>& v)
{
   return v.top();
}

template <typename E, int size>
const DiagMatrix<const FixedVector<E,size>&, false>
anti_diag(const E (&a)[size])
{
   return array2vector(a);
}


/// Create a unit_matrix of dimension dim.
template <typename E> inline
const DiagMatrix< SameElementVector<const E&> >
unit_matrix(int dim)
{
   return ones_vector<E>(dim);
}

// _main==true : blocks are laid out along the main diagonal
// _main==false : along the anti-diagonal, starting in the lower left corner

template <typename MatrixRef1, typename MatrixRef2, bool _main=true>
class BlockDiagMatrix
   : public container_pair_base<typename attrib<MatrixRef1>::plus_const, typename attrib<MatrixRef2>::plus_const>,
     public GenericMatrix< BlockDiagMatrix<MatrixRef1,MatrixRef2,_main>,
                           typename deref<MatrixRef1>::type::element_type > {
   typedef container_pair_base<typename attrib<MatrixRef1>::plus_const, typename attrib<MatrixRef2>::plus_const> _base;
public:
   typedef typename identical<typename container_traits<MatrixRef1>::value_type,
                              typename container_traits<MatrixRef2>::value_type>::type
      value_type;
   typedef typename compatible<typename container_traits<MatrixRef1>::const_reference,
                               typename container_traits<MatrixRef2>::const_reference>::type
      const_reference;
   typedef const_reference reference;

   BlockDiagMatrix(typename _base::first_arg_type matrix1_arg, typename _base::second_arg_type matrix2_arg)
      : _base(matrix1_arg, matrix2_arg) {}

   /// the number of rows
   int rows() const { return this->get_container1().rows() + this->get_container2().rows(); }

   /// the number of columns
   int cols() const { return this->get_container1().cols() + this->get_container2().cols(); }
};

template <typename MatrixRef1, typename MatrixRef2, bool _main>
struct spec_object_traits< BlockDiagMatrix<MatrixRef1, MatrixRef2, _main> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true, is_always_const=true;
};

template <typename MatrixRef1, typename MatrixRef2, bool _main>
struct check_container_feature<BlockDiagMatrix<MatrixRef1, MatrixRef2, _main>, sparse> : True {};

template <typename MatrixRef1, typename MatrixRef2, bool _main>
struct check_container_feature<BlockDiagMatrix<MatrixRef1, MatrixRef2, _main>, pure_sparse> {
   static const bool value=check_container_ref_feature<MatrixRef1, pure_sparse>::value &&
                           check_container_ref_feature<MatrixRef2, pure_sparse>::value;
};

template <typename MatrixRef1, typename MatrixRef2>
struct check_container_feature<BlockDiagMatrix<MatrixRef1, MatrixRef2, true>, Symmetric> {
   static const bool value=check_container_ref_feature<MatrixRef1, Symmetric>::value &&
                           check_container_ref_feature<MatrixRef2, Symmetric>::value;
};

template <typename MatrixRef1, typename MatrixRef2, bool _main>
class matrix_random_access_methods< BlockDiagMatrix<MatrixRef1, MatrixRef2, _main> > {
   typedef BlockDiagMatrix<MatrixRef1, MatrixRef2, _main> master;
public:
   typename compatible<typename container_traits<MatrixRef1>::const_reference,
                       typename container_traits<MatrixRef2>::const_reference>::type
   operator() (int i, int j) const
   {
      const master& me=static_cast<const master&>(*this);
      const int c1=me.get_container1().cols();
      if (_main) {
         const int r1=me.get_container1().rows();
         if (i<r1 && j<c1) return me.get_container1()(i,j);
         if (i>=r1 && j>=c1) return me.get_container2()(i-r1,j-c1);
      } else {
         const int r2=me.get_container2().rows();
         if (i>=r2 && j<c1) return me.get_container1()(i-r2,j);
         if (i<r2 && j>=c1) return me.get_container2()(i,j-c1);
      }
      return zero_value<typename master::element_type>();
   }
};

template <typename MatrixRef1, typename MatrixRef2, bool _main,
          template <typename> class RowsCols, bool _rows, bool _first>
class BlockDiagRowsCols
   : public modified_container_impl< BlockDiagRowsCols<MatrixRef1,MatrixRef2,_main,RowsCols,_rows,_first>,
                                     list( Container< masquerade<RowsCols, typename if_else<(_main ? _first : _rows^_first), MatrixRef1, MatrixRef2>::type> >,
                                           Operation< ExpandedVector_factory<> >,
                                           Hidden< BlockDiagMatrix<MatrixRef1,MatrixRef2,_main> > ) > {
   typedef modified_container_impl<BlockDiagRowsCols> _super;

   const typename _super::container& _get_container(True) const
   {
      return reinterpret_cast<const typename _super::container&>(this->hidden().get_container1());
   }
   const typename _super::container& _get_container(False) const
   {
      return reinterpret_cast<const typename _super::container&>(this->hidden().get_container2());
   }

   int _dim(True)  const { return this->hidden().get_container1().cols() + this->hidden().get_container2().cols(); }
   int _dim(False) const { return this->hidden().get_container1().rows() + this->hidden().get_container2().rows(); }

   int _offset(True,  True,  True)  const { return 0; }
   int _offset(True,  True,  False) const { return this->hidden().get_container1().cols(); }
   int _offset(True,  False, True)  const { return 0; }
   int _offset(True,  False, False) const { return this->hidden().get_container1().rows(); }
   int _offset(False, True,  True)  const { return this->hidden().get_container1().cols(); }
   int _offset(False, True,  False) const { return 0; }
   int _offset(False, False, True)  const { return this->hidden().get_container2().rows(); }
   int _offset(False, False, False) const { return 0; }

public:
   const typename _super::container& get_container() const
   {
      return _get_container(bool2type<(_main ? _first : _rows^_first)>());
   }
   typename _super::operation get_operation() const
   {
      return typename _super::operation(_offset(bool2type<_main>(), bool2type<_rows>(), bool2type<_first>()),
                                        _dim(bool2type<_rows>()));
   }
};

template <typename MatrixRef1, typename MatrixRef2, bool _main>
class Rows< BlockDiagMatrix<MatrixRef1, MatrixRef2, _main> >
   : public container_chain_impl< Rows< BlockDiagMatrix<MatrixRef1,MatrixRef2,_main> >,
                                  list( Container1< BlockDiagRowsCols<MatrixRef1,MatrixRef2,_main,Rows,true,true> >,
                                        Container2< BlockDiagRowsCols<MatrixRef1,MatrixRef2,_main,Rows,true,false> >,
                                        MasqueradedTop ) > {
protected:
   ~Rows();
};

template <typename MatrixRef1, typename MatrixRef2, bool _main>
class Cols< BlockDiagMatrix<MatrixRef1, MatrixRef2, _main> >
   : public container_chain_impl< Cols< BlockDiagMatrix<MatrixRef1,MatrixRef2,_main> >,
                                  list( Container1< BlockDiagRowsCols<MatrixRef1,MatrixRef2,_main,Cols,false,true> >,
                                        Container2< BlockDiagRowsCols<MatrixRef1,MatrixRef2,_main,Cols,false,false> >,
                                        MasqueradedTop ) > {
protected:
   ~Cols();
};

/// Create a block-diagonal matrix.
template <typename E, typename Matrix1, typename Matrix2> inline
const BlockDiagMatrix<const typename Unwary<Matrix1>::type&,
                      const typename Unwary<Matrix2>::type&, true>
diag(const GenericMatrix<Matrix1,E>& m1, const GenericMatrix<Matrix2,E>& m2)
{
   return BlockDiagMatrix<const typename Unwary<Matrix1>::type&,
                          const typename Unwary<Matrix2>::type&, true>(m1.top(), m2.top());
}

/// Create a block-diagonal matrix. 
/// The vector argument is treated as a square diagonal matrix.
template <typename E, typename Vector1, typename Matrix2> inline
const BlockDiagMatrix<DiagMatrix<const typename Unwary<Vector1>::type&>,
                      const typename Unwary<Matrix2>::type&, true>
diag(const GenericVector<Vector1,E>& v1, const GenericMatrix<Matrix2,E>& m2)
{
   return BlockDiagMatrix<DiagMatrix<const typename Unwary<Vector1>::type&>,
                          const typename Unwary<Matrix2>::type&, true>(diag(v1), m2.top());
}

/// Create a block-diagonal matrix. 
/// The vector argument is treated as a square diagonal matrix.
template <typename E, typename Matrix1, typename Vector2> inline
const BlockDiagMatrix<const typename Unwary<Matrix1>::type&,
                      DiagMatrix<const typename Unwary<Vector2>::type&>, true>
diag(const GenericMatrix<Matrix1,E>& m1, const GenericVector<Vector2,E>& v2)
{
   return BlockDiagMatrix<const typename Unwary<Matrix1>::type&,
                          DiagMatrix<const typename Unwary<Vector2>::type&>, true>(m1.top(), diag(v2));
}

template <typename Scalar1, typename Matrix2, typename E> inline
const BlockDiagMatrix<DiagMatrix< SingleElementVector<E> >,
                      const typename Unwary<Matrix2>::type&, true>
diag(const Scalar1& x1, const GenericMatrix<Matrix2,E>& m2,
     typename enable_if<void**, (isomorphic_types<Scalar1, E>::value && (convertible_to<Scalar1, E>::value || explicitly_convertible_to<Scalar1, E>::value))>::type=0)
{
   return BlockDiagMatrix<DiagMatrix< SingleElementVector<E> >,
                          const typename Unwary<Matrix2>::type&, true>
                         (DiagMatrix< SingleElementVector<E> >(SingleElementVector<E>(convert_to<E>(x1))), m2.top());
}

template <typename Matrix1, typename Scalar2, typename E> inline
const BlockDiagMatrix<const typename Unwary<Matrix1>::type&,
                      DiagMatrix< SingleElementVector<E> >, true>
diag(const GenericMatrix<Matrix1,E>& m1, const Scalar2& x2,
     typename enable_if<void**, (isomorphic_types<Scalar2, E>::value && (convertible_to<Scalar2, E>::value || explicitly_convertible_to<Scalar2, E>::value))>::type=0)
{
   return BlockDiagMatrix<const typename Unwary<Matrix1>::type&,
                          DiagMatrix< SingleElementVector<E> >, true>
                         (m1.top(), DiagMatrix< SingleElementVector<E> >(SingleElementVector<E>(convert_to<E>(x2))));
}

/// Create a block-anti-diagonal matrix. 
template <typename E, typename Matrix1, typename Matrix2> inline
const BlockDiagMatrix<const typename Unwary<Matrix1>::type&,
                      const typename Unwary<Matrix2>::type&, false>
anti_diag(const GenericMatrix<Matrix1,E>& m1, const GenericMatrix<Matrix2,E>& m2)
{
   return BlockDiagMatrix<const typename Unwary<Matrix1>::type&,
                          const typename Unwary<Matrix2>::type&, false>(m1.top(), m2.top());
}

/// Create a block-anti-diagonal matrix. 
/// The vector argument is treated as a square anti-diagonal matrix.
template <typename E, typename Vector1, typename Matrix2> inline
const BlockDiagMatrix<DiagMatrix<const typename Unwary<Vector1>::type&, false>,
                      const typename Unwary<Matrix2>::type&, false>
anti_diag(const GenericVector<Vector1,E>& v1, const GenericMatrix<Matrix2,E>& m2)
{
   return BlockDiagMatrix<DiagMatrix<const typename Unwary<Vector1>::type&, false>,
                          const typename Unwary<Matrix2>::type&, false>(anti_diag(v1), m2.top());
}

/// Create a block-anti-diagonal matrix. 
/// The vector argument is treated as a square anti-diagonal matrix.
template <typename E, typename Matrix1, typename Vector2> inline
const BlockDiagMatrix<const typename Unwary<Matrix1>::type&,
                      DiagMatrix<const typename Unwary<Vector2>::type&, false>, false>
anti_diag(const GenericMatrix<Matrix1,E>& m1, const GenericVector<Vector2,E>& v2)
{
   return BlockDiagMatrix<const typename Unwary<Matrix1>::type&,
                          DiagMatrix<const typename Unwary<Vector2>::type&, false>, false>(m1.top(), anti_diag(v2));
}

template <typename Scalar1, typename Matrix2, typename E> inline
const BlockDiagMatrix< DiagMatrix<SingleElementVector<E>, false>,
                       const typename Unwary<Matrix2>::type&, false>
anti_diag(const Scalar1& x1, const GenericMatrix<Matrix2,E>& m2,
          typename enable_if<void**, (isomorphic_types<Scalar1, E>::value && (convertible_to<Scalar1, E>::value || explicitly_convertible_to<Scalar1, E>::value))>::type=0)
{
   return BlockDiagMatrix<DiagMatrix<SingleElementVector<E>, false>,
                          const typename Unwary<Matrix2>::type&, false>
                         (DiagMatrix<SingleElementVector<E>, false>(SingleElementVector<E>(convert_to<E>(x1))), m2.top());
}

template <typename Matrix1, typename Scalar2, typename E> inline
const BlockDiagMatrix<const typename Unwary<Matrix1>::type&,
                      DiagMatrix<SingleElementVector<E>, false>, false>
anti_diag(const GenericMatrix<Matrix1,E>& m1, const Scalar2& x2,
          typename enable_if<void**, (isomorphic_types<Scalar2, E>::value && (convertible_to<Scalar2, E>::value || explicitly_convertible_to<Scalar2, E>::value))>::type=0)
{
   return BlockDiagMatrix<const typename Unwary<Matrix1>::type&,
                          DiagMatrix<SingleElementVector<E>, false>, false>
                         (m1.top(), DiagMatrix<SingleElementVector<E>, false>(SingleElementVector<E>(convert_to<E>(x2))));
}

/* -----------------
 *  SparseMatrix2x2
 * ----------------- */

/* This is a matrix of arbitrary dimensions.
   Only 4 elements have distinct values: a_ii, a_ij, a_ji, a_jj.
   The rest elements are implicitly 1 if on the diagonal and 0 otherwise.
*/
template <typename E>
class SparseMatrix2x2 {
public:
   typedef E value_type;
   typedef E& reference;
   typedef const E& const_reference;

   int i, j;    // row & col
   E a_ii, a_ij, a_ji, a_jj;

   SparseMatrix2x2() {}

   SparseMatrix2x2(int i_arg, int j_arg,
                   typename function_argument<E>::type a_ii_arg, typename function_argument<E>::type a_ij_arg,
                   typename function_argument<E>::type a_ji_arg, typename function_argument<E>::type a_jj_arg)
      : i(i_arg), j(j_arg), a_ii(a_ii_arg), a_ij(a_ij_arg), a_ji(a_ji_arg), a_jj(a_jj_arg) {}
};

template <typename E>
class Transposed< SparseMatrix2x2<E> > : public SparseMatrix2x2<E> {
protected:
   ~Transposed();
};

template <typename E> inline
E det(const SparseMatrix2x2<E>& U)
{
   return U.a_ii*U.a_jj-U.a_ij*U.a_ji;
}

/* ------------------------------------
 *  MatrixProduct
 * lazy evaluation of a matrix product
 * ------------------------------------ */

template <typename MatrixRef1, typename MatrixRef2>
struct lazy_product_traits {
   typedef typename deref<typename operations::mul<typename deref<MatrixRef1>::type::row_type,
                                                   typename deref<MatrixRef2>::type::col_type>::result_type>::type
      reference;
   typedef typename deref<reference>::type value_type;
   typedef typename object_traits<value_type>::persistent_type element_type;
};

template <typename MatrixRef1, typename MatrixRef2>
class MatrixProduct
   : public container_pair_base<MatrixRef1, MatrixRef2>,
     public GenericMatrix< MatrixProduct<MatrixRef1,MatrixRef2>,
                           typename lazy_product_traits<MatrixRef1,MatrixRef2>::element_type > {
   typedef container_pair_base<MatrixRef1, MatrixRef2> _base;
public:
   typedef typename lazy_product_traits<MatrixRef1,MatrixRef2>::reference reference;
   typedef typename lazy_product_traits<MatrixRef1,MatrixRef2>::value_type value_type;
   typedef reference const_reference;
   typedef typename lazy_product_traits<MatrixRef1,MatrixRef2>::element_type element_type;

   MatrixProduct(typename _base::first_arg_type src1_arg, typename _base::second_arg_type src2_arg)
      : _base(src1_arg,src2_arg) {}

   static const bool is_sparse=check_container_ref_feature<MatrixRef1, sparse>::value &&
                               check_container_ref_feature<MatrixRef2, sparse>::value;
   typedef typename if_else<is_sparse, SparseMatrix<element_type>, Matrix<element_type> >::type
      persistent_type;
};

template <typename MatrixRef1, typename MatrixRef2>
struct spec_object_traits< MatrixProduct<MatrixRef1, MatrixRef2> >
   : spec_object_traits<is_container> {
   static const bool is_lazy=true, is_temporary=true, is_always_const=true;
};

template <typename MatrixRef1, typename MatrixRef2>
class matrix_random_access_methods< MatrixProduct<MatrixRef1, MatrixRef2> > {
   typedef MatrixProduct<MatrixRef1,MatrixRef2> master;
public:
   typename operations::mul<typename container_traits<MatrixRef1>::const_reference,
                            typename container_traits<MatrixRef2>::const_reference>::result_type
   operator() (int i, int j) const
   {
      const master& me=static_cast<const master&>(*this);
      return me.get_container1().row(i) * me.get_container2().col(j);
   }
};

template <typename MatrixRef1, typename MatrixRef2>
class ConcatRows< MatrixProduct<MatrixRef1,MatrixRef2> >
   : public container_product_impl< ConcatRows< MatrixProduct<MatrixRef1,MatrixRef2> >,
                                    list( Container1< masquerade<Rows,MatrixRef1> >,
                                          Container2< masquerade<Cols,MatrixRef2> >,
                                          Operation< BuildBinary<operations::mul> >,
                                          MasqueradedTop ) >,
     public GenericVector< ConcatRows< MatrixProduct<MatrixRef1,MatrixRef2> >,
                           typename lazy_product_traits<MatrixRef1,MatrixRef2>::element_type > {
   typedef container_product_impl<ConcatRows> _super;
public:
   const typename _super::container1& get_container1() const
   {
      return rows(this->hidden().get_container1());
   }
   const typename _super::container2& get_container2() const
   {
      return cols(this->hidden().get_container2());
   }
};

template <typename MatrixRef1, typename MatrixRef2>
class Rows< MatrixProduct<MatrixRef1,MatrixRef2> >
   : public modified_container_pair_impl< Rows< MatrixProduct<MatrixRef1,MatrixRef2> >,
                                          list( Container1< masquerade<pm::Rows,MatrixRef1> >,
                                                Container2< constant_value_container<MatrixRef2> >,
                                                Operation< BuildBinary<operations::mul> >,
                                                MasqueradedTop ) > {
   typedef modified_container_pair_impl<Rows> _super;
protected:
   ~Rows();
public:
   const typename _super::container1& get_container1() const
   {
      return rows(this->hidden().get_container1());
   }
   const typename _super::container2& get_container2() const
   {
      return constant(this->hidden().get_container2_alias());
   }
};

template <typename MatrixRef1, typename MatrixRef2>
class Cols< MatrixProduct<MatrixRef1,MatrixRef2> >
   : public modified_container_pair_impl< Cols< MatrixProduct<MatrixRef1,MatrixRef2> >,
                                          list( Container1< constant_value_container<MatrixRef1> >,
                                                Container2< masquerade<pm::Cols,MatrixRef2> >,
                                                Operation< BuildBinary<operations::mul> >,
                                                MasqueradedTop ) > {
   typedef modified_container_pair_impl<Cols> _super;
protected:
   ~Cols();
public:
   const typename _super::container1& get_container1() const
   {
      return constant(this->hidden().get_container1_alias());
   }
   const typename _super::container2& get_container2() const
   {
      return cols(this->hidden().get_container2());
   }
};

/* ---------------------------------------
 *  matrix arithmetic operations
 * --------------------------------------- */
namespace operations {

template <typename OpRef>
struct neg_impl<OpRef, is_matrix> {
   typedef OpRef argument_type;
   typedef LazyMatrix1<typename attrib<typename Unwary<OpRef>::type>::plus_const, BuildUnary<neg> > result_type;

   result_type operator() (typename function_argument<OpRef>::const_type x) const
   {
      return unwary(x);
   }

   void assign(typename lvalue_arg<OpRef>::type x) const
   {
      x.negate();
   }
};

template <typename LeftRef, typename RightRef>
struct add_impl<LeftRef, RightRef, cons<is_matrix, is_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyMatrix2<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                       typename attrib<typename Unwary<RightRef>::type>::plus_const,
                       BuildBinary<add> >
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (l.rows() != r.rows() || l.cols() != r.cols())
            throw std::runtime_error("operator+(GenericMatrix,GenericMatrix) - dimension mismatch");
      }
      return result_type(unwary(l), unwary(r));
   }

   template <typename Iterator2>
   typename function_argument<LeftRef>::const_type
   operator() (partial_left, typename function_argument<LeftRef>::const_type l, const Iterator2&) const
   {
      return l;
   }

   template <typename Iterator1>
   typename function_argument<RightRef>::const_type
   operator() (partial_right, const Iterator1&, typename function_argument<RightRef>::const_type r) const
   {
      return r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l+=r;
   }
};

template <typename LeftRef, typename RightRef>
struct sub_impl<LeftRef, RightRef, cons<is_matrix, is_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyMatrix2<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                       typename attrib<typename Unwary<RightRef>::type>::plus_const,
                       BuildBinary<sub> >
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (l.rows() != r.rows() || l.cols() != r.cols())
            throw std::runtime_error("operator-(GenericMatrix,GenericMatrix) - dimension mismatch");
      }
      return result_type(unwary(l), unwary(r));
   }

   template <typename Iterator2>
   typename function_argument<LeftRef>::const_type
   operator() (partial_left, typename function_argument<LeftRef>::const_type l, const Iterator2&) const
   {
      return l;
   }

   template <typename Iterator1>
   typename neg<RightRef>::result_type
   operator() (partial_right, const Iterator1&, typename function_argument<RightRef>::const_type r) const
   {
      neg<RightRef> n;
      return n(r);
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l-=r;
   }
};

template <typename LeftRef, typename RightRef,
          typename Left =typename deref<typename Unwary<LeftRef>::type>::type,
          typename Right=typename deref<typename Unwary<RightRef>::type>::type>
struct matrix_prod_chooser {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef MatrixProduct<typename Diligent<typename Unwary<LeftRef>::type>::type,
                         typename Diligent<typename Unwary<RightRef>::type>::type>
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (l.cols() != r.rows())
            throw std::runtime_error("operator*(GenericMatrix,GenericMatrix) - dimension mismatch");
      }
      return result_type(diligent(unwary(l)), diligent(unwary(r)));
   }
};

template <typename LeftRef, typename RightRef, typename VectorRef, typename Right>
struct matrix_prod_chooser<LeftRef, RightRef, DiagMatrix<VectorRef,true>, Right> {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyMatrix2<RepeatedCol<VectorRef>,
                       typename attrib<typename Unwary<RightRef>::type>::plus_const,
                       BuildBinary<mul> >
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (l.cols() != r.rows())
            throw std::runtime_error("operator*(DiagMatrix,GenericMatrix) - dimension mismatch");
      }
      return result_type(RepeatedCol<VectorRef>(unwary(l).get_vector(), r.cols()), unwary(r));
   }
};

template <typename LeftRef, typename RightRef, typename Left, typename VectorRef>
struct matrix_prod_chooser<LeftRef, RightRef, Left, DiagMatrix<VectorRef,true> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyMatrix2<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                       RepeatedRow<VectorRef>,
                       BuildBinary<mul> >
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (l.cols() != r.rows())
            throw std::runtime_error("operator*(GenericMatrix,DiagMatrix) - dimension mismatch");
      }
      return result_type(unwary(l), RepeatedRow<VectorRef>(unwary(r).get_vector(), l.rows()));
   }
};

template <typename LeftRef, typename RightRef, typename LeftVectorRef, typename RightVectorRef>
struct matrix_prod_chooser<LeftRef, RightRef, DiagMatrix<LeftVectorRef,true>, DiagMatrix<RightVectorRef,true> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyVector2<LeftVectorRef, RightVectorRef, BuildBinary<mul> > prod_diag;
   typedef const DiagMatrix<prod_diag> result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (l.cols() != r.rows())
            throw std::runtime_error("operator*(DiagMatrix,DiagMatrix) - dimension mismatch");
      }
      return result_type(prod_diag(unwary(l).get_vector(), unwary(r).get_vector()));
   }
};

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_matrix, is_matrix> >
   : matrix_prod_chooser<LeftRef,RightRef> {

   void assign(typename lvalue_arg<LeftRef>::type l,
               typename function_argument<RightRef>::const_type r)
   {
      l=l*r;
   }
};

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_matrix, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyVector2<pm::masquerade<Rows, typename attrib<typename Unwary<LeftRef>::type>::plus_const>,
                       constant_value_container<typename Diligent<typename Unwary<RightRef>::type>::type>,
                       BuildBinary<mul> >
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (l.cols() != r.dim())
            throw std::runtime_error("operator*(GenericMatrix,GenericVector) - dimension mismatch");
      }
      return result_type(unwary(l), diligent(unwary(r)));
   }
};

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_vector, is_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyVector2<constant_value_container<typename Diligent<typename Unwary<LeftRef>::type>::type>,
                       pm::masquerade<Cols, typename attrib<typename Unwary<RightRef>::type>::plus_const>,
                       BuildBinary<mul> >
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (l.dim() != r.rows())
            throw std::runtime_error("operator*(GenericVector,GenericMatrix) - dimension mismatch");
      }
      return result_type(diligent(unwary(l)), unwary(r));
   }
};

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_matrix, is_scalar> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyMatrix2<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                       constant_value_matrix<typename Diligent<typename Unwary<RightRef>::type>::type>,
                       BuildBinary<mul> >
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(unwary(l), diligent(unwary(r)));
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l*=r;
   }
};

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_scalar, is_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyMatrix2<constant_value_matrix<typename Diligent<typename Unwary<LeftRef>::type>::type>,
                       typename attrib<typename Unwary<RightRef>::type>::plus_const,
                       BuildBinary<mul> >
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(diligent(unwary(l)), unwary(r));
   }
};

template <typename LeftRef, typename RightRef>
struct div_impl<LeftRef, RightRef, cons<is_matrix, is_scalar> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyMatrix2<typename attrib<typename Unwary<LeftRef>::type>::plus_const, 
                       constant_value_matrix<typename Diligent<typename Unwary<RightRef>::type>::type>,
                       BuildBinary<div> >
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(unwary(l), diligent(unwary(r)));
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l/=r;
   }
};

template <typename LeftRef, typename RightRef>
struct divexact_impl<LeftRef, RightRef, cons<is_matrix, is_scalar> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyMatrix2<typename attrib<typename Unwary<LeftRef>::type>::plus_const, 
                       constant_value_matrix<typename Diligent<typename Unwary<RightRef>::type>::type>,
                       BuildBinary<divexact> >
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(unwary(l), diligent(unwary(r)));
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l.div_exact(r);
   }
};

/* ------------------------------------
 *  operations building block matrices
 * ------------------------------------ */

template <typename LeftRef, typename RightRef>
struct div_impl<LeftRef, RightRef, cons<is_matrix, is_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef RowChain<typename coherent_const<typename Unwary<LeftRef>::type, typename Unwary<RightRef>::type>::first_type,
                    typename coherent_const<typename Unwary<LeftRef>::type, typename Unwary<RightRef>::type>::second_type>
      result_type;
   
   result_type operator() (typename function_argument<LeftRef>::type l,
                           typename function_argument<RightRef>::type r) const
   {
      return result_type(unwary(l), unwary(r));
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      return l/=r;
   }
};

template <typename LeftRef, typename RightRef>
struct div_impl<LeftRef, RightRef, cons<is_matrix, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef SingleRow<typename Unwary<RightRef>::type> Right;
   typedef RowChain<typename coherent_const<typename Unwary<LeftRef>::type, Right>::first_type,
                    typename coherent_const<typename Unwary<LeftRef>::type, Right>::second_type>
      result_type;

   result_type operator() (typename function_argument<LeftRef>::type l,
                           typename function_argument<RightRef>::type r) const
   {
      return result_type(unwary(l), unwary(r));
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l/=r;
   }
};

template <typename LeftRef, typename RightRef>
struct div_impl<LeftRef, RightRef, cons<is_vector, is_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef SingleRow<typename Unwary<LeftRef>::type> Left;
   typedef RowChain<typename coherent_const<Left, typename Unwary<RightRef>::type>::first_type,
                    typename coherent_const<Left, typename Unwary<RightRef>::type>::second_type>
      result_type;

   result_type operator() (typename function_argument<LeftRef>::type l,
                           typename function_argument<RightRef>::type r) const
   {
      return result_type(unwary(l), unwary(r));
   }
};

template <typename LeftRef, typename RightRef>
struct div_impl<LeftRef, RightRef, cons<is_vector, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef SingleRow<typename Unwary<LeftRef>::type>  Left;
   typedef SingleRow<typename Unwary<RightRef>::type> Right;
   typedef RowChain<typename coherent_const<Left, Right>::first_type,
                    typename coherent_const<Left, Right>::second_type>
      result_type;

   result_type operator() (typename function_argument<LeftRef>::type l,
                           typename function_argument<RightRef>::type r) const
   {
      return result_type(unwary(l), unwary(r));
   }
};

template <typename LeftRef, typename RightRef>
struct bitwise_or_impl<LeftRef, RightRef, cons<is_matrix, is_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef ColChain<typename coherent_const<typename Unwary<LeftRef>::type, typename Unwary<RightRef>::type>::first_type,
                    typename coherent_const<typename Unwary<LeftRef>::type, typename Unwary<RightRef>::type>::second_type>
      result_type;

   result_type operator() (typename function_argument<LeftRef>::type l,
                           typename function_argument<RightRef>::type r) const
   {
      return result_type(unwary(l), unwary(r));
   }

   void assign(typename lvalue_arg<LeftRef>::type l,
               typename function_argument<RightRef>::const_type r) const
   {
      l|=r;
   }
};

template <typename LeftRef, typename RightRef>
struct bitwise_or_impl<LeftRef, RightRef, cons<is_matrix, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef SingleCol<typename Unwary<RightRef>::type> Right;
   typedef ColChain<typename coherent_const<typename Unwary<LeftRef>::type, Right>::first_type,
                    typename coherent_const<typename Unwary<LeftRef>::type, Right>::second_type>
      result_type;

   result_type operator() (typename function_argument<LeftRef>::type l,
                           typename function_argument<RightRef>::type r) const
   {
      return result_type(unwary(l), unwary(r));
   }

   void assign(typename lvalue_arg<LeftRef>::type l,
               typename function_argument<RightRef>::const_type r) const
   {
      l|=r;
   }
};

template <typename LeftRef, typename RightRef>
struct bitwise_or_impl<LeftRef, RightRef, cons<is_vector, is_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef SingleCol<typename Unwary<LeftRef>::type> Left;
   typedef ColChain<typename coherent_const<Left, typename Unwary<RightRef>::type>::first_type,
                    typename coherent_const<Left, typename Unwary<RightRef>::type>::second_type>
      result_type;

   result_type operator() (typename function_argument<LeftRef>::type l,
                           typename function_argument<RightRef>::type r) const
   {
      return result_type(unwary(l), unwary(r));
   }
};

} // end namespace operations
namespace operators {

template <typename Matrix1, typename Matrix2> inline
typename operations::add_impl<const Matrix1&, const Matrix2&>::result_type
operator+ (const GenericMatrix<Matrix1>& l, const GenericMatrix<Matrix2>& r)
{
   operations::add_impl<const Matrix1&, const Matrix2&> op;
   return op(concrete(l), concrete(r));
}

template <typename Matrix1, typename Matrix2> inline
typename operations::sub_impl<const Matrix1&, const Matrix2&>::result_type
operator- (const GenericMatrix<Matrix1>& l, const GenericMatrix<Matrix2>& r)
{
   operations::sub_impl<const Matrix1&, const Matrix2&> op;
   return op(concrete(l), concrete(r));
}

template <typename Matrix1, typename Matrix2> inline
bool operator== (const GenericMatrix<Matrix1>& l, const GenericMatrix<Matrix2>& r)
{
   if ((l.rows()==0 || l.cols()==0) && (r.rows()==0 || r.cols()==0)) return true;
   if (l.rows() != r.rows() || l.cols() != r.cols()) return false;
   operations::eq<const typename Unwary<Matrix1>::type&, const typename Unwary<Matrix2>::type&> op;
   return op(l.top(), r.top());
}

template <typename Matrix1, typename Matrix2> inline
typename enable_if<bool, (is_ordered<typename Matrix1::element_type>::value && is_ordered<typename Matrix2::element_type>::value)>::type
operator< (const GenericMatrix<Matrix1>& l, const GenericMatrix<Matrix2>& r)
{
   if ((l.rows()==0 || l.cols()==0) && (r.rows()==0 || r.cols()==0)) return false;
   if (POLYMAKE_DEBUG || !Unwary<Matrix1>::value || !Unwary<Matrix2>::value ) {
      if (l.rows() != r.rows() || l.cols() != r.cols())
         throw std::runtime_error("operator<(GenericMatrix,GenericMatrix) - dimension mismatch");
   }
   operations::lt<const typename Unwary<Matrix1>::type&, const typename Unwary<Matrix2>::type&> op;
   return op(l.top(), r.top());
}

} // end namespace operators

template <typename Matrix, typename Right> inline
typename operations::divexact_impl<const Matrix&, const Right&>::result_type
div_exact(const GenericMatrix<Matrix>& l, const Right& r)
{
   operations::divexact_impl<const Matrix&, const Right&> op;
   return op(concrete(l), r);
}

template <typename Matrix>
struct hash_func<Matrix, is_matrix> : hash_func< ConcatRows<Matrix> > {
   size_t operator() (const Matrix& m)
   {
      return hash_func< ConcatRows<Matrix> >::operator()(concat_rows(m));
   }
};

/** @} */ // end of genericMatrices group

} // end namespace pm

namespace polymake {
   using pm::GenericMatrix;
   using pm::ConcatRows;
   using pm::same_element_matrix;
   using pm::convert_to;
   using pm::zero_matrix;
   using pm::ones_matrix;
   using pm::unit_matrix;
   using pm::diag;
   using pm::SparseMatrix2x2;
}

namespace std {
   template <typename Matrix1, typename Matrix2, typename E> inline
   void swap(pm::GenericMatrix<Matrix1,E>& M1, pm::GenericMatrix<Matrix2,E>& M2)
   {
      M1.top().swap(M2.top());
   }

   // due to silly overloading rules
   template <typename Matrix>
   void swap(pm::ConcatRows<Matrix>& M1, pm::ConcatRows<Matrix>& M2)
   {
      M1.swap(M2);
   }
}

#ifdef POLYMAKE_GENERIC_INCIDENCE_MATRIX_H
# include "polymake/internal/Incidence_and_SparseMatrix.h"
#endif
#endif // POLYMAKE_GENERIC_MATRIX_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
