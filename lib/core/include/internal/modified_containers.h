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

#ifndef POLYMAKE_INTERNAL_MODIFIED_CONTAINERS_H
#define POLYMAKE_INTERNAL_MODIFIED_CONTAINERS_H

#include "polymake/internal/constant_containers.h"
#include "polymake/Series.h"

namespace pm {

template <typename IteratorConstructor,
          bool maybe=(is_derived_from_instance_of<IteratorConstructor, unary_transform_constructor>::value ||
                      is_derived_from_instance_of<IteratorConstructor, binary_transform_constructor>::value)>
struct is_bijective : std::false_type {};

template <typename Operation>
struct is_identity_transform : std::false_type {
   typedef Operation type;
};

template <typename IteratorConstructor>
struct is_bijective<IteratorConstructor, true>
   : mtagged_list_extract<typename IteratorConstructor::params, BijectiveTag, std::true_type>::type {};

template <typename Operation>
struct is_identity_transform< pair<nothing, Operation> > : std::true_type {
   typedef Operation type;
};

template <typename Top, typename TParams>
class redirected_container_typebase : public manip_container_top<Top, TParams> {
   typedef manip_container_top<Top, TParams> base_t;
public:
   typedef typename mtagged_list_extract<TParams, ContainerTag, typename base_t::hidden_type>::type container_ref;
   typedef typename deref<container_ref>::minus_ref container;
   typedef typename temp_ref<container_ref>::type container_temp_ref;
   typedef typename base_t::expected_features needed_features;
   typedef typename ensure_features<container,needed_features>::iterator iterator;
   typedef typename ensure_features<container,needed_features>::const_iterator const_iterator;
   typedef typename enforce_feature_helper<container>::must_enforce_features must_enforce_features;
   typedef typename enforce_feature_helper<container>::can_enforce_features can_enforce_features;
   typedef typename enforce_feature_helper<container>::cannot_enforce_features cannot_enforce_features;
   typedef typename container_traits<container>::category container_category;
   typedef typename container_traits<container>::value_type value_type;
   typedef typename container_traits<container>::reference reference;
   typedef typename container_traits<container>::const_reference const_reference;
   static const int is_resizeable=object_traits<typename deref<container>::type>::is_resizeable;
};

template <typename Top, typename Params,
          bool _enable=redirected_container_typebase<Top,Params>::is_resizeable==1>
class redirected_container_resize {};

template <typename Top, typename Params=typename Top::manipulator_params,
          typename Category=typename redirected_container_typebase<Top,Params>::container_category>
class redirected_container 
   : public redirected_container_typebase<Top,Params>,
     public redirected_container_resize<Top,Params> {
   typedef redirected_container_typebase<Top,Params> _super;
public:
   typedef redirected_container<Top,Params> manipulator_impl;
   typedef Params manipulator_params;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef redirected_container<FeatureCollector,Params> type;
   };

   typename _super::iterator begin()
   {
      typename _super::container_temp_ref c=this->manip_top().get_container();
      return ensure(c, (typename _super::needed_features*)0).begin();
   }
   typename _super::iterator end()
   {
      typename _super::container_temp_ref c=this->manip_top().get_container();
      return ensure(c, (typename _super::needed_features*)0).end();
   }
   typename _super::const_iterator begin() const
   {
      return ensure(this->manip_top().get_container(), (typename _super::needed_features*)0).begin();
   }
   typename _super::const_iterator end() const
   {
      return ensure(this->manip_top().get_container(), (typename _super::needed_features*)0).end();
   }

   int size() const { return this->manip_top().get_container().size(); }
   bool empty() const { return this->manip_top().get_container().empty(); }
};

template <typename Top, typename Params>
class redirected_container<Top, Params, forward_iterator_tag>
   : public redirected_container<Top, Params, input_iterator_tag> {
   typedef redirected_container<Top, Params, input_iterator_tag> _super;
public:
   typename _super::reference front()
   {
      return this->manip_top().get_container().front();
   }
   typename _super::const_reference front() const
   {
      return this->manip_top().get_container().front();
   }
};

template <typename Top, typename Params>
class redirected_container<Top, Params, bidirectional_iterator_tag>
   : public redirected_container<Top, Params, forward_iterator_tag> {
   typedef redirected_container<Top, Params, forward_iterator_tag> _super;
public:
   typedef typename ensure_features<typename _super::container, typename _super::needed_features>::reverse_iterator
      reverse_iterator;
   typedef typename ensure_features<typename _super::container, typename _super::needed_features>::const_reverse_iterator
      const_reverse_iterator;

   reverse_iterator rbegin()
   {
      typename _super::container_temp_ref c=this->manip_top().get_container();
      return ensure(c, (typename _super::needed_features*)0).rbegin();
   }
   reverse_iterator rend()
   {
      typename _super::container_temp_ref c=this->manip_top().get_container();
      return ensure(c, (typename _super::needed_features*)0).rend();
   }
   const_reverse_iterator rbegin() const
   {
      return ensure(this->manip_top().get_container(), (typename _super::needed_features*)0).rbegin();
   }
   const_reverse_iterator rend() const
   {
      return ensure(this->manip_top().get_container(), (typename _super::needed_features*)0).rend();
   }

   typename _super::reference back()
   {
      return this->manip_top().get_container().back();
   }
   typename _super::const_reference back() const
   {
      return this->manip_top().get_container().back();
   }
};

template <typename Top, typename Params>
class redirected_container<Top, Params, random_access_iterator_tag>
   : public redirected_container<Top, Params, bidirectional_iterator_tag> {
   typedef redirected_container<Top, Params, bidirectional_iterator_tag> _super;
public:
   typename _super::reference operator[] (int i)
   {
      return this->manip_top().get_container()[i];
   }
   typename _super::const_reference operator[] (int i) const
   {
      return this->manip_top().get_container()[i];
   }
};

template <typename Top, typename Params>
class redirected_container_resize<Top, Params, true> {
   typedef redirected_container<Top, Params> master;
public:
   void resize(int n)
   {
      static_cast<master*>(this)->manip_top().get_container().resize(n);
   }
};

template <typename Top, typename Typebase, bool _reversible>
class modified_container_non_bijective_elem_access {
public:
   typename Typebase::reference front()
   {
      return *static_cast<Top&>(*this).begin();
   }
   typename Typebase::const_reference front() const
   {
      return *static_cast<const Top&>(*this).begin();
   }

   int size() const
   {
      return count_it(static_cast<const Top&>(*this).begin());
   }
   bool empty() const
   {
      return static_cast<const Top&>(*this).begin().at_end();
   }
};

template <typename Top, typename Typebase>
class modified_container_non_bijective_elem_access<Top, Typebase, true>
   : public modified_container_non_bijective_elem_access<Top, Typebase, false> {
public:
   typename Typebase::reference back()
   {
      return *static_cast<Top&>(*this).rbegin();
   }
   typename Typebase::const_reference back() const
   {
      return *static_cast<const Top&>(*this).rbegin();
   }
};

