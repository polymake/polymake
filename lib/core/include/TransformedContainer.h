/* Copyright (c) 1997-2020
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

#ifndef POLYMAKE_TRANSFORMED_CONTAINER_H
#define POLYMAKE_TRANSFORMED_CONTAINER_H

#include "polymake/internal/sparse.h"
#include "polymake/internal/modified_containers.h"
#include "polymake/internal/iterator_zipper.h"

namespace pm {
/* ----------------------
 *  TransformedContainer
 * ---------------------- */

template <typename ContainerRef, typename Operation>
class TransformedContainer
   : public modified_container_base<ContainerRef, Operation>
   , public modified_container_impl< TransformedContainer<ContainerRef, Operation>,
                                     mlist< ContainerRefTag< ContainerRef >,
                                            OperationTag< Operation > > > {
   using base_t = modified_container_base<ContainerRef, Operation>;
public:
   using modified_container_base<ContainerRef, Operation>::modified_container_base;
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
   static constexpr bool
      is_temporary = true,
      is_always_const = std::is_same<typename TransformedContainer<ContainerRef, Operation>::reference,
                                     typename TransformedContainer<ContainerRef, Operation>::const_reference>::value;
};

template <typename Container, typename Operation>
auto attach_operation(Container&& src, const Operation& op)
{
   return TransformedContainer<Container, Operation>(std::forward<Container>(src), op);
}

template <typename Container, typename dummy_arg>
Container&& attach_operation(Container&& src, operations::identity<dummy_arg>)
{
   return std::forward<Container>(src);
}

template <typename Container, typename Class, typename Member>
auto attach_member_accessor(Container&& c, Member Class::*ptr)
{
   return TransformedContainer<Container, operations::var_member<Class&, Member>>(std::forward<Container>(c), ptr);
}

template <typename Container, typename Class, typename Member, Member Class::*Ptr>
auto attach_member_accessor(Container&& c, ptr2type<Class, Member, Ptr>)
{
   return TransformedContainer<Container, operations::member<Class, Member, Ptr>>(std::forward<Container>(c));
}

/* --------------------------
 *  TransformedContainerPair
 * -------------------------- */

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
struct TransformedContainerPair_helper1 {
   typedef effectively_const_t<ContainerRef1> container1_ref;
   typedef effectively_const_t<ContainerRef2> container2_ref;
   typedef typename deref<container1_ref>::minus_ref container1;
   typedef typename deref<container2_ref>::minus_ref container2;

   typedef typename binary_op_builder<Operation, void, void,
                                      typename container_traits<container1>::const_reference,
                                      typename container_traits<container2>::const_reference>::operation
      element_operation;

   static const bool
      partially_defined = operations::is_partially_defined<element_operation>::value,
      sparse1 = check_container_feature<pure_type_t<container1>, sparse>::value,
      sparse2 = check_container_feature<pure_type_t<container2>, sparse>::value,
      constant1 = object_classifier::what_is<pure_type_t<container1>>::value == object_classifier::is_constant,
      constant2 = object_classifier::what_is<pure_type_t<container2>>::value == object_classifier::is_constant,
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
   using controller = std::conditional_t<partially_defined, set_union_zipper, set_intersection_zipper>;
   using operation = std::conditional_t<partially_defined,
                                        pair<Operation, BuildBinaryIt<operations::zipper_index>>,
                                        Operation>;

