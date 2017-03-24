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

#ifndef POLYMAKE_TRANSFORMED_CONTAINER_H
#define POLYMAKE_TRANSFORMED_CONTAINER_H

#include "polymake/internal/sparse.h"

namespace pm {
/* ----------------------
 *  TransformedContainer
 * ---------------------- */

template <typename ContainerRef, typename Operation>
class TransformedContainer
   : public modified_container_base<ContainerRef, Operation>,
     public modified_container_impl< TransformedContainer<ContainerRef, Operation>,
                                     mlist< ContainerTag< ContainerRef >,
                                            OperationTag< Operation > > > {
   typedef modified_container_base<ContainerRef, Operation> base_t;
public:
   TransformedContainer(typename base_t::arg_type src_arg,
                        const typename base_t::operation_type& op_arg=typename base_t::operation_type())
      : base_t(src_arg, op_arg) {}

   using base_t::get_operation;
};

template <typename ContainerRef, typename Operation>
struct check_container_feature<TransformedContainer<ContainerRef, Operation>, sparse>
   : check_container_ref_feature<ContainerRef, sparse> { };

template <typename ContainerRef, typename Operation>
struct check_container_feature<TransformedContainer<ContainerRef, Operation>, pure_sparse>
   : check_container_ref_feature<ContainerRef, pure_sparse> { };

template <typename ContainerRef, typename Operation>
struct spec_object_traits< TransformedContainer<ContainerRef, Operation> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true,
      is_always_const=std::is_same<typename TransformedContainer<ContainerRef, Operation>::reference,
                                   typename TransformedContainer<ContainerRef, Operation>::const_reference>::value;
};

template <typename Container, typename Operation> inline
TransformedContainer<Container&, Operation>
attach_operation(Container& src, const Operation& op)
{
   return TransformedContainer<Container&, Operation> (src,op);
}

template <typename Container, typename Operation> inline
TransformedContainer<const Container&, Operation>
attach_operation(const Container& src, const Operation& op)
{
   return TransformedContainer<const Container&, Operation> (src,op);
}

template <typename Container, typename dummy_arg> inline
Container& attach_operation(Container& src, operations::identity<dummy_arg>)
{
   return src;
}

template <typename Container, typename dummy_arg> inline
const Container& attach_operation(const Container& src, operations::identity<dummy_arg>)
{
   return src;
}

template <typename Container, typename Class, typename Member> inline
TransformedContainer<Container&, operations::var_member<Class&,Member> >
attach_member_accessor(Container& c, Member Class::*ptr)
{
   return TransformedContainer<Container&, operations::var_member<Class&,Member> > (c,ptr);
}

template <typename Container, typename Class, typename Member> inline
TransformedContainer<const Container&, operations::var_member<const Class&,Member> >
attach_member_accessor(const Container& c, Member Class::*ptr)
{
   return TransformedContainer<const Container&, operations::var_member<const Class&,Member> > (c,ptr);
}

template <typename Container, typename Class, typename Member, Member Class::*Ptr> inline
TransformedContainer<Container&, operations::member<Class,Member,Ptr> >
attach_member_accessor(Container& c, ptr2type<Class, Member, Ptr>)
{
   return c;
}

template <typename Container, typename Class, typename Member, Member Class::*Ptr> inline
TransformedContainer<const Container&, operations::member<Class,Member,Ptr> >
attach_member_accessor(const Container& c, ptr2type<Class, Member, Ptr>)
{
   return c;
}

/* --------------------------
 *  TransformedContainerPair
 * -------------------------- */

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
struct TransformedContainerPair_helper1 {
   typedef typename deref<ContainerRef1>::type container1;
   typedef typename deref<ContainerRef2>::type container2;
   typedef typename binary_op_builder<Operation, void, void,
                                      typename container_traits<ContainerRef1>::reference,
                                      typename container_traits<ContainerRef2>::reference>::operation
      element_operation;

   static const bool
      partially_defined = operations::is_partially_defined<element_operation>::value,
      sparse1 = check_container_feature<container1, sparse>::value,
      sparse2 = check_container_feature<container2, sparse>::value,
      constant1 = object_classifier::what_is<container1>::value == object_classifier::is_constant,
      constant2 = object_classifier::what_is<container2>::value == object_classifier::is_constant,
      use_sparse_coupler = (sparse1 || sparse2) && (partially_defined || !constant1 && !constant2),
      sparse_result = partially_defined ? sparse1 && sparse2 : sparse1 || sparse2;
};