template <typename Top, typename TParams>
class modified_container_typebase
   : public manip_container_top<Top, TParams> {
   typedef manip_container_top<Top, TParams> base_t;
public:
   typedef typename mtagged_list_extract<TParams, ContainerTag, typename base_t::hidden_type>::type container_ref;
   typedef typename deref<container_ref>::minus_ref container;
   typedef typename temp_ref<container_ref>::type container_temp_ref;
   typedef typename mtagged_list_extract<TParams, OperationTag>::type operation;
   typedef typename operation_cross_const_helper<operation>::const_operation const_operation;
   typedef typename mtagged_list_extract<TParams, IteratorConstructorTag, unary_transform_constructor<> >::type it_constructor;

   typedef typename it_constructor::template defs<typename container_traits<container>::iterator,
                                                  operation, typename base_t::expected_features>::needed_features
      needed_features;
   typedef typename it_constructor::template defs<typename ensure_features<container, needed_features>::iterator,
                                                  operation, typename base_t::expected_features>::iterator
      iterator;
   typedef typename it_constructor::template defs<typename ensure_features<container, needed_features>::const_iterator,
                                                  const_operation, typename base_t::expected_features>::iterator
      const_iterator;

   typedef typename least_derived_class<typename std::conditional<is_bijective<it_constructor>::value,
                                                                  random_access_iterator_tag,
                                                                  bidirectional_iterator_tag>::type,
                                        typename container_traits<container>::category>::type
      container_category;

   typedef typename enforce_feature_helper<typename deref<container>::type>::must_enforce_features must_enforce_features;

   typedef typename iterator_traits<iterator>::value_type value_type;
   typedef typename iterator_traits<iterator>::reference reference;
   typedef typename iterator_traits<const_iterator>::reference const_reference;
};

template <typename Top, typename TParams>
class reverse_modified_container_typebase {
   typedef modified_container_typebase<Top, TParams> base_t;
public:
   typedef typename base_t::it_constructor::template defs<
      typename ensure_features<typename base_t::container, typename base_t::needed_features>::reverse_iterator,
      typename base_t::operation, typename base_t::expected_features
   >::iterator reverse_iterator;
   typedef typename base_t::it_constructor::template defs<
      typename ensure_features<typename base_t::container, typename base_t::needed_features>::const_reverse_iterator,
      typename base_t::const_operation, typename base_t::expected_features
   >::iterator const_reverse_iterator;
};

template <typename Top, typename TParams,
          typename Category=typename modified_container_typebase<Top, TParams>::container_category,
          bool TBijective=is_bijective<typename modified_container_typebase<Top, TParams>::it_constructor>::value,
          bool TIdentity=is_identity_transform<typename modified_container_typebase<Top, TParams>::operation>::value>
class modified_container_elem_access;

template <typename Top, typename TParams=typename Top::manipulator_params,
          bool TReversible=is_derived_from<typename modified_container_typebase<Top, TParams>::container_category,
                                           bidirectional_iterator_tag>::value>
class modified_container_impl
   : public modified_container_typebase<Top, TParams>,
     public modified_container_elem_access<Top, TParams> {
   typedef modified_container_typebase<Top, TParams> base_t;
public:
   typedef modified_container_impl<Top, TParams> manipulator_impl;
   typedef TParams manipulator_params;
   typedef typename base_t::iterator iterator;
   typedef typename base_t::const_iterator const_iterator;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef modified_container_impl<FeatureCollector, TParams> type;
   };

   typename is_identity_transform<typename base_t::operation>::type get_operation() const
   {
      return typename is_identity_transform<typename base_t::operation>::type();
   }

   iterator begin()
   {
      typename base_t::container_temp_ref c=this->manip_top().get_container();
      return iterator(ensure(c, (typename base_t::needed_features*)0).begin(), this->manip_top().get_operation());
   }
   iterator end()
   {
      typename base_t::container_temp_ref c=this->manip_top().get_container();
      return iterator(ensure(c, (typename base_t::needed_features*)0).end(), this->manip_top().get_operation());
   }
   const_iterator begin() const
   {
      return const_iterator(ensure(this->manip_top().get_container(), (typename base_t::needed_features*)0).begin(),
                            this->manip_top().get_operation());
   }
   const_iterator end() const
   {
      return const_iterator(ensure(this->manip_top().get_container(), (typename base_t::needed_features*)0).end(),
                            this->manip_top().get_operation());
   }
};

template <typename Top, typename TParams>
class modified_container_impl<Top, TParams, true>
   : public modified_container_impl<Top, TParams, false>,
     public reverse_modified_container_typebase<Top, TParams> {
   typedef modified_container_impl<Top, TParams, false> base_t;
   typedef reverse_modified_container_typebase<Top, TParams> rbase_t;
public:
   typename rbase_t::reverse_iterator rbegin()
   {
      typename base_t::container_temp_ref c=this->manip_top().get_container();
      return typename rbase_t::reverse_iterator(ensure(c, (typename base_t::needed_features*)0).rbegin(),
                                                this->manip_top().get_operation());
   }
   typename rbase_t::reverse_iterator rend()
   {
      typename base_t::container_temp_ref c=this->manip_top().get_container();
      return typename rbase_t::reverse_iterator(ensure(c, (typename base_t::needed_features*)0).rend(),
                                                this->manip_top().get_operation());
   }
   typename rbase_t::const_reverse_iterator rbegin() const
   {
      return typename rbase_t::const_reverse_iterator(ensure(this->manip_top().get_container(),
                                                             (typename base_t::needed_features*)0).rbegin(),
                                                      this->manip_top().get_operation());
   }
   typename rbase_t::const_reverse_iterator rend() const
   {
      return typename rbase_t::const_reverse_iterator(ensure(this->manip_top().get_container(),
                                                             (typename base_t::needed_features*)0).rend(),
                                                      this->manip_top().get_operation());
   }
};

template <typename Top, typename TParams, typename Category, bool TBijective, bool TIdentity>
class modified_container_elem_access {
   typedef modified_container_typebase<Top, TParams> base_t;
protected:
   typename base_t::manip_top_type& _top()
   {
      return static_cast<modified_container_impl<Top, TParams>*>(this)->manip_top();
   }
   const typename base_t::manip_top_type& _top() const
   {
      return static_cast<const modified_container_impl<Top, TParams>*>(this)->manip_top();
   }
public:
   int size() const
   {
      return _top().get_container().size();
   }
   int dim() const
   {
      return get_dim(_top().get_container());
   }
   bool empty() const
   {
      return _top().get_container().empty();
   }
};