   using params2 = mlist< IteratorCouplerTag< sparse_coupler<controller> >,
                          IteratorConstructorTag< binary_transform_constructor< BijectiveTag<std::false_type>,
                                                                                PartiallyDefinedTag<bool_constant<partially_defined>> > >,
                          OperationTag<operation> >;
};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
struct TransformedContainerPair_helper
   : TransformedContainerPair_helper1<ContainerRef1, ContainerRef2, Operation> {
   using base_t = TransformedContainerPair_helper1<ContainerRef1, ContainerRef2, Operation> ;
   using first_type = std::conditional_t<!base_t::sparse1 && !base_t::constant1 && base_t::sparse2,
                                         masquerade_add_features<typename base_t::container1_ref, sparse_compatible>, typename base_t::container1_ref>;
   using second_type = std::conditional_t<!base_t::sparse2 && !base_t::constant2 && base_t::sparse1,
                                          masquerade_add_features<typename base_t::container2_ref, sparse_compatible>, typename base_t::container2_ref>;

   typedef TransformedContainerPair_helper2<Operation, base_t::use_sparse_coupler, base_t::partially_defined> helper2;
   typedef typename mlist_concat< Container1RefTag<first_type>, Container2RefTag<second_type>, typename helper2::params2>::type params;
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
   : public TransformedContainerPair_helper<ContainerRef1, ContainerRef2, Operation>::base
   , public modified_container_pair_impl< TransformedContainerPair<ContainerRef1, ContainerRef2, Operation>,
               typename TransformedContainerPair_helper<ContainerRef1, ContainerRef2, Operation>::params
            > {
   using impl_t = modified_container_pair_impl<TransformedContainerPair>;
   using helper = TransformedContainerPair_helper<ContainerRef1, ContainerRef2, Operation>;
   using base_t = typename helper::base;
   static const bool simple_operation=std::is_same<Operation, typename helper::operation>::value;
protected:
   Int size_impl(std::false_type, std::false_type) const { return impl_t::size(); }
   Int size_impl(std::true_type, std::false_type) const { return this->get_container1().size(); }
   Int size_impl(std::false_type, std::true_type) const { return this->get_container2().size(); }
public:
   TransformedContainerPair() = default;

   template <typename Arg1, typename Arg2,
             typename=std::enable_if_t<std::is_constructible<typename base_t::first_alias_t, Arg1>::value &&
                                       std::is_constructible<typename base_t::second_alias_t, Arg2>::value>>
   TransformedContainerPair(Arg1&& src1_arg, Arg2&& src2_arg,
                            const typename helper::op_arg_type& op_arg=typename helper::op_arg_type())
      : base_t(std::forward<Arg1>(src1_arg), std::forward<Arg2>(src2_arg),
               helper::get_operation(op_arg, bool_constant<simple_operation>())) { }

   using base_t::get_operation;

   Int size() const
   {
      return size_impl(bool_constant<helper::use_sparse_coupler && (helper::partially_defined ? !helper::sparse1 : !helper::sparse2)>(),
                       bool_constant<helper::use_sparse_coupler && (helper::partially_defined ? !helper::sparse2 : !helper::sparse1)>());
   }
};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
struct check_container_feature<TransformedContainerPair<ContainerRef1, ContainerRef2, Operation>, sparse>
   : bool_constant<TransformedContainerPair_helper<ContainerRef1, ContainerRef2, Operation>::sparse_result> {};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
struct check_container_feature<TransformedContainerPair<ContainerRef1, ContainerRef2, Operation>, sparse_compatible>
   : mlist_or< check_container_ref_feature<ContainerRef1, sparse_compatible>,
               check_container_ref_feature<ContainerRef2, sparse_compatible> > {};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
struct spec_object_traits< TransformedContainerPair<ContainerRef1, ContainerRef2, Operation> >
   : spec_object_traits<is_container> {
   static constexpr bool
      is_temporary = true,
      is_always_const = std::is_same<typename TransformedContainerPair<ContainerRef1, ContainerRef2, Operation>::reference,
                                     typename TransformedContainerPair<ContainerRef1, ContainerRef2, Operation>::const_reference>::value;
};

template <typename Container1, typename Container2, typename Operation>
auto attach_operation(Container1&& c1, Container2&& c2, Operation op)
{
   return TransformedContainerPair<Container1, Container2, Operation>(std::forward<Container1>(c1), std::forward<Container2>(c2), op);
}

template <typename Container, typename Scalar>
auto translate(Container&& c, Scalar&& x)
{
   return TransformedContainerPair<add_const_t<Container>, same_value_container<Scalar>, BuildBinary<operations::add>>
      (std::forward<Container>(c), same_value_container<Scalar>(std::forward<Scalar>(x)));
}

template <typename Container, typename Scalar>
auto scale(Container&& c, Scalar&& x)
{
   return TransformedContainerPair<add_const_t<Container>, same_value_container<Scalar>, BuildBinary<operations::mul>>
      (std::forward<Container>(c), same_value_container<Scalar>(std::forward<Scalar>(x)));
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

   iterator_product() = default;

   iterator_product(const iterator& it)
      : first_type(static_cast<const typename iterator::first_type&>(it))
      , second(it.second) {}

   template <typename SourceIterator1, typename SourceIterator2,
             typename suitable1=typename suitable_arg_for_iterator<SourceIterator1, Iterator1>::type,
             typename suitable2=typename suitable_arg_for_iterator<SourceIterator2, Iterator2>::type>
   iterator_product(const SourceIterator1& first_arg, const SourceIterator2& second_arg)
      : first_type(prepare_iterator_arg<Iterator1>(first_arg))
      , second(prepare_iterator_arg<Iterator2>(second_arg)) {}

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
   std::enable_if_t<is_derived_from_any<Other, iterator, const_iterator>::value, bool>
   operator== (const Other& it) const
   {
      return static_cast<const first_type&>(*this)==static_cast<const typename Other::first_type&>(it)  &&  second==it.second;
   }
   template <typename Other>
   std::enable_if_t<is_derived_from_any<Other, iterator, const_iterator>::value, bool>
   operator!= (const Other& it) const
   {
      return !operator==(it);
   }

private:
   void operator--() = delete;
   void operator+=(Int) = delete;
   void operator-=(Int) = delete;
   void operator+(Int) = delete;
   void operator-(Int) = delete;
   void operator[](Int) = delete;

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

   iterator_product() = default;

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
   typedef Int result_type;

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

   product_index_accessor(Int dim2_arg = 0) : dim2(dim2_arg) { }
protected:
   Int dim2;
};


template <typename Top, typename Params>
class container_product_typebase : public manip_container_top<Top, Params> {
   typedef manip_container_top<Top, Params> base_t;
public:
   using container1_ref_raw = typename extract_container_ref<Params, Container1RefTag, Container1Tag>::type;
   using container2_ref_raw = typename extract_container_ref<Params, Container2RefTag, Container2Tag>::type;
   typedef effectively_const_t<container1_ref_raw> container1_ref;
   typedef effectively_const_t<container2_ref_raw> container2_ref;
   typedef typename deref<container1_ref>::minus_ref container1;
   typedef typename deref<container2_ref>::minus_ref container2;

   typedef typename container_traits<container1>::iterator iterator1;
   typedef typename container_traits<container2>::iterator iterator2;
   typedef typename mtagged_list_extract<Params, OperationTag, operations::apply1< BuildUnaryIt<operations::dereference> > >::type
      value_operation;

   typedef typename binary_op_builder<value_operation, iterator1, iterator2>::operation value_operation_instance;
   static const bool
      sparse1=check_container_feature<pure_type_t<container1>, sparse>::value,
      sparse2=check_container_feature<pure_type_t<container2>, sparse>::value,
      partially_defined = (sparse1 || sparse2) && operations::is_partially_defined<value_operation_instance>::value,
      sparse_result = partially_defined ? sparse1 && sparse2 : sparse1 || sparse2,
      need_index= sparse_result || mlist_contains<typename base_t::expected_features, indexed, absorbing_feature>::value;

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

   typedef typename mtagged_list_extract<Params, IteratorConstructorTag, binary_transform_constructor<> >::type it_constructor;

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
   typedef typename mix_features<needed_features1, mlist<end_sensitive, rewindable>>::type
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
   typedef typename mix_features<typename enforce_feature_helper<prep_container1>::must_enforce_features,
                                 typename enforce_feature_helper<prep_container2>::must_enforce_features>::type
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

template <typename Top, typename Params>
class reverse_container_product_typebase {
   typedef container_product_typebase<Top, Params> base_t;
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

template <typename Top, typename Params=typename Top::manipulator_params,
          typename Category=typename container_product_typebase<Top, Params>::container_category>
class container_product_impl
   : public container_product_typebase<Top, Params>
   , public modified_container_non_bijective_elem_access<Top, is_derived_from<Category, bidirectional_iterator_tag>::value>
{
   using base_t = container_product_typebase<Top, Params>;
public:
   typedef container_product_impl<Top, Params> manipulator_impl;
   typedef Params manipulator_params;
   typedef typename base_t::iterator iterator;
   typedef typename base_t::const_iterator const_iterator;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef container_product_impl<FeatureCollector, Params> type;
   };

   iterator begin()
   {
      auto&& c1=this->manip_top().get_container1();
      auto&& c2=this->manip_top().get_container2();
      return iterator(c2.empty() ? ensure(base_t::prep1(c1), typename base_t::needed_features1()).end()
                                 : ensure(base_t::prep1(c1), typename base_t::needed_features1()).begin(),
                      ensure(base_t::prep2(c2), typename base_t::needed_features2()).begin(),
                      create_operation());
   }
   iterator end()
   {
      auto&& c1=this->manip_top().get_container1();
      auto&& c2=this->manip_top().get_container2();
      return iterator(ensure(base_t::prep1(c1), typename base_t::needed_features1()).end(),
                      ensure(base_t::prep2(c2), typename base_t::needed_features2()).begin(),
                      create_operation());
   }
   const_iterator begin() const
   {
      const typename base_t::container1& c1=this->manip_top().get_container1();
      const typename base_t::container2& c2=this->manip_top().get_container2();
      return const_iterator(c2.empty() ? ensure(base_t::prep1(c1), typename base_t::needed_features1()).end()
                                       : ensure(base_t::prep1(c1), typename base_t::needed_features1()).begin(),
                            ensure(base_t::prep2(c2), typename base_t::needed_features2()).begin(),
                            create_operation());
   }
   const_iterator end() const
   {
      return const_iterator(ensure(base_t::prep1(this->manip_top().get_container1()), typename base_t::needed_features1()).end(),
                            ensure(base_t::prep2(this->manip_top().get_container2()), typename base_t::needed_features2()).begin(),
                            create_operation());
   }
private:
   template <typename discr2>
   Int size_impl(std::false_type, discr2) const
   {
      return this->manip_top().get_container1().size() * this->manip_top().get_container2().size();
   }
   Int size_impl(std::true_type, std::false_type) const
   {
      return dim();
   }
   Int size_impl(std::true_type, std::true_type) const
   {
      const Int d1 = get_dim(this->manip_top().get_container1()), d2=get_dim(this->manip_top().get_container2());
      return d1*d2 - (d1-this->manip_top().get_container1().size())*(d2-this->manip_top().get_container2().size());
   }

public:
   Int size() const
   {
      return size_impl(bool_constant<base_t::partially_defined>(), bool_constant<base_t::sparse1 && base_t::sparse2>());
   }
   Int dim() const
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

template <typename Top, typename Params>
class container_product_impl<Top, Params, bidirectional_iterator_tag>
   : public container_product_impl<Top, Params, forward_iterator_tag>
   , public reverse_container_product_typebase<Top, Params> {
   typedef container_product_impl<Top, Params, forward_iterator_tag> base_t;
   typedef reverse_container_product_typebase<Top, Params> rbase_t;
public:
   typename rbase_t::reverse_iterator rbegin()
   {
      auto&& c1=this->manip_top().get_container1();
      auto&& c2=this->manip_top().get_container2();
      return typename rbase_t::reverse_iterator(c2.empty() ? ensure(base_t::prep1(c1), typename base_t::needed_features1()).rend()
                                                           : ensure(base_t::prep1(c1), typename base_t::needed_features1()).rbegin(),
                                                ensure(base_t::prep2(c2), typename base_t::needed_features2()).rbegin(),
                                                this->create_operation());
   }
   typename rbase_t::reverse_iterator rend()
   {
      auto&& c1=this->manip_top().get_container1();
      auto&& c2=this->manip_top().get_container2();
      return typename rbase_t::reverse_iterator(ensure(base_t::prep1(c1), typename base_t::needed_features1()).rend(),
                                                ensure(base_t::prep2(c2), typename base_t::needed_features2()).rbegin(),
                                                this->create_operation());
   }
   typename rbase_t::const_reverse_iterator rbegin() const
   {
      const typename base_t::container1& c1=this->manip_top().get_container1();
      const typename base_t::container2& c2=this->manip_top().get_container2();
      return typename rbase_t::const_reverse_iterator(c2.empty() ? ensure(base_t::prep1(c1), typename base_t::needed_features1()).rend()
                                                                 : ensure(base_t::prep1(c1), typename base_t::needed_features1()).rbegin(),
                                                      ensure(base_t::prep2(c2), typename base_t::needed_features2()).rbegin(),
                                                      this->create_operation());
   }
   typename rbase_t::const_reverse_iterator rend() const
   {
      return typename rbase_t::const_reverse_iterator(ensure(base_t::prep1(this->manip_top().get_container1()), typename base_t::needed_features1()).rend(),
                                                      ensure(base_t::prep2(this->manip_top().get_container2()), typename base_t::needed_features2()).rbegin(),
                                                      this->create_operation());
   }
};

template <typename Top, typename Params>
class container_product_impl<Top, Params, random_access_iterator_tag>
   : public container_product_impl<Top, Params, bidirectional_iterator_tag> {
   typedef container_product_impl<Top, Params, bidirectional_iterator_tag> base_t;
public:
   // TO BE DONE LATER
};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
class ContainerProduct
   : public modified_container_pair_base<ContainerRef1, ContainerRef2, Operation>
   , public container_product_impl< ContainerProduct<ContainerRef1, ContainerRef2, Operation>,
                                    mlist< Container1RefTag<ContainerRef1>,
                                           Container2RefTag<ContainerRef2>,
                                           OperationTag<Operation> > > {
   using base_t = modified_container_pair_base<ContainerRef1, ContainerRef2, Operation>;
public:
   using modified_container_pair_base<ContainerRef1, ContainerRef2, Operation>::modified_container_pair_base;
   using base_t::get_operation;
};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
struct check_container_feature<ContainerProduct<ContainerRef1, ContainerRef2, Operation>, sparse>
   : bool_constant<ContainerProduct<ContainerRef1, ContainerRef2, Operation>::sparse_result> {};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
struct spec_object_traits< ContainerProduct<ContainerRef1, ContainerRef2, Operation> >
   : spec_object_traits<is_container> {
   static constexpr bool
      is_temporary = true,
      is_always_const = std::is_same<typename ContainerProduct<ContainerRef1, ContainerRef2, Operation>::reference,
                                     typename ContainerProduct<ContainerRef1, ContainerRef2, Operation>::const_reference>::value;
};

template <typename Container1, typename Container2, typename Operation>
auto product(Container1&& c1, Container2&& c2, const Operation& op)
{
   return ContainerProduct<Container1, Container2, Operation>(std::forward<Container1>(c1), std::forward<Container2>(c2), op);
}

template <typename ContainerRef>
struct repeated_container {
   using type = ContainerProduct<const count_down, add_const_t<ContainerRef>, operations::apply2< BuildUnaryIt<operations::dereference> > >;
   using params = typename type::manipulator_params;
};

template <typename Container>
auto repeat(Container&& c, Int n)
{
   return typename repeated_container<Container>::type(count_down(n), std::forward<Container>(c));
}

} // end namespace pm

namespace polymake {
   using pm::attach_operation;
   using pm::attach_member_accessor;
   using pm::translate;
   using pm::scale;
   using pm::product;
   using pm::repeat;
}

#endif // POLYMAKE_TRANSFORMED_CONTAINER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
