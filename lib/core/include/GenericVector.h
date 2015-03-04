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

#ifndef POLYMAKE_GENERIC_VECTOR_H
#define POLYMAKE_GENERIC_VECTOR_H

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

/** @file GenericVector.h
    @class GenericVector
    @brief @ref generic "Generic type" for @ref vector_sec "vectors"
 */

template <typename VectorTop, typename E=typename VectorTop::element_type>
class GenericVector : public Generic<VectorTop>, public operators::base {
   template <typename, typename> friend class GenericVector;

protected:
   GenericVector() {}
   GenericVector(const GenericVector&) {}
#if POLYMAKE_DEBUG
   ~GenericVector() { POLYMAKE_DEBUG_METHOD(GenericVector,dump); }
   void dump() const { cerr << *this << endl; }
#endif

public:
   /// element type
   typedef E element_type;
   /// determine if the persistent type is sparse
   static const bool is_sparse=check_container_feature<VectorTop, sparse>::value;
   /// @ref generic "persistent type"
   typedef typename if_else<is_sparse, SparseVector<E>, Vector<E> >::type persistent_type;
   /// top type
   typedef typename Generic<VectorTop>::top_type top_type;
   /// @ref generic "generic type"
   typedef GenericVector generic_type;
   typedef typename if_else<check_container_feature<VectorTop, pure_sparse>::value, pure_sparse,
           typename if_else<is_sparse, sparse, dense>::type>::type
      sparse_discr;
protected:
   template <typename Vector2>
   typename enable_if<void, convertible_to<typename Vector2::value_type, E>::value>::type
   _assign(const Vector2& v, dense)
   {
      copy(ensure(v, (dense*)0).begin(), entire(this->top()));
   }

   template <typename Vector2>
   typename enable_if<void, explicitly_convertible_to<typename Vector2::value_type, E>::value>::type
   _assign(const Vector2& v, dense)
   {
      copy(ensure(attach_converter<E>(v), (dense*)0).begin(), entire(this->top()));
   }

   template <typename Vector2>
   typename enable_if<void, convertible_to<typename Vector2::value_type, E>::value>::type
   _assign(const Vector2& v, pure_sparse)
   {
      assign_sparse(this->top(), ensure(v, (pure_sparse*)0).begin());
   }

   template <typename Vector2>
   typename enable_if<void, explicitly_convertible_to<typename Vector2::value_type, E>::value>::type
   _assign(const Vector2& v, pure_sparse)
   {
      assign_sparse(this->top(), make_converting_iterator<E>(ensure(v, (pure_sparse*)0).begin()));
   }

   template <typename Vector2>
   bool trivial_assignment(const GenericVector<Vector2, E>&) const { return false; }

   bool trivial_assignment(const GenericVector& v) const { return this==&v; }

   template <typename Vector2>
   void swap(GenericVector<Vector2, E>& v, dense)
   {
      swap_ranges(entire(this->top()), v.top().begin());
   }

   template <typename Vector2>
   void swap(GenericVector<Vector2, E>& v, pure_sparse)
   {
      swap_sparse(this->top(), ensure(v.top(), (pure_sparse*)0));
   }

   template <typename Vector2>
   void assign(const Vector2& v)
   {
      _assign(v, sparse_discr());
   }

   template <typename Vector2, typename Operation>
   void _assign_op(const Vector2& v, const Operation& op_arg, dense, dense)
   {
      perform_assign(entire(this->top()), v.begin(), op_arg);
   }

   template <typename Vector2, typename Operation>
   void _assign_op(const Vector2& v, const Operation& op_arg, dense, sparse)
   {
      typedef typename Entire<Vector2>::const_iterator iterator2;
      typedef binary_op_builder<Operation, typename VectorTop::const_iterator, iterator2> opb;
      const typename opb::operation& op=opb::create(op_arg);
      int i_prev=0;
      typename VectorTop::iterator dst=this->top().begin();
      for (iterator2 src2=entire(v); !src2.at_end(); ++src2) {
         int i=src2.index();
         std::advance(dst, i-i_prev);
         op.assign(*dst, *src2);
         i_prev=i;
      }
   }

   template <typename Vector2, typename Operation, typename discr2>
   typename disable_if<void, operations::is_partially_defined_for<Operation, VectorTop, Vector2>::value>::type
   _assign_op(const Vector2& v, const Operation& op, sparse, discr2)
   {
      perform_assign(entire(this->top()), v.begin(), op);
   }

   template <typename Vector2, typename Operation, typename discr2>
   typename enable_if<void, operations::is_partially_defined_for<Operation, VectorTop, Vector2>::value>::type
   _assign_op(const Vector2& v, const Operation& op, sparse, discr2)
   {
      perform_assign_sparse(this->top(), ensure(v, (pure_sparse*)0).begin(), op);
   }

   template <typename E2>
   void _fill(const E2& x, dense)
   {
      pm::fill(entire(this->top()), x);
   }

   template <typename E2>
   void _fill(const E2& x, pure_sparse)
   {
      if (x)
         fill_sparse(this->top(), ensure(constant(x), (indexed*)0).begin());
      else
         this->top().clear();
   }

   // undefined for dense
   void remove0s(dense);

   void remove0s(pure_sparse)
   {
      top_type& me=this->top();
      typename VectorTop::iterator e=me.begin();
      while (!e.at_end())
         if (*e) ++e;
         else me.erase(e++);
   }

public:
   int dim() const { return get_dim(this->top()); }

   template <typename Vector2>
   void swap(GenericVector<Vector2,E>& v)
   {
      if (trivial_assignment(v)) return;

      if (POLYMAKE_DEBUG || !Unwary<VectorTop>::value || !Unwary<Vector2>::value) {
         if (dim() != v.dim())
            throw std::runtime_error("GenericVector::swap - dimension mismatch");
      }
      typedef typename if_else<check_container_feature<VectorTop, sparse>::value ||
                               check_container_feature<Vector2, sparse>::value,
                               pure_sparse, dense>::type
         sparse_discr2;
      swap(v, sparse_discr2());
   }