template <typename Top, typename TParams>
class modified_container_elem_access<Top, TParams, forward_iterator_tag, true, false>
   : public modified_container_elem_access<Top, TParams, input_iterator_tag, true, false> {
   typedef modified_container_typebase<Top, TParams> base_t;

   typename base_t::reference front_impl(const typename base_t::iterator::operation& op, std::true_type)
   {
      return op(this->_top().get_container().front());
   }
   typename base_t::const_reference front_impl(const typename base_t::const_iterator::operation& op, std::true_type) const
   {
      return op(this->_top().get_container().front());
   }
   typename base_t::reference front_impl(const typename base_t::iterator::operation& op, std::false_type)
   {
      return op(this->_top().get_container().begin());
   }
   typename base_t::const_reference front_impl(const typename base_t::const_iterator::operation& op, std::false_type) const
   {
      return op(this->_top().get_container().begin());
   }
public:
   typename base_t::reference front()
   {
      typedef typename base_t::iterator::helper opb;
      return front_impl(opb::create(this->_top().get_operation()), bool_constant<opb::data_arg>());
   }
   typename base_t::const_reference front() const
   {
      typedef typename base_t::const_iterator::helper opb;
      return front_impl(opb::create(this->_top().get_operation()), bool_constant<opb::data_arg>());
   }
};

template <typename Top, typename TParams>
class modified_container_elem_access<Top, TParams, forward_iterator_tag, true, true>
   : public modified_container_elem_access<Top, TParams, input_iterator_tag, true, true> {
   typedef modified_container_typebase<Top, TParams> base_t;
public:
   typename base_t::reference front()
   {
      return this->_top().get_container().front();
   }
   typename base_t::const_reference front() const
   {
      return this->_top().get_container().front();
   }
};

template <typename Top, typename TParams>
class modified_container_elem_access<Top, TParams, bidirectional_iterator_tag, true, false>
   : public modified_container_elem_access<Top, TParams, forward_iterator_tag, true, false> {
   typedef modified_container_typebase<Top, TParams> base_t;
   typedef reverse_modified_container_typebase<Top, TParams> rbase_t;

   typename base_t::reference back_impl(const typename rbase_t::reverse_iterator::operation& op, std::true_type)
   {
      return op(this->_top().get_container().back());
   }
   typename base_t::const_reference back_impl(const typename rbase_t::const_reverse_iterator::operation& op, std::true_type) const
   {
      return op(this->_top().get_container().back());
   }
   typename base_t::reference back_impl(const typename rbase_t::reverse_iterator::operation& op, std::false_type)
   {
      return op(this->_top().get_container().rbegin());
   }
   typename base_t::const_reference back_impl(const typename rbase_t::const_reverse_iterator::operation& op, std::false_type) const
   {
      return op(this->_top().get_container().rbegin());
   }
public:
   typename base_t::reference back()
   {
      typedef typename rbase_t::reverse_iterator::helper opb;
      return back_impl(opb::create(this->_top().get_operation()), bool_constant<opb::data_arg>());
   }
   typename base_t::const_reference back() const
   {
      typedef typename rbase_t::const_reverse_iterator::helper opb;
      return back_impl(opb::create(this->_top().get_operation()), bool_constant<opb::data_arg>());
   }
};

template <typename Top, typename TParams>
class modified_container_elem_access<Top, TParams, bidirectional_iterator_tag, true, true>
   : public modified_container_elem_access<Top, TParams, forward_iterator_tag, true, true> {
   typedef modified_container_typebase<Top, TParams> base_t;
public:
   typename base_t::reference back()
   {
      return this->_top().get_container().back();
   }
   typename base_t::const_reference back() const
   {
      return this->_top().get_container().back();
   }
};

template <typename Top, typename TParams>
class modified_container_elem_access<Top, TParams, random_access_iterator_tag, true, false>
   : public modified_container_elem_access<Top, TParams, bidirectional_iterator_tag, true, false> {
   typedef modified_container_typebase<Top, TParams> base_t;

   typename base_t::reference random_impl(int i, const typename base_t::iterator::operation& op, std::true_type)
   {
      return op(this->_top().get_container()[i]);
   }
   typename base_t::const_reference random_impl(int i, const typename base_t::const_iterator::operation& op, std::true_type) const
   {
      return op(this->_top().get_container()[i]);
   }
   typename base_t::reference random_impl(int i, const typename base_t::iterator::operation& op, std::false_type)
   {
      return op(this->_top().get_container().begin() + i);
   }
   typename base_t::const_reference random_impl(int i, const typename base_t::const_iterator::operation& op, std::false_type) const
   {
      return op(this->_top().get_container().begin() + i);
   }
public:
   typename base_t::reference operator[] (int i)
   {
      typedef typename base_t::iterator::helper opb;
      const bool via_container=opb::data_arg || !iterator_traits<typename base_t::container::iterator>::is_random;
      return random_impl(i, opb::create(this->_top().get_operation()), bool_constant<via_container>());
   }
   typename base_t::const_reference operator[] (int i) const
   {
      typedef typename base_t::const_iterator::helper opb;
      const bool via_container=opb::data_arg || !iterator_traits<typename base_t::container::const_iterator>::is_random;
      return random_impl(i, opb::create(this->_top().get_operation()), bool_constant<via_container>());
   }
};

template <typename Top, typename TParams>
class modified_container_elem_access<Top, TParams, random_access_iterator_tag, true, true>
   : public modified_container_elem_access<Top, TParams, bidirectional_iterator_tag, true, true> {
   typedef modified_container_typebase<Top, TParams> base_t;
public:
   typename base_t::reference operator[] (int i)
   {
      return this->_top().get_container()[i];
   }
   typename base_t::const_reference operator[] (int i) const
   {
      return this->_top().get_container()[i];
   }
};

template <typename Top, typename TParams, typename Category>
class modified_container_elem_access<Top, TParams, Category, false, false>
   : public modified_container_non_bijective_elem_access<Top, modified_container_typebase<Top, TParams>,
                                                         is_derived_from<Category, bidirectional_iterator_tag>::value> {};

template <typename Top, typename TParams>
class container_pair_typebase : public manip_container_top<Top, TParams> {
   typedef manip_container_top<Top, TParams> base_t;
public:
   typedef typename mtagged_list_extract<TParams, Container1Tag>::type container1_ref;
   typedef typename mtagged_list_extract<TParams, Container2Tag>::type container2_ref;
   typedef typename deref<container1_ref>::minus_ref container1;
   typedef typename deref<container2_ref>::minus_ref container2;
   typedef typename temp_ref<container1_ref>::type container1_temp_ref;
   typedef typename temp_ref<container2_ref>::type container2_temp_ref;

   typedef typename mtagged_list_extract<TParams, IteratorCouplerTag, pair_coupler<> >::type it_coupler;
   typedef typename it_coupler::template defs<typename container_traits<container1>::iterator,
                                              typename container_traits<container2>::iterator,
                                              typename base_t::expected_features>::needed_features1
      needed_features1;
   typedef typename it_coupler::template defs<typename container_traits<container1>::iterator,
                                              typename container_traits<container2>::iterator,
                                              typename base_t::expected_features>::needed_features2
      needed_features2;
   typedef typename it_coupler::template defs<typename ensure_features<container1, needed_features1>::iterator,
                                              typename ensure_features<container2, needed_features2>::iterator,
                                              typename base_t::expected_features>::iterator
      iterator;
   typedef typename it_coupler:: template defs<typename ensure_features<container1, needed_features1>::const_iterator,
                                               typename ensure_features<container2, needed_features2>::const_iterator,
                                               typename base_t::expected_features>::iterator
      const_iterator;

