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

/** @file SparseVector.h
    @brief Implementation of pm::SparseVector class
 */

#ifndef POLYMAKE_SPARSE_VECTOR_H
#define POLYMAKE_SPARSE_VECTOR_H

#include "polymake/GenericVector.h"
#include "polymake/internal/tree_containers.h"
#include "polymake/internal/sparse.h"
#include "polymake/internal/shared_object.h"
#include "polymake/internal/AVL.h"

namespace pm {

template <typename NodeRef>
class sparse_vector_accessor : public AVL::node_accessor<NodeRef> {
   typedef AVL::node_accessor<NodeRef> _super;
public:
   typedef typename inherit_ref<typename deref<NodeRef>::type::mapped_type, typename _super::argument_type>::type result_type;
   result_type operator() (typename _super::argument_type n) const { return n.key_and_data.second; }
};

template <typename NodeRef>
struct sparse_vector_index_accessor {
   typedef NodeRef argument_type;
   typedef const int result_type;
   result_type operator() (NodeRef n) const { return n.key_and_data.first; }
};

/** \ref vector_sec "Vector type" class which is an associative container with
    element indices (coordinates) as keys; elements equal to the default value
    (@c ElementType(), which is 0 for most numerical types) are not stored,
    but implicitly encoded by the gaps in the key set.  It is based on an AVL
    tree.
*/

template <typename E>
class SparseVector
   : public modified_tree< SparseVector<E>,
                           list( Container< AVL::tree< AVL::traits<int, E, operations::cmp> > >,
                                 Operation< pair< BuildUnary<sparse_vector_accessor>,
                                                  BuildUnary<sparse_vector_index_accessor> > > ) >,
     public GenericVector<SparseVector<E>, E> {
   typedef modified_tree<SparseVector> _super;
protected:
   typedef AVL::tree< AVL::traits<int, E, operations::cmp> > tree_type;
   struct impl {
      tree_type tree;
      int d;
 
      impl() : d(0) {}
      void clear() { d=0; tree.clear(); }
   };
   shared_object<impl, AliasHandler<shared_alias_handler> > data;

   friend SparseVector& make_mutable_alias(SparseVector& alias, SparseVector& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }

   typedef sparse_proxy_base<SparseVector> proxy_base;

   template <typename Iterator>
   void init(Iterator src, int dim_arg)
   {
      data.get()->d=dim_arg;
      data.get()->tree.assign(src);
   }

public:
   typedef random_access_iterator_tag container_category;
   typedef sparse_elem_proxy<proxy_base> reference;

   tree_type& get_container() { return data->tree; }
   const tree_type& get_container() const { return data->tree; }

   /// tell the current vector dimension, i.e., the number of non-zero elements (may differ from size)
   int dim() const { return data->d; }
   
   /// create as empty
   SparseVector() {}

   /// create vector of length n, (implicitly) initialize all elements with 0  
   explicit SparseVector(int dim_arg) { data.get()->d=dim_arg; }

   /** Create a vector of dimension n, initialize the elements from a data sequence.
       Iterator can be either indexed, or supply index-value pairs, e.g. std::pair<int,ElementType> or a plain sequence of data items. In the latter case zero elements are filtered out.*/ 
   template <typename Iterator>
   SparseVector(int dim_arg, Iterator src,
                typename construct_sparse_iterator<Iterator, E>::enabled=0)
   {
      init(construct_sparse_iterator<Iterator, E>()(src, dim_arg), dim_arg);
   }

   /// Copy of a disguised SparseVector object.
   SparseVector(const GenericVector<SparseVector>& v)
      : data(v.top().data) {}

   /// Create a vector as a copy of another vector of the same element type.
   template <typename Vector2>
   SparseVector(const GenericVector<Vector2, E>& v)
   {
      init(ensure(v.top(), (pure_sparse*)0).begin(), v.dim());
   }

   /// Create a vector as a copy of another vector with a different element type. 
   template <typename Vector2, typename E2>
   explicit SparseVector(const GenericVector<Vector2, E2>& v,
                         typename enable_if<void**, convertible_to<E2, E>::value>::type=0)
   {
      init(ensure(v.top(), (pure_sparse*)0).begin(), v.dim());
   }

   template <typename Vector2, typename E2>
   explicit SparseVector(const GenericVector<Vector2, E2>& v,
                         typename enable_if<void**, explicitly_convertible_to<E2, E>::value>::type=0)
   {
      init(ensure(attach_converter<E>(v.top()), (pure_sparse*)0).begin(), v.dim());
   }

   /// Create a vector of dimension n, copy element values from a built-in array. 
   template <typename E2, size_t n>
   explicit SparseVector(const E2 (&a)[n],
                         typename enable_if<void**, convertible_to<E2, E>::value>::type=0)
   {
      init(ensure(array2container(a), (pure_sparse*)0).begin(), n);
   }

   /// Create a vector with another element type, using explicit type conversion. 
   template <typename E2, size_t n>
   explicit SparseVector(const E2 (&a)[n],
                         typename enable_if<void**, explicitly_convertible_to<E2, E>::value>::type=0)
   {
      init(ensure(attach_converter<E>(array2container(a)), (pure_sparse*)0).begin(), n);
   }

   /// truncate to zero size
   void clear() { data.apply(shared_clear()); }

   /// change the size, initialize appended elements with default constructor
   void resize(int n)
   {
      if (n<data->d) {
         typename _super::reverse_iterator i=this->rbegin();
         while (!i.at_end() && i.index()>=n) this->erase(i++);
      }
      data->d=n;
   }

   SparseVector& operator= (const SparseVector& other) { assign(other); return *this; }
   using SparseVector::generic_type::operator=;

   void swap(SparseVector& v) { data.swap(v.data); }

   friend void relocate(SparseVector* from, SparseVector* to)
   {
      relocate(&from->data, &to->data);
   }

   /// random access, may cost O(log(n)) time; \ref vector_performance
   reference operator[] (int i)
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=dim())
            throw std::runtime_error("SparseVector::operator[] - index out of range");
      }
      return proxy_base(*this,i);
   }

   /// constant random access, may cost O(log(n)) time; \ref vector_performance
   const E& operator[] (int i) const
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=dim())
            throw std::runtime_error("SparseVector::operator[] - index out of range");
      }
      return deref_sparse_iterator(this->find(i));
   }

   /// appending a GenericVector
   template <typename Vector2, typename E2>
   typename enable_if<SparseVector, convertible_to<E2, E>::value>::type&
   operator|= (const GenericVector<Vector2, E2>& v)
   {
      append(v.dim(), ensure(v.top(), (pure_sparse*)0).begin());
      return *this;
   }

   template <typename Vector2, typename E2>
   typename enable_if<SparseVector, explicitly_convertible_to<E2, E>::value>::type&
   operator|= (const GenericVector<Vector2, E2>& v)
   {
      append(v.dim(), make_converting_iterator<E>(ensure(v.top(), (pure_sparse*)0).begin()));
      return *this;
   }

   /// appending an element
   template <typename E2>
   typename enable_if<SparseVector, convertible_to<E2, E>::value>::type&
   operator|= (const E2& r)
   {
      if (!is_zero(r)) data->tree.push_back(data->d, r);
      data->d++;
      return *this;
   }

   template <typename E2>
   typename enable_if<SparseVector, explicitly_convertible_to<E2, E>::value>::type&
   operator|= (const E2& r)
   {
      if (!is_zero(r)) data->tree.push_back(data->d, conv<E2, E>()(r));
      data->d++;
      return *this;
   }