   template <typename Operation>
   void assign_op(const Operation& op)
   {
      perform_assign(entire(this->top()), op);
   }

   template <typename Vector2, typename Operation>
   void assign_op(const Vector2& v, const Operation& op)
   {
      typedef typename if_else<check_container_feature<Vector2,sparse>::value, sparse, dense>::type sparse_discr2;
      _assign_op(v, op, sparse_discr(), sparse_discr2());
   }

   template <typename E2>
   void fill(const E2& x)
   {
      this->top()._fill(x, sparse_discr());
   }

   top_type& operator= (const GenericVector& v)
   {
      if (!trivial_assignment(v)) {
         if (!object_traits<VectorTop>::is_resizeable && (POLYMAKE_DEBUG || !Unwary<VectorTop>::value)) {
            if (dim() != v.dim())
               throw std::runtime_error("GenericVector::operator= - dimension mismatch");
         }
         this->top().assign(v.top());
      }
      return this->top();
   }

   template <typename Vector2, typename E2>
   typename enable_if<top_type, (convertible_to<E2, E>::value || explicitly_convertible_to<E2, E>::value)>::type&
   operator= (const GenericVector<Vector2, E2>& v)
   {
      if (!object_traits<VectorTop>::is_resizeable && (POLYMAKE_DEBUG || !Unwary<VectorTop>::value)) {
         if (dim() != v.dim())
            throw std::runtime_error("GenericVector::operator= - dimension mismatch");
      }
      this->top().assign(v.top());
      return this->top();
   }

   template <typename E2, size_t size>
   typename enable_if<top_type, (convertible_to<E2, E>::value || explicitly_convertible_to<E2, E>::value)>::type&
   operator= (const E2 (&a)[size])
   {
      if (!object_traits<VectorTop>::is_resizeable && (POLYMAKE_DEBUG || !Unwary<VectorTop>::value)) {
         if (dim() != size)
            throw std::runtime_error("GenericVector::operator= - dimension mismatch");
      }
      this->top().assign(array2container(a));
      return this->top();
   }

   top_type& negate()
   {
      this->top().assign_op(BuildUnary<operations::neg>());
      return this->top();
   }

   /// adding a GenericVector
   template <typename Vector2>
   top_type& operator+= (const GenericVector<Vector2>& v)
   {
      if (POLYMAKE_DEBUG || !Unwary<VectorTop>::value || !Unwary<Vector2>::value) {
         if (dim() != v.dim())
            throw std::runtime_error("GenericVector::operator+= - dimension mismatch");
      }
      this->top().assign_op(v.top(), BuildBinary<operations::add>());
      return this->top();
   }

   /// subtracting a GenericVector
   template <typename Vector2>
   top_type& operator-= (const GenericVector<Vector2>& v)
   {
      if (POLYMAKE_DEBUG || !Unwary<VectorTop>::value || !Unwary<Vector2>::value) {
         if (dim() != v.dim())
            throw std::runtime_error("GenericVector::operator-= - dimension mismatch");
      }
      this->top().assign_op(v.top(), BuildBinary<operations::sub>());
      return this->top();
   }

   /// multiply with an element
   template <typename Right>
   top_type& operator*= (const Right& r)
   {
      if (!is_zero(r))
         this->top().assign_op(constant(r), BuildBinary<operations::mul>());
      else
         fill(r);
      return this->top();
   }

   /// appending an element
   template <typename Right>
   top_type& operator/= (const Right& r)
   {
      this->top().assign_op(constant(r), BuildBinary<operations::div>());
      return this->top();
   }

   /// divide by an element
   template <typename Right>
   top_type& div_exact(const Right& r)
   {
      this->top().assign_op(constant(r), BuildBinary<operations::divexact>());
      return this->top();
   }

   /// divides by the first element
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

   //@{ 
   /** 
    * Select a vector slice consisting of elements with given indices. 
    * The last variant selects a contiguous range of indices beginning 
    * with start. 
    * size==0 means up to the end of the vector. 
    * The const variants of these methods create immutable slice objects.
    * The indices must lie in the valid range.
    */
   template <typename IndexSet>
   typename enable_if< IndexedSlice<typename Unwary<VectorTop>::type&, const typename Concrete<IndexSet>::type&>,
                       isomorphic_to_container_of<IndexSet, int>::value >::type
   slice(const IndexSet& indices)
   {
      if (POLYMAKE_DEBUG || !Unwary<VectorTop>::value) {
         if (!set_within_range(indices, dim()))
            throw std::runtime_error("GenericVector::slice - indices out of range");
      }
      return IndexedSlice<typename Unwary<VectorTop>::type&, const typename Concrete<IndexSet>::type&>(this->top(), concrete(indices));
   }

   template <typename IndexSet>
   typename enable_if< const IndexedSlice<const typename Unwary<VectorTop>::type&, const typename Concrete<IndexSet>::type&>,
                       isomorphic_to_container_of<IndexSet, int>::value >::type
   slice(const IndexSet& indices) const
   {
      if (POLYMAKE_DEBUG || !Unwary<VectorTop>::value) {
         if (!set_within_range(indices, dim()))
            throw std::runtime_error("GenericVector::slice - indices out of range");
      }
      return IndexedSlice<const typename Unwary<VectorTop>::type&, const typename Concrete<IndexSet>::type&>(this->top(), concrete(indices));
   }


   IndexedSlice<typename Unwary<VectorTop>::type&, sequence>
   slice(int sstart, int ssize=0)
   {
      if (sstart<0) sstart+=dim();
      if (!ssize) ssize=dim()-sstart;
      if (POLYMAKE_DEBUG || !Unwary<VectorTop>::value) {
         if (ssize<0 || sstart<0 || sstart+ssize>dim())
            throw std::runtime_error("GenericVector::slice - indices out of range");
      }
      return IndexedSlice<typename Unwary<VectorTop>::type&, sequence>(this->top(), sequence(sstart,ssize));
   }