   typedef typename least_derived_class<typename container_traits<container1>::category,
                                        typename container_traits<container2>::category>::type
      container_category;
   typedef typename mix_features<typename enforce_feature_helper<typename deref<container1>::type>::must_enforce_features,
                                 typename enforce_feature_helper<typename deref<container2>::type>::must_enforce_features>::type
      must_enforce_features;

   typedef typename iterator_traits<iterator>::value_type value_type;
   typedef typename iterator_traits<iterator>::reference reference;
   typedef typename iterator_traits<const_iterator>::reference const_reference;
};

template <typename IteratorCoupler>
struct reverse_coupler {
   typedef IteratorCoupler type;
};

template <typename IteratorCoupler, typename TParams, bool TReverse=list_contains<TParams, _reversed>::value>
struct reverse_coupler_helper {
   typedef IteratorCoupler type;
};

template <typename IteratorCoupler, typename TParams>
struct reverse_coupler_helper<IteratorCoupler, TParams, true> : reverse_coupler<IteratorCoupler> {};

template <typename Top, typename TParams>
class reverse_container_pair_typebase {
   typedef container_pair_typebase<Top, TParams> base_t;
   typedef typename reverse_coupler<typename base_t::it_coupler>::type rev_it_coupler;
public:
   typedef typename rev_it_coupler::template defs<typename ensure_features<typename base_t::container1,
                                                                           typename base_t::needed_features1>::reverse_iterator,
                                                  typename ensure_features<typename base_t::container2,
                                                                           typename base_t::needed_features2>::reverse_iterator,
                                                  typename base_t::expected_features>::iterator
      reverse_iterator;
   typedef typename rev_it_coupler::template defs<typename ensure_features<typename base_t::container1,
                                                                           typename base_t::needed_features1>::const_reverse_iterator,
                                                  typename ensure_features<typename base_t::container2,
                                                                           typename base_t::needed_features2>::const_reverse_iterator,
                                                  typename base_t::expected_features>::iterator
      const_reverse_iterator;
};

template <typename Top, typename TParams=typename Top::manipulator_params,
          typename Category=typename container_pair_typebase<Top, TParams>::container_category>
class container_pair_impl
   : public container_pair_typebase<Top, TParams> {
   typedef container_pair_typebase<Top, TParams> base_t;
   int size_impl(std::false_type) const { return this->manip_top().get_container1().size(); }
   int size_impl(std::true_type) const { return this->manip_top().get_container2().size(); }
   int dim_impl(std::false_type) const { return get_dim(this->manip_top().get_container1()); }
   int dim_impl(std::true_type) const { return get_dim(this->manip_top().get_container2()); }
   bool empty_impl(std::false_type) const { return this->manip_top().get_container1().empty(); }
   bool empty_impl(std::true_type) const { return this->manip_top().get_container2().empty(); }

   typedef bool_constant<object_classifier::what_is<typename deref<typename base_t::container1>::type>::value
                         == object_classifier::is_constant> unlimited1;
public:
   typedef container_pair_impl<Top, TParams> manipulator_impl;
   typedef TParams manipulator_params;
   typedef typename base_t::iterator iterator;
   typedef typename base_t::const_iterator const_iterator;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef container_pair_impl<FeatureCollector, TParams> type;
   };

   iterator begin()
   {
      typename base_t::container1_temp_ref c1=this->manip_top().get_container1();
      typename base_t::container2_temp_ref c2=this->manip_top().get_container2();
      return iterator(ensure(c1, (typename base_t::needed_features1*)0).begin(),
                      ensure(c2, (typename base_t::needed_features2*)0).begin());
   }
   iterator end()
   {
      typename base_t::container1_temp_ref c1=this->manip_top().get_container1();
      typename base_t::container2_temp_ref c2=this->manip_top().get_container2();
      return iterator(ensure(c1, (typename base_t::needed_features1*)0).end(),
                      ensure(c2, (typename base_t::needed_features2*)0).end());
   }
   const_iterator begin() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), (typename base_t::needed_features1*)0).begin(),
                            ensure(this->manip_top().get_container2(), (typename base_t::needed_features2*)0).begin());
   }
   const_iterator end() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), (typename base_t::needed_features1*)0).end(),
                            ensure(this->manip_top().get_container2(), (typename base_t::needed_features2*)0).end());
   }

   int size() const { return size_impl(unlimited1()); }
   int dim() const { return dim_impl(unlimited1()); }
   bool empty() const { return empty_impl(unlimited1()); }
};

template <typename Top, typename TParams>
class container_pair_impl<Top, TParams, forward_iterator_tag>
   : public container_pair_impl<Top, TParams, input_iterator_tag> {
   typedef container_pair_impl<Top, TParams, input_iterator_tag> base_t;
public:
   typename base_t::reference front()
   {
      return this->manip_top().get_container1().front();
   }
   typename base_t::const_reference front() const
   {
      return this->manip_top().get_container1().front();
   }
};

template <typename Top, typename TParams>
class container_pair_impl<Top, TParams, bidirectional_iterator_tag>
   : public container_pair_impl<Top, TParams, forward_iterator_tag>,
     public reverse_container_pair_typebase<Top, TParams> {
   typedef container_pair_impl<Top, TParams, forward_iterator_tag> base_t;
   typedef reverse_container_pair_typebase<Top, TParams> rbase_t;
public:
   typename rbase_t::reverse_iterator rbegin()
   {
      typename base_t::container1_temp_ref c1=this->manip_top().get_container1();
      typename base_t::container2_temp_ref c2=this->manip_top().get_container2();
      return typename rbase_t::reverse_iterator(ensure(c1, (typename base_t::needed_features1*)0).rbegin(),
                                                ensure(c2, (typename base_t::needed_features2*)0).rbegin());
   }
   typename rbase_t::reverse_iterator rend()
   {
      typename base_t::container1_temp_ref c1=this->manip_top().get_container1();
      typename base_t::container2_temp_ref c2=this->manip_top().get_container2();
      return typename rbase_t::reverse_iterator(ensure(c1, (typename base_t::needed_features1*)0).rend(),
                                                ensure(c2, (typename base_t::needed_features2*)0).rend());
   }
   typename rbase_t::const_reverse_iterator rbegin() const
   {
      return typename rbase_t::const_reverse_iterator(ensure(this->manip_top().get_container1(),
                                                             (typename base_t::needed_features1*)0).rbegin(),
                                                      ensure(this->manip_top().get_container2(),
                                                             (typename base_t::needed_features2*)0).rbegin());
   }
   typename rbase_t::const_reverse_iterator rend() const
   {
      return typename rbase_t::const_reverse_iterator(ensure(this->manip_top().get_container1(),
                                                             (typename base_t::needed_features1*)0).rend(),
                                                      ensure(this->manip_top().get_container2(),
                                                             (typename base_t::needed_features2*)0).rend());
   }

   typename base_t::reference back()
   {
      return this->manip_top().get_container1().back();
   }
   typename base_t::const_reference back() const
   {
      return this->manip_top().get_container1().back();
   }
};