template <typename Operation, bool use_sparse_coupler, bool partially_defined>
struct TransformedContainerPair_helper2 {
   typedef Operation operation;
   typedef OperationTag<operation> params2;
};

template <typename Operation, bool partially_defined>
struct TransformedContainerPair_helper2<Operation, true, partially_defined> {
   typedef typename std::conditional<partially_defined, set_union_zipper, set_intersection_zipper>::type controller;
   typedef typename std::conditional<partially_defined,
                                     pair<Operation, BuildBinaryIt<operations::zipper_index>>,
                                     Operation>::type
      operation;

   typedef mlist< IteratorCouplerTag< sparse_coupler<controller> >,
                  IteratorConstructorTag< binary_transform_constructor< BijectiveTag<std::false_type>,
                                                                        PartiallyDefinedTag<bool_constant<partially_defined>> > >,
                  OperationTag<operation> > params2;
};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
struct TransformedContainerPair_helper
   : TransformedContainerPair_helper1<ContainerRef1, ContainerRef2, Operation> {
   typedef TransformedContainerPair_helper1<ContainerRef1, ContainerRef2, Operation> base_t;
   typedef typename std::conditional<!base_t::sparse1 && !base_t::constant1 && base_t::sparse2,
                                     masquerade_add_features<ContainerRef1, sparse_compatible>, ContainerRef1>::type
      first_type;
   typedef typename std::conditional<!base_t::sparse2 && !base_t::constant2 && base_t::sparse1,
                                     masquerade_add_features<ContainerRef2, sparse_compatible>, ContainerRef2>::type
      second_type;

   typedef TransformedContainerPair_helper2<Operation, base_t::use_sparse_coupler, base_t::partially_defined> helper2;
   typedef typename mlist_concat< Container1Tag<first_type>, Container2Tag<second_type>, typename helper2::params2>::type params;
   typedef typename helper2::operation operation;
   typedef modified_container_pair_base<first_type, second_type, operation> base;
   typedef typename is_identity_transform<Operation>::type op_arg_type;

   static const op_arg_type& get_operation(const op_arg_type& op_arg, std::true_type)
   {
      return op_arg;
   }
   static operation get_operation(const Operation& op_arg, std::false_type)
   {
      return operation(op_arg, typename operation::second_type());
   }
};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
class TransformedContainerPair
   : public TransformedContainerPair_helper<ContainerRef1, ContainerRef2, Operation>::base,
     public modified_container_pair_impl< TransformedContainerPair<ContainerRef1, ContainerRef2, Operation>,
               typename TransformedContainerPair_helper<ContainerRef1, ContainerRef2, Operation>::params
            > {
   typedef modified_container_pair_impl<TransformedContainerPair> impl_t;
   typedef TransformedContainerPair_helper<ContainerRef1, ContainerRef2, Operation> helper;
   typedef typename helper::base base_t;
   static const bool simple_operation=std::is_same<Operation, typename helper::operation>::value;
protected:
   int size_impl(std::false_type, std::false_type) const { return impl_t::size(); }
   int size_impl(std::true_type, std::false_type) const { return this->get_container1().size(); }
   int size_impl(std::false_type, std::true_type) const { return this->get_container2().size(); }
public:
   TransformedContainerPair(typename base_t::first_arg_type src1_arg, typename base_t::second_arg_type src2_arg,
                            const typename helper::op_arg_type& op_arg=typename helper::op_arg_type())
      : base_t(src1_arg, src2_arg, helper::get_operation(op_arg, bool_constant<simple_operation>())) { }

   using base_t::get_operation;

   int size() const
   {
      return size_impl(bool_constant<helper::use_sparse_coupler && (helper::partially_defined ? !helper::sparse1 : !helper::sparse2)>(),
                       bool_constant<helper::use_sparse_coupler && (helper::partially_defined ? !helper::sparse2 : !helper::sparse1)>());
   }
};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
struct check_container_feature<TransformedContainerPair<ContainerRef1, ContainerRef2, Operation>, sparse> {
   static const bool value=TransformedContainerPair_helper<ContainerRef1, ContainerRef2, Operation>::sparse_result;
};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
struct check_container_feature<TransformedContainerPair<ContainerRef1, ContainerRef2, Operation>, sparse_compatible> {
   static const bool value=check_container_ref_feature<ContainerRef1, sparse_compatible>::value ||
                           check_container_ref_feature<ContainerRef2, sparse_compatible>::value;
};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
struct spec_object_traits< TransformedContainerPair<ContainerRef1, ContainerRef2, Operation> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true,
                     is_always_const=std::is_same<typename TransformedContainerPair<ContainerRef1, ContainerRef2, Operation>::reference,
                                                  typename TransformedContainerPair<ContainerRef1, ContainerRef2, Operation>::const_reference>::value;
};