   const IndexedSlice<const typename Unwary<VectorTop>::type&, sequence>
   slice(int sstart, int ssize=0) const
   {
      if (sstart<0) sstart+=dim();
      if (!ssize) ssize=dim()-sstart;
      if (POLYMAKE_DEBUG || !Unwary<VectorTop>::value) {
         if (ssize<0 || sstart<0 || sstart+ssize>dim())
            throw std::runtime_error("GenericVector::slice - indices out of range");
      }
      return IndexedSlice<const typename Unwary<VectorTop>::type&, sequence>(this->top(), sequence(sstart,ssize));
   }
   //@}

   template <typename Result>
   struct rebind_generic {
      typedef GenericVector<Result, E> type;
   };

   // stub for RowChain/ColChain
   void stretch_dim(int d) const
   {
      if (d) throw std::runtime_error("dimension mismatch");
   }
};

struct is_vector;

template <typename Vector, typename E>
struct spec_object_traits< GenericVector<Vector, E> >
   : spec_or_model_traits<Vector,is_container> {
   typedef is_vector generic_tag;
   static const bool allow_sparse=true;

   static bool is_zero(const Vector& v)
   {
      return entire(attach_selector(v, BuildUnary<operations::non_zero>())).at_end();
   }
};


/** @class FixedVector
    @brief Built-in array decorated as a vector
*/

template <typename E, size_t _size>
class FixedVector
   : public fixed_array<E,_size>,
     public GenericVector<FixedVector<E,_size>, E> {
public:
   /// create empty vector
   FixedVector(int=0)
   {
      if (is_pod<E>::value) clear();
   }

   /// create vector of given length with constant element 
   FixedVector(int, const E& init)
   {
      fill(entire(*this), init);
   }

   FixedVector(const E (&init)[_size])
   {
      copy(entire(*this), init+0);
   }

   template <typename E2>
   explicit FixedVector(const E2 (&init)[_size])
   {
      copy(entire(*this), attach_converter<E>(array2container(init)).begin());
   }

   /// create vector from iterator
   template <typename Iterator>
   FixedVector(int, Iterator src)
   {
      copy(src, entire(*this));
   }

   /// create vector from GenericVector
   template <typename Vector>
   FixedVector(const GenericVector<Vector, E>& v)
   {
      if (POLYMAKE_DEBUG) {
         if (v.dim() != this->size())
            throw std::runtime_error("FixedVector constructor - dimension mismatch");
      }
      this->_assign(v.top(), dense());
   }

   template <typename Vector, typename E2>
   explicit FixedVector(const GenericVector<Vector, E2>& v,
                        typename enable_if<void**, (convertible_to<E2, E>::value || explicitly_convertible_to<E2, E>::value)>::type=0)
   {
      if (POLYMAKE_DEBUG) {
         if (v.dim() != this->size())
            throw std::runtime_error("FixedVector constructor - dimension mismatch");
      }
      this->_assign(v.top(), dense());
   }

   FixedVector& operator= (const FixedVector& other) { return FixedVector::generic_type::operator=(other); }
   using FixedVector::generic_type::operator=;

   /// fill with zeroes
   void clear()
   {
      fill(entire(*this), zero_value<E>());
   }
protected:
   friend class GenericVector<FixedVector>;
};

template <typename E, size_t size>
struct spec_object_traits< FixedVector<E,size> >
   : spec_object_traits<is_container> {
   static const int is_resizeable=0;
};

template <typename E, size_t size> inline
FixedVector<E,size>& array2vector(E (&a)[size])
{
   return reinterpret_cast<FixedVector<E,size>&>(a);
}

template <typename E, size_t size> inline
const FixedVector<E,size>& array2vector(const E (&a)[size])
{
   return reinterpret_cast<const FixedVector<E,size>&>(a);
}

/* --------------------------------------------
 *  LazyVector1
 * lazy evaluation of an unary vector operator
 * -------------------------------------------- */

template <typename VectorRef, typename Operation>
class LazyVector1
   : public TransformedContainer<VectorRef, Operation>,
     public GenericVector< LazyVector1<VectorRef,Operation>,
                           typename object_traits<typename TransformedContainer<VectorRef,Operation>::value_type>::persistent_type > {
   typedef TransformedContainer<VectorRef, Operation> _super;
public:
   LazyVector1(typename _super::arg_type src_arg, const Operation& op_arg=Operation())
      : _super(src_arg, op_arg) {}

   int dim() const { return get_dim(this->get_container()); }
};