template <typename Top, typename TParams>
class container_pair_impl<Top, TParams, random_access_iterator_tag>
   : public container_pair_impl<Top, TParams, bidirectional_iterator_tag> {
   typedef container_pair_impl<Top, TParams, bidirectional_iterator_tag> base_t;
public:
   typename base_t::reference operator[] (int i)
   {
      return this->manip_top().get_container1()[i];
   }
   typename base_t::const_reference operator[] (int i) const
   {
      return this->manip_top().get_container1()[i];
   }
};

template <typename Top, typename TParams>
class modified_container_pair_typebase
   : public manip_container_top<Top, TParams> {
   typedef manip_container_top<Top, TParams> base_t;
public:
   typedef typename mtagged_list_extract<TParams, Container1Tag>::type container1_ref;
   typedef typename mtagged_list_extract<TParams, Container2Tag>::type container2_ref;
   typedef typename deref<container1_ref>::minus_ref container1;
   typedef typename deref<container2_ref>::minus_ref container2;
   typedef typename temp_ref<container1_ref>::type container1_temp_ref;
   typedef typename temp_ref<container2_ref>::type container2_temp_ref;

   typedef typename mtagged_list_extract<TParams, OperationTag>::type operation;
   typedef typename operation_cross_const_helper<operation>::const_operation const_operation;
   typedef typename mtagged_list_extract<TParams, IteratorCouplerTag, pair_coupler<> >::type it_coupler;
   typedef typename it_coupler::template defs<typename container_traits<container1>::iterator,
                                              typename container_traits<container2>::iterator, void>
      coupler_defs;
   typedef typename mtagged_list_extract<TParams, IteratorConstructorTag, binary_transform_constructor<> >::type it_constructor;

   typedef typename it_constructor::template defs<typename coupler_defs::iterator, operation, typename base_t::expected_features> first_try_defs;
   typedef typename first_try_defs::needed_pair_features needed_pair_features;

   typedef typename mix_features<typename it_coupler::template defs<typename container_traits<container1>::iterator,
                                                                    typename container_traits<container2>::iterator,
                                                                    needed_pair_features>::needed_features1,
                                 typename first_try_defs::needed_features1>::type
      needed_features1;
   typedef typename mix_features<typename it_coupler::template defs<typename container_traits<container1>::iterator,
                                                                    typename container_traits<container2>::iterator,
                                                                    needed_pair_features>::needed_features2,
                                 typename first_try_defs::needed_features2>::type
      needed_features2;
   typedef typename it_coupler::template defs<typename ensure_features<container1, needed_features1>::iterator,
                                              typename ensure_features<container2, needed_features2>::iterator,
                                              needed_pair_features>::iterator
      it_pair;
   typedef typename it_coupler::template defs<typename ensure_features<container1, needed_features1>::const_iterator,
                                              typename ensure_features<container2, needed_features2>::const_iterator,
                                              needed_pair_features>::iterator
      const_it_pair;
   typedef typename it_constructor::template defs<it_pair, operation, typename base_t::expected_features>::iterator
      iterator;
   typedef typename it_constructor::template defs<const_it_pair, const_operation, typename base_t::expected_features>::iterator
      const_iterator;

   typedef typename least_derived_class< typename std::conditional<is_bijective<it_constructor>::value,
                                                                   random_access_iterator_tag,
                                                                   bidirectional_iterator_tag>::type,
                                         typename container_traits<container1>::category,
                                         typename container_traits<container2>::category >::type
      container_category;

   typedef typename mix_features<typename enforce_feature_helper<typename deref<container1>::type>::must_enforce_features,
                                 typename enforce_feature_helper<typename deref<container2>::type>::must_enforce_features>::type
      must_enforce_features;

   typedef typename iterator_traits<iterator>::value_type value_type;
   typedef typename iterator_traits<iterator>::reference reference;
   typedef typename iterator_traits<const_iterator>::reference const_reference;
};

template <typename Top, typename TParams>
class reverse_modified_container_pair_typebase {
   typedef modified_container_pair_typebase<Top, TParams> base_t;
   typedef typename reverse_coupler<typename base_t::it_coupler>::type rev_it_coupler;
public:
   typedef typename rev_it_coupler::template defs<typename ensure_features<typename base_t::container1,
                                                                           typename base_t::needed_features1>::reverse_iterator,
                                                  typename ensure_features<typename base_t::container2,
                                                                           typename base_t::needed_features2>::reverse_iterator,
                                                  typename base_t::needed_pair_features>::iterator
      reverse_it_pair;
   typedef typename rev_it_coupler::template defs<typename ensure_features<typename base_t::container1,
                                                                           typename base_t::needed_features1>::const_reverse_iterator,
                                                  typename ensure_features<typename base_t::container2,
                                                                           typename base_t::needed_features2>::const_reverse_iterator,
                                                  typename base_t::needed_pair_features>::iterator
      const_reverse_it_pair;
   typedef typename base_t::it_constructor::template defs<reverse_it_pair, typename base_t::operation,
                                                          typename base_t::expected_features>::iterator
      reverse_iterator;
   typedef typename base_t::it_constructor::template defs<const_reverse_it_pair, typename base_t::const_operation,
                                                          typename base_t::expected_features>::iterator
      const_reverse_iterator;
};

template <typename Top, typename TParams,
          typename Category=typename modified_container_pair_typebase<Top, TParams>::container_category,
          bool TBijective=is_bijective<typename modified_container_pair_typebase<Top, TParams>::it_constructor>::value,
          bool TIdentity=is_identity_transform<typename modified_container_pair_typebase<Top, TParams>::operation>::value>
class modified_container_pair_elem_access;

template <typename Top, typename TParams=typename Top::manipulator_params,
          bool TReversible=is_derived_from<typename modified_container_pair_typebase<Top, TParams>::container_category,
                                           bidirectional_iterator_tag>::value>