template <typename Container1, typename Container2, typename Operation> inline
TransformedContainerPair<Container1&, Container2&, Operation>
attach_operation(Container1& c1, Container2& c2, Operation op)
{
   return TransformedContainerPair<Container1&, Container2&, Operation> (c1,c2,op);
}

template <typename Container1, typename Container2, typename Operation> inline
TransformedContainerPair<const Container1&, Container2&, Operation>
attach_operation(const Container1& c1, Container2& c2, Operation op)
{
   return TransformedContainerPair<const Container1&, Container2&, Operation> (c1,c2,op);
}

template <typename Container1, typename Container2, typename Operation> inline
TransformedContainerPair<Container1&, const Container2&, Operation>
attach_operation(Container1& c1, const Container2& c2, Operation op)
{
   return TransformedContainerPair<Container1&, const Container2&, Operation> (c1,c2,op);
}

template <typename Container1, typename Container2, typename Operation> inline
TransformedContainerPair<const Container1&, const Container2&, Operation>
attach_operation(const Container1& c1, const Container2& c2, Operation op)
{
   return TransformedContainerPair<const Container1&, const Container2&, Operation> (c1,c2,op);
}

template <typename Container, typename Scalar> inline
TransformedContainerPair<const Container&, constant_value_container<const Scalar&>, BuildBinary<operations::add> >
translate(const Container& c, const Scalar& x)
{
   return TransformedContainerPair<const Container&, constant_value_container<const Scalar&>, BuildBinary<operations::add> > (c,x);
}

template <typename Container, typename Scalar> inline
TransformedContainerPair<const Container&, constant_value_container<const Scalar&>, BuildBinary<operations::mul> >
scale(const Container& c, const Scalar& x)
{
   return TransformedContainerPair<const Container&, constant_value_container<const Scalar&>, BuildBinary<operations::mul> > (c,x);
}

/* ------------------
 *  ContainerProduct
 * ------------------ */

/** Common base for iterators over cartesian product of two sequences
    It runs over all possible pairs of data items, with the second component changing first.
*/
template <typename Iterator1, typename Iterator2, bool has_state1=false, bool has_state2=false>
class iterator_product : public Iterator1 {
public:
   typedef Iterator1 first_type;
   typedef Iterator2 second_type;
   Iterator2 second;

   typedef forward_iterator_tag iterator_category;
   typedef iterator_product<typename iterator_traits<Iterator1>::iterator,
                            typename iterator_traits<Iterator2>::iterator, has_state1, has_state2>
      iterator;
   typedef iterator_product<typename iterator_traits<Iterator1>::const_iterator,
                            typename iterator_traits<Iterator2>::const_iterator, has_state1, has_state2>
      const_iterator;

   iterator_product() { }

   iterator_product(const iterator& it)
      : first_type(static_cast<const typename iterator::first_type&>(it))
      , second(it.second) { }

   template <typename SourceIterator1, typename SourceIterator2,
             typename suitable1=typename suitable_arg_for_iterator<SourceIterator1, Iterator1>::type,
             typename suitable2=typename suitable_arg_for_iterator<SourceIterator2, Iterator2>::type>
   iterator_product(const SourceIterator1& first_arg, const SourceIterator2& second_arg)
      : first_type(prepare_iterator_arg<Iterator1>(first_arg))
      , second(prepare_iterator_arg<Iterator2>(second_arg)) { }

   iterator_product& operator= (const iterator& it)
   {
      first_type::operator=(static_cast<const typename iterator::first_type&>(it));
      second=it.second;
      return *this;
   }

   iterator_product& operator++()
   {
      ++second;
      if (second.at_end()) {
         first_type::operator++();
         second.rewind();
      }
      return *this;
   }

   const iterator_product operator++(int) { iterator_product copy=*this; operator++(); return copy; }