template <typename VectorRef, typename Operation>
struct spec_object_traits< LazyVector1<VectorRef,Operation> >
   : spec_object_traits<is_container> {
   static const bool is_lazy=true, is_temporary=true, is_always_const=true;
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
   : public TransformedContainerPair<VectorRef1, VectorRef2, Operation>,
     public GenericVector< LazyVector2<VectorRef1,VectorRef2,Operation>,
                           typename object_traits<typename TransformedContainerPair<VectorRef1,VectorRef2,Operation>::value_type>::persistent_type > {
   typedef TransformedContainerPair<VectorRef1, VectorRef2, Operation> _super;
protected:
   int dim(True) const { return get_dim(this->get_container2()); }
   int dim(False) const { return get_dim(this->get_container1()); }
public:
   LazyVector2(typename _super::first_arg_type src1_arg, typename _super::second_arg_type src2_arg, const Operation& op_arg=Operation())
      : _super(src1_arg, src2_arg, op_arg) {}

   int dim() const
   {
      return dim(bool2type< check_container_ref_feature<VectorRef1,unlimited>::value >());
   }
};

template <typename VectorRef1, typename VectorRef2, typename Operation>
struct spec_object_traits< LazyVector2<VectorRef1, VectorRef2, Operation> >
   : spec_object_traits<is_container> {
   static const bool is_lazy=true, is_temporary=true, is_always_const=true;
};

template <typename VectorRef1, typename VectorRef2, typename Operation, typename Feature>
struct check_container_feature<LazyVector2<VectorRef1, VectorRef2, Operation>, Feature>
   : check_container_feature<TransformedContainerPair<VectorRef1, VectorRef2, Operation>, Feature> {};

/// explicit conversion of vector elements to another type
template <typename TargetType, typename Vector> inline
const Vector&
convert_to(const GenericVector<Vector, TargetType>& v)
{
   return v.top();
}

template <typename TargetType, typename Vector, typename E> inline
const LazyVector1<const Vector&, conv_by_cast<E, TargetType> >
convert_to(const GenericVector<Vector, E>& v,
           typename enable_if<void**, (convertible_to<E, TargetType>::value && !identical<E, TargetType>::value)>::type=0)
{
   return v.top();
}

template <typename TargetType, typename Vector, typename E> inline
const LazyVector1<const Vector&, conv<E, TargetType> >
convert_to(const GenericVector<Vector, E>& v,
           typename enable_if<void**, explicitly_convertible_to<E, TargetType>::value>::type=0)
{
   return v.top();
}

template <typename Vector, typename Operation> inline
const LazyVector1<const Vector&, Operation>
apply_operation(const GenericVector<Vector>& v, const Operation& op)
{
   return LazyVector1<const Vector&, Operation>(v.top(), op);
}

/* --------------------------------------------
 *  VectorTensorProduct
 * lazy evaluation of a vector tensor product
 * -------------------------------------------- */
template <typename VectorRef1, typename VectorRef2, typename Operation=BuildBinary<operations::mul> >
class VectorTensorProduct
   : public ContainerProduct<VectorRef1, VectorRef2, Operation>,
     public GenericVector< VectorTensorProduct<VectorRef1,VectorRef2,Operation>,
                           typename object_traits<typename ContainerProduct<VectorRef1, VectorRef2, Operation>::value_type>::persistent_type  > {
   typedef ContainerProduct<VectorRef1, VectorRef2, Operation> _super;
public:
   VectorTensorProduct(typename _super::first_arg_type src1_arg, typename _super::second_arg_type src2_arg,
                       const Operation& op_arg=Operation())
      : _super(src1_arg, src2_arg, op_arg) {}

   using _super::dim;
};

template <typename VectorRef1, typename VectorRef2, typename Operation>
struct spec_object_traits< VectorTensorProduct<VectorRef1,VectorRef2,Operation> >
   : spec_object_traits<is_container> {
   static const bool is_lazy=true;
};

template <typename VectorRef1, typename VectorRef2, typename Operation, typename Feature>
struct check_container_feature<VectorTensorProduct<VectorRef1,VectorRef2,Operation>, Feature>
   : check_container_feature<ContainerProduct<VectorRef1, VectorRef2, Operation>, Feature> {};

/* -------------
 *  VectorChain
 * ------------- */

template <typename VectorRef1, typename VectorRef2>
class VectorChain
   : public ContainerChain<VectorRef1, VectorRef2>,
     public GenericVector< VectorChain<VectorRef1,VectorRef2>,
                           typename identical<typename deref<VectorRef1>::type::element_type,
                                              typename deref<VectorRef2>::type::element_type>::type > {
   typedef ContainerChain<VectorRef1, VectorRef2> _super;
public:
   VectorChain(typename _super::first_arg_type src1_arg, typename _super::second_arg_type src2_arg)
      : _super(src1_arg,src2_arg) {}

   VectorChain& operator= (const VectorChain& other) { return VectorChain::generic_type::operator=(other); }
   using VectorChain::generic_type::operator=;
   using _super::dim;
};

template <typename VectorRef1, typename VectorRef2>
struct spec_object_traits< VectorChain<VectorRef1, VectorRef2> >
   : spec_object_traits< ContainerChain<VectorRef1, VectorRef2> > {};

template <typename VectorRef1, typename VectorRef2, typename Feature>
struct check_container_feature< VectorChain<VectorRef1, VectorRef2>, Feature>
   : check_container_feature< ContainerChain<VectorRef1, VectorRef2>, Feature> {};

/* ---------------------
 *  SingleElementVector
 * --------------------- */

template <typename E>
class SingleElementVector
   : public single_value_container<E>,
     public GenericVector<SingleElementVector<E>, typename deref<E>::type> {
   typedef single_value_container<E> _super;
public:
   SingleElementVector(typename _super::arg_type arg)
      : _super(arg) {}

   using SingleElementVector::generic_type::operator=;
};

template <typename E>
struct spec_object_traits< SingleElementVector<E> > : spec_object_traits< single_value_container<E> > {};

template <typename E>
class SingleElementSparseVector
   : public single_value_container<E, true>,
     public GenericVector<SingleElementSparseVector<E>, typename deref<E>::type> {
   typedef single_value_container<E, true> _super;
public:
   SingleElementSparseVector() {}
   SingleElementSparseVector(typename _super::arg_type arg)
      : _super(arg) {}
   using _super::dim;
};

template <typename E>
struct spec_object_traits< SingleElementSparseVector<E> > : spec_object_traits< single_value_container<E, true> > {
   static const bool is_always_const=true;
};

template <typename E>
struct check_container_feature<SingleElementSparseVector<E>, pure_sparse> : True {};

template <typename ElementRef, typename IndexRef>
struct SingleElementSparseVector_factory {
   typedef ElementRef first_argument_type;
   typedef int second_argument_type;
   typedef SingleElementSparseVector<ElementRef> result_type;

   result_type operator() (typename function_argument<ElementRef>::type x, int) const
   {
      return result_type(x);
   }
   // can never happen, but must be defined
   template <typename Iterator2>
   result_type operator() (operations::partial_left, typename function_argument<ElementRef>::type x, const Iterator2&) const
   {
      return result_type(x);
   }

   template <typename Iterator1>
   result_type operator() (operations::partial_right, const Iterator1&, int) const
   {
      return result_type();
   }
};

template <typename E> inline
SingleElementVector<E&>
scalar2vector(E& x)
{
   return x;
}

template <typename E> inline
const SingleElementVector<const E&>
scalar2vector(const E& x)
{
   return x;
}

template <typename E> inline
const SingleElementSparseVector<const E&>
scalar2sparse_vector(const E& x)
{
   if (!is_zero(x))
      return x;
   return SingleElementSparseVector<const E&>();
}

/* -------------------
 *  SameElementVector
 * ------------------- */

template <typename ElemRef>
class SameElementVector
   : public repeated_value_container<ElemRef>,
     public GenericVector<SameElementVector<ElemRef>,
                          typename object_traits<typename deref<ElemRef>::type>::persistent_type> {
   typedef repeated_value_container<ElemRef> _super;
public:
   SameElementVector(typename _super::arg_type value_arg, int dim_arg)
      : _super(value_arg, dim_arg) {}

   using _super::dim;
   using _super::stretch_dim;
};

template <typename ElemRef>
struct spec_object_traits< SameElementVector<ElemRef> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true, is_always_const=true;
};