class modified_container_pair_impl
   : public modified_container_pair_typebase<Top, TParams>,
     public modified_container_pair_elem_access<Top, TParams> {
   typedef modified_container_pair_typebase<Top, TParams> base_t;
public:
   typedef modified_container_pair_impl<Top, TParams> manipulator_impl;
   typedef TParams manipulator_params;
   typedef typename base_t::iterator iterator;
   typedef typename base_t::const_iterator const_iterator;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef modified_container_pair_impl<FeatureCollector, TParams> type;
   };

   typename is_identity_transform<typename base_t::operation>::type get_operation() const
   {
      return typename is_identity_transform<typename base_t::operation>::type();
   }

   iterator begin()
   {
      typename base_t::container1_temp_ref c1=this->manip_top().get_container1();
      typename base_t::container2_temp_ref c2=this->manip_top().get_container2();
      return iterator(ensure(c1, (typename base_t::needed_features1*)0).begin(),
                      ensure(c2, (typename base_t::needed_features2*)0).begin(),
                      this->manip_top().get_operation());
   }
   iterator end()
   {
      typename base_t::container1_temp_ref c1=this->manip_top().get_container1();
      typename base_t::container2_temp_ref c2=this->manip_top().get_container2();
      return iterator(ensure(c1, (typename base_t::needed_features1*)0).end(),
                      ensure(c2, (typename base_t::needed_features2*)0).end(),
                      this->manip_top().get_operation());
   }
   const_iterator begin() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), (typename base_t::needed_features1*)0).begin(),
                            ensure(this->manip_top().get_container2(), (typename base_t::needed_features2*)0).begin(),
                            this->manip_top().get_operation());
   }
   const_iterator end() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), (typename base_t::needed_features1*)0).end(),
                            ensure(this->manip_top().get_container2(), (typename base_t::needed_features2*)0).end(),
                            this->manip_top().get_operation());
   }
};

template <typename Top, typename TParams>
class modified_container_pair_impl<Top, TParams, true>
   : public modified_container_pair_impl<Top, TParams, false>,
     public reverse_modified_container_pair_typebase<Top, TParams> {
   typedef modified_container_pair_impl<Top, TParams, false> base_t;
   typedef reverse_modified_container_pair_typebase<Top, TParams> rbase_t;
public:
   typename rbase_t::reverse_iterator rbegin()
   {
      typename base_t::container1_temp_ref c1=this->manip_top().get_container1();
      typename base_t::container2_temp_ref c2=this->manip_top().get_container2();
      return typename rbase_t::reverse_iterator(ensure(c1, (typename base_t::needed_features1*)0).rbegin(),
                                                ensure(c2, (typename base_t::needed_features2*)0).rbegin(),
                                                this->manip_top().get_operation());
   }
   typename rbase_t::reverse_iterator rend()
   {
      typename base_t::container1_temp_ref c1=this->manip_top().get_container1();
      typename base_t::container2_temp_ref c2=this->manip_top().get_container2();
      return typename rbase_t::reverse_iterator(ensure(c1, (typename base_t::needed_features1*)0).rend(),
                                                ensure(c2, (typename base_t::needed_features2*)0).rend(),
                                                this->manip_top().get_operation());
   }
   typename rbase_t::const_reverse_iterator rbegin() const
   {
      return typename rbase_t::const_reverse_iterator(ensure(this->manip_top().get_container1(),
                                                             (typename base_t::needed_features1*)0).rbegin(),
                                                      ensure(this->manip_top().get_container2(),
                                                             (typename base_t::needed_features2*)0).rbegin(),
                                                      this->manip_top().get_operation());
   }
   typename rbase_t::const_reverse_iterator rend() const
   {
      return typename rbase_t::const_reverse_iterator(ensure(this->manip_top().get_container1(),
                                                             (typename base_t::needed_features1*)0).rend(),
                                                      ensure(this->manip_top().get_container2(),
                                                             (typename base_t::needed_features2*)0).rend(),
                                                      this->manip_top().get_operation());
   }
};

template <typename Top, typename TParams, typename Category, bool TBijective, bool TIdentity>
class modified_container_pair_elem_access {
   typedef modified_container_pair_typebase<Top, TParams> base_t;
protected:
   typename base_t::manip_top_type& _top()
   {
      return static_cast<modified_container_pair_impl<Top, TParams>*>(this)->manip_top();
   }
   const typename base_t::manip_top_type& _top() const
   {
      return static_cast<const modified_container_pair_impl<Top, TParams>*>(this)->manip_top();
   }
private:
   int size_impl(std::false_type) const { return _top().get_container1().size(); }
   int size_impl(std::true_type) const { return _top().get_container2().size(); }
   int dim_impl(std::false_type) const { return get_dim(_top().get_container1()); }
   int dim_impl(std::true_type) const { return get_dim(_top().get_container2()); }
   bool empty_impl(std::false_type) const { return _top().get_container1().empty(); }
   bool empty_impl(std::true_type) const { return _top().get_container2().empty(); }

   typedef bool_constant<(object_classifier::what_is<typename deref<typename base_t::container1>::type>::value
                          == object_classifier::is_constant)> unlimited1;
public:
   int size() const { return size_impl(unlimited1()); }
   int dim() const { return dim_impl(unlimited1()); }
   bool empty() const { return empty_impl(unlimited1()); }
};

template <typename Top, typename TParams>
class modified_container_pair_elem_access<Top, TParams, forward_iterator_tag, true, false>
   : public modified_container_pair_elem_access<Top, TParams, input_iterator_tag, true, false> {
   typedef modified_container_pair_typebase<Top, TParams> base_t;

   typename base_t::reference front_impl(const typename base_t::iterator::operation& op, std::true_type, std::true_type)
   {
      return op(this->_top().get_container1().front(),
                this->_top().get_container2().front());
   }
   typename base_t::const_reference front_impl(const typename base_t::const_iterator::operation& op, std::true_type, std::true_type) const
   {
      return op(this->_top().get_container1().front(),
                this->_top().get_container2().front());
   }
   typename base_t::reference front_impl(const typename base_t::iterator::operation& op, std::false_type, std::true_type)
   {
      return op(this->_top().get_container1().begin(),
                this->_top().get_container2().front());
   }
   typename base_t::const_reference front_impl(const typename base_t::const_iterator::operation& op, std::false_type, std::true_type) const
   {
      return op(this->_top().get_container1().begin(),
                this->_top().get_container2().front());
   }
   typename base_t::reference front_impl(const typename base_t::iterator::operation& op, std::true_type, std::false_type)
   {
      return op(this->_top().get_container1().front(),
                this->_top().get_container2().begin());
   }
   typename base_t::const_reference front_impl(const typename base_t::const_iterator::operation& op, std::true_type, std::false_type) const
   {
      return op(this->_top().get_container1().front(),
                this->_top().get_container2().begin());
   }
   typename base_t::reference front_impl(const typename base_t::iterator::operation& op, std::false_type, std::false_type)
   {
      return op(this->_top().get_container1().begin(),
                this->_top().get_container2().begin());
   }
   typename base_t::const_reference front_impl(const typename base_t::const_iterator::operation& op, std::false_type, std::false_type) const
   {
      return op(this->_top().get_container1().begin(),
                this->_top().get_container2().begin());
   }
public:
   typename base_t::reference front()
   {
      typedef typename base_t::iterator::helper opb;
      return front_impl(opb::create(this->_top().get_operation()),
                        bool_constant<opb::first_data_arg>(), bool_constant<opb::second_data_arg>());
   }
   typename base_t::const_reference front() const
   {
      typedef typename base_t::const_iterator::helper opb;
      return front_impl(opb::create(this->_top().get_operation()),
                        bool_constant<opb::first_data_arg>(), bool_constant<opb::second_data_arg>());
   }
};