   template <typename Other>
   typename std::enable_if<is_derived_from_any<Other, iterator, const_iterator>::value, bool>::type
   operator== (const Other& it) const
   {
      return static_cast<const first_type&>(*this)==static_cast<const typename Other::first_type&>(it)  &&  second==it.second;
   }
   template <typename Other>
   typename std::enable_if<is_derived_from_any<Other, iterator, const_iterator>::value, bool>::type
   operator!= (const Other& it) const
   {
      return !operator==(it);
   }

private:
   void operator--();
   void operator+=(int);
   void operator-=(int);
   void operator+(int);
   void operator-(int);
   void operator[](int);

   template <typename,typename,bool,bool> friend class iterator_product;
};

template <typename Iterator1, typename Iterator2, bool has_state1>
class iterator_product<Iterator1, Iterator2, has_state1, true>
   : public iterator_product<Iterator1, Iterator2, has_state1, false> {
   typedef iterator_product<Iterator1, Iterator2, has_state1, false> base_t;
   template <typename,typename,bool,bool> friend class iterator_product;
protected:
   int state;

   void set_state(std::false_type)
   {
      state= this->second.state & zipper_gt ? zipper_lt : this->second.state;
   }
   void set_state(std::true_type)
   {
      state= (this->second.state & zipper_gt) && (base_t::state & zipper_eq) ? zipper_lt : base_t::state;
   }
   void set_state()
   {
      set_state(bool_constant<has_state1>());
   }

   void incr(std::false_type)
   {
      base_t::operator++();
      set_state(std::false_type());
   }
   void incr(std::true_type)
   {
      if (base_t::state & zipper_eq) {
         base_t::operator++();
         set_state(std::true_type());
      } else {
         typedef typename base_t::super super1;
         typedef typename Iterator2::super super2;
         this->second.super2::operator++();
         if (this->second.super2::at_end()) {
            super1::operator++();
            this->second.rewind();
            set_state(std::true_type());
         }
      }
   }

public:
   typedef iterator_product<typename iterator_traits<Iterator1>::iterator,
                            typename iterator_traits<Iterator2>::iterator, has_state1, true>
      iterator;
   typedef iterator_product<typename iterator_traits<Iterator1>::const_iterator,
                            typename iterator_traits<Iterator2>::const_iterator, has_state1, true>
      const_iterator;

   iterator_product() { }
   iterator_product(const iterator& it)
      : base_t(static_cast<const typename iterator::base_t&>(it)) { }

   template <typename SourceIterator1, typename SourceIterator2,
             typename enable=typename std::enable_if<is_const_compatible_with<SourceIterator1, Iterator1>::value &&
                                                     is_const_compatible_with<SourceIterator2, Iterator2>::value>::type>
   iterator_product(const SourceIterator1& first_arg, SourceIterator2& second_arg)
      : base_t(first_arg, second_arg)
   {
      set_state();
   }

   iterator_product& operator= (const iterator& it)
   {
      base_t::operator=(static_cast<const typename iterator::base_t&>(it));
      return *this;
   }

   iterator_product& operator++ ()
   {
      incr(bool_constant<has_state1>());
      return *this;
   }

   const iterator_product operator++(int) { iterator_product copy=*this; operator++(); return copy; }

   void rewind()
   {
      static_assert(check_iterator_feature<Iterator1, rewindable>::value, "iterator is not rewindable");
      base_t::rewind(); set_state();
   }
};

template <typename Iterator1, typename Iterator2, bool has_state1, bool has_state2, typename Feature>
struct check_iterator_feature<iterator_product<Iterator1, Iterator2, has_state1, has_state2>, Feature>
   : check_iterator_feature<Iterator1,Feature> { };

template <typename Iterator1, typename Iterator2, bool has_state1, bool has_state2>
struct check_iterator_feature<iterator_product<Iterator1, Iterator2, has_state1, has_state2>, indexed> : std::false_type { };

template <typename Iterator1, typename Iterator2, bool has_state1, bool has_state2>
struct has_partial_state< iterator_product<Iterator1, Iterator2, has_state1, has_state2> > {
   static const bool value=has_state1 || has_state2;
};

struct product_index_accessor {
   typedef void first_argument_type;
   typedef void second_argument_type;
   typedef int result_type;