/// Create a vector with all entries equal to the given element x.
template <typename E> inline
const SameElementVector<const E&>
same_element_vector(const E& x, int dim=0)
{
   return SameElementVector<const E&>(x, dim);
}

/// Create a vector with all entries equal to 1.
template <typename E> inline
const SameElementVector<const E&>
ones_vector(int dim=0)
{
   return SameElementVector<const E&>(one_value<E>(), dim);
}

/// Create a vector with all entries equal to 0.
template <typename E> inline
const SameElementVector<const E&>
zero_vector(int dim=0)
{
   return SameElementVector<const E&>(zero_value<E>(), dim);
}

template <typename ElemRef, bool _inverse=false>
struct apparent_data_accessor {
   typedef void argument_type;
   typedef typename attrib<ElemRef>::plus_const_ref result_type;

   apparent_data_accessor() {}
   apparent_data_accessor(const alias<ElemRef>& arg) : _data(arg) {}

   template <typename Iterator>
   result_type operator() (const Iterator&) const
   {
      return *_data;
   }

   // these methods are for construct_dense
   typedef void first_argument_type;
   typedef void second_argument_type;

   template <typename Iterator1, typename Iterator2>
   result_type operator() (const Iterator1&, const Iterator2&) const
   {
      if (_inverse) return zero_value<typename deref<ElemRef>::type>();
      return *_data;
   }
   template <typename Iterator1, typename Iterator2>
   result_type operator() (operations::partial_left, const Iterator1&, const Iterator2&) const
   {
      if (_inverse) return zero_value<typename deref<ElemRef>::type>();
      return *_data;
   }
   template <typename Iterator1, typename Iterator2>
   result_type operator() (operations::partial_right, const Iterator1&, const Iterator2&) const
   {
      if (_inverse) return *_data;
      return zero_value<typename deref<ElemRef>::type>();
   }
protected:
   alias<ElemRef> _data;
};

template <typename SetRef, typename ElemRef>
class SameElementSparseVector
   : public modified_container_impl< SameElementSparseVector<SetRef,ElemRef>,
                                     list( Container< typename attrib<typename Set_with_dim_helper<SetRef>::container>::plus_const >,
                                           Operation< pair<apparent_data_accessor<ElemRef, complement_helper<SetRef>::value>,
                                                           operations::identity<int> > > ) >,
     public GenericVector< SameElementSparseVector<SetRef,ElemRef>,
                           typename object_traits<typename deref<ElemRef>::type>::persistent_type > {
   typedef modified_container_impl<SameElementSparseVector> _super;
protected:
   typedef Set_with_dim_helper<SetRef> helper;
   typename helper::alias_type set;
   alias<ElemRef> apparent_elem;

public:
   typedef typename least_derived<cons< bidirectional_iterator_tag, typename container_traits<SetRef>::category> >::type container_category;
   typedef typename helper::alias_type::arg_type first_arg_type;
   typedef typename alias<ElemRef>::arg_type second_arg_type;

   SameElementSparseVector(first_arg_type set_arg, second_arg_type data_arg)
      : set(helper::create(set_arg,-1)), apparent_elem(data_arg) {}

   SameElementSparseVector(first_arg_type set_arg, second_arg_type data_arg, int dim_arg)
      : set(helper::create(set_arg,dim_arg)), apparent_elem(data_arg) {}

   const typename _super::container& get_container() const
   {
      return helper::deref(set);
   }
   typename _super::operation get_operation() const
   {
      return typename _super::operation(apparent_elem, operations::identity<int>());
   }
   typename _super::const_iterator find(int i) const
   {
      return typename _super::const_iterator(get_container().find(i), get_operation());
   }

   typename _super::const_reference operator[] (int i) const
   {
      if (i<0 || i>=_super::dim())
         throw std::runtime_error("same_element_sparse_vector - index out of range");
      if (get_container().contains(i))
         return *apparent_elem;
      return zero_value<typename deref<ElemRef>::type>();
   }

   using _super::dim;
};

template <typename SetRef, typename ElemRef>
struct spec_object_traits< SameElementSparseVector<SetRef,ElemRef> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true, is_always_const=true;
};

template <typename SetRef, typename ElemRef>
struct check_container_feature<SameElementSparseVector<SetRef,ElemRef>, pure_sparse> : True {};

template <typename E, typename Set> inline
const SameElementSparseVector<const typename Unwary<Set>::type&, E>
same_element_sparse_vector(const GenericSet<Set,int>& s, int dim)
{
   if (POLYMAKE_DEBUG || !Unwary<Set>::value) {
      if (!set_within_range(s.top(),dim))
         throw std::runtime_error("same_element_sparse_vector - dimension mismatch");
   }
   return SameElementSparseVector<const typename Unwary<Set>::type&, E>(s.top(), E(1), dim);
}