template <typename Top, typename TParams>
class modified_container_pair_elem_access<Top, TParams, forward_iterator_tag, true, true>
   : public modified_container_pair_elem_access<Top, TParams, input_iterator_tag, true, true> {
   typedef modified_container_pair_typebase<Top, TParams> base_t;
public:
   typename base_t::reference front()
   {
      return this->_top().get_container1().front();
   }
   typename base_t::const_reference front() const
   {
      return this->_top().get_container1().front();
   }
};

template <typename Top, typename TParams>
class modified_container_pair_elem_access<Top, TParams, bidirectional_iterator_tag, true, false>
   : public modified_container_pair_elem_access<Top, TParams, forward_iterator_tag, true, false> {
   typedef modified_container_pair_typebase<Top, TParams> base_t;
   typedef reverse_modified_container_pair_typebase<Top, TParams> rbase_t;

   typename base_t::reference back_impl(const typename rbase_t::reverse_iterator::operation& op, std::true_type, std::true_type)
   {
      return op(this->_top().get_container1().back(),
                this->_top().get_container2().back());
   }
   typename base_t::const_reference back_impl(const typename rbase_t::const_reverse_iterator::operation& op, std::true_type, std::true_type) const
   {
      return op(this->_top().get_container1().back(),
                this->_top().get_container2().back());
   }
   typename base_t::reference back_impl(const typename rbase_t::reverse_iterator::operation& op, std::false_type, std::true_type)
   {
      return op(this->_top().get_container1().rbegin(),
                this->_top().get_container2().back());
   }
   typename base_t::const_reference back_impl(const typename rbase_t::const_reverse_iterator::operation& op, std::false_type, std::true_type) const
   {
      return op(this->_top().get_container1().rbegin(),
                this->_top().get_container2().back());
   }
   typename base_t::reference back_impl(const typename rbase_t::reverse_iterator::operation& op, std::true_type, std::false_type)
   {
      return op(this->_top().get_container1().back(),
                this->_top().get_container2().rbegin());
   }
   typename base_t::const_reference back_impl(const typename rbase_t::const_reverse_iterator::operation& op, std::true_type, std::false_type) const
   {
      return op(this->_top().get_container1().back(),
                this->_top().get_container2().rbegin());
   }
   typename base_t::reference back_impl(const typename rbase_t::reverse_iterator::operation& op, std::false_type, std::false_type)
   {
      return op(this->_top().get_container1().rbegin(),
                this->_top().get_container2().rbegin());
   }
   typename base_t::const_reference back_impl(const typename rbase_t::const_reverse_iterator::operation& op, std::false_type, std::false_type) const
   {
      return op(this->_top().get_container1().rbegin(),
                this->_top().get_container2().rbegin());
   }
public:
   typename base_t::reference back()
   {
      typedef typename rbase_t::reverse_iterator::helper opb;
      return back_impl(opb::create(this->_top().get_operation()),
                       bool_constant<opb::first_data_arg>(), bool_constant<opb::second_data_arg>());
   }
   typename base_t::const_reference back() const
   {
      typedef typename rbase_t::const_reverse_iterator::helper opb;
      return back_impl(opb::create(this->_top().get_operation()),
                       bool_constant<opb::first_data_arg>(), bool_constant<opb::second_data_arg>());
   }
};

template <typename Top, typename TParams>
class modified_container_pair_elem_access<Top, TParams, bidirectional_iterator_tag, true, true>
   : public modified_container_pair_elem_access<Top, TParams, forward_iterator_tag, true, true> {
   typedef modified_container_pair_typebase<Top, TParams> base_t;
public:
   typename base_t::reference back()
   {
      return this->_top().get_container1().back();
   }
   typename base_t::const_reference back() const
   {
      return this->_top().get_container1().back();
   }
};

template <typename Top, typename TParams>
class modified_container_pair_elem_access<Top, TParams, random_access_iterator_tag, true, false>
   : public modified_container_pair_elem_access<Top, TParams, bidirectional_iterator_tag, true, false> {
   typedef modified_container_pair_typebase<Top, TParams> base_t;

   typename base_t::reference random_impl(int i, const typename base_t::iterator::operation& op, std::true_type, std::true_type)
   {
      return op(this->_top().get_container1()[i],
                this->_top().get_container2()[i]);
   }
   typename base_t::const_reference random_impl(int i, const typename base_t::const_iterator::operation& op, std::true_type, std::true_type) const
   {
      return op(this->_top().get_container1()[i],
                this->_top().get_container2()[i]);
   }
   typename base_t::reference random_impl(int i, const typename base_t::iterator::operation& op, std::false_type, std::true_type)
   {
      return op(this->_top().get_container1().begin()+i,
                this->_top().get_container2()[i]);
   }
   typename base_t::const_reference random_impl(int i, const typename base_t::const_iterator::operation& op, std::false_type, std::true_type) const
   {
      return op(this->_top().get_container1().begin()+i,
                this->_top().get_container2()[i]);
   }
   typename base_t::reference random_impl(int i, const typename base_t::iterator::operation& op, std::true_type, std::false_type)
   {
      return op(this->_top().get_container1()[i],
                this->_top().get_container2().begin()+i);
   }
   typename base_t::const_reference random_impl(int i, const typename base_t::const_iterator::operation& op, std::true_type, std::false_type) const
   {
      return op(this->_top().get_container1()[i],
                this->_top().get_container2().begin()+i);
   }
   typename base_t::reference random_impl(int i, const typename base_t::iterator::operation& op, std::false_type, std::false_type)
   {
      return op(this->_top().get_container1().begin()+i,
                this->_top().get_container2().begin()+i);
   }
   typename base_t::const_reference random_impl(int i, const typename base_t::const_iterator::operation& op, std::false_type, std::false_type) const
   {
      return op(this->_top().get_container1().begin()+i,
                this->_top().get_container2().begin()+i);
   }
public:
   typename base_t::reference operator[] (int i)
   {
      typedef typename base_t::iterator::helper opb;
      const bool via_container1=opb::first_data_arg || !iterator_traits<typename base_t::container1::iterator>::is_random,
                 via_container2=opb::second_data_arg || !iterator_traits<typename base_t::container2::iterator>::is_random;
      return random_impl(i, opb::create(this->_top().get_operation()),
                         bool_constant<via_container1>(), bool_constant<via_container2>());
   }
   typename base_t::const_reference operator[] (int i) const
   {
      typedef typename base_t::const_iterator::helper opb;
      const bool via_container1=opb::first_data_arg || !iterator_traits<typename base_t::container1::const_iterator>::is_random,
                 via_container2=opb::second_data_arg || !iterator_traits<typename base_t::container2::const_iterator>::is_random;
      return random_impl(i, opb::create(this->_top().get_operation()),
                         bool_constant<via_container1>(), bool_constant<via_container2>());
   }
};