   template <typename Iterator1, typename Iterator2>
   result_type operator() (const Iterator1& it1, const Iterator2& it2) const
   {
      return it1.index()*dim2 + it2.index();
   }
   template <typename Iterator1, typename Iterator2>
   result_type operator() (operations::partial_left, const Iterator1& it1, const Iterator2& it2) const
   {
      return it1.index()*dim2 + *it2.second;
   }
   template <typename Iterator1, typename Iterator2>
   result_type operator() (operations::partial_right, const Iterator1& it1, const Iterator2& it2) const
   {
      return *it1.second*dim2 + it2.index();
   }

   product_index_accessor(int dim2_arg=0) : dim2(dim2_arg) { }
protected:
   int dim2;
};

template <typename Top, typename TParams>
class container_product_typebase : public manip_container_top<Top, TParams> {
   typedef manip_container_top<Top, TParams> base_t;
public:
   typedef typename mtagged_list_extract<TParams, Container1Tag>::type container1_ref;
   typedef typename mtagged_list_extract<TParams, Container2Tag>::type container2_ref;
   typedef typename deref<container1_ref>::minus_ref container1;
   typedef typename deref<container2_ref>::minus_ref container2;
   typedef typename temp_ref<container1_ref>::type container1_temp_ref;
   typedef typename temp_ref<container2_ref>::type container2_temp_ref;

   typedef typename container_traits<container1>::iterator iterator1;
   typedef typename container_traits<container2>::iterator iterator2;
   typedef typename mtagged_list_extract<TParams, OperationTag, operations::apply1< BuildUnaryIt<operations::dereference> > >::type
      value_operation;

   typedef typename binary_op_builder<value_operation, iterator1, iterator2>::operation value_operation_instance;
   static const bool
      sparse1=check_container_ref_feature<container1, sparse>::value,
      sparse2=check_container_ref_feature<container2, sparse>::value,
      partially_defined = (sparse1 || sparse2) && operations::is_partially_defined<value_operation_instance>::value,
      sparse_result = partially_defined ? sparse1 && sparse2 : sparse1 || sparse2,
      need_index= sparse_result || list_search<typename base_t::expected_features, indexed, absorbing_feature>::value;

   typedef typename mselect< std::enable_if< partially_defined && sparse1,
                                             masquerade<construct_dense_pair, container1_ref> >,
                             std::enable_if< !partially_defined && !sparse1 && sparse2,
                                             masquerade_add_features<container1_ref, sparse_compatible> >,
                             container1_ref >::type
      prep_container1_ref;
   typedef typename mselect< std::enable_if< partially_defined && sparse2,
                                             masquerade<construct_dense_pair, container2_ref> >,
                             std::enable_if< !partially_defined && !sparse2 && sparse1,
                                             masquerade_add_features<container2_ref, sparse_compatible> >,
                             container2_ref >::type
      prep_container2_ref;
   typedef typename deref<prep_container1_ref>::minus_ref prep_container1;
   typedef typename deref<prep_container2_ref>::minus_ref prep_container2;

   typedef typename std::conditional<need_index, pair<value_operation, product_index_accessor>, value_operation>::type operation;

   typedef typename mtagged_list_extract<TParams, IteratorConstructorTag, binary_transform_constructor<> >::type it_constructor;

   typedef iterator_product<typename container_traits<prep_container1>::iterator,
                            typename container_traits<prep_container2>::iterator,
                            partially_defined && sparse1, partially_defined && sparse2>
      first_try_it_pair;
   typedef typename it_constructor::template defs<first_try_it_pair, operation, typename base_t::expected_features>::needed_pair_features
      needed_pair_features;
   typedef typename std::conditional<need_index,
                                     typename mix_features<needed_pair_features, indexed>::type,
                                     needed_pair_features>::type
      needed_features1;
   typedef typename mix_features<needed_features1, cons<end_sensitive, rewindable> >::type
      needed_features2;
   typedef iterator_product<typename ensure_features<prep_container1, needed_features1>::iterator,
                            typename ensure_features<prep_container2, needed_features2>::iterator,
                            partially_defined && sparse1, partially_defined && sparse2>
      it_pair;
   typedef iterator_product<typename ensure_features<prep_container1, needed_features1>::const_iterator,
                            typename ensure_features<prep_container2, needed_features2>::const_iterator,
                            partially_defined && sparse1, partially_defined && sparse2>
      const_it_pair;
   typedef typename it_constructor::template defs<it_pair, operation, typename base_t::expected_features>::iterator
      iterator;
   typedef typename it_constructor::template defs<const_it_pair, operation, typename base_t::expected_features>::iterator
      const_iterator;