template <typename E, typename Set> inline
const SameElementSparseVector<const typename Unwary<Set>::type&, const E&>
same_element_sparse_vector(const GenericSet<Set,int>& s, const E& x, int dim)
{
   if (POLYMAKE_DEBUG || !Unwary<Set>::value) {
      if (!set_within_range(s.top(),dim))
         throw std::runtime_error("same_element_sparse_vector - dimension mismatch");
   }
   return SameElementSparseVector<const typename Unwary<Set>::type&, const E&>(s.top(), x, dim);
}

template <typename E, typename Set> inline
const SameElementSparseVector<const Complement<Set>&, E>
same_element_sparse_vector(const Complement<Set,int>& s, int dim)
{
   return SameElementSparseVector<const Complement<Set>&, E>(s, E(1), dim);
}

template <typename E, typename Set> inline
const SameElementSparseVector<const Complement<Set>&, const E&>
same_element_sparse_vector(const Complement<Set,int>& s, const E& x, int dim)
{
   return SameElementSparseVector<const Complement<Set>&, const E&>(s, x, dim);
}

template <typename E, typename Set> inline
typename enable_if<const SameElementSparseVector<const Set&, E>, Set_with_dim_helper<Set>::value>::type
same_element_sparse_vector(const GenericSet<Set,int>& s)
{
   return SameElementSparseVector<const Set&, E>(s.top(), E(1));
}

/// Create a SparseVector with all entries equal to the given element x
/// at the coordinates defined in the GenericSet s.
template <typename E, typename Set> inline
typename enable_if<const SameElementSparseVector<const Set&, const E&>, Set_with_dim_helper<Set>::value>::type
same_element_sparse_vector(const GenericSet<Set,int>& s, const E& x)
{
   return SameElementSparseVector<const Set&, const E&>(s.top(), x);
}

template <typename E, typename Set> inline
typename enable_if<const SameElementSparseVector<const Complement<Set>&, E>, Set_with_dim_helper<Set>::value>::type
same_element_sparse_vector(const Complement<Set,int>& s)
{
   return SameElementSparseVector<const Complement<Set>&, E>(s, E(1));
}

template <typename E, typename Set> inline
typename enable_if<const SameElementSparseVector<const Complement<Set>&, const E&>, Set_with_dim_helper<Set>::value>::type
same_element_sparse_vector(const Complement<Set,int>& s, const E& x)
{
   return SameElementSparseVector<const Complement<Set>&, const E&>(s, x);
}

/* kind = 1: constructs SameElementSparseVector<sequence> of full size or empty
 * kind = 2: always constructs SameElementSparseVector< SingleElementSet<int> > aka unit vector
 * kind = 3: constructs SameElementSparseVector<sequence> with one element or empty
 */
template <int kind, typename ElemRef=void>
class SameElementSparseVector_factory {
protected:
   int dim;
public:
   typedef int first_argument_type;
   typedef ElemRef second_argument_type;
   typedef SameElementSparseVector<typename if_else<kind==2, SingleElementSet<int>, sequence>::type, ElemRef>
      result_type;

   SameElementSparseVector_factory(int dim_arg=0) : dim(dim_arg) {}

   result_type operator() (first_argument_type pos,
                           typename function_argument<ElemRef>::type elem) const
   {
      return result_type(index_set(pos,int2type<kind>()), elem, dim);
   }

   template <typename Iterator2>
   result_type operator() (operations::partial_left, int pos, const Iterator2&) const
   {
      return result_type(sequence(pos,0), zero_value<typename deref<ElemRef>::type>(), dim);
   }

   // can never happen, but must be defined
   template <typename Iterator1>
   result_type operator() (operations::partial_right, const Iterator1&, typename function_argument<ElemRef>::type elem) const
   {
      return result_type(sequence(0,0), elem, dim);
   }

protected:
   sequence index_set(int, int2type<1>) const
   {
      return sequence(0,dim);
   }
   int index_set(int pos, int2type<2>) const
   {
      return pos;
   }
   sequence index_set(int pos, int2type<3>) const
   {
      return sequence(pos,1);
   }
};

template <int kind>
class SameElementSparseVector_factory<kind, void> : public operations::incomplete {
protected:
   int _dim;
public:
   SameElementSparseVector_factory(int dim_arg=0) : _dim(dim_arg) {}
   int dim() const { return _dim; }
};

template <int kind, typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder< SameElementSparseVector_factory<kind,void>, Iterator1, Iterator2, Reference1, Reference2 > {
   typedef SameElementSparseVector_factory<kind,Reference2> operation;
   static operation create(const SameElementSparseVector_factory<kind,void>& op) { return operation(op.dim()); }
};

/* ------------
 *  UnitVector
 * ------------ */

template <typename E> inline
const SameElementSparseVector<SingleElementSet<int>, const E&>
unit_vector(int dim, int i, const E& x)
{
   if (POLYMAKE_DEBUG) {
      if (i<0 || i>=dim)
         throw std::runtime_error("unit_vector - index out of range");
   }
   return SameElementSparseVector<SingleElementSet<int>, const E&>(i,x,dim);
}

template <typename E> inline
const SameElementSparseVector<SingleElementSet<int>, E>
unit_vector(int dim, int i)
{
   if (POLYMAKE_DEBUG) {
      if (i<0 || i>=dim)
         throw std::runtime_error("unit_vector - index out of range");
   }
   return SameElementSparseVector<SingleElementSet<int>, E>(i,E(1),dim);
}

/* ----------------
 *  ExpandedVector
 * ---------------- */

template <typename VectorRef>
class ExpandedVector
   : public TransformedContainer<masquerade_add_features<VectorRef, sparse_compatible>,
                                 pair<nothing, operations::fix2<int, operations::composed12< BuildUnaryIt<operations::index2element>, void, BuildBinary<operations::add> > > > >,
     public GenericVector< ExpandedVector<VectorRef>, typename deref<VectorRef>::type::element_type > {
   typedef TransformedContainer<masquerade_add_features<VectorRef, sparse_compatible>,
                                pair<nothing, operations::fix2<int, operations::composed12< BuildUnaryIt<operations::index2element>, void, BuildBinary<operations::add> > > > >
      _super;
   int _dim;
public:
   ExpandedVector(typename _super::arg_type src_arg, int offset, int dim_arg)
      : _super(src_arg, offset), _dim(dim_arg) {}

   int dim() const { return _dim; }
};

