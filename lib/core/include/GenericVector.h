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

#include "polymake/GenericIO.h"
#include "polymake/TransformedContainer.h"
#include "polymake/internal/converters.h"
#include "polymake/IndexedSubset.h"
#include "polymake/SelectedSubset.h"
#include "polymake/ContainerChain.h"
#include "polymake/internal/sparse.h"

#include <algorithm>
#include <stdexcept>

namespace pm {

template <typename E> class Vector;
template <typename E> class SparseVector;

template <typename TVector, typename E=typename TVector::element_type>
class GenericVector;

template <typename T, typename... E>
using is_generic_vector = is_derived_from_instance_of<pure_type_t<T>, GenericVector, E...>;

template <typename VectorRef, typename Operation> class LazyVector1;
template <typename VectorRef1, typename VectorRef2, typename Operation> class LazyVector2;
template <typename VectorRef1, typename VectorRef2, typename Operation> class VectorTensorProduct;
template <typename VectorList> class VectorChain;
template <typename ElemRef> class SameElementVector;

/** @file GenericVector.h
    @class GenericVector
    @brief @ref generic "Generic type" for @ref vector_sec "vectors"
 */

template <typename TVector, typename E>
class GenericVector
   : public Generic<TVector> {
   template <typename, typename> friend class GenericVector;

   // TODO: allow only numeric types (after refactoring in atint)
   static_assert(!is_among<E, bool, int>::value, "invalid Vector element type");

protected:
   GenericVector() {}
   GenericVector(const GenericVector&) {}
public:
   /// element type
   using element_type = E;
   /// determine if the persistent type is sparse
   static constexpr bool is_sparse = check_container_feature<TVector, sparse>::value;
   /// @ref generic "persistent type"
   using persistent_type = std::conditional_t<is_sparse, SparseVector<E>, Vector<E>>;
   using persistent_dense_type = Vector<E>;
   /// top type
   using typename Generic<TVector>::top_type;
   /// @ref generic "generic type"
   using generic_type = GenericVector;
   using sparse_discr = std::conditional_t<check_container_feature<TVector, pure_sparse>::value, pure_sparse,
                                           std::conditional_t<is_sparse, sparse, dense>>;

protected:
   template <typename TVector2>
   void assign_impl(const TVector2& v, dense)
   {
      copy_range(ensure(v, dense()).begin(), entire(this->top()));
   }

   template <typename TVector2>
   void assign_impl(const TVector2& v, pure_sparse)
   {
      assign_sparse(this->top(), ensure(v, pure_sparse()).begin());
   }

   template <typename TVector2, typename E2>
   constexpr bool trivial_assignment(const GenericVector<TVector2, E2>&) const { return false; }

   constexpr bool trivial_assignment(const GenericVector& v) const { return this==&v; }

   template <typename TVector2>
   void swap(GenericVector<TVector2, E>& v, dense)
   {
      swap_ranges(entire(this->top()), v.top().begin());
   }

   template <typename TVector2>
   void swap(GenericVector<TVector2, E>& v, pure_sparse)
   {
      swap_sparse(this->top(), ensure(v.top(), pure_sparse()));
   }

   template <typename TVector2>
   void assign(const TVector2& v)
   {
      assign_impl(v, sparse_discr());
   }

   template <typename TVector2, typename Operation>
   void assign_op_impl(const TVector2& v, const Operation& op_arg, dense, dense)
   {
      perform_assign(entire(this->top()), v.begin(), op_arg);
   }

   template <typename TVector2, typename Operation>
   void assign_op_impl(const TVector2& v, const Operation& op_arg, dense, sparse)
   {
      typedef binary_op_builder<Operation, typename ensure_features<TVector, end_sensitive>::const_iterator, typename ensure_features<TVector2, end_sensitive>::const_iterator> opb;
      const typename opb::operation& op=opb::create(op_arg);
      Int i_prev = 0;
      auto dst = this->top().begin();
      for (auto src2 = entire(v); !src2.at_end(); ++src2) {
         Int i = src2.index();
         std::advance(dst, i-i_prev);
         op.assign(*dst, *src2);
         i_prev = i;
      }
   }

   template <typename TVector2, typename Operation, typename discr2>
   typename std::enable_if<!operations::is_partially_defined_for<Operation, TVector, TVector2>::value, void>::type
   assign_op_impl(const TVector2& v, const Operation& op, sparse, discr2)
   {
      perform_assign(entire(this->top()), v.begin(), op);
   }

   template <typename TVector2, typename Operation, typename discr2>
   typename std::enable_if<operations::is_partially_defined_for<Operation, TVector, TVector2>::value, void>::type
   assign_op_impl(const TVector2& v, const Operation& op, sparse, discr2)
   {
      perform_assign_sparse(this->top(), ensure(v, pure_sparse()).begin(), op);
   }

   template <typename E2>
   void fill_impl(const E2& x, dense)
   {
      fill_range(entire(this->top()), x);
   }

   template <typename E2>
   void fill_impl(const E2& x, pure_sparse)
   {
      if (!is_zero(x))
         fill_sparse(this->top(), ensure(same_value_in_context<E>(x), indexed()).begin());
      else
         this->top().clear();
   }

   // undefined for dense
   void remove0s(dense);

   void remove0s(pure_sparse)
   {
      top_type& me=this->top();
      for (auto e=me.begin(); !e.at_end(); ) {
         if (!is_zero(*e))
            ++e;
         else
            me.erase(e++);
      }
   }

public:
   Int dim() const { return get_dim(this->top()); }

   bool prefer_sparse_representation() const
   {
      return is_sparse && this->top().size()*2 < dim();
   }

   template <typename TVector2>
   void swap(GenericVector<TVector2, E>& v)
   {
      if (trivial_assignment(v)) return;

      if (POLYMAKE_DEBUG || is_wary<TVector>() || is_wary<TVector2>()) {
         if (dim() != v.dim())
            throw std::runtime_error("GenericVector::swap - dimension mismatch");
      }
      using sparse_discr2 = std::conditional_t<(check_container_feature<TVector, sparse>::value ||
                                                check_container_feature<TVector2, sparse>::value),
                                               pure_sparse, dense>;
      swap(v, sparse_discr2());
   }

   template <typename Operation>
   void assign_op(const Operation& op)
   {
      perform_assign(entire(this->top()), op);
   }

   template <typename TVector2, typename Operation>
   void assign_op(const TVector2& v, const Operation& op)
   {
      using sparse_discr2 = std::conditional_t<check_container_feature<TVector2, sparse>::value, sparse, dense>;
      assign_op_impl(v, op, sparse_discr(), sparse_discr2());
   }

   template <typename E2>
   void fill(const E2& x)
   {
      this->top().fill_impl(x, sparse_discr());
   }

   top_type& operator= (const GenericVector& v)
   {
      if (!trivial_assignment(v)) {
         if (!object_traits<TVector>::is_resizeable && (POLYMAKE_DEBUG || is_wary<TVector>())) {
            if (dim() != v.dim())
               throw std::runtime_error("GenericVector::operator= - dimension mismatch");
         }
         this->top().assign(v.top());
      }
      return this->top();
   }

   template <typename TVector2, typename E2,
             typename=typename std::enable_if<can_assign_to<E2, E>::value>::type>
   top_type& operator= (const GenericVector<TVector2, E2>& v)
   {
      if (!object_traits<TVector>::is_resizeable && (POLYMAKE_DEBUG || is_wary<TVector>())) {
         if (dim() != v.dim())
            throw std::runtime_error("GenericVector::operator= - dimension mismatch");
      }
      this->top().assign(v.top());
      return this->top();
   }

   template <typename E2,
             typename=typename std::enable_if<can_assign_to<E2, E>::value>::type>
   top_type& operator= (std::initializer_list<E2> l)
   {
      if (!object_traits<TVector>::is_resizeable && (POLYMAKE_DEBUG || is_wary<TVector>())) {
         if (dim() != l.size())
            throw std::runtime_error("GenericVector::operator= - dimension mismatch");
      }
      this->top().assign(l);
      return this->top();
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
      using v_type = LazyVector1<add_const_t<unwary_t<Left>>, Operation>;
   };

   template <typename Operation>
   struct lazy_op<persistent_type, void, Operation, void> {
      using r_type = persistent_type&&;
   };

   // TODO: && is_defined<Operation>
   template <typename Left, typename Right, typename Operation>
   struct lazy_op<Left, Right, Operation,
                  std::enable_if_t<!std::is_same<Operation, BuildBinary<operations::mul>>::value &&
                                   is_this<Left>::value &&
                                   temp_ignore<true, !std::is_same<Left, persistent_type>::value>::value &&
                                   is_generic_vector<Right>::value &&
                                   isomorphic_types<E, typename pure_type_t<Right>::element_type>::value>> {
      using v_v_type = LazyVector2<add_const_t<unwary_t<Left>>, add_const_t<unwary_t<Right>>, Operation>;
   };

   // TODO: && is_defined<Operation>
   template <typename Right, typename Operation>
   struct lazy_op<persistent_type, Right, Operation,
                  std::enable_if_t<!std::is_same<Operation, BuildBinary<operations::mul>>::value &&
                                   temp_ignore<false, std::is_same<TVector, persistent_type>::value>::value &&
                                   is_generic_vector<Right>::value &&
                                   isomorphic_types<E, typename pure_type_t<Right>::element_type>::value>> {
      using r_v_type = persistent_type&&;
   };

   template <typename Left, typename Operation>
   struct lazy_op<Left, persistent_type, Operation,
                  std::enable_if_t<!std::is_same<Operation, BuildBinary<operations::mul>>::value &&
                                   is_generic_vector<Left>::value &&
                                   temp_ignore<false, !std::is_same<Left, persistent_type>::value>::value &&
                                   isomorphic_types<E, typename pure_type_t<Left>::element_type>::value>> {
      using v_r_type = persistent_type&&;
   };

   template <typename Left, typename Right, typename Operation>
   struct lazy_op<Left, Right, Operation,
                  std::enable_if_t<isomorphic_types<E, pure_type_t<Right>>::value &&
                                   is_this<Left>::value &&
                                   temp_ignore<true, !std::is_same<Left, persistent_type>::value>::value>> {
      using scalar_type = same_value_container<diligent_ref_t<unwary_t<Right>>>;
      using v_s_type = LazyVector2<add_const_t<unwary_t<Left>>, scalar_type, Operation>;

      static v_s_type make(Left&& l, Right&& r)
      {
         return v_s_type(unwary(std::forward<Left>(l)), scalar_type(diligent(unwary(std::forward<Right>(r)))));
      }
   };

#if 0
   // TODO enable when temp_ignore goes
   template <typename Right, typename Operation>
   struct lazy_op<persistent_type, Right, Operation,
                  std::enable_if_t<isomorphic_types<E, pure_type_t<Right>>>> {
      using r_s_type = persistent_type&&;
   };
#endif

   template <typename Left, typename Right, typename Operation>
   struct lazy_op<Left, Right, Operation,
                  std::enable_if_t<isomorphic_types<E, pure_type_t<Left>>::value &&
                                   is_this<Right>::value &&
                                   temp_ignore<true, !std::is_same<Right, persistent_type>::value>::value>> {
      using scalar_type = same_value_container<diligent_ref_t<unwary_t<Left>>>;
      using s_v_type = LazyVector2<scalar_type, add_const_t<unwary_t<Right>>, Operation>;

      static s_v_type make(Left&& l, Right&& r)
      {
         return s_v_type(scalar_type(diligent(unwary(std::forward<Left>(l)))), unwary(std::forward<Right>(r)));
      }
   };

#if 0
   // TODO enable when temp_ignore goes
   template <typename Left, typename Operation>
   struct lazy_op<Left, persistent_type, Operation,
                  std::enable_if_t<isomorphic_types<E, pure_type_t<Left>>>> {
      using s_r_type = persistent_type&&;
   };
#endif

   // TODO: && is_defined<mul>
   template <typename Left, typename Right>
   struct lazy_op<Left, Right, BuildBinary<operations::mul>,
                  std::enable_if_t<is_this<Left>::value &&
                                   is_generic_vector<Right>::value &&
                                   isomorphic_types<E, typename pure_type_t<Right>::element_type>::value &&
                                   cleanOperations::can<cleanOperations::mul, E, typename pure_type_t<Right>::element_type>::value>> {
      using v_v_type = typename cleanOperations::can<cleanOperations::mul, E, typename pure_type_t<Right>::element_type>::type;
   };

   template <typename Left, typename Right, typename Operation, typename=void>
   struct lazy_tensor {};

   // TODO: is_defined<Operation>
   template <typename Left, typename Right, typename Operation>
   struct lazy_tensor<Left, Right, Operation,
                      std::enable_if_t<is_this<Left>::value &&
                                       is_generic_vector<Right>::value &&
                                       isomorphic_types<E, typename pure_type_t<Right>::element_type>::value>> {
      using v_v_type = VectorTensorProduct<add_const_t<unwary_t<Left>>, add_const_t<unwary_t<Right>>, Operation>;
   };

#define PmCheckVectorDim(Left, l, Right, r, sign) \
   if (POLYMAKE_DEBUG || is_wary<Left>() || is_wary<Right>()) \
      if (l.dim() != r.dim()) \
         throw std::runtime_error("GenericVector::operator" sign " - dimension mismatch")

public:
   /// negate
   template <typename Op>
   friend
   typename lazy_op<Op, void, BuildUnary<operations::neg>>::v_type
   operator- (Op&& m)
   {
      return typename lazy_op<Op, void, BuildUnary<operations::neg>>::v_type(unwary(std::forward<Op>(m)));
   }

   template <typename Op>
   friend
   typename lazy_op<Op, void, BuildUnary<operations::neg>>::r_type
   operator- (Op&& v)
   {
      return std::move(v.negate());
   }

   /// negate elements in place
   top_type& negate()
   {
      this->top().assign_op(BuildUnary<operations::neg>());
      return this->top();
   }

   /// add a vector
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::add>>::v_v_type
   operator+ (Left&& l, Right&& r)
   {
      PmCheckVectorDim(Left, l, Right, r, "+");
      return typename lazy_op<Left, Right, BuildBinary<operations::add>>::v_v_type(unwary(std::forward<Left>(l)),
                                                                                   unwary(std::forward<Right>(r)));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::add>>::r_v_type
   operator+ (Left&& l, Right&& r)
   {
      PmCheckVectorDim(Left, l, Right, r, "+");
      return std::move(l += r);
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::add>>::v_r_type
   operator+ (Left&& l, Right&& r)
   {
      PmCheckVectorDim(Left, l, Right, r, "+");
      return std::move(r += l);
   }

   template <typename Right, typename E2>
   std::enable_if_t<isomorphic_types<E, E2>::value &&
                    cleanOperations::can<cleanOperations::add, E, E2>::value, top_type&>
   operator+= (const GenericVector<Right, E2>& r)
   {
      PmCheckVectorDim(TVector, (*this), Right, r, "+=");
      this->top().assign_op(r.top(), BuildBinary<operations::add>());
      return this->top();
   }

   /// subtract a vector
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::sub>>::v_v_type
   operator- (Left&& l, Right&& r)
   {
      PmCheckVectorDim(Left, l, Right, r, "-");
      return typename lazy_op<Left, Right, BuildBinary<operations::sub>>::v_v_type(unwary(std::forward<Left>(l)),
                                                                                   unwary(std::forward<Right>(r)));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::add>>::r_v_type
   operator- (Left&& l, Right&& r)
   {
      PmCheckVectorDim(Left, l, Right, r, "-");
      return std::move(l -= r);
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::add>>::v_r_type
   operator- (Left&& l, Right&& r)
   {
      PmCheckVectorDim(Left, l, Right, r, "-");
      return std::move(r.negate() += l);
   }

   template <typename Right, typename E2>
   std::enable_if_t<isomorphic_types<E, E2>::value &&
                    cleanOperations::can<cleanOperations::sub, E, E2>::value, top_type&>
   operator-= (const GenericVector<Right, E2>& r)
   {
      PmCheckVectorDim(TVector, (*this), Right, r, "-=");
      this->top().assign_op(r.top(), BuildBinary<operations::sub>());
      return this->top();
   }

   /// multiply with a scalar
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::mul>>::v_s_type
   operator* (Left&& l, Right&& r)
   {
      return lazy_op<Left, Right, BuildBinary<operations::mul>>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::mul>>::s_v_type
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

   /// scalar (dot) product
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::mul>>::v_v_type
   operator* (Left&& l, Right&& r)
   {
      if (POLYMAKE_DEBUG || is_wary<Left>() || is_wary<Right>()) {
         if (l.dim() != r.dim())
            throw std::runtime_error("GenericVector::operator* - dimension mismatch");
      }
      return accumulate(attach_operation(unwary(l), unwary(r), BuildBinary<operations::mul>()), BuildBinary<operations::add>());
   }

   template <typename Right>
   std::enable_if_t<isomorphic_types<E, Right>::value &&
                    cleanOperations::can<cleanOperations::mul, E, Right>::value, top_type&>
   operator*= (const Right& r)
   {
      if (!is_zero(r))
         this->top().assign_op(same_value_in_context<E>(r), BuildBinary<operations::mul>());
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
         this->top().assign_op(same_value_in_context<E>(l), BuildBinary<operations::mul_from_left>());
      else
         fill(l);
      return this->top();
   }

   /// multiply a vector with itself
   friend
   E sqr(const GenericVector& v)
   {
      return accumulate(attach_operation(v.top(), BuildUnary<operations::square>()), BuildBinary<operations::add>());
   }

   /// tensor product of two vectors
   template <typename Left, typename Right>
   friend
   typename lazy_tensor<Left, Right, BuildBinary<operations::mul>>::v_v_type
   tensor_product(Left&& l, Right&& r)
   {
      return typename lazy_tensor<Left, Right, BuildBinary<operations::mul>>::v_v_type(unwary(std::forward<Left>(l)),
                                                                                       unwary(std::forward<Right>(r)));
   }

   /// divide by a scalar
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::div>>::v_s_type
   operator/ (Left&& l, Right&& r)
   {
      return lazy_op<Left, Right, BuildBinary<operations::div>>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::div>>::r_s_type
   operator/ (Left&& l, Right&& r)
   {
      return std::move(l /= r);
   }

   template <typename Right>
   std::enable_if_t<isomorphic_types<E, Right>::value &&
                    cleanOperations::can<cleanOperations::div, E, Right>::value, top_type&>
   operator/= (const Right& r)
   {
      this->top().assign_op(same_value_in_context<E>(r), BuildBinary<operations::div>());
      return this->top();
   }

   /// divide without residue by a scalar
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::divexact>>::v_s_type
   div_exact(Left&& l, Right&& r)
   {
      return lazy_op<Left, Right, BuildBinary<operations::divexact>>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, BuildBinary<operations::divexact>>::r_s_type
   div_exact(Left&& l, Right&& r)
   {
      return std::move(l.div_exact(r));
   }

   template <typename Right>
   std::enable_if_t<isomorphic_types<E, Right>::value &&
                    cleanOperations::can<cleanOperations::div, E, Right>::value, top_type&>
   div_exact(const Right& r)
   {
      this->top().assign_op(same_value_in_context<E>(r), BuildBinary<operations::divexact>());
      return this->top();
   }

#undef PmCheckVectorDim

   /// divide by the first element
   top_type& dehomogenize()
   {
      const E first=this->top().front();
      this->top()/=first;
      return this->top();
   }

   /// subtracts first element
   top_type& dehomogenize_trop()
   {
      const E first=this->top().front();
      this->top()-=first;
      return this->top();
   }

   /// remove all zero elements which might have been overseen in some previous operation
   void remove0s()
   {
      remove0s(sparse_discr());
   }

protected:
   template <typename VectorRef, typename IndexSetRef>
   static auto make_slice(VectorRef&& vector, IndexSetRef&& indices)
   {
      if (POLYMAKE_DEBUG || is_wary<TVector>()) {
         if (!set_within_range(indices, vector.dim()))
            throw std::runtime_error("GenericVector::slice - indices out of range");
      }
      using result_type = IndexedSlice<VectorRef, typename final_index_set<IndexSetRef>::type>;
      return result_type(std::forward<VectorRef>(vector), prepare_index_set(std::forward<IndexSetRef>(indices), [&](){ return vector.dim(); }));
   }

public:
   //@{ 
   /** 
    * Select a vector slice consisting of elements with given indices. 
    * The last variant selects a contiguous range of indices beginning 
    * with start. 
    * size==-1 means up to the end of the vector.
    * The const variants of these methods create immutable slice objects.
    * The indices must lie in the valid range.
    */
   template <typename IndexSetRef>
   // gcc 5 can't digest auto here
   IndexedSlice<const typename Unwary<TVector>::type&, typename final_index_set<IndexSetRef>::type>
   slice(IndexSetRef&& indices,
         std::enable_if_t<isomorphic_to_container_of<pure_type_t<IndexSetRef>, Int>::value, std::nullptr_t> = nullptr) const &
   {
      return make_slice(unwary(*this), std::forward<IndexSetRef>(indices));
   }

   template <typename IndexSetRef>
   // gcc 5 can't digest auto here
   IndexedSlice<typename Unwary<TVector>::type&, typename final_index_set<IndexSetRef>::type>
   slice(IndexSetRef&& indices,
         std::enable_if_t<isomorphic_to_container_of<pure_type_t<IndexSetRef>, Int>::value, std::nullptr_t> = nullptr) &
   {
      return make_slice(unwary(*this), std::forward<IndexSetRef>(indices));
   }

   template <typename IndexSetRef>
   // gcc 5 can't digest auto here
   IndexedSlice<typename Unwary<TVector>::type, typename final_index_set<IndexSetRef>::type>
   slice(IndexSetRef&& indices,
         std::enable_if_t<isomorphic_to_container_of<pure_type_t<IndexSetRef>, Int>::value, std::nullptr_t> = nullptr) &&
   {
      return make_slice(unwary(std::move(*this)), std::forward<IndexSetRef>(indices));
   }
   //@}

   template <typename Result>
   struct rebind_generic {
      typedef GenericVector<Result, E> type;
   };

   // stub for BlockMatrix
   void stretch_dim(Int d) const
   {
      throw std::runtime_error("dimension mismatch");
   }

protected:
   template <typename Left, typename Right, typename = void>
   struct concat {};

   template <typename Left, typename Right>
   struct concat<Left, Right, std::enable_if_t<is_this<Left>::value &&
                                               is_generic_vector<Right, E>::value>> {
      using v_v_type = typename chain_compose<VectorChain, true>::template with<unwary_t<Left>, unwary_t<Right>>;
   };

   template <typename Left, typename Right>
   struct concat<Left, Right, std::enable_if_t<is_this<Left>::value &&
                                               isomorphic_types<E, pure_type_t<Right>>::value>> {
      static constexpr bool homogeneous = std::is_same<E, pure_type_t<Right>>::value;
      using scalar_type = SameElementVector<std::conditional_t<homogeneous, unwary_t<Right>, E>>;
      using v_s_type = typename chain_compose<VectorChain, true>::template with<unwary_t<Left>, scalar_type>;

      static v_s_type make(Left&& l, Right&& r)
      {
         return v_s_type(unwary(std::forward<Left>(l)),
                         scalar_type(convert_to<E>(unwary(std::forward<Right>(r))), 1));
      }
   };

   template <typename Left, typename Right>
   struct concat<Left, Right, std::enable_if_t<is_this<Right>::value &&
                                               isomorphic_types<E, pure_type_t<Left>>::value>> {
      static constexpr bool homogeneous = std::is_same<E, pure_type_t<Left>>::value;
      using scalar_type = SameElementVector<std::conditional_t<homogeneous, unwary_t<Left>, E>>;
      using s_v_type = typename chain_compose<VectorChain, true>::template with<scalar_type, unwary_t<Right>>;

      static s_v_type make(Left&& l, Right&& r)
      {
         return s_v_type(scalar_type(convert_to<E>(unwary(std::forward<Left>(l))), 1),
                         unwary(std::forward<Right>(r)));
      }
   };

public:
   template <typename Left, typename Right>
   friend
   typename concat<Left, Right>::v_v_type operator| (Left&& l, Right&& r)
   {
      return typename concat<Left, Right>::v_v_type(unwary(std::forward<Left>(l)),
                                                    unwary(std::forward<Right>(r)));
   }

   template <typename Left, typename Right>
   friend
   typename concat<Left, Right>::v_s_type operator| (Left&& l, Right&& r)
   {
      return concat<Left, Right>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right>
   friend
   typename concat<Left, Right>::s_v_type operator| (Left&& l, Right&& r)
   {
      return concat<Left, Right>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   // comparisons

   template <typename TVector2, typename E2>
   std::enable_if_t<are_comparable<E, E2>::value, bool>
   operator== (const GenericVector<TVector2, E2>& v2) const
   {
      return operations::cmp_unordered()(this->top(), v2.top()) == cmp_eq;
   }

   template <typename TVector2, typename E2>
   std::enable_if_t<are_comparable<E, E2>::value, bool>
   operator!= (const GenericVector<TVector2, E2>& v2) const
   {
      return !(*this == v2);
   }

#if POLYMAKE_DEBUG
   void dump() const __attribute__((used)) { cerr << this->top() << endl; }
#endif
};

struct is_vector;

template <typename TVector, typename E>
struct spec_object_traits< GenericVector<TVector, E> >
   : spec_or_model_traits<TVector, is_container> {
   typedef is_vector generic_tag;
   static const bool allow_sparse=true;

   static bool is_zero(const GenericVector<TVector>& v)
   {
      return entire(attach_selector(v.top(), BuildUnary<operations::non_zero>())).at_end();
   }
};

/* --------------------------------------------
 *  LazyVector1
 * lazy evaluation of an unary vector operator
 * -------------------------------------------- */

template <typename VectorRef, typename Operation>
class LazyVector1
   : public TransformedContainer<VectorRef, Operation>
   , public GenericVector< LazyVector1<VectorRef,Operation>,
                           typename object_traits<typename TransformedContainer<VectorRef, Operation>::value_type>::persistent_type > {
   using base_t = TransformedContainer<VectorRef, Operation>;
public:
   using TransformedContainer<VectorRef, Operation>::TransformedContainer;

   Int dim() const { return get_dim(this->get_container()); }
};

template <typename VectorRef, typename Operation>
struct spec_object_traits< LazyVector1<VectorRef, Operation> >
   : spec_object_traits<is_container> {
   static constexpr bool is_lazy = true, is_temporary = true, is_always_const = true;
};

template <typename VectorRef, typename Operation, typename Feature>
struct check_container_feature<LazyVector1<VectorRef,Operation>, Feature>
   : check_container_feature<TransformedContainer<VectorRef,Operation>, Feature> {};

/* --------------------------------------------
 *  LazyVector2
 * lazy evaluation of a binary vector operator
 * -------------------------------------------- */

template <typename VectorRef1, typename VectorRef2, typename Operation>
class LazyVector2
   : public TransformedContainerPair<VectorRef1, VectorRef2, Operation>
   , public GenericVector< LazyVector2<VectorRef1,VectorRef2,Operation>,
                           typename object_traits<typename TransformedContainerPair<VectorRef1, VectorRef2, Operation>::value_type>::persistent_type > {
   using base_t = TransformedContainerPair<VectorRef1, VectorRef2, Operation>;
protected:
   Int dim_impl(std::true_type) const { return get_dim(this->get_container2()); }
   Int dim_impl(std::false_type) const { return get_dim(this->get_container1()); }
public:
   using TransformedContainerPair<VectorRef1, VectorRef2, Operation>::TransformedContainerPair;

   Int dim() const
   {
      return dim_impl(bool_constant< check_container_ref_feature<VectorRef1,unlimited>::value >());
   }
};

template <typename VectorRef1, typename VectorRef2, typename Operation>
struct spec_object_traits< LazyVector2<VectorRef1, VectorRef2, Operation> >
   : spec_object_traits<is_container> {
   static constexpr bool is_lazy = true, is_temporary = true, is_always_const = true;
};

template <typename VectorRef1, typename VectorRef2, typename Operation, typename Feature>
struct check_container_feature<LazyVector2<VectorRef1, VectorRef2, Operation>, Feature>
   : check_container_feature<TransformedContainerPair<VectorRef1, VectorRef2, Operation>, Feature> {};

/// explicit conversion of vector elements to another type
template <typename TargetType, typename TVector>
const TVector& convert_to(const GenericVector<TVector, TargetType>& v)
{
   return v.top();
}

template <typename TargetType, typename TVector, typename E,
          typename=std::enable_if_t<can_initialize<E, TargetType>::value && !std::is_same<E, TargetType>::value>>
auto convert_to(const GenericVector<TVector, E>& v)
{
   return LazyVector1<const TVector&, conv<E, TargetType>>(v.top());
}

template <typename T>
decltype(auto) convert_to_persistent(T&& x, std::enable_if_t<std::is_same<pure_type_t<T>, typename pure_type_t<T>::persistent_type>::value, std::nullptr_t> = nullptr)
{
   return std::forward<T>(x);
}

template <typename T>
decltype(auto) convert_to_persistent(T&& x, std::enable_if_t<!std::is_same<pure_type_t<T>, typename pure_type_t<T>::persistent_type>::value, std::nullptr_t> = nullptr)
{
   return typename pure_type_t<T>::persistent_type(std::forward<T>(x));
}

template <typename T>
decltype(auto) convert_to_persistent_dense(T&& x, std::enable_if_t<std::is_same<pure_type_t<T>, typename pure_type_t<T>::persistent_dense_type>::value, std::nullptr_t> =nullptr)
{
   return std::forward<T>(x);
}

template <typename T>
decltype(auto) convert_to_persistent_dense(T&& x, std::enable_if_t<!std::is_same<pure_type_t<T>, typename pure_type_t<T>::persistent_dense_type>::value, std::nullptr_t> =nullptr)
{
   return typename pure_type_t<T>::persistent_dense_type(std::forward<T>(x));
}

template <typename TargetType, typename TVector, typename E>
const TVector& convert_lazily(const GenericVector<TVector, E>& v,
                              std::enable_if_t<std::is_convertible<E, TargetType>::value, void**> =nullptr)
{
   return v.top();
}

template <typename TargetType, typename TVector, typename E>
auto convert_lazily(const GenericVector<TVector, E>& v,
                    std::enable_if_t<can_initialize<E, TargetType>::value && !std::is_convertible<E, TargetType>::value, void**> =nullptr)
{
   return LazyVector1<const TVector&, conv<E, TargetType>>(v.top());
}

template <typename Vector, typename Operation>
auto apply_operation(const GenericVector<Vector>& v, const Operation& op)
{
   return LazyVector1<const Vector&, Operation>(v.top(), op);
}

/* --------------------------------------------
 *  VectorTensorProduct
 * lazy evaluation of a vector tensor product
 * -------------------------------------------- */
template <typename VectorRef1, typename VectorRef2, typename Operation>
class VectorTensorProduct
   : public ContainerProduct<VectorRef1, VectorRef2, Operation>
   , public GenericVector< VectorTensorProduct<VectorRef1, VectorRef2, Operation>,
                           typename object_traits<typename ContainerProduct<VectorRef1, VectorRef2, Operation>::value_type>::persistent_type > {
   using base_t = ContainerProduct<VectorRef1, VectorRef2, Operation>;
public:
   using ContainerProduct<VectorRef1, VectorRef2, Operation>::ContainerProduct;
   using base_t::dim;
};

template <typename VectorRef1, typename VectorRef2, typename Operation>
struct spec_object_traits< VectorTensorProduct<VectorRef1,VectorRef2,Operation> >
   : spec_object_traits<is_container> {
   static const bool is_lazy = true, is_temporary = true, is_always_const = true;
};

template <typename VectorRef1, typename VectorRef2, typename Operation, typename Feature>
struct check_container_feature<VectorTensorProduct<VectorRef1,VectorRef2,Operation>, Feature>
   : check_container_feature<ContainerProduct<VectorRef1, VectorRef2, Operation>, Feature> {};

/* -------------
 *  VectorChain
 * ------------- */

template <typename VectorList>
class VectorChain
   : public ContainerChain<VectorList>
   , public GenericVector< VectorChain<VectorList>,
                           typename pure_type_t<typename mlist_head<VectorList>::type>::element_type > {

   using element_types = typename mlist_transform_unary<VectorList, extract_element_type>::type;
   static_assert(mlist_length<typename mlist_remove_duplicates<element_types>::type>::value == 1,
                 "concatenated vectors of different element types");

   using base_t = ContainerChain<VectorList>;
   using arg_helper = chain_arg_helper<pm::VectorChain>;
public:
   template <typename... Args, typename=std::enable_if_t<arg_helper::allow(VectorList(), mlist<Args...>())>>
   explicit VectorChain(Args&&... args)
      : base_t(arg_helper(), std::forward<Args>(args)...) {}

   // TODO: =delete
   VectorChain(const VectorChain&) = default;
   VectorChain(VectorChain&&) = default;

   VectorChain& operator= (const VectorChain& other) { return VectorChain::generic_type::operator=(other); }
   using VectorChain::generic_type::operator=;
   using base_t::dim;
};

template <typename VectorList>
struct spec_object_traits< VectorChain<VectorList> >
   : spec_object_traits< ContainerChain<VectorList> > {};

template <typename VectorList, typename Feature>
struct check_container_feature< VectorChain<VectorList>, Feature>
   : check_container_feature< ContainerChain<VectorList>, Feature> {};

/* -------------------
 *  SameElementVector
 * ------------------- */

template <typename ElemRef>
class SameElementVector
   : public repeated_value_container<ElemRef>
   , public GenericVector<SameElementVector<ElemRef>,
                          typename object_traits<pure_type_t<ElemRef>>::persistent_type> {
   using base_t = repeated_value_container<ElemRef>;
public:
   using repeated_value_container<ElemRef>::repeated_value_container;
   using base_t::dim;
   using base_t::stretch_dim;
   using GenericVector<SameElementVector>::stretch_dim;
};

template <typename ElemRef>
struct spec_object_traits< SameElementVector<ElemRef> >
   : spec_object_traits< repeated_value_container<ElemRef> > {};

/// Create a vector with all entries equal to the given element x.
template <typename E>
auto same_element_vector(E&& x, Int dim = 0)
{
   return SameElementVector<prevent_int_element<E>>(std::forward<E>(x), dim);
}

/// Create a vector with all entries equal to 1.
template <typename E>
auto ones_vector(Int dim = 0)
{
   return same_element_vector(one_value<prevent_int_element<E>>(), dim);
}

/// Create a vector with all entries equal to 0.
template <typename E>
auto zero_vector(Int dim = 0)
{
   return same_element_vector(zero_value<prevent_int_element<E>>(), dim);
}

template <typename E>
auto scalar2vector(E&& x)
{
   return same_element_vector(std::forward<E>(x), 1);
}

template <typename SetRef,
          bool = check_container_ref_feature<SetRef, sparse_compatible>::value,
          bool = is_instance_of<pure_type_t<SetRef>, Complement>::value>
struct SameElementSparseVector_helper {
   static constexpr bool has_dim = true;
   using container = SetRef;
   using alias_type = alias<container>;

   template <typename Arg>
   using accept_arg = std::is_constructible<alias_type, Arg>;

   template <typename Arg, typename=std::enable_if_t<accept_arg<Arg>::value>>
   static Arg&& create(Arg&& c, Int)
   {
      return std::forward<Arg>(c);
   }
   static decltype(auto) get_container(const alias_type& a)
   {
      return *a;
   }
};

template <typename SetRef>
struct SameElementSparseVector_helper<SetRef, false, true> {
   static constexpr bool has_dim = check_container_ref_feature<typename mget_template_parameter<pure_type_t<SetRef>, 0>::type, sparse_compatible>::value;
   using container = SetRef;
   using alias_type = pure_type_t<SetRef>;

   template <typename Arg>
   using accept_arg = std::is_constructible<alias_type, Arg, Int>;

   template <typename Arg, typename=std::enable_if_t<accept_arg<Arg>::value>>
   static alias_type create(Arg&& c, Int d)
   {
      return alias_type(std::forward<Arg>(c), d);
   }
   static const alias_type& get_container(const alias_type& a)
   {
      return a;
   }
};

template <typename SetRef>
struct SameElementSparseVector_helper<SetRef, false, false>
   : SameElementSparseVector_helper<Set_with_dim<SetRef>, false, true> {};

template <typename SetRef, typename ElemRef>
class SameElementSparseVector
   : public modified_container_pair_impl< SameElementSparseVector<SetRef, ElemRef>,
                                          mlist< Container1RefTag< same_value_container<ElemRef> >,
                                                 Container2RefTag< typename SameElementSparseVector_helper<SetRef>::container >,
                                                 OperationTag< pair<nothing, BuildBinaryIt<operations::dereference2> > > > >
   , public GenericVector< SameElementSparseVector<SetRef, ElemRef>,
                           typename object_traits<typename deref<ElemRef>::type>::persistent_type > {
   using base_t = modified_container_pair_impl<SameElementSparseVector>;
   using helper = SameElementSparseVector_helper<SetRef>;
protected:
   typename helper::alias_type set;
   alias<ElemRef> apparent_elem;

public:
   using container_category = typename least_derived_class<bidirectional_iterator_tag, typename container_traits<SetRef>::category>::type;

   SameElementSparseVector() = default;

   template <typename Arg1, typename Arg2,
             typename=std::enable_if_t<helper::template accept_arg<Arg1>::value &&
                                       std::is_constructible<alias<ElemRef>, Arg2>::value>>
   SameElementSparseVector(Arg1&& set_arg, Arg2&& data_arg, Int dim_arg = -1)
      : set(helper::create(std::forward<Arg1>(set_arg), dim_arg))
      , apparent_elem(std::forward<Arg2>(data_arg)) {}

   decltype(auto) get_container1() const
   {
      return as_same_value_container(apparent_elem);
   }
   decltype(auto) get_container2() const
   {
      return helper::get_container(set);
   }

   auto find(Int i) const
   {
      return typename base_t::const_iterator(get_container1().begin(), get_container2().find(i), base_t::get_operation());
   }

   typename base_t::const_reference operator[] (Int i) const
   {
      if (i<0 || i>=base_t::dim())
         throw std::runtime_error("SameElementSparseVector - index out of range");
      if (get_container2().contains(i))
         return *apparent_elem;
      return zero_value<typename deref<ElemRef>::type>();
   }

   using base_t::dim;
};

template <typename SetRef, typename ElemRef>
struct spec_object_traits< SameElementSparseVector<SetRef, ElemRef> >
   : spec_object_traits<is_container> {
   static constexpr bool
      is_temporary = true,
      is_always_const = true;
};

template <typename SetRef, typename ElemRef>
struct check_container_feature<SameElementSparseVector<SetRef,ElemRef>, pure_sparse> : std::true_type {};

template <typename E, typename TSet>
auto same_element_sparse_vector(TSet&& s, E&& elem, std::enable_if_t<is_integer_set<TSet>::value, Int> dim)
{
   if (POLYMAKE_DEBUG || is_wary<TSet>()) {
      if (!set_within_range(s.top(), dim))
         throw std::runtime_error("same_element_sparse_vector - dimension mismatch");
   }
   using result_type = SameElementSparseVector<add_const_t<unwary_t<TSet>>, prevent_int_element<add_const_t<E>>>;
   return result_type(unwary(std::forward<TSet>(s)), std::forward<E>(elem), dim);
}

template <typename E, typename TSet>
auto same_element_sparse_vector(TSet&& s, std::enable_if_t<is_integer_set<TSet>::value, Int> dim)
{
   return same_element_sparse_vector(std::forward<TSet>(s), one_value<E>(), dim);
}

template <typename E, typename TSet,
          typename = std::enable_if_t<is_integer_set<TSet>::value &&
                                      SameElementSparseVector_helper<unwary_t<TSet>>::has_dim>>
auto same_element_sparse_vector(TSet&& s)
{
   return same_element_sparse_vector>(std::forward<TSet>(s), one_value<E>(), s.dim());
}


/* kind = 1: constructs SameElementSparseVector<sequence> of full size or empty
 * kind = 2: always constructs SameElementSparseVector< SingleElementSet<Int> > aka unit vector
 * kind = 3: constructs SameElementSparseVector<sequence> with one element or empty
 */
template <int kind, typename ElemRef=void>
class SameElementSparseVector_factory {
protected:
   Int dim;
public:
   typedef Int first_argument_type;
   typedef ElemRef second_argument_type;
   typedef SameElementSparseVector<typename std::conditional<kind==2, SingleElementSet<Int>, sequence>::type, ElemRef>
      result_type;

   SameElementSparseVector_factory(Int dim_arg = 0) : dim(dim_arg) {}

   template <typename R>
   result_type operator() (first_argument_type pos, R&& elem) const
   {
      return result_type(index_set(pos, int_constant<kind>()), std::forward<R>(elem), dim);
   }

   template <typename Iterator2>
   result_type operator() (operations::partial_left, Int pos, const Iterator2&) const
   {
      return result_type(sequence(pos, 0), zero_value<typename deref<ElemRef>::type>(), dim);
   }

   // can never happen, but must be defined
   template <typename Iterator1, typename R>
   result_type operator() (operations::partial_right, const Iterator1&, R&& elem) const
   {
      return result_type(sequence(0,0), std::forward<R>(elem), dim);
   }

protected:
   sequence index_set(Int, int_constant<1>) const
   {
      return sequence(0, dim);
   }
   SingleElementSet<Int> index_set(Int pos, int_constant<2>) const
   {
      return SingleElementSet<Int>(pos);
   }
   sequence index_set(Int pos, int_constant<3>) const
   {
      return sequence(pos, 1);
   }
};

template <int kind>
class SameElementSparseVector_factory<kind, void> : public operations::incomplete {
protected:
   Int dim_;
public:
   SameElementSparseVector_factory(Int dim_arg = 0) : dim_(dim_arg) {}
   Int dim() const { return dim_; }
};

template <int kind, typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder< SameElementSparseVector_factory<kind, void>, Iterator1, Iterator2, Reference1, Reference2 > {
   typedef SameElementSparseVector_factory<kind, prevent_int_element<add_const_t<Reference2>>> operation;
   static operation create(const SameElementSparseVector_factory<kind,void>& op) { return operation(op.dim()); }
};

/* ------------
 *  UnitVector
 * ------------ */

template <typename E>
auto unit_vector(Int dim, Int i, E&& x)
{
   if (POLYMAKE_DEBUG) {
      if (i < 0 || i >= dim)
         throw std::runtime_error("unit_vector - index out of range");
   }
   return same_element_sparse_vector(SingleElementSet<Int>(i), std::forward<E>(x), dim);
}

template <typename E>
auto unit_vector(Int dim, Int i)
{
   if (POLYMAKE_DEBUG) {
      if (i < 0 || i >= dim)
         throw std::runtime_error("unit_vector - index out of range");
   }
   return same_element_sparse_vector(SingleElementSet<Int>(i), one_value<E>(), dim);
}

/* ----------------
 *  ExpandedVector
 * ---------------- */

template <typename VectorRef>
class ExpandedVector
   : public TransformedContainer<masquerade_add_features<VectorRef, sparse_compatible>,
                                 pair<nothing, operations::fix2<Int, operations::composed12< BuildUnaryIt<operations::index2element>, void, BuildBinary<operations::add> > > > >
   , public GenericVector< ExpandedVector<VectorRef>, typename deref<VectorRef>::type::element_type > {
   using base_t = TransformedContainer<masquerade_add_features<VectorRef, sparse_compatible>,
                                       pair<nothing, operations::fix2<Int, operations::composed12< BuildUnaryIt<operations::index2element>, void, BuildBinary<operations::add> > > > >;
   Int dim_;
public:
   template <typename Arg, typename=std::enable_if_t<std::is_constructible<typename base_t::alias_t, Arg>::value>>
   ExpandedVector(Arg&& src_arg, Int offset, Int dim_arg)
      : base_t(std::forward<Arg>(src_arg), offset)
      , dim_(dim_arg) {}

   Int dim() const { return dim_; }
};

template <typename VectorRef>
struct spec_object_traits< ExpandedVector<VectorRef> >
   : spec_object_traits<is_container> {
   static constexpr bool is_temporary = true, is_always_const = true;
};

template <typename VectorRef>
struct check_container_feature<ExpandedVector<VectorRef>, sparse> : std::true_type {};

template <typename VectorRef>
struct check_container_feature<ExpandedVector<VectorRef>, pure_sparse>
   : check_container_ref_feature<VectorRef, pure_sparse> {};

template <typename VectorRef=void>
class ExpandedVector_factory {
protected:
   Int offset, dim;
public:
   typedef VectorRef argument_type;
   typedef ExpandedVector<VectorRef> vector_type;
   typedef vector_type result_type;

   ExpandedVector_factory(Int offset_arg = 0, Int dim_arg = 0)
      : offset(offset_arg), dim(dim_arg) {}

   template <typename Ref2>
   ExpandedVector_factory(const ExpandedVector_factory<Ref2>& f)
      : offset(f.offset), dim(f.dim) {}

   template <typename T>
   result_type operator() (T&& vec) const
   {
      return vector_type(std::forward<T>(vec), offset,dim);
   }

   template <typename> friend class ExpandedVector_factory;
};

template <>
class ExpandedVector_factory<void> : public operations::incomplete {
protected:
   Int offset, dim;
public:
   ExpandedVector_factory(Int offset_arg = 0, Int dim_arg = 0)
      : offset(offset_arg), dim(dim_arg) {}
   template <typename> friend class ExpandedVector_factory;
};

template <typename Iterator, typename Reference>
struct unary_op_builder< ExpandedVector_factory<void>, Iterator, Reference > {
   typedef ExpandedVector_factory<Reference> operation;

   template <typename Ref2>
   static operation create(const ExpandedVector_factory<Ref2>& f) { return f; }
};

/* ---------------------------------------
 *  functor objects for vector operations
 * --------------------------------------- */

namespace operations {

template <typename LeftRef, typename RightRef>
struct add_impl<LeftRef, RightRef, cons<is_vector, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef decltype(std::declval<LeftRef>() + std::declval<RightRef>()) result_type;

   template <typename L, typename R>
   result_type operator() (L&& l, R&& r) const
   {
      return std::forward<L>(l) + std::forward<R>(r);
   }

   template <typename L, typename R>
   void assign(L&& l, const R& r) const
   {
      l += r;
   }
};

template <typename LeftRef, typename RightRef>
struct sub_impl<LeftRef, RightRef, cons<is_vector, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef decltype(std::declval<LeftRef>() - std::declval<RightRef>()) result_type;

   template <typename L, typename R>
   result_type operator() (L&& l, R&& r) const
   {
      return std::forward<L>(l) - std::forward<R>(r);
   }

   template <typename L, typename R>
   void assign(L&& l, const R& r) const
   {
      l -= r;
   }
};

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_vector, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef decltype(std::declval<LeftRef>() * std::declval<RightRef>()) result_type;

   result_type operator() (first_argument_type l, second_argument_type r) const
   {
      return l*r;
   }
};

template <typename LeftRef, typename RightRef>
struct tensor_impl<LeftRef, RightRef, cons<is_vector, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef decltype(tensor_product(std::declval<LeftRef>(), std::declval<RightRef>())) result_type;

   template <typename L, typename R>
   result_type operator() (L&& l, R&& r) const
   {
      return tensor_product(std::forward<L>(l), std::forward<R>(r));
   }
};

template <typename LeftRef, typename RightRef>
struct concat_impl<LeftRef, RightRef, cons<is_vector, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef decltype(std::declval<LeftRef>() | std::declval<RightRef>()) result_type;

   template <typename L, typename R>
   result_type operator() (L&& l, R&& r) const
   {
      return std::forward<L>(l) | std::forward<R>(r);
   }
};

} // end namespace operations

template <typename TVector>
struct hash_func<TVector, is_vector> {
protected:
   hash_func<typename TVector::value_type> hash_elem;
public:
   size_t operator() (const TVector& v) const
   {
      size_t h=1;
      for (auto e=ensure(v, sparse_compatible()).begin(); !e.at_end(); ++e)
         h += (hash_elem(*e) * (e.index()+1));
      return h;
   }
};

template <typename TVector>
typename std::enable_if<!TVector::is_sparse, Indices<SelectedSubset<const GenericVector<TVector>&, BuildUnary<operations::non_zero>>>>::type
indices(const GenericVector<TVector>& v) 
{ 
   return indices(attach_selector(v, BuildUnary<operations::non_zero>())); 
}

template <typename TVector1, typename TVector2, typename E>
cmp_value lex_compare(const GenericVector<TVector1, E>& l, const GenericVector<TVector2, E>& r)
{
   return operations::cmp()(l.top(), r.top());
}

} // end namespace pm

namespace polymake {
   using pm::GenericVector;
   using pm::scalar2vector;
   using pm::same_element_vector;
   using pm::same_element_sparse_vector;
   using pm::convert_to;
   using pm::ones_vector;
   using pm::zero_vector;
   using pm::unit_vector;
   using pm::convert_to_persistent;
   using pm::convert_to_persistent_dense;
}

namespace std {

template <typename Vector1, typename Vector2, typename E>
void swap(pm::GenericVector<Vector1, E>& v1, pm::GenericVector<Vector2, E>& v2)
{
   v1.top().swap(v2.top());
}

}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