   // TODO: remove the first term after implementing random access
   typedef typename least_derived_class<bidirectional_iterator_tag,
                                        typename container_traits<container1>::category,
                                        typename container_traits<container2>::category>::type
      container_category;
   typedef typename mix_features<typename enforce_feature_helper<typename deref<prep_container1>::type>::must_enforce_features,
                                 typename enforce_feature_helper<typename deref<prep_container2>::type>::must_enforce_features>::type
      must_enforce_features;

   typedef typename iterator_traits<iterator>::value_type value_type;
   typedef typename iterator_traits<iterator>::reference reference;
   typedef typename iterator_traits<const_iterator>::reference const_reference;
protected:
   static prep_container1& prep1(container1& c1) { return reinterpret_cast<prep_container1&>(c1); }
   static prep_container2& prep2(container2& c2) { return reinterpret_cast<prep_container2&>(c2); }

   typedef typename std::conditional<attrib<container1>::is_const, type2type<container1>, container1>::type ct1;
   typedef typename std::conditional<attrib<container2>::is_const, type2type<container2>, container2>::type ct2;
   static const prep_container1& prep1(const ct1& c1) { return reinterpret_cast<const prep_container1&>(c1); }
   static const prep_container2& prep2(const ct2& c2) { return reinterpret_cast<const prep_container2&>(c2); }
};

template <typename Top, typename TParams>
class reverse_container_product_typebase {
   typedef container_product_typebase<Top, TParams> base_t;
   typedef iterator_product<typename ensure_features<typename base_t::prep_container1, typename base_t::needed_features1>::reverse_iterator,
                            typename ensure_features<typename base_t::prep_container2, typename base_t::needed_features2>::reverse_iterator,
                            base_t::partially_defined && base_t::sparse1, base_t::partially_defined && base_t::sparse2>
      reverse_it_pair;
   typedef iterator_product<typename ensure_features<typename base_t::prep_container1, typename base_t::needed_features1>::const_reverse_iterator,
                            typename ensure_features<typename base_t::prep_container2, typename base_t::needed_features2>::const_reverse_iterator,
                            base_t::partially_defined && base_t::sparse1, base_t::partially_defined && base_t::sparse2>
      const_reverse_it_pair;
public:
   typedef typename base_t::it_constructor::template defs<reverse_it_pair, typename base_t::operation,
                                                          typename base_t::expected_features>::iterator
      reverse_iterator;
   typedef typename base_t::it_constructor::template defs<const_reverse_it_pair, typename base_t::operation,
                                                          typename base_t::expected_features>::iterator
      const_reverse_iterator;
};

template <typename Top, typename TParams=typename Top::manipulator_params,
          typename Category=typename container_product_typebase<Top, TParams>::container_category>
class container_product_impl
   : public container_product_typebase<Top, TParams>
   , public modified_container_non_bijective_elem_access<Top, container_product_typebase<Top, TParams>,
                                                         is_derived_from<Category, bidirectional_iterator_tag>::value>
{
   typedef container_product_typebase<Top, TParams> base_t;
public:
   typedef container_product_impl<Top, TParams> manipulator_impl;
   typedef TParams manipulator_params;
   typedef typename base_t::iterator iterator;
   typedef typename base_t::const_iterator const_iterator;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef container_product_impl<FeatureCollector, TParams> type;
   };

   iterator begin()
   {
      typename base_t::container1_temp_ref c1=this->manip_top().get_container1();
      typename base_t::container2_temp_ref c2=this->manip_top().get_container2();
      return iterator(c2.empty() ? ensure(base_t::prep1(c1), (typename base_t::needed_features1*)0).end()
                                 : ensure(base_t::prep1(c1), (typename base_t::needed_features1*)0).begin(),
                      ensure(base_t::prep2(c2), (typename base_t::needed_features2*)0).begin(),
                      create_operation());
   }
   iterator end()
   {
      typename base_t::container1_temp_ref c1=this->manip_top().get_container1();
      typename base_t::container2_temp_ref c2=this->manip_top().get_container2();
      return iterator(ensure(base_t::prep1(c1), (typename base_t::needed_features1*)0).end(),
                      ensure(base_t::prep2(c2), (typename base_t::needed_features2*)0).begin(),
                      create_operation());
   }
   const_iterator begin() const
   {
      const typename base_t::container1& c1=this->manip_top().get_container1();
      const typename base_t::container2& c2=this->manip_top().get_container2();
      return const_iterator(c2.empty() ? ensure(base_t::prep1(c1), (typename base_t::needed_features1*)0).end()
                                       : ensure(base_t::prep1(c1), (typename base_t::needed_features1*)0).begin(),
                            ensure(base_t::prep2(c2), (typename base_t::needed_features2*)0).begin(),
                            create_operation());
   }
   const_iterator end() const
   {
      return const_iterator(ensure(base_t::prep1(this->manip_top().get_container1()), (typename base_t::needed_features1*)0).end(),
                            ensure(base_t::prep2(this->manip_top().get_container2()), (typename base_t::needed_features2*)0).begin(),
                            create_operation());
   }