template <typename VectorRef>
struct spec_object_traits< ExpandedVector<VectorRef> > :
   spec_object_traits<is_container> {
   static const bool is_temporary=true, is_always_const=true;
};

template <typename VectorRef>
struct check_container_feature<ExpandedVector<VectorRef>, sparse> : True {};

template <typename VectorRef>
struct check_container_feature<ExpandedVector<VectorRef>, pure_sparse>
   : check_container_ref_feature<VectorRef, pure_sparse> {};

template <typename VectorRef=void>
class ExpandedVector_factory {
protected:
   int offset, dim;
public:
   typedef VectorRef argument_type;
   typedef ExpandedVector<VectorRef> vector_type;
   typedef const vector_type result_type;

   ExpandedVector_factory(int offset_arg=0, int dim_arg=0)
      : offset(offset_arg), dim(dim_arg) {}

   template <typename Ref2>
   ExpandedVector_factory(const ExpandedVector_factory<Ref2>& f)
      : offset(f.offset), dim(f.dim) {}

   result_type operator() (typename function_argument<VectorRef>::type vec) const
   {
      return vector_type(vec,offset,dim);
   }

   template <typename> friend class ExpandedVector_factory;
};

template <>
class ExpandedVector_factory<void> : public operations::incomplete {
protected:
   int offset, dim;
public:
   ExpandedVector_factory(int offset_arg=0, int dim_arg=0)
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

template <typename OpRef>
struct neg_impl<OpRef, is_vector> {
   typedef OpRef argument_type;
   typedef LazyVector1<typename attrib<typename Unwary<OpRef>::type>::plus_const, BuildUnary<neg> > result_type;

   result_type operator() (typename function_argument<OpRef>::const_type x) const
   {
      return result_type(unwary(x));
   }

   void assign(typename lvalue_arg<OpRef>::type x) const
   {
      x.negate();
   }
};

template <typename LeftRef, typename RightRef>
struct add_impl<LeftRef, RightRef, cons<is_vector, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyVector2<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                       typename attrib<typename Unwary<RightRef>::type>::plus_const,
                       BuildBinary<add> >
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (l.dim() != r.dim())
            throw std::runtime_error("operator+(GenericVector,GenericVector) - dimension mismatch");
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
struct sub_impl<LeftRef, RightRef, cons<is_vector, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyVector2<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                       typename attrib<typename Unwary<RightRef>::type>::plus_const,
                       BuildBinary<sub> >
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (l.dim() != r.dim())
            throw std::runtime_error("operator-(GenericVector,GenericVector) - dimension mismatch");
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

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_vector, is_scalar> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyVector2<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                       constant_value_container<typename Diligent<typename Unwary<RightRef>::type>::type>,
                       BuildBinary<mul> >
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const {
      return result_type(unwary(l), diligent(unwary(r)));
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l*=r;
   }
};

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_scalar, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyVector2<constant_value_container<typename Diligent<typename Unwary<LeftRef>::type>::type>,
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
struct mul_impl<LeftRef, RightRef, cons<is_vector, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename add<typename deref<LeftRef>::type::value_type,
                        typename deref<RightRef>::type::value_type>::result_type
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (l.dim() != r.dim())
            throw std::runtime_error("operator*(GenericVector,GenericVector) - dimension mismatch");
      }
      return accumulate(attach_operation(unwary(l), unwary(r), BuildBinary<mul>()), BuildBinary<add>());
   }
};

template <typename LeftRef, typename RightRef>
struct tensor_impl<LeftRef, RightRef, cons<is_vector, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef VectorTensorProduct<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                               typename attrib<typename Unwary<RightRef>::type>::plus_const,
                               BuildBinary<mul> >
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(unwary(l), unwary(r));
   }
};

template <typename LeftRef, typename RightRef>
struct div_impl<LeftRef, RightRef, cons<is_vector, is_scalar> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyVector2<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                       constant_value_container<typename Diligent<typename Unwary<RightRef>::type>::type>,
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
struct divexact_impl<LeftRef, RightRef, cons<is_vector, is_scalar> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyVector2<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                       constant_value_container<typename Diligent<typename Unwary<RightRef>::type>::type>,
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

template <typename OpRef>
struct square_impl<OpRef, is_vector> {
   typedef OpRef argument_type;
   typedef typename mul_impl<typename Unwary<OpRef>::type, typename Unwary<OpRef>::type, cons<is_vector, is_vector> >::result_type
      result_type;

   result_type operator() (typename function_argument<OpRef>::const_type x) const
   {
      return accumulate(attach_operation(unwary(x), BuildUnary<square>()), BuildBinary<add>());
   }
};

template <typename LeftRef, typename RightRef>
struct concat_impl<LeftRef, RightRef, cons<is_vector, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef VectorChain<typename coherent_const<typename Unwary<LeftRef>::type, typename Unwary<RightRef>::type>::first_type,
                       typename coherent_const<typename Unwary<LeftRef>::type, typename Unwary<RightRef>::type>::second_type>
      result_type;

   result_type operator() (typename function_argument<LeftRef>::type l,
                           typename function_argument<RightRef>::type r) const
   {
      return result_type(unwary(l), unwary(r));
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l|=r;
   }
};