#if POLYMAKE_DEBUG
   void check() const { data->tree.check(""); }
   void tree_dump() const { data->tree.dump(); }
#endif

protected:
   template <typename, typename> friend class GenericVector;

   void assign(const SparseVector& v) { data=v.data; }

   template <typename Vector2>
   void assign(const Vector2& v,
               typename enable_if<void**, convertible_to<typename Vector2::element_type, E>::value>::type=0)
   {
      if (data.is_shared()) {
         *this=SparseVector(v);
      } else {
         data.get()->tree.assign(ensure(v, (pure_sparse*)0).begin());
         data.get()->d=v.dim();
      }
   }

   template <typename Vector2>
   void assign(const Vector2& v,
               typename enable_if<void**, explicitly_convertible_to<typename Vector2::element_type, E>::value>::type=0)
   {
      if (data.is_shared()) {
         *this=SparseVector(v);
      } else {
         data.get()->tree.assign(ensure(attach_converter<E>(v), (pure_sparse*)0).begin());
         data.get()->d=v.dim();
      }
   }

   template <typename Operation>
   void assign_op(const Operation& op)
   {
      if (data.is_shared())
         *this=SparseVector(LazyVector1<const SparseVector&, Operation>(*this,op));
      else
         SparseVector::generic_type::assign_op(op);
   }

   template <typename Vector2, typename Operation>
   void assign_op(const Vector2& v, const Operation& op)
   {
      if (data.is_shared())
         *this=SparseVector(LazyVector2<const SparseVector&, const Vector2&, Operation>(*this,v,op));
      else
         SparseVector::generic_type::assign_op(v,op);
   }

   template <typename Iterator>
   void append(int added, Iterator src)
   {
      const int d=data->d;
      tree_type& t=data->tree;
      for (; !src.at_end();  ++src)
         t.push_back(src.index()+d, *src);
      data->d += added;
   }

   template <typename E2>
   void _fill(const E2& x, pure_sparse,
              typename enable_if<void**, convertible_to<E2, E>::value>::type=0)
   {
      data->tree.clear();
      if (!is_zero(x)) {
         tree_type& t=data.get()->tree;
         const int d=data.get()->d;
         for (int i=0; i<d; ++i) t.push_back(i,x);
      }
   }
};