private:
   template <typename discr2>
   int size_impl(std::false_type, discr2) const
   {
      return this->manip_top().get_container1().size() * this->manip_top().get_container2().size();
   }
   int size_impl(std::true_type, std::false_type) const
   {
      return dim();
   }
   int size_impl(std::true_type, std::true_type) const
   {
      const int d1=get_dim(this->manip_top().get_container1()), d2=get_dim(this->manip_top().get_container2());
      return d1*d2 - (d1-this->manip_top().get_container1().size())*(d2-this->manip_top().get_container2().size());
   }

public:
   int size() const
   {
      return size_impl(bool_constant<base_t::partially_defined>(), bool_constant<base_t::sparse1 && base_t::sparse2>());
   }
   int dim() const
   {
      return get_dim(this->manip_top().get_container1()) * get_dim(this->manip_top().get_container2());
   }
   bool empty() const
   {
      return this->manip_top().get_container1().empty() || this->manip_top().get_container2().empty();
   }

private:
   typename base_t::value_operation
   create_operation_impl(std::false_type) const
   {
      return this->manip_top().get_operation();
   }

   typename base_t::operation
   create_operation_impl(std::true_type) const
   {
      return typename base_t::operation(this->manip_top().get_operation(), get_dim(this->manip_top().get_container2()));
   }
public:
   typename base_t::value_operation get_operation() const { return typename base_t::value_operation(); }
   typename base_t::operation create_operation() const { return create_operation_impl(bool_constant<base_t::need_index>()); }
};

template <typename Top, typename TParams>
class container_product_impl<Top, TParams, bidirectional_iterator_tag>
   : public container_product_impl<Top, TParams, forward_iterator_tag>,
     public reverse_container_product_typebase<Top, TParams> {
   typedef container_product_impl<Top, TParams, forward_iterator_tag> base_t;
   typedef reverse_container_product_typebase<Top, TParams> rbase_t;
public:
   typename rbase_t::reverse_iterator rbegin()
   {
      typename base_t::container1_temp_ref c1=this->manip_top().get_container1();
      typename base_t::container2_temp_ref c2=this->manip_top().get_container2();
      return typename rbase_t::reverse_iterator(c2.empty() ? ensure(base_t::prep1(c1), (typename base_t::needed_features1*)0).rend()
                                                           : ensure(base_t::prep1(c1), (typename base_t::needed_features1*)0).rbegin(),
                                                ensure(base_t::prep2(c2), (typename base_t::needed_features2*)0).rbegin(),
                                                this->create_operation());
   }
   typename rbase_t::reverse_iterator rend()
   {
      typename base_t::container1_temp_ref c1=this->manip_top().get_container1();
      typename base_t::container2_temp_ref c2=this->manip_top().get_container2();
      return typename rbase_t::reverse_iterator(ensure(base_t::prep1(c1), (typename base_t::needed_features1*)0).rend(),
                                                ensure(base_t::prep2(c2), (typename base_t::needed_features2*)0).rbegin(),
                                                this->create_operation());
   }
   typename rbase_t::const_reverse_iterator rbegin() const
   {
      const typename base_t::container1& c1=this->manip_top().get_container1();
      const typename base_t::container2& c2=this->manip_top().get_container2();
      return typename rbase_t::const_reverse_iterator(c2.empty() ? ensure(base_t::prep1(c1), (typename base_t::needed_features1*)0).rend()
                                                                 : ensure(base_t::prep1(c1), (typename base_t::needed_features1*)0).rbegin(),
                                                      ensure(base_t::prep2(c2), (typename base_t::needed_features2*)0).rbegin(),
                                                      this->create_operation());
   }
   typename rbase_t::const_reverse_iterator rend() const
   {
      return typename rbase_t::const_reverse_iterator(ensure(base_t::prep1(this->manip_top().get_container1()), (typename base_t::needed_features1*)0).rend(),
                                                      ensure(base_t::prep2(this->manip_top().get_container2()), (typename base_t::needed_features2*)0).rbegin(),
                                                      this->create_operation());
   }
};