template <typename LeftRef, typename RightRef>
struct concat_impl<LeftRef, RightRef, cons<is_vector, is_scalar> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;

   // provisions for the type-heterogeneous case, where the scalar value needs to be converted first
   typedef typename deref<LeftRef>::type::element_type element_type;
   typedef typename deref<typename Unwary<RightRef>::type>::type right_src_type;
   static const bool homogeneous=identical<right_src_type, element_type>::value;
   typedef SingleElementVector<typename if_else<homogeneous, typename Unwary<RightRef>::type, element_type>::type> Right;

   typedef VectorChain<typename coherent_const<typename Unwary<LeftRef>::type, Right>::first_type,
                       typename coherent_const<typename Unwary<LeftRef>::type, Right>::second_type>
      result_type;

   result_type operator() (typename function_argument<LeftRef>::type l,
                           typename function_argument<RightRef>::type r) const
   {
      return result_type(unwary(l), convert_to<element_type>(unwary(r)));
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l|=r;
   }
};

template <typename LeftRef, typename RightRef>
struct concat_impl<LeftRef, RightRef, cons<is_scalar, is_vector> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;

   // provisions for the type-heterogeneous case, where the scalar value needs to be converted first
   typedef typename deref<RightRef>::type::element_type element_type;
   typedef typename deref<typename Unwary<LeftRef>::type>::type left_src_type;
   static const bool homogeneous=identical<left_src_type, element_type>::value;
   typedef SingleElementVector<typename if_else<homogeneous, typename Unwary<LeftRef>::type, element_type>::type> Left;

   typedef VectorChain<typename coherent_const<Left, typename Unwary<RightRef>::type>::first_type,
                       typename coherent_const<Left, typename Unwary<RightRef>::type>::second_type>
      result_type;

   result_type operator() (typename function_argument<LeftRef>::type l,
                           typename function_argument<RightRef>::type r) const
   {
      return result_type(convert_to<element_type>(unwary(l)), unwary(r));
   }
};

template <typename LeftRef, typename RightRef>
struct bitwise_or_impl<LeftRef, RightRef, cons<is_vector, is_vector> >
   : concat_impl<LeftRef, RightRef, cons<is_vector, is_vector> > {};

template <typename LeftRef, typename RightRef>
struct bitwise_or_impl<LeftRef, RightRef, cons<is_vector, is_scalar> >
   : concat_impl<LeftRef, RightRef, cons<is_vector, is_scalar> > {};

template <typename LeftRef, typename RightRef>
struct bitwise_or_impl<LeftRef, RightRef, cons<is_scalar, is_vector> >
   : concat_impl<LeftRef, RightRef, cons<is_scalar, is_vector> > {};

} // end namespace operations
namespace operators {

template <typename Vector1, typename Vector2> inline
typename operations::add_impl<const Vector1&, const Vector2&>::result_type
operator+ (const GenericVector<Vector1>& l, const GenericVector<Vector2>& r)
{
   operations::add_impl<const Vector1&, const Vector2&> op;
   return op(concrete(l), concrete(r));
}

template <typename Vector1, typename Vector2> inline
typename operations::sub_impl<const Vector1&, const Vector2&>::result_type
operator- (const GenericVector<Vector1>& l, const GenericVector<Vector2>& r)
{
   operations::sub_impl<const Vector1&, const Vector2&> op;
   return op(concrete(l), concrete(r));
}

template <typename Vector1, typename Vector2> inline
bool operator== (const GenericVector<Vector1>& l, const GenericVector<Vector2>& r)
{
   if (l.dim() != r.dim()) return false;
   operations::eq<const typename Unwary<Vector1>::type&, const typename Unwary<Vector2>::type&> op;
   return op(l.top(), r.top());
}

template <typename Vector1, typename Vector2> inline
typename enable_if<bool, (is_ordered<typename Vector1::element_type>::value && is_ordered<typename Vector2::element_type>::value)>::type
operator< (const GenericVector<Vector1>& l, const GenericVector<Vector2>& r)
{
   if (POLYMAKE_DEBUG || !Unwary<Vector1>::value || !Unwary<Vector2>::value) {
      if (l.dim() != r.dim())
         throw std::runtime_error("operator<(GenericVector,GenericVector) - dimension mismatch");
   }
   operations::lt<const typename Unwary<Vector1>::type&, const typename Unwary<Vector2>::type&> op;
   return op(l.top(), r.top());
}

} // end namespace operators

template <typename Vector, typename Right> inline
typename operations::divexact_impl<const Vector&, const Right&>::result_type
div_exact(const GenericVector<Vector>& l, const Right& r)
{
   operations::divexact_impl<const Vector&, const Right&> op;
   return op(concrete(l), r);
}

template <typename Vector>
struct hash_func<Vector, is_vector> {
protected:
   hash_func<typename Vector::value_type> hash_elem;
public:
   size_t operator() (const Vector& v) const
   {
      size_t h=1;
      for (typename ensure_features<Vector, cons<end_sensitive,sparse_compatible> >::const_iterator e=ensure(v, (cons<end_sensitive, sparse_compatible>*)0).begin(); !e.at_end(); ++e)
         h += (hash_elem(*e) * (e.index()+1));
      return h;
   }
};

} // end namespace pm

namespace polymake {
   using pm::GenericVector;
   using pm::FixedVector;
   using pm::scalar2vector;
   using pm::array2vector;
   using pm::same_element_vector;
   using pm::same_element_sparse_vector;
   using pm::convert_to;
   using pm::ones_vector;
   using pm::zero_vector;
   using pm::unit_vector;
}

namespace std {
   template <typename Vector1, typename Vector2, typename E> inline
   void swap(pm::GenericVector<Vector1,E>& v1, pm::GenericVector<Vector2,E>& v2)
   {
      v1.top().swap(v2.top());
   }

   // due to silly overloading rules
   template <typename VectorRef1, typename VectorRef2> inline
   void swap(pm::VectorChain<VectorRef1,VectorRef2>& v1, pm::VectorChain<VectorRef1,VectorRef2>& v2)
   {
      v1.swap(v2);
   }

   template <typename E, int _size> inline
   void swap(pm::FixedVector<E,_size>& v1, pm::FixedVector<E,_size>& v2)
   {
      v1.swap(v2);
   }
}

#endif // POLYMAKE_GENERIC_VECTOR_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