template <typename E>
struct check_container_feature<SparseVector<E>, pure_sparse> : True {};

template <typename Vector, typename E, typename Permutation> inline
typename enable_if<SparseVector<E>, Vector::is_sparse>::type
permuted(const GenericVector<Vector,E>& v, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Vector>::value) {
      if (v.dim() != perm.size())
         throw std::runtime_error("permuted - dimension mismatch");
   }
   SparseVector<E> result(v.dim());
   for (typename ensure_features<Permutation, cons<end_sensitive,indexed> >::const_iterator p=ensure(perm, (cons<end_sensitive,indexed>*)0).begin();
        !p.at_end();  ++p) {
      typename Vector::const_iterator e=v.top().find(*p);
      if (!e.at_end()) result.push_back(p.index(), *e);
   }
   return result;
}

template <typename Vector, typename E, typename Permutation> inline
typename enable_if<SparseVector<E>, Vector::is_sparse>::type
permuted_inv(const GenericVector<Vector,E>& v, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Vector>::value) {
      if (v.dim() != perm.size())
         throw std::runtime_error("permuted_inv - dimension mismatch");
   }
   SparseVector<E> result(v.dim());
   int pos=0;
   typename Permutation::const_iterator p=perm.begin();
   for (typename Entire<Vector>::const_iterator e=entire(v.top());  !e.at_end();  ++e) {
      std::advance(p, e.index()-pos);
      pos=e.index();
      result.insert(*p,*e);
   }
   return result;
}

} // end namespace pm

namespace polymake {
   using pm::SparseVector;
}

namespace std {
   template <typename E> inline
   void swap(pm::SparseVector<E>& v1, pm::SparseVector<E>& v2) { v1.swap(v2); }
}

#endif // POLYMAKE_SPARSE_VECTOR_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