template <typename Top, typename TParams>
class modified_container_pair_elem_access<Top, TParams, random_access_iterator_tag, true, true>
   : public modified_container_pair_elem_access<Top, TParams, bidirectional_iterator_tag, true, true> {
   typedef modified_container_pair_typebase<Top, TParams> base_t;
public:
   typename base_t::reference operator[] (int i)
   {
      return this->_top().get_container1()[i];
   }
   typename base_t::const_reference operator[] (int i) const
   {
      return this->_top().get_container1()[i];
   }
};

template <typename Top, typename TParams, typename Category>
class modified_container_pair_elem_access<Top, TParams, Category, false, false>
   : public modified_container_non_bijective_elem_access<Top, modified_container_pair_typebase<Top, TParams>,
                                                         is_derived_from<Category, bidirectional_iterator_tag>::value> {};

template <typename ContainerRef, typename Operation>
class modified_container_base {
protected:
   alias<ContainerRef> src;
   typedef typename is_identity_transform<Operation>::type operation_type;
   operation_type op;
public:
   typedef typename alias<ContainerRef>::arg_type arg_type;

   modified_container_base(arg_type src_arg, const operation_type& op_arg)
      : src(src_arg), op(op_arg) {}

   typename alias<ContainerRef>::reference get_container() { return *src; }
   typename alias<ContainerRef>::const_reference get_container() const { return *src; }
   const alias<ContainerRef>& get_container_alias() const { return src; }
   const operation_type& get_operation() const { return op; }
};

template <typename ContainerRef1, typename ContainerRef2>
class container_pair_base {
protected:
   alias<ContainerRef1> src1;
   alias<ContainerRef2> src2;
public:
   typedef typename alias<ContainerRef1>::arg_type first_arg_type;
   typedef typename alias<ContainerRef2>::arg_type second_arg_type;

   container_pair_base(first_arg_type src1_arg, second_arg_type src2_arg)
      : src1(src1_arg), src2(src2_arg) {}

   typename alias<ContainerRef1>::reference get_container1() { return *src1; }
   typename alias<ContainerRef2>::reference get_container2() { return *src2; }
   typename alias<ContainerRef1>::const_reference get_container1() const { return *src1; }
   typename alias<ContainerRef2>::const_reference get_container2() const { return *src2; }
   const alias<ContainerRef1>& get_container1_alias() const { return src1; }
   const alias<ContainerRef2>& get_container2_alias() const { return src2; }
};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
class modified_container_pair_base
   : public container_pair_base<ContainerRef1, ContainerRef2> {
   typedef container_pair_base<ContainerRef1, ContainerRef2> _super;
protected:
   typedef typename is_identity_transform<Operation>::type operation_type;
   operation_type op;
public:
   modified_container_pair_base(typename _super::first_arg_type src1_arg,
                                typename _super::second_arg_type src2_arg,
                                const operation_type& op_arg)
      : _super(src1_arg,src2_arg), op(op_arg) {}

   const operation_type& get_operation() const { return op; }
};

template <typename Top, typename T=typename Top::element_reference, typename TParams=mlist<>>
class repeated_value_container_impl
   : public modified_container_pair_impl< Top,
                                          typename mlist_concat<
                                             Container1Tag< constant_value_container<T> >,
                                             Container2Tag< sequence >,
                                             OperationTag< pair<nothing,
                                                                operations::apply2< BuildUnaryIt<operations::dereference> > > >,
                                             TParams >::type > {
public:
   typedef T element_reference;
   const constant_value_container<T>& get_container1() const
   {
      return constant(this->manip_top().get_elem_alias());
   }
   sequence get_container2() const { return sequence(0, this->manip_top().size()); }
};

template <typename T>
class repeated_value_container
   : public repeated_value_container_impl<repeated_value_container<T>, T> {
protected:
   alias<T> value;
   int d;
public:
   typedef typename alias<T>::arg_type arg_type;

   repeated_value_container(arg_type value_arg, int dim_arg)
      : value(value_arg), d(dim_arg) {}

   alias<T>& get_elem_alias() { return value; }
   const alias<T>& get_elem_alias() const { return value; }

   int dim() const { return d; }
   int size() const { return d; }
   bool empty() const { return d==0; }

   void stretch_dim(int to_dim) { d=to_dim; }
};

template <typename T>
struct spec_object_traits< repeated_value_container<T> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true, is_always_const=true;
};

template <typename Container>
class construct_sequence_indexed
   : public modified_container_pair_impl< construct_sequence_indexed<Container>,
                                          mlist< Container1Tag< Container >,
                                                 Container2Tag< sequence >,
                                                 OperationTag< pair<nothing, operations::apply2< BuildUnaryIt<operations::dereference> > > >,
                                                 HiddenTag< Container > > > {
public:
   sequence get_container2() const
   {
      // the size is being determined on the first (main) container unless it is of unlimited-const nature
      return sequence(0, object_classifier::what_is<Container>::value==object_classifier::is_constant ? this->hidden().size() : 1);
   }
};

template <typename Container>
class construct_random_indexed : public Container {
protected:
   construct_random_indexed();
   ~construct_random_indexed();
public:
   typedef indexed_random_iterator<typename Container::iterator> iterator;
   typedef indexed_random_iterator<typename Container::const_iterator> const_iterator;
   typedef indexed_random_iterator<typename Container::reverse_iterator, true> reverse_iterator;
   typedef indexed_random_iterator<typename Container::const_reverse_iterator, true> const_reverse_iterator;

   iterator begin() { return iterator(Container::begin()); }
   iterator end() { return iterator(Container::end(), Container::begin()); }
   const_iterator begin() const { return const_iterator(Container::begin()); }
   const_iterator end() const { return const_iterator(Container::end(), Container::begin()); }
   reverse_iterator rbegin() { return reverse_iterator(Container::rbegin(), Container::rend()); }
   reverse_iterator rend() { return reverse_iterator(Container::rend()); }
   const_reverse_iterator rbegin() const { return const_reverse_iterator(Container::rbegin(), Container::rend()); }
   const_reverse_iterator rend() const { return const_reverse_iterator(Container::rend()); }
};

template <typename Container>
struct default_enforce_feature<Container, indexed> {
   typedef typename std::conditional<std::is_same<typename iterator_traits<typename container_traits<Container>::iterator>::iterator_category,
                                                  random_access_iterator_tag>::value,
                                     construct_random_indexed<Container>, construct_sequence_indexed<Container> >::type
      container;
};

template <typename Container>
struct redirect_object_traits< construct_random_indexed<Container> >
   : object_traits<Container> {
   typedef Container masquerade_for;
   static const bool is_temporary=false;
};

template <typename Container, typename Features>
struct default_enforce_features<Container, Features, object_classifier::is_constant>
   : default_enforce_features<construct_sequence_indexed<Container>, Features, object_classifier::is_manip> {};

template <typename Container>
struct default_enforce_features<Container, _reversed, object_classifier::is_constant> {
   typedef Container container;
};

} // end namespace pm

#endif // POLYMAKE_INTERNAL_MODIFIED_CONTAINERS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