template <typename Top, typename TParams>
class container_product_impl<Top, TParams, random_access_iterator_tag>
   : public container_product_impl<Top, TParams, bidirectional_iterator_tag> {
   typedef container_product_impl<Top, TParams, bidirectional_iterator_tag> base_t;
public:
   // TO BE DONE LATER
};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
class ContainerProduct
   : public modified_container_pair_base<ContainerRef1, ContainerRef2, Operation>
   , public container_product_impl< ContainerProduct<ContainerRef1, ContainerRef2, Operation>,
                                    mlist< Container1Tag<ContainerRef1>,
                                           Container2Tag<ContainerRef2>,
                                           OperationTag<Operation> > > {
   typedef modified_container_pair_base<ContainerRef1, ContainerRef2, Operation> base_t;
public:
   ContainerProduct(typename base_t::first_arg_type src1_arg, typename base_t::second_arg_type src2_arg,
                    const typename base_t::operation_type& op_arg=typename base_t::operation_type())
      : base_t(src1_arg, src2_arg, op_arg) {}

   using base_t::get_operation;
};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
struct check_container_feature<ContainerProduct<ContainerRef1, ContainerRef2, Operation>, sparse> {
   static const bool value=ContainerProduct<ContainerRef1, ContainerRef2, Operation>::sparse_result;
};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
struct spec_object_traits< ContainerProduct<ContainerRef1, ContainerRef2, Operation> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true,
                     is_always_const=std::is_same<typename ContainerProduct<ContainerRef1, ContainerRef2, Operation>::reference,
                                                  typename ContainerProduct<ContainerRef1, ContainerRef2, Operation>::const_reference>::value;
};

template <typename Container1, typename Container2, typename Operation> inline
ContainerProduct<Container1&, Container2&, Operation>
product(Container1& c1, Container2& c2, Operation op)
{
   return ContainerProduct<Container1&, Container2&, Operation> (c1,c2,op);
}

template <typename Container1, typename Container2, typename Operation> inline
ContainerProduct<const Container1&, Container2&, Operation>
product(const Container1& c1, Container2& c2, Operation op)
{
   return ContainerProduct<const Container1&, Container2&, Operation> (c1,c2,op);
}

template <typename Container1, typename Container2, typename Operation> inline
ContainerProduct<Container1&, const Container2&, Operation>
product(Container1& c1, const Container2& c2, Operation op)
{
   return ContainerProduct<Container1&, const Container2&, Operation> (c1,c2,op);
}

template <typename Container1, typename Container2, typename Operation> inline
ContainerProduct<const Container1&, const Container2&, Operation>
product(const Container1& c1, const Container2& c2, Operation op)
{
   return ContainerProduct<const Container1&, const Container2&, Operation> (c1,c2,op);
}

template <typename ContainerRef>
struct repeated_container {
   typedef ContainerProduct<count_down, ContainerRef, operations::apply2< BuildUnaryIt<operations::dereference> > > type;
   typedef typename type::manipulator_params params;
};

template <typename Container>
inline
const typename repeated_container<const Container&>::type
repeat(const Container& c, int n)
{
   return typename repeated_container<const Container&>::type(count_down(n),c);
}

template <typename Iterator>
inline
Iterator&& enforce_movable_values(Iterator&& it,
                                  typename std::enable_if<!std::is_lvalue_reference<typename iterator_traits<Iterator>::reference>::value, void**>::type=nullptr)
{
   return std::forward<Iterator>(it);
}

template <typename Iterator>
inline
unary_transform_iterator<pointer2iterator_t<Iterator>, BuildUnary<operations::move>>
enforce_movable_values(Iterator&& it,
                       typename std::enable_if<std::is_lvalue_reference<typename iterator_traits<Iterator>::reference>::value, void**>::type=nullptr)
{
   return pointer2iterator(std::forward<Iterator>(it));
}

} // end namespace pm

namespace polymake {
   using pm::attach_operation;
   using pm::attach_member_accessor;
   using pm::translate;
   using pm::scale;
   using pm::product;
   using pm::repeat;
   using pm::enforce_movable_values;
}

#endif // POLYMAKE_TRANSFORMED_CONTAINER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
