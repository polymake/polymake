/* Copyright (c) 1997-2019
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

#ifndef POLYMAKE_GENERIC_MATRIX_H
#define POLYMAKE_GENERIC_MATRIX_H

#include "polymake/Vector.h"
#include "polymake/CascadedContainer.h"
#include "polymake/internal/matrix_methods.h"

namespace pm {

/* ------------------------------------------
 *  Matrix masquerade: ConcatRows, ConcatCols
 * ------------------------------------------ */

template <typename TMatrix>
class ConcatRows_default
   : public cascade_impl< ConcatRows_default<TMatrix>,
                          mlist< ContainerTag< Rows<TMatrix> >,
                                 CascadeDepth< int_constant<2> >,
                                 MasqueradedTop > > {
   using base_t = cascade_impl<ConcatRows_default>;

   int size_impl(std::false_type) const { return dim(); }
   int size_impl(std::true_type) const { return base_t::size(); }
public:
   int dim() const
   {
      return this->hidden().rows() * this->hidden().cols();
   }
   int size() const
   {
      return size_impl(bool_constant<check_container_feature<TMatrix, sparse>::value>());
   }
};

template <typename TMatrix>
class ConcatRows
   : public ConcatRows_default<TMatrix>
   , public GenericVector< ConcatRows<TMatrix>,
                           typename object_traits<typename TMatrix::value_type>::persistent_type > {
protected:
   ~ConcatRows();
public:
   ConcatRows& operator= (const ConcatRows& other) { return ConcatRows::generic_type::operator=(other); }
   using ConcatRows::generic_type::operator=;
   using ConcatRows_default<TMatrix>::dim;
};

template <typename TMatrix>
class ConcatCols
   : public ConcatRows< Transposed<TMatrix> > {};

template <typename TMatrix>
struct spec_object_traits< ConcatRows<TMatrix> >
   : spec_object_traits<is_container> {
   static const bool is_always_const=object_traits<TMatrix>::is_always_const;
   static const int is_resizeable=0;
   typedef TMatrix masquerade_for;
};

template <typename TMatrix>
struct spec_object_traits< ConcatCols<TMatrix> >
   : spec_object_traits< ConcatRows<TMatrix> > {};

template <typename TMatrix>
struct check_container_feature<ConcatRows<TMatrix>, sparse>
   : check_container_feature<TMatrix, sparse> {};

template <typename TMatrix>
struct check_container_feature<ConcatRows<TMatrix>, pure_sparse>
   : check_container_feature<TMatrix, pure_sparse> {};

template <typename TMatrix>
struct check_container_feature<ConcatCols<TMatrix>, sparse>
   : check_container_feature<TMatrix, sparse> {};

template <typename TMatrix>
struct check_container_feature<ConcatCols<TMatrix>, pure_sparse>
   : check_container_feature<TMatrix, pure_sparse> {};

template <typename T>
struct masquerade_as_ConcatRows {
   using type = masquerade<ConcatRows, T>;
};
template <typename T>
struct masquerade_as_ConcatCols {
   using type = masquerade<ConcatCols, T>;
};

/* ----------------
 *  Generic matrix
 * ---------------- */

/** @defgroup genericMatrices Generic Matrices
 *  @{
 */

template <typename TMatrix, typename E = typename TMatrix::element_type>
class GenericMatrix;

template <typename T, typename... E>
using is_generic_matrix = is_derived_from_instance_of<pure_type_t<T>, GenericMatrix, E...>;

template <typename E> class Matrix;
template <typename E, typename is_symmetric=NonSymmetric> class SparseMatrix;
template <typename E> class SparseMatrix2x2;
template <typename E> class SameElementMatrix;

template <typename MatrixRef, typename Operation> class LazyMatrix1;
template <typename MatrixRef1, typename MatrixRef2, typename Operation> class LazyMatrix2;
template <typename VectorRef> class RepeatedRow;
template <typename VectorRef> class RepeatedCol;

namespace internal {

template <typename LeftRef, typename RightRef,
          typename Left =typename deref<unwary_t<LeftRef>>::type,
          typename Right=typename deref<unwary_t<RightRef>>::type>
struct matrix_product;

}

/** @file GenericMatrix.h
    @class GenericMatrix
    @brief @ref generic "Generic type" for @ref matrix_sec "matrices"
 */
template <typename TMatrix, typename E>
class GenericMatrix
   : public Generic<TMatrix>
   , public matrix_methods<TMatrix, E> {
   template <typename, typename> friend class GenericMatrix;

protected:
   GenericMatrix() {}
   GenericMatrix(const GenericMatrix&) {}

public:
   static const bool
      is_sparse= check_container_feature<TMatrix, sparse>::value,
      is_symmetric=check_container_feature<TMatrix, Symmetric>::value,
      is_skew=check_container_feature<TMatrix, SkewSymmetric>::value,
      is_nonsymmetric=!is_symmetric && !is_skew,
      is_flat=check_container_feature<TMatrix, FlatStorage>::value;

   using sym_discr = typename matrix_symmetry_type<TMatrix>::type;
   using typename Generic<TMatrix>::top_type;
   using persistent_type = std::conditional_t<is_sparse, SparseMatrix<E, sym_discr>, Matrix<E>>;
   using persistent_nonsymmetric_type = std::conditional_t<is_sparse, SparseMatrix<E>, Matrix<E>>;
   using persistent_dense_type = Matrix<E>;
   using generic_type = GenericMatrix;

   template <typename Other>
   static constexpr bool is_expandable_by()
   {
      return is_nonsymmetric && object_traits<TMatrix>::is_resizeable!=0 &&
             can_initialize<typename Other::element_type, E>::value;
   }

   template <typename Matrix2>
   static constexpr bool compatible_symmetry_types()
   {
      return (is_nonsymmetric || std::is_same<sym_discr, typename Matrix2::sym_discr>::value) &&
             isomorphic_types<E, typename Matrix2::element_type>::value;
   }

   template <typename Matrix2>
   static constexpr bool is_assignable_from()
   {
      return !spec_object_traits<top_type>::is_always_const &&
             compatible_symmetry_types<Matrix2>() &&
             can_assign_to<typename Matrix2::element_type, E>::value;
   }

protected:
   template <typename TMatrix2>
   void assign_impl(const GenericMatrix<TMatrix2>& m, std::true_type, NonSymmetric)
   {
      concat_rows(*this) = concat_rows(convert_lazily<E>(m));
   }

   template <typename TMatrix2>
   void assign_impl(const GenericMatrix<TMatrix2>& m, std::false_type, NonSymmetric)
   {
      copy_range(pm::rows(m).begin(), entire(pm::rows(*this)));
   }

   template <typename TMatrix2>
   void assign_impl(const GenericMatrix<TMatrix2>& m, std::false_type, Symmetric)
   {
      auto src=pm::rows(m).begin();
      for (auto r_i=entire(pm::rows(*this)); !r_i.at_end(); ++r_i, ++src)
         r_i->assign(*src, sym_discr());
   }

   template <typename TMatrix2>
   void assign(const GenericMatrix<TMatrix2>& m)
   {
      assign_impl(m, bool_constant<is_flat && check_container_feature<TMatrix2, FlatStorage>::value>(), sym_discr());
   }

   template <typename Operation>
   void assign_op_impl(const Operation& op, std::true_type, NonSymmetric)
   {
      concat_rows(*this).assign_op(op);
   }

   template <typename Operation>
   void assign_op_impl(const Operation& op, std::false_type, NonSymmetric)
   {
      for (auto r_i=entire(pm::rows(*this)); !r_i.at_end(); ++r_i)
         r_i->assign_op(op);
   }

   template <typename Operation>
   void assign_op_impl(const Operation& op, std::false_type, Symmetric)
   {
      for (auto r_i=entire(pm::rows(*this)); !r_i.at_end(); ++r_i)
         r_i->assign_op(op, sym_discr());
   }

   template <typename TMatrix2, typename Operation>
   void assign_op_impl(const GenericMatrix<TMatrix2>& m, const Operation& op, std::true_type, NonSymmetric)
   {
      concat_rows(*this).assign_op(concat_rows(m), op);
   }

   template <typename TMatrix2, typename Operation>
   void assign_op_impl(const GenericMatrix<TMatrix2>& m, const Operation& op, std::false_type, NonSymmetric)
   {
      auto src=pm::rows(m).begin();
      for (auto r_i=entire(pm::rows(*this)); !r_i.at_end(); ++r_i, ++src)
         r_i->assign_op(*src, op);
   }

   template <typename TMatrix2, typename Operation>
   void assign_op_impl(const GenericMatrix<TMatrix2>& m, const Operation& op, std::false_type, Symmetric)
   {
      auto src=pm::rows(m).begin();
      for (auto r_i=entire(pm::rows(*this)); !r_i.at_end(); ++r_i, ++src)
         r_i->assign_op(*src, op, sym_discr());
   }

   template <typename E2>
   void fill_impl(const E2& x, std::true_type)
   {
      concat_rows(*this).fill(x);
   }

   template <typename E2>
   void fill_impl(const E2& x, std::false_type)
   {
      for (auto r_i=entire(pm::rows(*this)); !r_i.at_end(); ++r_i)
         r_i->fill(x);
   }

   bool non_zero(std::true_type) const
   {
      return !is_zero(concat_rows(*this));
   }

   bool non_zero(std::false_type) const
   {
      return !entire(attach_selector(pm::rows(*this), BuildUnary<operations::non_zero>())).at_end();
   }

   template <typename TMatrix2>
   constexpr bool trivial_assignment(const GenericMatrix<TMatrix2,E>&) const { return false; }

   constexpr bool trivial_assignment(const GenericMatrix& other) const { return this==&other; }

   template <typename TMatrix2>
   bool equal(const GenericMatrix<TMatrix2>& b, std::true_type) const
   {
      return concat_rows(*this)==concat_rows(b);
   }

   template <typename TMatrix2>
   bool equal(const GenericMatrix<TMatrix2>& b, std::false_type) const
   {
      return operations::cmp_unordered()(rows(*this), rows(b)) == cmp_eq;
   }

public:
   /**
      Persistent matrix objects have after the assignment the same dimensions as 
      the right hand side operand. Alias objects, such as matrix minor or block matrix, 
      cannot be resized, thus must have the same dimensions as on the right hand side. 
    */
   top_type& operator= (const GenericMatrix& m)
   {
      if (!trivial_assignment(m)) {
         if (!object_traits<TMatrix>::is_resizeable && (POLYMAKE_DEBUG || is_wary<TMatrix>())) {
            if (this->rows() != m.rows() || this->cols() != m.cols())
               throw std::runtime_error("GenericMatrix::operator= - dimension mismatch");
         }
         this->top().assign(m.top());
      }
      return this->top();
   }

   template <typename TMatrix2, typename=typename std::enable_if<is_assignable_from<TMatrix2>()>::type>
   top_type& operator= (const GenericMatrix<TMatrix2>& m)
   {
      if (!object_traits<TMatrix>::is_resizeable && (POLYMAKE_DEBUG || is_wary<TMatrix>())) {
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
   template <typename TMatrix2>
   void swap(GenericMatrix<TMatrix2, E>& m)
   {
      if (trivial_assignment(m)) return;

      if (POLYMAKE_DEBUG || is_wary<TMatrix>() || is_wary<TMatrix2>()) {
         if (this->rows() != m.rows() || this->cols() != m.cols())
            throw std::runtime_error("GenericMatrix::swap - dimension mismatch");
      }
      swap_ranges(entire(pm::rows(*this)), pm::rows(m).begin());
   }

   template <typename Operation>
   void assign_op(const Operation& op)
   {
      assign_op_impl(op, bool_constant<is_flat>(), sym_discr());
   }

   template <typename Matrix2, typename Operation>
   void assign_op(const GenericMatrix<Matrix2>& m, const Operation& op)
   {
      assign_op_impl(m.top(), op, bool_constant<is_flat && Matrix2::is_flat>(), sym_discr());
   }

   /**
      Fill with given value without changing the dimensions. 
      x can be of arbitrary type assignable to the type E2. 
    */
   template <typename E2>
   void fill(const E2& x)
   {
      this->top().fill_impl(x, bool_constant<is_flat>());
   }

protected:
   // TODO: enable variants with persistent_type&&
   // when defined<Operation> is properly implemented
   template <bool now, bool>
   using temp_ignore = bool_constant<now>;

   template <typename T>
   using is_this = std::is_same<generic_type, typename unwary_t<pure_type_t<T>>::generic_type>;

   template <typename Left, typename Right, typename Operation, typename=void>
   struct lazy_op {};

   template <typename Left, typename Operation>
   struct lazy_op<Left, void, Operation,
                  std::enable_if_t<is_this<Left>::value &&
                                   !std::is_same<Left, persistent_type>::value>> {
      using m_type = LazyMatrix1<add_const_t<unwary_t<Left>>, Operation>;
   };

   template <typename Operation>
   struct lazy_op<persistent_type, void, Operation, void> {
      using r_type = persistent_type&&;
   };

   // TODO: && is_defined<Operation>
   template <typename Left, typename Right, typename Operation>
   struct lazy_op<Left, Right, Operation,
                  std::enable_if_t<isomorphic_types<E, pure_type_t<Right>>::value &&
                                   is_this<Left>::value &&
                                   temp_ignore<true, !std::is_same<Left, persistent_type>::value>::value>> {
      using scalar_type = SameElementMatrix<diligent_ref_t<unwary_t<Right>>>;
      using m_s_type = LazyMatrix2<add_const_t<unwary_t<Left>>, scalar_type, Operation>;

      static m_s_type make(Left&& l, Right&& r)
      {
         return m_s_type(unwary(std::forward<Left>(l)), scalar_type(diligent(unwary(std::forward<Right>(r)))));
      }
   };

#if 0
   // TODO enable when temp_ignore goes
   template <typename Right, typename Operation>
   struct lazy_op<persistent_type, Right, Operation,
                  std::enable_if_t<isomorphic_types<E, pure_type_t<Right>>::value>> {
      using r_s_type = persistent_type&&;
   };
#endif

   template <typename Left, typename Right, typename Operation>
   struct lazy_op<Left, Right, Operation,
                  std::enable_if_t<isomorphic_types<E, pure_type_t<Left>>::value &&
                                   is_this<Right>::value &&
                                   temp_ignore<true, !std::is_same<Right, persistent_type>::value>::value>> {
      using scalar_type = SameElementMatrix<diligent_ref_t<unwary_t<Left>>>;
      using s_m_type = LazyMatrix2<scalar_type, add_const_t<unwary_t<Right>>, Operation>;

      static s_m_type make(Left&& l, Right&& r)
      {
         return s_m_type(scalar_type(diligent(unwary(std::forward<Left>(l)))), unwary(std::forward<Right>(r)));
      }
   };

#if 0
   // TODO enable when temp_ignore goes
   template <typename Left, typename Operation>
   struct lazy_op<Left, persistent_type, Operation,
                  std::enable_if_t<isomorphic_types<E, pure_type_t<Left>>::value>> {
      using s_r_type = persistent_type&&;
   };
#endif

   // TODO: && is_defined<Operation>
   template <typename Left, typename Right, typename Operation>
   struct lazy_op<Left, Right, Operation,
                  std::enable_if_t<is_this<Left>::value &&
                                   is_generic_vector<Right>::value &&
                                   isomorphic_types<E, typename pure_type_t<Right>::element_type>::value>> {
      using vec_type = same_value_container<diligent_ref_t<unwary_t<Right>>>;
      using m_v_type = LazyVector2<masquerade<Rows, add_const_t<unwary_t<Left>>>, vec_type, Operation>;

      static m_v_type make(Left&& l, Right&& r)
      {
         return m_v_type(unwary(std::forward<Left>(l)), vec_type(diligent(unwary(std::forward<Right>(r)))));
      }
   };

   template <typename Left, typename Right, typename Operation>
   struct lazy_op<Left, Right, Operation,
                  std::enable_if_t<is_this<Right>::value &&
                                   is_generic_vector<Left>::value &&
                                   isomorphic_types<E, typename pure_type_t<Left>::element_type>::value>> {
      using vec_type = same_value_container<diligent_ref_t<unwary_t<Left>>>;
      using v_m_type = LazyVector2<vec_type, masquerade<Cols, add_const_t<unwary_t<Right>>>, Operation>;

      static v_m_type make(Left&& l, Right&& r)
      {
         return v_m_type(vec_type(diligent(unwary(std::forward<Left>(l)))), std::forward<Right>(r));
      }
   };

   // TODO: && is_defined<Operation>
   template <typename Left, typename Right, typename Operation>
   struct lazy_op<Left, Right, Operation,
                  std::enable_if_t<!std::is_same<Operation, BuildBinary<operations::mul>>::value &&
                                   is_this<Left>::value &&
                                   temp_ignore<true, !std::is_same<Left, persistent_type>::value>::value &&
                                   is_generic_matrix<Right>::value &&
                                   isomorphic_types<E, typename pure_type_t<Right>::element_type>::value>> {
      using m_m_type = LazyMatrix2<add_const_t<unwary_t<Left>>, add_const_t<unwary_t<Right>>, Operation>;
   };

   template <typename Right, typename Operation>
   struct lazy_op<persistent_type, Right, Operation,
                  std::enable_if_t<!std::is_same<Operation, BuildBinary<operations::mul>>::value &&
                                   temp_ignore<false, std::is_same<TMatrix, persistent_type>::value>::value &&
                                   is_generic_matrix<Right>::value &&
                                   isomorphic_types<E, typename pure_type_t<Right>::element_type>::value>> {
      using r_m_type = persistent_type&&;
   };

   template <typename Left, typename Operation>
   struct lazy_op<Left, persistent_type, Operation,
                  std::enable_if_t<!std::is_same<Operation, BuildBinary<operations::mul>>::value &&
                                   is_generic_matrix<Left>::value &&
                                   temp_ignore<false, !std::is_same<Left, persistent_type>::value>::value &&
                                   isomorphic_types<E, typename pure_type_t<Left>::element_type>::value>> {
      using m_r_type = persistent_type&&;
   };

   // TODO: && is_defined<mul>
   template <typename Left, typename Right>
   struct lazy_op<Left, Right, BuildBinary<operations::mul>,
                  std::enable_if_t<is_this<Left>::value &&
                                   is_generic_matrix<Right>::value &&
                                   isomorphic_types<E, typename pure_type_t<Right>::element_type>::value &&
                                   cleanOperations::can<cleanOperations::mul, E, typename pure_type_t<Right>::element_type>::value>>
      : public internal::matrix_product<Left, Right> {};

#define PmCheckMatrixDim(Left, l, Right, r, sign) \
   if (POLYMAKE_DEBUG || is_wary<Left>() || is_wary<Right>()) \
      if (l.rows() != r.rows() || l.cols() != r.cols()) \
         throw std::runtime_error("GenericMatrix::operator" sign " - dimension mismatch")

public:
   /// negate
   template <typename Op>
   friend
   typename lazy_op<Op, void, BuildUnary<operations::neg>>::m_type
   operator- (Op&& m)
   {
      return typename lazy_op<Op, void, BuildUnary<operations::neg>>::m_type(unwary(std::forward<Op>(m)));
   }

   template <typename Op>
   friend
   typename lazy_op<Op, void, BuildUnary<operations::neg>>::r_type
   operator- (Op&& m)
   {
      return std::move(m.negate());
   }

   /// negate elements in place
   top_type& negate()
   {
      this->top().assign_op(BuildUnary<operations::neg>());
      return this->top();
   }

   /// add a matrix
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::add>>::m_m_type
   operator+ (Left&& l, Right&& r)
   {
      PmCheckMatrixDim(Left, l, Right, r, "+");
      return typename lazy_op<Left, Right, BuildBinary<operations::add>>::m_m_type(unwary(std::forward<Left>(l)),
                                                                                   unwary(std::forward<Right>(r)));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::add>>::r_m_type
   operator+ (Left&& l, Right&& r)
   {
      PmCheckMatrixDim(Left, l, Right, r, "+");
      return std::move(l += r);
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::add>>::m_r_type
   operator+ (Left&& l, Right&& r)
   {
      PmCheckMatrixDim(Left, l, Right, r, "+");
      return std::move(r += l);
   }

   template <typename Right, typename E2>
   std::enable_if_t<compatible_symmetry_types<Right>() &&
                    isomorphic_types<E, E2>::value &&
                    cleanOperations::can<cleanOperations::add, E, E2>::value, top_type&>
   operator+= (const GenericMatrix<Right, E2>& r)
   {
      PmCheckMatrixDim(TMatrix, (*this), Right, r, "+=");
      this->top().assign_op(r.top(), BuildBinary<operations::add>());
      return this->top();
   }

   /// subtract a matrix
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::sub>>::m_m_type
   operator- (Left&& l, Right&& r)
   {
      PmCheckMatrixDim(Left, l, Right, r, "-");
      return typename lazy_op<Left, Right, BuildBinary<operations::sub>>::m_m_type(unwary(std::forward<Left>(l)),
                                                                                   unwary(std::forward<Right>(r)));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::sub>>::r_m_type
   operator- (Left&& l, Right&& r)
   {
      PmCheckMatrixDim(Left, l, Right, r, "-");
      return std::move(l -= r);
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::sub>>::m_r_type
   operator- (Left&& l, Right&& r)
   {
      PmCheckMatrixDim(Left, l, Right, r, "-");
      return std::move(r.negate() += l);
   }

   template <typename Right, typename E2>
   std::enable_if_t<compatible_symmetry_types<Right>() &&
                    isomorphic_types<E, E2>::value &&
                    cleanOperations::can<cleanOperations::sub, E, E2>::value, top_type&>
   operator-= (const GenericMatrix<Right, E2>& r)
   {
      PmCheckMatrixDim(TMatrix, (*this), Right, r, "-=");
      this->top().assign_op(r.top(), BuildBinary<operations::sub>());
      return this->top();
   }

#undef PmCheckMatrixDim

   /// multiply with a scalar
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::mul>>::m_s_type
   operator* (Left&& l, Right&& r)
   {
      return lazy_op<Left, Right, BuildBinary<operations::mul>>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::mul>>::s_m_type
   operator* (Left&& l, Right&& r)
   {
      return lazy_op<Left, Right, BuildBinary<operations::mul>>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::mul>>::r_s_type
   operator* (Left&& l, Right&& r)
   {
      return std::move(l *= r);
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::mul>>::s_r_type
   operator* (Left&& l, Right&& r)
   {
      return std::move(r.mul_from_left(l));
   }

   /// multiply with a vector
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::mul>>::m_v_type
   operator* (Left&& l, Right&& r)
   {
      if (POLYMAKE_DEBUG || is_wary<Left>() || is_wary<Right>()) {
         if (l.cols() != r.dim())
            throw std::runtime_error("GenericMatrix::operator* - dimension mismatch");
      }
      return lazy_op<Left, Right, BuildBinary<operations::mul>>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::mul>>::v_m_type
   operator* (Left&& l, Right&& r)
   {
      if (POLYMAKE_DEBUG || is_wary<Left>() || is_wary<Right>()) {
         if (l.dim() != r.rows())
            throw std::runtime_error("GenericMatrix::operator* - dimension mismatch");
      }
      return lazy_op<Left, Right, BuildBinary<operations::mul>>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::mul>>::m_m_type
   operator* (Left&& l, Right&& r)
   {
      if (POLYMAKE_DEBUG || is_wary<Left>() || is_wary<Right>()) {
         if (l.cols() != r.rows())
            throw std::runtime_error("GenericMatrix::operator* - dimension mismatch");
      }
      return internal::matrix_product<Left, Right>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Right>
   std::enable_if_t<isomorphic_types<E, Right>::value &&
                    cleanOperations::can<cleanOperations::mul, E, Right>::value, top_type&>
   operator*= (const Right& r)
   {
      if (!is_zero(r))
         this->top().assign_op(SameElementMatrix<const Right&>(r), BuildBinary<operations::mul>());
      else
         fill(r);
      return this->top();
   }

   template <typename Left>
   std::enable_if_t<isomorphic_types<E, Left>::value &&
                    cleanOperations::can<cleanOperations::mul, Left, E>::value, top_type&>
   mul_from_left(const Left& l)
   {
      if (!is_zero(l))
         this->top().assign_op(SameElementMatrix<const Left&>(l), BuildBinary<operations::mul_from_left>());
      else
         fill(l);
      return this->top();
   }

   /// divide by a scalar  
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::div>>::m_s_type
   operator/ (Left&& l, Right&& r)
   {
      return lazy_op<Left, Right, BuildBinary<operations::div>>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::div>>::r_s_type
   operator/ (Left&& l, Right&& r)
   {
      std::move(l /= r);
   }

   template <typename Right>
   std::enable_if_t<isomorphic_types<E, Right>::value &&
                    cleanOperations::can<cleanOperations::div, E, Right>::value, top_type&>
   operator/= (const Right& r)
   {
      this->top().assign_op(SameElementMatrix<const Right&>(r), BuildBinary<operations::div>());
      return this->top();
   }

   /// divide without residue by a scalar
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::divexact>>::m_s_type
   div_exact(Left&& l, Right&& r)
   {
      return lazy_op<Left, Right, BuildBinary<operations::divexact>>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::divexact>>::r_s_type
   div_exact(Left&& l, Right&& r)
   {
      std::move(l.div_exact(r));
   }

   template <typename Right>
   std::enable_if_t<isomorphic_types<E, Right>::value &&
                    cleanOperations::can<cleanOperations::div, E, Right>::value, top_type&>
   div_exact(const Right& r)
   {
      this->top().assign_op(SameElementMatrix<const Right&>(r), BuildBinary<operations::divexact>());
      return this->top();
   }

protected:
   template <typename Left, typename Right, typename Rowwise, typename=void>
   struct block_matrix {};
   using rowwise = std::true_type;
   using columnwise = std::false_type;

   template <typename Left, typename Right, typename Rowwise>
   struct block_matrix<Left, Right, Rowwise, std::enable_if_t<is_this<Left>::value &&
                                                              is_generic_matrix<Right, E>::value>> {
      using m_m_type = typename chain_compose<BlockMatrix, true, Rowwise>::template with<unwary_t<Left>, unwary_t<Right>>;
   };

   template <typename Left, typename Right, typename Rowwise>
   struct block_matrix<Left, Right, Rowwise, std::enable_if_t<is_this<Left>::value &&
                                                              is_generic_vector<Right, E>::value>> {
      using line_type = std::conditional_t<Rowwise::value, RepeatedRow<unwary_t<Right>>, RepeatedCol<unwary_t<Right>>>;
      using m_v_type = typename chain_compose<BlockMatrix, true, Rowwise>::template with<unwary_t<Left>, line_type>;

      static m_v_type make(Left&& l, Right&& r)
      {
         return m_v_type(unwary(std::forward<Left>(l)), line_type(unwary(std::forward<Right>(r)), 1));
      }
   };

   template <typename Left, typename Right, typename Rowwise>
   struct block_matrix<Left, Right, Rowwise, std::enable_if_t<is_generic_vector<Left, E>::value &&
                                                              is_this<Right>::value>> {
      using line_type = std::conditional_t<Rowwise::value, RepeatedRow<unwary_t<Left>>, RepeatedCol<unwary_t<Left>>>;
      using v_m_type = typename chain_compose<BlockMatrix, true, Rowwise>::template with<line_type, unwary_t<Right>>;

      static v_m_type make(Left&& l, Right&& r)
      {
         return v_m_type(line_type(unwary(std::forward<Left>(l)), 1), unwary(std::forward<Right>(r)));
      }
   };

public:
   template <typename Left, typename Right> friend
   typename block_matrix<Left, Right, rowwise>::m_m_type operator/ (Left&& l, Right&& r)
   {
      return typename block_matrix<Left, Right, rowwise>::m_m_type(unwary(std::forward<Left>(l)),
                                                                   unwary(std::forward<Right>(r)));
   }

   template <typename Left, typename Right> friend
   typename block_matrix<Left, Right, rowwise>::m_v_type operator/ (Left&& l, Right&& r)
   {
      return block_matrix<Left, Right, rowwise>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right> friend
   typename block_matrix<Left, Right, rowwise>::v_m_type operator/ (Left&& l, Right&& r)
   {
      return block_matrix<Left, Right, rowwise>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right> friend
   typename block_matrix<Left, Right, columnwise>::m_m_type operator| (Left&& l, Right&& r)
   {
      return typename block_matrix<Left, Right, columnwise>::m_m_type(unwary(std::forward<Left>(l)),
                                                                      unwary(std::forward<Right>(r)));
   }

   template <typename Left, typename Right> friend
   typename block_matrix<Left, Right, columnwise>::m_v_type operator| (Left&& l, Right&& r)
   {
      return block_matrix<Left, Right, columnwise>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right> friend
   typename block_matrix<Left, Right, columnwise>::v_m_type operator| (Left&& l, Right&& r)
   {
      return block_matrix<Left, Right, columnwise>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   /// append rows of another matrix
   template <typename TMatrix2>
   typename std::enable_if<is_expandable_by<TMatrix2>(), top_type&>::type
   operator/= (const GenericMatrix<TMatrix2>& m)
   {
      if (m.rows()) {
         if (this->rows()) {
            if (POLYMAKE_DEBUG || is_wary<TMatrix>() || is_wary<TMatrix2>()) {
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

   /// append a vector as a row
   template <typename TVector>
   typename std::enable_if<is_expandable_by<TVector>(), top_type&>::type
   operator/= (const GenericVector<TVector>& v)
   {
      if (this->rows()) {
         if (POLYMAKE_DEBUG || is_wary<TMatrix>() || is_wary<TVector>()) {
            if (this->cols() != v.dim())
               throw std::runtime_error("GenericMatrix::operator/= - dimension mismatch");
         }
         this->top().append_row(v.top());
      } else {
         this->top().assign(vector2row(v));
      }
      return this->top();
   }

   /// append columns of another matrix
   template <typename TMatrix2>
   std::enable_if_t<is_expandable_by<TMatrix2>(), top_type&>
   operator|= (const GenericMatrix<TMatrix2>& m)
   {
      if (m.cols()) {
         if (this->cols()) {
            if (POLYMAKE_DEBUG || is_wary<TMatrix>() || is_wary<TMatrix2>()) {
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

   /// append a vector as a column
   template <typename TVector>
   std::enable_if_t<is_expandable_by<TVector>(), top_type&>
   operator|= (const GenericVector<TVector>& v)
   {
      if (this->cols()) {
         if (POLYMAKE_DEBUG || is_wary<TMatrix>() || is_wary<TVector>()) {
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
      return non_zero(bool_constant<is_flat>());
   }

   template <typename TMatrix2, typename E2>
   std::enable_if_t<are_comparable<E, E2>::value, bool>
   operator== (const GenericMatrix<TMatrix2, E2>& m2) const
   {
      if (this->rows() != m2.rows() || this->cols() != m2.cols()) return false;
      return equal(m2, bool_constant<is_flat && check_container_feature<TMatrix2, FlatStorage>::value>());
   }

   template <typename TMatrix2, typename E2>
   std::enable_if_t<are_comparable<E, E2>::value, bool>
   operator!= (const GenericMatrix<TMatrix2, E2>& m2) const
   {
      return !(*this == m2);
   }

private:
   template <typename MatrixRef>
   static auto make_diagonal(MatrixRef&& matrix, int i, bool anti)
   {
      const int r = matrix.rows(),
                c = matrix.cols();
      if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
         if ((i>0 ? i>=r : -i >= c) && i!=0)
            throw std::runtime_error("GenericMatrix::diagonal/anti_diagonal - index out of range");
      }
      using result_type = IndexedSlice<masquerade<ConcatRows, MatrixRef>, const series>;

      const int start = anti ? (i>0 ? (i+1)*c-1 : c+i-1) : (i>0 ? i*c : -i);
      const int size = i>0 ? std::min(r-i, c) : std::min(r, c+i);
      const int step = anti ? c-1 : c+1;
      return result_type(std::forward<MatrixRef>(matrix), series(start, size, step));
   }

public:
   /// @param i==0: main diagonal; i>0: i-th diagonal below the main; i<0: (-i)-th above the main
   // gcc 5 can't digest auto here
   IndexedSlice<masquerade<ConcatRows, const typename Unwary<TMatrix>::type&>, const series>
   diagonal(int i=0) const &
   {
      return make_diagonal(unwary(*this), i, false);
   }

   // gcc 5 can't digest auto here
   IndexedSlice<masquerade<ConcatRows, typename Unwary<TMatrix>::type&>, const series>
   diagonal(int i=0) &
   {
      return make_diagonal(unwary(*this), i, false);
   }

   // gcc 5 can't digest auto here
   IndexedSlice<masquerade<ConcatRows, typename Unwary<TMatrix>::type>, const series>
   diagonal(int i=0) &&
   {
      return make_diagonal(unwary(std::move(*this)), i, false);
   }

   /// @param i==0: main anti-diagonal; i>0: i-th diagonal below the main; i<0: (-i)-th above the main
   // gcc 5 can't digest auto here
   IndexedSlice<masquerade<ConcatRows, const typename Unwary<TMatrix>::type&>, const series>
   anti_diagonal(int i=0) const &
   {
      return make_diagonal(unwary(*this), i, true);
   }

   // gcc 5 can't digest auto here
   IndexedSlice<masquerade<ConcatRows, typename Unwary<TMatrix>::type&>, const series>
   anti_diagonal(int i=0) &
   {
      return make_diagonal(unwary(*this), i, true);
   }

   // gcc 5 can't digest auto here
   IndexedSlice<masquerade<ConcatRows, typename Unwary<TMatrix>::type>, const series>
   anti_diagonal(int i=0) &&
   {
      return make_diagonal(unwary(std::move(*this)), i, true);
   }

protected:
   template <typename Line, typename E2>
   static void multiply_with2x2(Line&& l_i, Line&& l_j,
                                const E2& a_ii, const E2& a_ij, const E2& a_ji, const E2& a_jj, std::false_type)
   {
      auto e_j=l_j.begin();
      for (auto e_i=entire(l_i); !e_i.at_end(); ++e_i, ++e_j) {
         const E x_i= (*e_i)*a_ii + (*e_j)*a_ij;
         *e_j =       (*e_i)*a_ji + (*e_j)*a_jj;
         *e_i =  x_i;
      }
   }

   template <typename Line, typename E2>
   static void multiply_with2x2(Line&& l_i, Line&& l_j,
                                const E2& a_ii, const E2& a_ij, const E2& a_ji, const E2& a_jj, std::true_type)
   {
      auto e_i=l_i.begin(), e_j=l_j.begin();
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
      if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
         if (U.i<0 || U.j<0 || U.i >= this->rows() || U.j >= this->rows())
            throw std::runtime_error("GenericMatrix::multiply_from_left - dimension mismatch");
      }
      multiply_with2x2(this->row(U.i), this->row(U.j),
                       U.a_ii, U.a_ij, U.a_ji, U.a_jj, bool_constant<is_sparse>());
   }

   void multiply_from_left(const Transposed< SparseMatrix2x2<E> >& U)
   {
      if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
         if (U.i<0 || U.j<0 || U.i >= this->rows() || U.j >= this->rows())
            throw std::runtime_error("GenericMatrix::multiply_from_left - dimension mismatch");
      }
      multiply_with2x2(this->row(U.i), this->row(U.j),
                       U.a_ii, U.a_ji, U.a_ij, U.a_jj, bool_constant<is_sparse>());
   }

   void multiply_from_right(const SparseMatrix2x2<E>& U)
   {
      if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
         if (U.i<0 || U.j<0 || U.i >= this->cols() || U.j >= this->cols())
            throw std::runtime_error("GenericMatrix::multiply_from_right - dimension mismatch");
      }
      multiply_with2x2(this->col(U.i), this->col(U.j),
                       U.a_ii, U.a_ji, U.a_ij, U.a_jj, bool_constant<is_sparse>());
   }

   void multiply_from_right(const Transposed< SparseMatrix2x2<E> >& U)
   {
      if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
         if (U.i<0 || U.j<0 || U.i >= this->cols() || U.j >= this->cols())
            throw std::runtime_error("GenericMatrix::multiply_from_right - dimension mismatch");
      }
      multiply_with2x2(this->col(U.i), this->col(U.j),
                       U.a_ii, U.a_ij, U.a_ji, U.a_jj, bool_constant<is_sparse>());
   }

   template <typename Result>
   struct rebind_generic {
      typedef GenericMatrix<Result, E> type;
   };

#if POLYMAKE_DEBUG
   void dump() const __attribute__((used)) { cerr << this->top() << std::flush; }
#endif
};

struct is_matrix;

template <typename TMatrix, typename E>
struct spec_object_traits< GenericMatrix<TMatrix, E> >
   : spec_or_model_traits<TMatrix, is_container> {
   static const int dimension=2, is_resizeable= spec_or_model_traits<TMatrix, is_container>::is_resizeable ? 2 : 0;
   static const bool allow_sparse=true;
   typedef is_matrix generic_tag;

   static bool is_zero(const TMatrix& m) { return !m.non_zero(); }
};


template <typename Result, typename TVector, typename E>
struct generic_of_repeated_line<Result, GenericVector<TVector, E>> {
   using type = GenericMatrix<Result, E>;
};

template <typename TMatrix, typename E>
struct concat_lines_op<GenericMatrix<TMatrix, E>> {
   using type = polymake::operations::concat_tuple<VectorChain>;
};


template <typename TMatrix>
ConcatRows<unwary_t<TMatrix>>&
concat_rows(GenericMatrix<TMatrix>& m)
{
   return reinterpret_cast<ConcatRows<unwary_t<TMatrix>>&>(m.top());
}

template <typename TMatrix>
const ConcatRows<unwary_t<TMatrix>>&
concat_rows(const GenericMatrix<TMatrix>& m)
{
   return reinterpret_cast<const ConcatRows<unwary_t<TMatrix>>&>(m.top());
}

template <typename TMatrix>
ConcatRows<unwary_t<TMatrix>>&
concat_rows(GenericMatrix<TMatrix>&& m)
{
   return reinterpret_cast<ConcatRows<unwary_t<TMatrix>>&>(m.top());
}

template <typename TMatrix>
ConcatCols<unwary_t<TMatrix>>&
concat_cols(GenericMatrix<TMatrix>& m)
{
   return reinterpret_cast<ConcatCols<unwary_t<TMatrix>>&>(m.top());
}

template <typename TMatrix>
const ConcatCols<unwary_t<TMatrix>>&
concat_cols(const GenericMatrix<TMatrix>& m)
{
   return reinterpret_cast<const ConcatCols<unwary_t<TMatrix>>&>(m.top());
}

template <typename TMatrix>
ConcatCols<unwary_t<TMatrix>>&
concat_cols(GenericMatrix<TMatrix>&& m)
{
   return reinterpret_cast<ConcatCols<unwary_t<TMatrix>>&>(m.top());
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
public:
   using value_type = typename lazy1_traits<MatrixRef,Operation>::value_type;
   using reference = typename lazy1_traits<MatrixRef,Operation>::reference;
   using const_reference = reference;
   using matrix_type = typename deref<MatrixRef>::type;

   using modified_container_base<MatrixRef, Operation>::modified_container_base;
};

template <typename MatrixRef, typename Operation>
struct spec_object_traits< LazyMatrix1<MatrixRef, Operation> >
   : spec_object_traits<is_container> {
   static constexpr bool is_lazy = true, is_temporary = true, is_always_const = true;
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
   using self_t = LazyMatrix1<MatrixRef,Operation>;
public:
   typename lazy1_traits<MatrixRef,Operation>::reference
   operator() (int i, int j) const
   {
      const self_t& me=static_cast<const self_t&>(*this);
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
                                     mlist< ContainerRefTag< masquerade<pm::Rows, MatrixRef> >,
                                            OperationTag< operations::construct_unary2_with_arg<LazyVector1, OperationVal> >,
                                            MasqueradedTop > > {
   typedef modified_container_impl<Rows> base_t;
protected:
   ~Rows();
public:
   const typename base_t::container& get_container() const
   {
      return rows(this->hidden().get_container());
   }
   typename base_t::operation get_operation() const
   {
      return this->hidden().get_operation();
   }
};

template <typename MatrixRef, typename OperationVal>
class Cols< LazyMatrix1<MatrixRef, OperationVal> >
   : public modified_container_impl< Cols< LazyMatrix1<MatrixRef, OperationVal> >,
                                     mlist< ContainerRefTag< masquerade<pm::Cols, MatrixRef> >,
                                            OperationTag< operations::construct_unary2_with_arg<LazyVector1, OperationVal> >,
                                            MasqueradedTop > > {
   typedef modified_container_impl<Cols> base_t;
protected:
   ~Cols();
public:
   const typename base_t::container& get_container() const
   {
      return cols(this->hidden().get_container());
   }
   typename base_t::operation get_operation() const
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
   : public modified_container_pair_base<MatrixRef1, MatrixRef2, Operation>
   , public GenericMatrix< LazyMatrix2<MatrixRef1,MatrixRef2,Operation>,
                           typename lazy2_traits<MatrixRef1,MatrixRef2,Operation>::element_type > {
public:
   using value_type = typename lazy2_traits<MatrixRef1,MatrixRef2,Operation>::value_type;
   using reference = typename lazy2_traits<MatrixRef1,MatrixRef2,Operation>::reference;
   using const_reference = reference;
   using modified_container_pair_base<MatrixRef1, MatrixRef2, Operation>::modified_container_pair_base;
};

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
struct spec_object_traits< LazyMatrix2<MatrixRef1, MatrixRef2, Operation> >
   : spec_object_traits<is_container> {
   static constexpr bool is_lazy = true, is_temporary = true, is_always_const = true;
};

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
struct check_container_feature<LazyMatrix2<MatrixRef1, MatrixRef2, Operation>, sparse>
   : bool_constant< TransformedContainerPair_helper1<MatrixRef1, MatrixRef2, Operation>::sparse_result > {};

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
struct check_container_feature<LazyMatrix2<MatrixRef1, MatrixRef2, Operation>, Symmetric>
   : mlist_and< check_container_ref_feature<MatrixRef1,Symmetric>,
                check_container_ref_feature<MatrixRef2,Symmetric> > {};

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
struct check_container_feature<LazyMatrix2<MatrixRef1, MatrixRef2, Operation>, SkewSymmetric>
   : mlist_and< check_container_ref_feature<MatrixRef1,SkewSymmetric>,
                check_container_ref_feature<MatrixRef2,SkewSymmetric> > {};

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
struct check_container_feature<LazyMatrix2<MatrixRef1, MatrixRef2, Operation>, FlatStorage>
   : mlist_and< check_container_ref_feature<MatrixRef1,FlatStorage>,
                check_container_ref_feature<MatrixRef2,FlatStorage> > {};

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
class matrix_random_access_methods< LazyMatrix2<MatrixRef1, MatrixRef2, Operation> > {
   using self_t = LazyMatrix2<MatrixRef1,MatrixRef2,Operation>;
public:
   typename lazy2_traits<MatrixRef1,MatrixRef2,Operation>::reference
   operator() (int i, int j) const
   {
      const self_t& me=static_cast<const self_t&>(*this);
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

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
class Rows< LazyMatrix2<MatrixRef1, MatrixRef2, Operation> >
   : public modified_container_pair_impl< Rows< LazyMatrix2<MatrixRef1, MatrixRef2, Operation> >,
                                          mlist< Container1RefTag< masquerade<pm::Rows, MatrixRef1> >,
                                                 Container2RefTag< masquerade<pm::Rows, MatrixRef2> >,
                                                 OperationTag< operations::construct_binary2_with_arg<LazyVector2, Operation> >,
                                                 MasqueradedTop > > {
   typedef modified_container_pair_impl<Rows> base_t;
protected:
   ~Rows();
public:
   const typename base_t::container1& get_container1() const
   {
      return rows(this->hidden().get_container1());
   }
   const typename base_t::container2& get_container2() const
   {
      return rows(this->hidden().get_container2());
   }
   typename base_t::operation get_operation() const
   {
      return this->hidden().get_operation();
   }
};

template <typename MatrixRef1, typename MatrixRef2, typename Operation>
class Cols< LazyMatrix2<MatrixRef1, MatrixRef2, Operation> >
   : public modified_container_pair_impl< Cols< LazyMatrix2<MatrixRef1, MatrixRef2, Operation> >,
                                          mlist< Container1RefTag< masquerade<pm::Cols, MatrixRef1> >,
                                                 Container2RefTag< masquerade<pm::Cols, MatrixRef2> >,
                                                 OperationTag< operations::construct_binary2_with_arg<LazyVector2, Operation> >,
                                                 MasqueradedTop > > {
   typedef modified_container_pair_impl<Cols> base_t;
protected:
   ~Cols();
public:
   const typename base_t::container1& get_container1() const
   {
      return cols(this->hidden().get_container1());
   }
   const typename base_t::container2& get_container2() const
   {
      return cols(this->hidden().get_container2());
   }
   typename base_t::operation get_operation() const
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
template <typename TargetType, typename TMatrix>
const TMatrix& convert_to(const GenericMatrix<TMatrix, TargetType>& m)
{
   return m.top();
}

template <typename TargetType, typename TMatrix, typename E,
          typename=std::enable_if_t<can_initialize<E, TargetType>::value && !std::is_same<E, TargetType>::value>>
auto convert_to(const GenericMatrix<TMatrix, E>& m)
{
   return LazyMatrix1<const unwary_t<TMatrix>&, conv<E, TargetType>>(m.top());
}

template <typename TargetType, typename TMatrix, typename E>
const TMatrix& convert_lazily(const GenericMatrix<TMatrix, E>& m,
                              std::enable_if_t<std::is_convertible<E, TargetType>::value, void**> =nullptr)
{
   return m.top();
}

template <typename TargetType, typename TMatrix, typename E>
auto convert_lazily(const GenericMatrix<TMatrix, E>& m,
                    std::enable_if_t<can_initialize<E, TargetType>::value && !std::is_convertible<E, TargetType>::value, void**> =nullptr)
{
   return LazyMatrix1<const unwary_t<TMatrix>&, conv<E, TargetType>>(m.top());
}

template <typename TMatrix, typename Operation>
auto apply_operation(const GenericMatrix<TMatrix>& m, const Operation& op)
{
   return LazyMatrix1<const TMatrix&, Operation>(m.top(), op);
}

/* --------------------------
 *  MatrixMinor concatenated
 * -------------------------- */

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef,
          bool contiguous=is_derived_from<typename deref<RowIndexSetRef>::type, sequence>::value>
class MatrixMinorConcatRows
   : public ConcatRows_default< MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef> > {};

template <typename MatrixRef, typename RowIndexSetRef>
class MatrixMinorConcatRows<MatrixRef, RowIndexSetRef, const all_selector&, true>
   : public indexed_subset_impl< MatrixMinorConcatRows<MatrixRef, RowIndexSetRef, const all_selector&, true>,
                                 mlist< Container1RefTag< masquerade<ConcatRows, MatrixRef> >,
                                        Container2RefTag< sequence >,
                                        HiddenTag< MatrixMinor<MatrixRef, RowIndexSetRef, const all_selector&> > > > {
   typedef indexed_subset_impl<MatrixMinorConcatRows> base_t;
public:
   typename base_t::container1& get_container1()
   {
      return concat_rows(this->hidden().get_matrix());
   }
   const typename base_t::container1& get_container1() const
   {
      return concat_rows(this->hidden().get_matrix());
   }
   sequence get_container2() const
   {
      return sequence(this->hidden().get_subset(int_constant<1>()).front() * this->hidden().cols(), dim());
   }

   int dim() const { return this->hidden().rows() * this->hidden().cols(); }
};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
class ConcatRows< MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef> >
   : public MatrixMinorConcatRows<MatrixRef,RowIndexSetRef,ColIndexSetRef>
   , public GenericVector< ConcatRows< MatrixMinor<MatrixRef,RowIndexSetRef,ColIndexSetRef> >,
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

template <typename MatrixList>
class ConcatRows< BlockMatrix<MatrixList, std::true_type> >
   : public container_chain_impl< ConcatRows< BlockMatrix<MatrixList, std::true_type> >,
                                  mlist< ContainerRefTag< typename mlist_transform_unary<MatrixList, masquerade_as_ConcatRows>::type >,
                                         MasqueradedTop > >
   , public GenericVector< ConcatRows< BlockMatrix<MatrixList, std::true_type> >,
                           typename deref<typename mlist_head<MatrixList>::type>::type::element_type> {
   using base_t = container_chain_impl<ConcatRows>;
protected:
   ~ConcatRows();
public:
   template <int i>
   decltype(auto) get_container(int_constant<i>)
   {
      return concat_rows(this->hidden().template get_container(int_constant<i>()));
   }
   template <int i>
   decltype(auto) get_container(int_constant<i>) const
   {
      return concat_rows(this->hidden().template get_container(int_constant<i>()));
   }

   ConcatRows& operator= (const ConcatRows& other) { return ConcatRows::generic_type::operator=(other); }
   using ConcatRows::generic_type::operator=;
   using base_t::dim;
};

/* --------------------------
 *  RepeatedRow, RepeatedCol
 * -------------------------- */

template <typename VectorRef>
class RepeatedRow
   : public repeated_line_matrix<VectorRef>
   , public GenericMatrix< RepeatedRow<VectorRef>,
                           typename pure_type_t<VectorRef>::element_type> {
public:
   using repeated_line_matrix<VectorRef>::repeated_line_matrix;

protected:
   using GenericMatrix<RepeatedRow>::stretch_rows;
   using GenericMatrix<RepeatedRow>::stretch_cols;

   void stretch_rows(int r)
   {
      this->line_container.stretch_dim(r);
   }

   void stretch_cols(int c)
   {
      this->line_container.get_elem_alias().get_object().stretch_dim(c);
   }

   friend class Rows<RepeatedRow>;
   template <typename, typename> friend class BlockMatrix;
};

template <typename VectorRef>
class RepeatedCol
   : public repeated_line_matrix<VectorRef>
   , public GenericMatrix< RepeatedCol<VectorRef>,
                           typename pure_type_t<VectorRef>::element_type> {
public:
   using repeated_line_matrix<VectorRef>::repeated_line_matrix;

protected:
   using GenericMatrix<RepeatedCol>::stretch_rows;
   using GenericMatrix<RepeatedCol>::stretch_cols;

   void stretch_rows(int r)
   {
      this->line_container.get_elem_alias().get_object().stretch_dim(r);
   }

   void stretch_cols(int c)
   {
      this->line_container.stretch_dim(c);
   }

   template <typename, typename> friend class BlockMatrix;
};

template <typename VectorRef>
struct spec_object_traits< RepeatedRow<VectorRef> >
   : spec_object_traits<is_container> {
   static constexpr bool is_temporary = true, is_always_const = true;
};

template <typename VectorRef>
struct spec_object_traits< RepeatedCol<VectorRef> >
   : spec_object_traits<is_container> {
   static constexpr bool is_temporary = true, is_always_const = true;
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
   : is_derived_from_instance_of<pure_type_t<VectorRef>, SameElementVector> {};

template <typename VectorRef>
struct check_container_feature<RepeatedCol<VectorRef>, FlatStorage>
   : check_container_feature< RepeatedRow<VectorRef>, FlatStorage> {};

template <typename VectorRef>
class matrix_random_access_methods< RepeatedRow<VectorRef> > {
   using self_t = RepeatedRow<VectorRef>;
public:
   decltype(auto) operator() (int i, int j)
   {
      self_t& me=static_cast<self_t&>(*this);
      if (POLYMAKE_DEBUG && (i<0 || i>me.rows()))
         throw std::runtime_error("RepeatedRow::operator() - index out of bound");
      return me.get_line()[j];
   }

   decltype(auto) operator() (int i, int j) const
   {
      const self_t& me=static_cast<const self_t&>(*this);
      if (POLYMAKE_DEBUG && (i<0 || i>me.rows()))
         throw std::runtime_error("RepeatedRow::operator() - index out of bound");
      return me.get_line()[j];
   }
};

template <typename VectorRef>
class matrix_random_access_methods< RepeatedCol<VectorRef> > {
   using self_t = RepeatedCol<VectorRef>;
public:
   decltype(auto) operator() (int i, int j)
   {
      self_t& me=static_cast<self_t&>(*this);
      if (POLYMAKE_DEBUG && (j<0 || j>me.cols()))
         throw std::runtime_error("RepeatedCol::operator() - index out of bound");
      return me.get_line()[i];
   }

   decltype(auto) operator() (int i, int j) const
   {
      const self_t& me=static_cast<const self_t&>(*this);
      if (POLYMAKE_DEBUG && (j<0 || j>me.cols()))
         throw std::runtime_error("RepeatedCol::operator() - index out of bound");
      return me.get_line()[i];
   }
};

template <typename VectorRef,
          bool same_elem = check_container_feature<RepeatedRow<VectorRef>, FlatStorage>::value>
class ConcatRepeatedRow_impl
   : public container_product_impl< ConcatRepeatedRow_impl<VectorRef, same_elem>,
                                    typename mlist_concat< typename repeated_container<VectorRef>::params,
                                                           HiddenTag< repeated_line_matrix<VectorRef> > >::type > {
public:
   count_down get_container1() const { return count_down(this->hidden().get_count()); }
   decltype(auto) get_container2() const { return this->hidden().get_line(); }

   int dim() const
   {
      return this->hidden().get_count() * get_container2().dim();
   }
};

template <typename VectorRef>
class ConcatRepeatedRow_impl<VectorRef, true>
   : public repeated_value_container_impl< ConcatRepeatedRow_impl<VectorRef, true>,
                                           typename pure_type_t<VectorRef>::element_reference,
                                           HiddenTag< repeated_line_matrix<VectorRef> > > {
public:
   decltype(auto) get_elem_alias() const
   {
      return this->hidden().get_line().get_elem_alias();
   }
   int size() const
   {
      return this->hidden().get_count() * this->hidden().get_line().size();
   }
   int dim() const
   {
      return this->hidden().get_count() * this->hidden().get_line().dim();
   }
};

template <typename VectorRef,
          bool same_elem = check_container_feature<RepeatedCol<VectorRef>, FlatStorage>::value>
class ConcatRepeatedCol_impl
   : public container_product_impl< ConcatRepeatedCol_impl<VectorRef, same_elem>,
                                    mlist< Container1RefTag<VectorRef>,
                                           Container2RefTag<count_down>,
                                           HiddenTag< repeated_line_matrix<VectorRef> > > > {
public:
   decltype(auto) get_container1() const { return this->hidden().get_line(); }
   count_down get_container2() const { return count_down(this->hidden().get_count()); }

   int dim() const
   {
      return this->hidden().get_count() * get_container1().dim();
   }
};

template <typename VectorRef>
class ConcatRepeatedCol_impl<VectorRef, true>
   : public ConcatRepeatedRow_impl<VectorRef, true> {};

template <typename VectorRef>
class ConcatRows< RepeatedRow<VectorRef> >
   : public ConcatRepeatedRow_impl<VectorRef>
   , public GenericVector< ConcatRows< RepeatedRow<VectorRef> >,
                           typename pure_type_t<VectorRef>::element_type > {
protected:
   ~ConcatRows();
public:
   using ConcatRepeatedRow_impl<VectorRef>::dim;
};

template <typename VectorRef>
class ConcatRows< RepeatedCol<VectorRef> >
   : public ConcatRepeatedCol_impl<VectorRef>
   , public GenericVector< ConcatRows< RepeatedCol<VectorRef> >,
                           typename pure_type_t<VectorRef>::element_type > {
protected:
   ~ConcatRows();
public:
   using ConcatRepeatedCol_impl<VectorRef>::dim;
};

template <typename VectorRef>
class Rows< RepeatedRow<VectorRef> >
   : public redirected_container< Rows< RepeatedRow<VectorRef> >,
                                  mlist< ContainerTag< repeated_value_container<VectorRef> >,
                                         MasqueradedTop > > {
protected:
   ~Rows();
public:
   decltype(auto) get_container() const
   {
      return this->hidden().get_line_container();
   }
};

template <typename VectorRef>
class Cols< RepeatedCol<VectorRef> >
   : public redirected_container< Cols< RepeatedCol<VectorRef> >,
                                  mlist< ContainerTag< repeated_value_container<VectorRef> >,
                                         MasqueradedTop > > {
protected:
   ~Cols();
public:
   decltype(auto) get_container() const
   {
      return this->hidden().get_line_container();
   }
};

template <typename VectorRef, bool is_sparse=check_container_ref_feature<VectorRef, sparse>::value>
class repeated_line_across
   : public modified_container_impl< repeated_line_across<VectorRef, is_sparse>,
                                     mlist< ContainerRefTag< typename attrib<VectorRef>::plus_const >,
                                            OperationTag< operations::construct_unary_with_arg<SameElementVector, int> >,
                                            HiddenTag< repeated_line_matrix<VectorRef> > > > {
   using base_t = modified_container_impl<repeated_line_across>;
public:
   decltype(auto) get_container() const { return this->hidden().get_line(); }
   typename base_t::operation get_operation() const { return this->hidden().get_count(); }
};

template <typename VectorRef>
class repeated_line_across<VectorRef, true>
   : public modified_container_pair_impl< repeated_line_across<VectorRef, true>,
                                          mlist< Container1RefTag< sequence >,
                                                 Container2RefTag< typename attrib<VectorRef>::plus_const >,
                                                 IteratorCouplerTag< zipping_coupler< operations::cmp, set_union_zipper, false, true> >,
                                                 OperationTag< SameElementSparseVector_factory<1> >,
                                                 HiddenTag< repeated_line_matrix<VectorRef> > > > {
   using base_t = modified_container_pair_impl<repeated_line_across>;
public:
   sequence get_container1() const { return sequence(0, size()); }
   decltype(auto) get_container2() const { return this->hidden().get_line(); }
   typename base_t::operation get_operation() const { return this->hidden().get_count(); }
   int size() const { return get_container2().dim(); }
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


/// Create a matrix with n rows, each equal to v.
template <typename TVector, typename=std::enable_if_t<is_generic_vector<TVector>::value>>
auto repeat_row(TVector&& v, int n=0)
     // gcc 5 needs this crutch
     -> RepeatedRow<diligent_ref_t<unwary_t<TVector>>>
{
   using result_type = RepeatedRow<diligent_ref_t<unwary_t<TVector>>>;
   return result_type(diligent(unwary(std::forward<TVector>(v))), n);
}

/// Create a matrix with n columns, each equal to v.
template <typename TVector, typename=std::enable_if_t<is_generic_vector<TVector>::value>>
auto repeat_col(TVector&& v, int n=0)
     // gcc 5 needs this crutch
     -> RepeatedCol<diligent_ref_t<unwary_t<TVector>>>
{
   using result_type = RepeatedCol<diligent_ref_t<unwary_t<TVector>>>;
   return result_type(diligent(unwary(std::forward<TVector>(v))), n);
}

/// Create a matrix with m rows and n columns whose entries
/// are all equal to the given element x.
template <typename E>
auto same_element_matrix(E&& x, int m, int n)
{
   return RepeatedRow<SameElementVector<E>>(same_element_vector(std::forward<E>(x), n), m);
}

/// Create a matrix with m rows and n columns whose entries
/// are all equal to 1.
template <typename E>
auto ones_matrix(int m, int n)
{
   using vector_t = decltype(ones_vector<E>(0));
   return RepeatedRow<vector_t>(ones_vector<E>(n), m);
}

/// Create a matrix with m rows and n columns whose entries
/// are all equal to 0.
template <typename E>
auto zero_matrix(int m, int n)
{
   using vector_t = decltype(zero_vector<E>(0));
   return RepeatedRow<vector_t>(zero_vector<E>(n), m);
}

/// disguise a GenericVector as a matrix with 1 row 
template <typename TVector>
auto vector2row(GenericVector<TVector>& v)
{
   return RepeatedRow<unwary_t<TVector>&>(v.top(), 1);
}

template <typename TVector>
auto vector2row(const GenericVector<TVector>& v)
{
   return RepeatedRow<const unwary_t<TVector>&>(v.top(), 1);
}

/// disguise a GenericVector as a matrix with 1 column 
template <typename TVector>
auto vector2col(GenericVector<TVector>& v)
{
   return RepeatedCol<unwary_t<TVector>&>(v.top(), 1);
}

template <typename TVector>
auto vector2col(const GenericVector<TVector>& v)
{
   return RepeatedCol<const unwary_t<TVector>&>(v.top(), 1);
}

template <typename ERef>
class SameElementMatrix
   : public same_value_container<ERef>
   , public GenericMatrix<SameElementMatrix<ERef>, pure_type_t<ERef>> {
public:
   using same_value_container<ERef>::same_value_container;

   decltype(auto) operator() (int, int) const { return this->front(); }
};

template <typename ERef>
struct spec_object_traits< SameElementMatrix<ERef> >
   : spec_object_traits< same_value_container<ERef> > {};

template <typename ERef>
struct check_container_feature<SameElementMatrix<ERef>, FlatStorage> : std::true_type {};

template <typename ERef>
class ConcatRows< SameElementMatrix<ERef> >
   : public same_value_container<ERef> {
protected:
   ConcatRows();
   ~ConcatRows();
};

template <typename ERef>
class Rows< SameElementMatrix<ERef> >
   : public constant_masquerade_container< same_value_container<ERef> > {
protected:
   ~Rows();
};

template <typename ERef>
class Cols< SameElementMatrix<ERef> >
   : public constant_masquerade_container< same_value_container<ERef> > {
protected:
   ~Cols();
};

/* --------------------------------------
 *  diagonal and block-diagonal matrices
 * -------------------------------------- */

// is_main==true: the vector is laid out along the main diagonal
// is_main==false: along the anti-diagonal (starting in the lower left corner)

template <typename VectorRef, bool is_main=true>
class DiagMatrix
   : public GenericMatrix< DiagMatrix<VectorRef, is_main>, typename deref<VectorRef>::type::element_type > {
protected:
   using alias_t = alias<VectorRef>;
   alias_t vector;
public:
   using value_type = typename deref<VectorRef>::type::value_type;
   using const_reference = typename deref<VectorRef>::type::const_reference;
   using reference = const_reference;

   template <typename Arg, typename=std::enable_if_t<std::is_constructible<alias_t, Arg>::value>>
   explicit DiagMatrix(Arg&& vector_arg)
      : vector(std::forward<Arg>(vector_arg)) {}

   decltype(auto) get_vector() const &
   {
      return *vector;
   }
   decltype(auto) get_vector() &&
   {
      return vector.get_object();
   }
protected:
   using GenericMatrix<DiagMatrix>::stretch_rows;
   using GenericMatrix<DiagMatrix>::stretch_cols;

   void stretch_rows(int r)
   {
      vector.get_object().stretch_dim(r);
   }
   void stretch_cols(int c)
   {
      vector.get_object().stretch_dim(c);
   }

   template <typename, typename> friend class BlockMatrix;
};

template <typename VectorRef, bool is_main>
struct spec_object_traits< DiagMatrix<VectorRef, is_main> >
   : spec_object_traits<is_container> {
   static constexpr bool is_temporary = true, is_always_const = true;
};

template <typename VectorRef, bool is_main>
class matrix_random_access_methods< DiagMatrix<VectorRef, is_main> > {
   using self_t = DiagMatrix<VectorRef, is_main>;
public:
   typename container_traits<VectorRef>::const_reference
   operator() (int i, int j) const
   {
      const self_t& me=static_cast<const self_t&>(*this);
      if (is_main ? i==j : i+j==me.get_vector().dim()-1)
         return me.get_vector()[j];
      return zero_value<typename deref<VectorRef>::type::element_type>();
   }
};

template <typename VectorRef, bool main_diag>
class ConcatRows< DiagMatrix<VectorRef, main_diag> >
   : public modified_container_pair_impl< ConcatRows< DiagMatrix<VectorRef, main_diag> >,
                                          mlist< Container1RefTag< masquerade_add_features<VectorRef,
                                                                     typename mlist_prepend_if<!main_diag, reversed, mlist<indexed, end_sensitive> >::type> >,
                                                 Container2RefTag< series >,
                                                 OperationTag< pair<nothing, BuildBinaryIt<operations::dereference2> > >,
                                                 MasqueradedTop > >
   , public GenericVector< ConcatRows< DiagMatrix<VectorRef, main_diag> >,
                           typename deref<VectorRef>::type::element_type > {
   typedef modified_container_pair_impl<ConcatRows> base_t;
public:
   const typename base_t::container1& get_container1() const
   {
      return reinterpret_cast<const typename base_t::container1&>(this->hidden().get_vector());
   }

   series get_container2() const
   {
      const int d=this->hidden().get_vector().dim();
      return series(main_diag ? 0 : d-1, d, main_diag ? d+1 : d-1);
   }

   int dim() const
   {
      const int d=this->hidden().get_vector().dim();
      return d*d;
   }
};

template <typename VectorRef, bool main_diag>
struct check_container_feature< DiagMatrix<VectorRef, main_diag>, pure_sparse> : std::true_type {};

template <typename VectorRef, bool main_diag>
struct check_container_feature< DiagMatrix<VectorRef, main_diag>, Symmetric> : bool_constant<main_diag> {};

template <typename VectorRef, bool main_diag, typename TReversed,
          bool is_simple=is_derived_from_instance_of<typename deref<VectorRef>::type, SameElementVector>::value>
struct DiagRowsCols_helper {
   static const bool use_sequence_index= !main_diag && std::is_same<TReversed, void>::value;
   typedef zipping_coupler< operations::cmp, set_union_zipper, use_sequence_index, true > zipper;
   typedef mlist< Container1RefTag< Series<int, main_diag> >,
                  Container2RefTag< masquerade_add_features<VectorRef, typename mlist_concat<pure_sparse, TReversed>::type> >,
                  IteratorCouplerTag< typename reverse_coupler_helper<zipper, TReversed>::type >,
                  OperationTag< SameElementSparseVector_factory<3> >,
                  HiddenTag< DiagMatrix<VectorRef, main_diag> > > params;
};

template <typename VectorRef, bool main_diag, typename TReversed>
struct DiagRowsCols_helper<VectorRef, main_diag, TReversed, true> {
   typedef mlist< Container1RefTag< Series<int, main_diag> >,
                  Container2RefTag< VectorRef >,
                  OperationTag< SameElementSparseVector_factory<2> >,
                  HiddenTag< DiagMatrix<VectorRef, main_diag> > > params;
};


template <typename VectorRef, bool main_diag, typename TReversed=void>
class DiagRowsCols
   : public modified_container_pair_impl< DiagRowsCols<VectorRef, main_diag, TReversed>,
                                          typename DiagRowsCols_helper<VectorRef, main_diag, TReversed>::params> {
   typedef modified_container_pair_impl<DiagRowsCols> base_t;
public:
   Series<int, main_diag> get_container1() const
   {
      const int n=size();
      return Series<int, main_diag>(main_diag ? 0 : n-1, n, main_diag ? 1 : -1);
   }
   const typename base_t::container2& get_container2() const
   {
      return reinterpret_cast<const typename base_t::container2&>(this->hidden().get_vector());
   }

   typename base_t::operation get_operation() const { return size(); }
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
class Rows< DiagMatrix<VectorRef, false> > : public DiagRowsCols<VectorRef, false, reversed> {
protected:
   ~Rows();
};

template <typename VectorRef>
class Cols< DiagMatrix<VectorRef, false> > : public DiagRowsCols<VectorRef, false> {
protected:
   ~Cols();
};

/// Create a square diagonal matrix from a GenericVector. 
template <typename TVector>
auto diag(const GenericVector<TVector>& v)
{
   return DiagMatrix<const unwary_t<TVector>&>(v.top());
}

/// Create a anti-diagonal matrix. 
template <typename TVector>
auto anti_diag(const GenericVector<TVector>& v)
{
   return DiagMatrix<const unwary_t<TVector>&, false>(v.top());
}

/// Create a unit_matrix of dimension dim.
template <typename E>
auto unit_matrix(int dim)
{
   // TODO: replace with direct call to diag() when it starts using urefs
   using vector_t = decltype(ones_vector<E>(dim));
   return DiagMatrix<vector_t>(ones_vector<E>(dim));
}

// is_main==true : blocks are laid out along the main diagonal
// is_main==false : along the anti-diagonal, starting in the lower left corner
// TODO: migrate to a tuple
template <typename MatrixRef1, typename MatrixRef2, bool is_main=true>
class BlockDiagMatrix
   : public container_pair_base<typename attrib<MatrixRef1>::plus_const, typename attrib<MatrixRef2>::plus_const>
   , public GenericMatrix< BlockDiagMatrix<MatrixRef1, MatrixRef2, is_main>,
                           typename deref<MatrixRef1>::type::element_type > {
public:
   using value_type = typename container_traits<MatrixRef1>::value_type;
   static_assert(std::is_same<value_type, typename container_traits<MatrixRef2>::value_type>::value,
                 "blocks with inhomogeneous element types");
      
   using const_reference = typename compatible<typename container_traits<MatrixRef1>::const_reference,
                                               typename container_traits<MatrixRef2>::const_reference>::type;
   using reference = const_reference;

   using container_pair_base<typename attrib<MatrixRef1>::plus_const, typename attrib<MatrixRef2>::plus_const>::container_pair_base;

   /// the number of rows
   int rows() const { return this->get_container1().rows() + this->get_container2().rows(); }

   /// the number of columns
   int cols() const { return this->get_container1().cols() + this->get_container2().cols(); }
};

template <typename MatrixRef1, typename MatrixRef2, bool is_main>
struct spec_object_traits< BlockDiagMatrix<MatrixRef1, MatrixRef2, is_main> >
   : spec_object_traits<is_container> {
   static const bool is_temporary = true, is_always_const = true;
};

template <typename MatrixRef1, typename MatrixRef2, bool is_main>
struct check_container_feature<BlockDiagMatrix<MatrixRef1, MatrixRef2, is_main>, sparse> : std::true_type {};

template <typename MatrixRef1, typename MatrixRef2, bool is_main>
struct check_container_feature<BlockDiagMatrix<MatrixRef1, MatrixRef2, is_main>, pure_sparse>
   : mlist_and< check_container_ref_feature<MatrixRef1, pure_sparse>,
                check_container_ref_feature<MatrixRef2, pure_sparse> > {};

template <typename MatrixRef1, typename MatrixRef2>
struct check_container_feature<BlockDiagMatrix<MatrixRef1, MatrixRef2, true>, Symmetric>
   : mlist_and< check_container_ref_feature<MatrixRef1, Symmetric>,
                check_container_ref_feature<MatrixRef2, Symmetric> > {};

template <typename MatrixRef1, typename MatrixRef2, bool is_main>
class matrix_random_access_methods< BlockDiagMatrix<MatrixRef1, MatrixRef2, is_main> > {
   using self_t = BlockDiagMatrix<MatrixRef1, MatrixRef2, is_main>;
public:
   typename compatible<typename container_traits<MatrixRef1>::const_reference,
                       typename container_traits<MatrixRef2>::const_reference>::type
   operator() (int i, int j) const
   {
      const self_t& me=static_cast<const self_t&>(*this);
      const int c1=me.get_container1().cols();
      if (is_main) {
         const int r1=me.get_container1().rows();
         if (i<r1 && j<c1) return me.get_container1()(i,j);
         if (i>=r1 && j>=c1) return me.get_container2()(i-r1,j-c1);
      } else {
         const int r2=me.get_container2().rows();
         if (i>=r2 && j<c1) return me.get_container1()(i-r2,j);
         if (i<r2 && j>=c1) return me.get_container2()(i,j-c1);
      }
      return zero_value<typename self_t::element_type>();
   }
};

template <typename MatrixRef1, typename MatrixRef2, bool main_diag,
          template <typename> class RowsCols, bool for_rows, bool is_first>
class BlockDiagRowsCols
   : public modified_container_impl< BlockDiagRowsCols<MatrixRef1, MatrixRef2, main_diag, RowsCols, for_rows, is_first>,
                                     mlist< ContainerRefTag< masquerade<RowsCols, std::conditional_t<(main_diag ? is_first : for_rows^is_first), MatrixRef1, MatrixRef2>> >,
                                            OperationTag< ExpandedVector_factory<> >,
                                            HiddenTag< BlockDiagMatrix<MatrixRef1, MatrixRef2, main_diag> > > > {
   using base_t = modified_container_impl<BlockDiagRowsCols>;

   const typename base_t::container& get_container_impl(std::true_type) const
   {
      return reinterpret_cast<const typename base_t::container&>(this->hidden().get_container1());
   }
   const typename base_t::container& get_container_impl(std::false_type) const
   {
      return reinterpret_cast<const typename base_t::container&>(this->hidden().get_container2());
   }

   int dim_impl(std::true_type)  const { return this->hidden().get_container1().cols() + this->hidden().get_container2().cols(); }
   int dim_impl(std::false_type) const { return this->hidden().get_container1().rows() + this->hidden().get_container2().rows(); }

   int offset_impl(std::true_type,  std::true_type,  std::true_type)  const { return 0; }
   int offset_impl(std::true_type,  std::true_type,  std::false_type) const { return this->hidden().get_container1().cols(); }
   int offset_impl(std::true_type,  std::false_type, std::true_type)  const { return 0; }
   int offset_impl(std::true_type,  std::false_type, std::false_type) const { return this->hidden().get_container1().rows(); }
   int offset_impl(std::false_type, std::true_type,  std::true_type)  const { return this->hidden().get_container1().cols(); }
   int offset_impl(std::false_type, std::true_type,  std::false_type) const { return 0; }
   int offset_impl(std::false_type, std::false_type, std::true_type)  const { return this->hidden().get_container2().rows(); }
   int offset_impl(std::false_type, std::false_type, std::false_type) const { return 0; }

public:
   const typename base_t::container& get_container() const
   {
      return get_container_impl(bool_constant<(main_diag ? is_first : for_rows^is_first)>());
   }
   typename base_t::operation get_operation() const
   {
      return typename base_t::operation(offset_impl(bool_constant<main_diag>(), bool_constant<for_rows>(), bool_constant<is_first>()),
                                        dim_impl(bool_constant<for_rows>()));
   }
};

template <typename MatrixRef1, typename MatrixRef2, bool main_diag>
class Rows< BlockDiagMatrix<MatrixRef1, MatrixRef2, main_diag> >
   : public container_chain_impl< Rows< BlockDiagMatrix<MatrixRef1, MatrixRef2, main_diag> >,
                                  mlist< ContainerRefTag< mlist< BlockDiagRowsCols<MatrixRef1, MatrixRef2, main_diag, Rows, true, true>,
                                                                 BlockDiagRowsCols<MatrixRef1, MatrixRef2, main_diag, Rows, true, false> > >,
                                         MasqueradedTop > > {
protected:
   ~Rows();
public:
   decltype(auto) get_container(int_constant<0>) const
   {
      return reinterpret_cast<const BlockDiagRowsCols<MatrixRef1, MatrixRef2, main_diag, pm::Rows, true, true>&>(this->hidden());
   }
   decltype(auto) get_container(int_constant<1>) const
   {
      return reinterpret_cast<const BlockDiagRowsCols<MatrixRef1, MatrixRef2, main_diag, pm::Rows, true, false>&>(this->hidden());
   }
};

template <typename MatrixRef1, typename MatrixRef2, bool main_diag>
class Cols< BlockDiagMatrix<MatrixRef1, MatrixRef2, main_diag> >
   : public container_chain_impl< Cols< BlockDiagMatrix<MatrixRef1, MatrixRef2, main_diag> >,
                                  mlist< ContainerRefTag< mlist< BlockDiagRowsCols<MatrixRef1, MatrixRef2, main_diag, Cols, false, true>,
                                                                 BlockDiagRowsCols<MatrixRef1, MatrixRef2, main_diag, Cols, false, false> > >,
                                         MasqueradedTop > > {
protected:
   ~Cols();
public:
   decltype(auto) get_container(int_constant<0>) const
   {
      return reinterpret_cast<const BlockDiagRowsCols<MatrixRef1, MatrixRef2, main_diag, pm::Cols, false, true>&>(this->hidden());
   }
   decltype(auto) get_container(int_constant<1>) const
   {
      return reinterpret_cast<const BlockDiagRowsCols<MatrixRef1, MatrixRef2, main_diag, pm::Cols, false, false>&>(this->hidden());
   }
};

/// Create a block-diagonal matrix.
template <typename E, typename Matrix1, typename Matrix2>
auto diag(const GenericMatrix<Matrix1, E>& m1, const GenericMatrix<Matrix2, E>& m2)
{
   return BlockDiagMatrix<const unwary_t<Matrix1>&,
                          const unwary_t<Matrix2>&, true>(m1.top(), m2.top());
}

/// Create a block-diagonal matrix. 
/// The vector argument is treated as a square diagonal matrix.
template <typename E, typename Vector1, typename Matrix2>
auto diag(const GenericVector<Vector1, E>& v1, const GenericMatrix<Matrix2, E>& m2)
{
   return BlockDiagMatrix<DiagMatrix<const unwary_t<Vector1>&>,
                          const unwary_t<Matrix2>&, true>(diag(v1), m2.top());
}

/// Create a block-diagonal matrix. 
/// The vector argument is treated as a square diagonal matrix.
template <typename E, typename Matrix1, typename Vector2>
auto diag(const GenericMatrix<Matrix1, E>& m1, const GenericVector<Vector2, E>& v2)
{
   return BlockDiagMatrix<const unwary_t<Matrix1>&,
                          DiagMatrix<const unwary_t<Vector2>&>, true>(m1.top(), diag(v2));
}

template <typename Scalar1, typename Matrix2, typename E,
          typename=std::enable_if_t<can_initialize<pure_type_t<Scalar1>, E>::value>>
auto diag(Scalar1&& x1, const GenericMatrix<Matrix2, E>& m2)
{
   using corner_elem_t = std::conditional_t<std::is_same<pure_type_t<Scalar1>, E>::value, Scalar1, E>;
   using corner_t = DiagMatrix< SameElementVector<corner_elem_t> >;
   return BlockDiagMatrix<corner_t, const unwary_t<Matrix2>&, true>
          (corner_t(SameElementVector<corner_elem_t>(convert_to<E>(std::forward<Scalar1>(x1)), 1)), m2.top());
}

template <typename Matrix1, typename Scalar2, typename E,
          typename=std::enable_if_t<can_initialize<pure_type_t<Scalar2>, E>::value>>
auto diag(const GenericMatrix<Matrix1, E>& m1, Scalar2&& x2)
{
   using corner_elem_t = std::conditional_t<std::is_same<pure_type_t<Scalar2>, E>::value, Scalar2, E>;
   using corner_t = DiagMatrix< SameElementVector<corner_elem_t> >;
   return BlockDiagMatrix<const unwary_t<Matrix1>&, corner_t>
         (m1.top(), corner_t(SameElementVector<corner_elem_t>(convert_to<E>(std::forward<Scalar2>(x2)), 1)));
}

/// Create a block-anti-diagonal matrix. 
template <typename E, typename Matrix1, typename Matrix2>
auto anti_diag(const GenericMatrix<Matrix1, E>& m1, const GenericMatrix<Matrix2, E>& m2)
{
   return BlockDiagMatrix<const unwary_t<Matrix1>&,
                          const unwary_t<Matrix2>&, false>(m1.top(), m2.top());
}

/// Create a block-anti-diagonal matrix. 
/// The vector argument is treated as a square anti-diagonal matrix.
template <typename E, typename Vector1, typename Matrix2>
auto anti_diag(const GenericVector<Vector1, E>& v1, const GenericMatrix<Matrix2, E>& m2)
{
   return BlockDiagMatrix<DiagMatrix<const unwary_t<Vector1>&, false>,
                          const unwary_t<Matrix2>&, false>(anti_diag(v1), m2.top());
}

/// Create a block-anti-diagonal matrix. 
/// The vector argument is treated as a square anti-diagonal matrix.
template <typename E, typename Matrix1, typename Vector2>
auto anti_diag(const GenericMatrix<Matrix1, E>& m1, const GenericVector<Vector2, E>& v2)
{
   return BlockDiagMatrix<const unwary_t<Matrix1>&,
                          DiagMatrix<const unwary_t<Vector2>&, false>, false>(m1.top(), anti_diag(v2));
}

template <typename Scalar1, typename Matrix2, typename E,
          typename=std::enable_if_t<can_initialize<pure_type_t<Scalar1>, E>::value>>
auto anti_diag(Scalar1&& x1, const GenericMatrix<Matrix2, E>& m2)
{
   using corner_elem_t = std::conditional_t<std::is_same<pure_type_t<Scalar1>, E>::value, Scalar1, E>;
   using corner_t = DiagMatrix<SameElementVector<corner_elem_t>, false>;
   return BlockDiagMatrix<corner_t, const unwary_t<Matrix2>&, false>
          (corner_t(SameElementVector<corner_elem_t>(convert_to<E>(std::forward<Scalar1>(x1)), 1)), m2.top());
}

template <typename Matrix1, typename Scalar2, typename E,
          typename=std::enable_if_t<can_initialize<pure_type_t<Scalar2>, E>::value>>
auto anti_diag(const GenericMatrix<Matrix1, E>& m1, Scalar2&& x2)
{
   using corner_elem_t = std::conditional_t<std::is_same<pure_type_t<Scalar2>, E>::value, Scalar2, E>;
   using corner_t = DiagMatrix<SameElementVector<corner_elem_t>, false>;
   return BlockDiagMatrix<const unwary_t<Matrix1>&, corner_t, false>
          (m1.top(), corner_t(SameElementVector<corner_elem_t>(convert_to<E>(std::forward<Scalar2>(x2)), 1)));
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
   using value_type = E;
   using reference = E&;
   using const_reference = const E&;

   int i, j;    // row & col
   E a_ii, a_ij, a_ji, a_jj;

   SparseMatrix2x2() {}

   SparseMatrix2x2(int i_arg, int j_arg,
                   const E& a_ii_arg, const E& a_ij_arg,
                   const E& a_ji_arg, const E& a_jj_arg)
      : i(i_arg), j(j_arg), a_ii(a_ii_arg), a_ij(a_ij_arg), a_ji(a_ji_arg), a_jj(a_jj_arg) {}
};

template <typename E>
class Transposed< SparseMatrix2x2<E> > : public SparseMatrix2x2<E> {
protected:
   ~Transposed();
};

template <typename E>
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
   : public container_pair_base<MatrixRef1, MatrixRef2>
   , public GenericMatrix< MatrixProduct<MatrixRef1, MatrixRef2>,
                           typename lazy_product_traits<MatrixRef1, MatrixRef2>::element_type > {
public:
   using reference = typename lazy_product_traits<MatrixRef1,MatrixRef2>::reference;
   using value_type = typename lazy_product_traits<MatrixRef1,MatrixRef2>::value_type;
   using const_reference = reference;
   using typename GenericMatrix<MatrixProduct>::element_type;

   using container_pair_base<MatrixRef1, MatrixRef2>::container_pair_base;

   static constexpr bool is_sparse=check_container_ref_feature<MatrixRef1, sparse>::value &&
                                   check_container_ref_feature<MatrixRef2, sparse>::value;
   using persistent_type = std::conditional_t<is_sparse, SparseMatrix<element_type>, Matrix<element_type>>;
};

template <typename MatrixRef1, typename MatrixRef2>
struct spec_object_traits< MatrixProduct<MatrixRef1, MatrixRef2> >
   : spec_object_traits<is_container> {
   static constexpr bool is_lazy = true, is_temporary = true, is_always_const = true;
};

template <typename MatrixRef1, typename MatrixRef2>
class matrix_random_access_methods< MatrixProduct<MatrixRef1, MatrixRef2> > {
   using self_t = MatrixProduct<MatrixRef1,MatrixRef2>;
public:
   typename operations::mul<typename container_traits<MatrixRef1>::const_reference,
                            typename container_traits<MatrixRef2>::const_reference>::result_type
   operator() (int i, int j) const
   {
      const self_t& me=static_cast<const self_t&>(*this);
      return me.get_container1().row(i) * me.get_container2().col(j);
   }
};

template <typename MatrixRef1, typename MatrixRef2>
class ConcatRows< MatrixProduct<MatrixRef1, MatrixRef2> >
   : public container_product_impl< ConcatRows< MatrixProduct<MatrixRef1, MatrixRef2> >,
                                    mlist< Container1RefTag< masquerade<Rows, MatrixRef1> >,
                                           Container2RefTag< masquerade<Cols, MatrixRef2> >,
                                           OperationTag< BuildBinary<operations::mul> >,
                                           MasqueradedTop > >,
     public GenericVector< ConcatRows< MatrixProduct<MatrixRef1, MatrixRef2> >,
                           typename lazy_product_traits<MatrixRef1, MatrixRef2>::element_type > {
   typedef container_product_impl<ConcatRows> base_t;
public:
   const typename base_t::container1& get_container1() const
   {
      return rows(this->hidden().get_container1());
   }
   const typename base_t::container2& get_container2() const
   {
      return cols(this->hidden().get_container2());
   }
};

template <typename MatrixRef1, typename MatrixRef2>
class Rows< MatrixProduct<MatrixRef1, MatrixRef2> >
   : public modified_container_pair_impl< Rows< MatrixProduct<MatrixRef1, MatrixRef2> >,
                                          mlist< Container1RefTag< masquerade<pm::Rows, MatrixRef1> >,
                                                 Container2RefTag< same_value_container<MatrixRef2> >,
                                                 OperationTag< BuildBinary<operations::mul> >,
                                                 MasqueradedTop > > {
   typedef modified_container_pair_impl<Rows> base_t;
protected:
   ~Rows();
public:
   decltype(auto) get_container1() const
   {
      return rows(this->hidden().get_container1());
   }
   decltype(auto) get_container2() const
   {
      return as_same_value_container(this->hidden().get_container2_alias());
   }
};

template <typename MatrixRef1, typename MatrixRef2>
class Cols< MatrixProduct<MatrixRef1, MatrixRef2> >
   : public modified_container_pair_impl< Cols< MatrixProduct<MatrixRef1, MatrixRef2> >,
                                          mlist< Container1RefTag< same_value_container<MatrixRef1> >,
                                                 Container2RefTag< masquerade<pm::Cols, MatrixRef2> >,
                                                 OperationTag< BuildBinary<operations::mul> >,
                                                 MasqueradedTop > > {
   typedef modified_container_pair_impl<Cols> base_t;
protected:
   ~Cols();
public:
   decltype(auto) get_container1() const
   {
      return as_same_value_container(this->hidden().get_container1_alias());
   }
   decltype(auto) get_container2() const
   {
      return cols(this->hidden().get_container2());
   }
};

namespace internal {

template <typename LeftRef, typename RightRef, typename Left, typename Right>
struct matrix_product {
   using m_m_type = MatrixProduct<diligent_ref_t<unwary_t<LeftRef>>,
                                  diligent_ref_t<unwary_t<RightRef>>>;

   static m_m_type make(LeftRef&& l, RightRef&& r)
   {
      return m_m_type(diligent(unwary(std::forward<LeftRef>(l))),
                      diligent(unwary(std::forward<RightRef>(r))));
   }
};

template <typename LeftRef, typename RightRef, typename VectorRef, typename Right>
struct matrix_product<LeftRef, RightRef, DiagMatrix<VectorRef, true>, Right> {
   using m_m_type = LazyMatrix2<RepeatedCol<VectorRef>,
                                add_const_t<unwary_t<RightRef>>, BuildBinary<operations::mul>>;

   static m_m_type make(LeftRef&& l, RightRef&& r)
   {
      return m_m_type(RepeatedCol<VectorRef>(unwary(std::forward<LeftRef>(l)).get_vector(), r.cols()),
                      unwary(std::forward<RightRef>(r)));
   }
};

template <typename LeftRef, typename RightRef, typename Left, typename VectorRef>
struct matrix_product<LeftRef, RightRef, Left, DiagMatrix<VectorRef, true>> {
   using m_m_type = LazyMatrix2<add_const_t<unwary_t<LeftRef>>,
                                RepeatedRow<VectorRef>, BuildBinary<operations::mul>>;

   static m_m_type make(LeftRef&& l, RightRef&& r)
   {
      return m_m_type(unwary(std::forward<LeftRef>(l)),
                      RepeatedRow<VectorRef>(unwary(std::forward<RightRef>(r)).get_vector(), l.rows()));
   }
};

template <typename LeftRef, typename RightRef, typename LeftVectorRef, typename RightVectorRef>
struct matrix_product<LeftRef, RightRef, DiagMatrix<LeftVectorRef, true>, DiagMatrix<RightVectorRef, true> > {
   using diag_type = LazyVector2<LeftVectorRef, RightVectorRef, BuildBinary<operations::mul>>;
   using m_m_type = DiagMatrix<diag_type>;

   static m_m_type make(LeftRef&& l, RightRef&& r)
   {
      return m_m_type(diag_type(unwary(std::forward<LeftRef>(l)).get_vector(),
                                unwary(std::forward<RightRef>(r)).get_vector()));
   }
};

}

/* ---------------------------------------
 *  matrix arithmetic operations
 * --------------------------------------- */
namespace operations {

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_matrix, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef decltype(std::declval<LeftRef>() * std::declval<RightRef>()) result_type;

   template <typename L, typename R>
   result_type operator() (L&& l, R&& r) const
   {
      return std::forward<L>(l) * std::forward<R>(r);
   }
};

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_vector, is_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef decltype(std::declval<LeftRef>() * std::declval<RightRef>()) result_type;

   template <typename L, typename R>
   result_type operator() (L&& l, R&& r) const
   {
      return std::forward<L>(l) * std::forward<R>(r);
   }
};

} // end namespace operations

template <typename Matrix>
struct hash_func<Matrix, is_matrix> : hash_func< ConcatRows<Matrix> > {
   size_t operator() (const Matrix& m) const
   {
      return hash_func< ConcatRows<Matrix> >::operator()(concat_rows(m));
   }
};

template <typename TMatrix1, typename TMatrix2, typename E>
cmp_value lex_compare(const GenericMatrix<TMatrix1, E>& l, const GenericMatrix<TMatrix2, E>& r)
{
   if (l.rows()==0 || l.cols()==0) {
      return r.rows()==0 || r.cols()==0 ? cmp_eq : cmp_lt;
   }
   if (r.rows()==0 || r.cols()==0)
      return cmp_gt;
   return operations::cmp()(rows(l), rows(r));
}

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
   template <typename Matrix1, typename Matrix2, typename E>
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
