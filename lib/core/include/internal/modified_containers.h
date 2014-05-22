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

#ifndef POLYMAKE_INTERNAL_MODIFIED_CONTAINERS_H
#define POLYMAKE_INTERNAL_MODIFIED_CONTAINERS_H

#include "polymake/internal/constant_containers.h"
#include "polymake/Series.h"

namespace pm {

template <typename IteratorConstructor,
          bool _try=(derived_from_instance<IteratorConstructor, unary_transform_constructor>::value ||
                     derived_from_instance<IteratorConstructor, binary_transform_constructor>::value)>
struct is_bijective : False {};

template <typename Operation>
struct is_identity_transform : False {
   typedef Operation type;
};

template <typename IteratorConstructor>
struct is_bijective<IteratorConstructor, true> : extract_bool_param<typename IteratorConstructor::params, Bijective, true> {};

template <typename Operation>
struct is_identity_transform< pair<nothing,Operation> > : True {
   typedef Operation type;
};

template <typename Top, typename Params>
class redirected_container_typebase : public manip_container_top<Top,Params> {
   typedef manip_container_top<Top,Params> _super;
public:
   typedef typename extract_type_param<Params, Container, typename _super::hidden_type>::type container_ref;
   typedef typename deref<container_ref>::minus_ref container;
   typedef typename temp_ref<container_ref>::type container_temp_ref;
   typedef typename _super::expected_features needed_features;
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

template <typename Top, typename Params>
class modified_container_typebase
   : public manip_container_top<Top, Params> {
   typedef manip_container_top<Top, Params> _super;
public:
   typedef typename extract_type_param<Params, Container, typename _super::hidden_type>::type container_ref;
   typedef typename deref<container_ref>::minus_ref container;
   typedef typename temp_ref<container_ref>::type container_temp_ref;
   typedef typename extract_type_param<Params, Operation>::type operation;
   typedef typename operation_cross_const_helper<operation>::const_operation const_operation;
   typedef typename extract_type_param<Params, IteratorConstructor, unary_transform_constructor<> >::type it_constructor;

   typedef typename it_constructor::template defs<typename container_traits<container>::iterator,
                                                  operation, typename _super::expected_features>::needed_features
      needed_features;
   typedef typename it_constructor::template defs<typename ensure_features<container,needed_features>::iterator,
                                                  operation, typename _super::expected_features>::iterator
      iterator;
   typedef typename it_constructor::template defs<typename ensure_features<container,needed_features>::const_iterator,
                                                  const_operation, typename _super::expected_features>::iterator
      const_iterator;

   typedef typename container_traits<container>::category category_list;
   typedef typename if_else<is_bijective<it_constructor>::value,
                            category_list, typename least_derived< cons<bidirectional_iterator_tag,category_list> >::type >::type
      container_category;

   typedef typename enforce_feature_helper<typename deref<container>::type>::must_enforce_features must_enforce_features;

   typedef typename iterator_traits<iterator>::value_type value_type;
   typedef typename iterator_traits<iterator>::reference reference;
   typedef typename iterator_traits<const_iterator>::reference const_reference;
};

template <typename Top, typename Params>
class reverse_modified_container_typebase {
   typedef modified_container_typebase<Top, Params> _base;
public:
   typedef typename _base::it_constructor::template defs<
      typename ensure_features<typename _base::container, typename _base::needed_features>::reverse_iterator,
      typename _base::operation, typename _base::expected_features
   >::iterator reverse_iterator;
   typedef typename _base::it_constructor::template defs<
      typename ensure_features<typename _base::container, typename _base::needed_features>::const_reverse_iterator,
      typename _base::const_operation, typename _base::expected_features
   >::iterator const_reverse_iterator;
};

template <typename Top, typename Params,
          typename Category=typename modified_container_typebase<Top,Params>::container_category,
          bool _bijective=is_bijective<typename modified_container_typebase<Top,Params>::it_constructor>::value,
          bool _identity=is_identity_transform<typename modified_container_typebase<Top,Params>::operation>::value>
class modified_container_elem_access;

template <typename Top, typename Params=typename Top::manipulator_params,
          bool _reversible=derived_from<typename modified_container_typebase<Top,Params>::container_category,
                                        bidirectional_iterator_tag>::value>
class modified_container_impl
   : public modified_container_typebase<Top,Params>,
     public modified_container_elem_access<Top,Params> {
   typedef modified_container_typebase<Top,Params> _super;
public:
   typedef modified_container_impl<Top,Params> manipulator_impl;
   typedef Params manipulator_params;
   typedef typename _super::iterator iterator;
   typedef typename _super::const_iterator const_iterator;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef modified_container_impl<FeatureCollector,Params> type;
   };

   typename is_identity_transform<typename _super::operation>::type get_operation() const
   {
      return typename is_identity_transform<typename _super::operation>::type();
   }

   iterator begin()
   {
      typename _super::container_temp_ref c=this->manip_top().get_container();
      return iterator(ensure(c, (typename _super::needed_features*)0).begin(), this->manip_top().get_operation());
   }
   iterator end()
   {
      typename _super::container_temp_ref c=this->manip_top().get_container();
      return iterator(ensure(c, (typename _super::needed_features*)0).end(), this->manip_top().get_operation());
   }
   const_iterator begin() const
   {
      return const_iterator(ensure(this->manip_top().get_container(), (typename _super::needed_features*)0).begin(),
                            this->manip_top().get_operation());
   }
   const_iterator end() const
   {
      return const_iterator(ensure(this->manip_top().get_container(), (typename _super::needed_features*)0).end(),
                            this->manip_top().get_operation());
   }
};

template <typename Top, typename Params>
class modified_container_impl<Top, Params, true>
   : public modified_container_impl<Top, Params, false>,
     public reverse_modified_container_typebase<Top, Params> {
   typedef modified_container_impl<Top,Params,false> _super;
   typedef reverse_modified_container_typebase<Top, Params> _rbase;
public:
   typename _rbase::reverse_iterator rbegin()
   {
      typename _super::container_temp_ref c=this->manip_top().get_container();
      return typename _rbase::reverse_iterator(ensure(c, (typename _super::needed_features*)0).rbegin(),
                                               this->manip_top().get_operation());
   }
   typename _rbase::reverse_iterator rend()
   {
      typename _super::container_temp_ref c=this->manip_top().get_container();
      return typename _rbase::reverse_iterator(ensure(c, (typename _super::needed_features*)0).rend(),
                                               this->manip_top().get_operation());
   }
   typename _rbase::const_reverse_iterator rbegin() const
   {
      return typename _rbase::const_reverse_iterator(ensure(this->manip_top().get_container(),
                                                            (typename _super::needed_features*)0).rbegin(),
                                                     this->manip_top().get_operation());
   }
   typename _rbase::const_reverse_iterator rend() const
   {
      return typename _rbase::const_reverse_iterator(ensure(this->manip_top().get_container(),
                                                            (typename _super::needed_features*)0).rend(),
                                                     this->manip_top().get_operation());
   }
};

template <typename Top, typename Params, typename Category, bool _bijective, bool _identity>
class modified_container_elem_access {
   typedef modified_container_typebase<Top,Params> _base;
protected:
   typename _base::manip_top_type& _top()
   {
      return static_cast<modified_container_impl<Top,Params>*>(this)->manip_top();
   }
   const typename _base::manip_top_type& _top() const
   {
      return static_cast<const modified_container_impl<Top,Params>*>(this)->manip_top();
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

template <typename Top, typename Params>
class modified_container_elem_access<Top, Params, forward_iterator_tag, true, false>
   : public modified_container_elem_access<Top, Params, input_iterator_tag, true, false> {
   typedef modified_container_typebase<Top,Params> _base;

   typename _base::reference _front(const typename _base::iterator::operation& op, True)
   {
      return op(this->_top().get_container().front());
   }
   typename _base::const_reference _front(const typename _base::const_iterator::operation& op, True) const
   {
      return op(this->_top().get_container().front());
   }
   typename _base::reference _front(const typename _base::iterator::operation& op, False)
   {
      return op(this->_top().get_container().begin());
   }
   typename _base::const_reference _front(const typename _base::const_iterator::operation& op, False) const
   {
      return op(this->_top().get_container().begin());
   }
public:
   typename _base::reference front()
   {
      typedef typename _base::iterator::helper opb;
      return _front(opb::create(this->_top().get_operation()), bool2type<opb::data_arg>());
   }
   typename _base::const_reference front() const
   {
      typedef typename _base::const_iterator::helper opb;
      return _front(opb::create(this->_top().get_operation()), bool2type<opb::data_arg>());
   }
};

template <typename Top, typename Params>
class modified_container_elem_access<Top, Params, forward_iterator_tag, true, true>
   : public modified_container_elem_access<Top, Params, input_iterator_tag, true, true> {
   typedef modified_container_typebase<Top,Params> _base;
public:
   typename _base::reference front()
   {
      return this->_top().get_container().front();
   }
   typename _base::const_reference front() const
   {
      return this->_top().get_container().front();
   }
};

template <typename Top, typename Params>
class modified_container_elem_access<Top, Params, bidirectional_iterator_tag, true, false>
   : public modified_container_elem_access<Top, Params, forward_iterator_tag, true, false> {
   typedef modified_container_typebase<Top,Params> _base;
   typedef reverse_modified_container_typebase<Top,Params> _rbase;

   typename _base::reference _back(const typename _rbase::reverse_iterator::operation& op, True)
   {
      return op(this->_top().get_container().back());
   }
   typename _base::const_reference _back(const typename _rbase::const_reverse_iterator::operation& op, True) const
   {
      return op(this->_top().get_container().back());
   }
   typename _base::reference _back(const typename _rbase::reverse_iterator::operation& op, False)
   {
      return op(this->_top().get_container().rbegin());
   }
   typename _base::const_reference _back(const typename _rbase::const_reverse_iterator::operation& op, False) const
   {
      return op(this->_top().get_container().rbegin());
   }
public:
   typename _base::reference back()
   {
      typedef typename _rbase::reverse_iterator::helper opb;
      return _back(opb::create(this->_top().get_operation()), bool2type<opb::data_arg>());
   }
   typename _base::const_reference back() const
   {
      typedef typename _rbase::const_reverse_iterator::helper opb;
      return _back(opb::create(this->_top().get_operation()), bool2type<opb::data_arg>());
   }
};

template <typename Top, typename Params>
class modified_container_elem_access<Top, Params, bidirectional_iterator_tag, true, true>
   : public modified_container_elem_access<Top, Params, forward_iterator_tag, true, true> {
   typedef modified_container_typebase<Top,Params> _base;
public:
   typename _base::reference back()
   {
      return this->_top().get_container().back();
   }
   typename _base::const_reference back() const
   {
      return this->_top().get_container().back();
   }
};

template <typename Top, typename Params>
class modified_container_elem_access<Top, Params, random_access_iterator_tag, true, false>
   : public modified_container_elem_access<Top, Params, bidirectional_iterator_tag, true, false> {
   typedef modified_container_typebase<Top,Params> _base;

   typename _base::reference _random(int i, const typename _base::iterator::operation& op, True)
   {
      return op(this->_top().get_container()[i]);
   }
   typename _base::const_reference _random(int i, const typename _base::const_iterator::operation& op, True) const
   {
      return op(this->_top().get_container()[i]);
   }
   typename _base::reference _random(int i, const typename _base::iterator::operation& op, False)
   {
      return op(this->_top().get_container().begin() + i);
   }
   typename _base::const_reference _random(int i, const typename _base::const_iterator::operation& op, False) const
   {
      return op(this->_top().get_container().begin() + i);
   }
public:
   typename _base::reference operator[] (int i)
   {
      typedef typename _base::iterator::helper opb;
      const bool via_container=opb::data_arg || !iterator_traits<typename _base::container::iterator>::is_random;
      return _random(i, opb::create(this->_top().get_operation()), bool2type<via_container>());
   }
   typename _base::const_reference operator[] (int i) const
   {
      typedef typename _base::const_iterator::helper opb;
      const bool via_container=opb::data_arg || !iterator_traits<typename _base::container::const_iterator>::is_random;
      return _random(i, opb::create(this->_top().get_operation()), bool2type<via_container>());
   }
};

template <typename Top, typename Params>
class modified_container_elem_access<Top, Params, random_access_iterator_tag, true, true>
   : public modified_container_elem_access<Top, Params, bidirectional_iterator_tag, true, true> {
   typedef modified_container_typebase<Top,Params> _base;
public:
   typename _base::reference operator[] (int i)
   {
      return this->_top().get_container()[i];
   }
   typename _base::const_reference operator[] (int i) const
   {
      return this->_top().get_container()[i];
   }
};

template <typename Top, typename Params, typename Category>
class modified_container_elem_access<Top, Params, Category, false, false>
   : public modified_container_non_bijective_elem_access<Top, modified_container_typebase<Top,Params>,
                                                         derived_from<Category, bidirectional_iterator_tag>::value> {};

template <typename Top, typename Params>
class container_pair_typebase : public manip_container_top<Top, Params> {
   typedef manip_container_top<Top, Params> _super;
public:
   typedef typename extract_type_param<Params,Container1>::type container1_ref;
   typedef typename extract_type_param<Params,Container2>::type container2_ref;
   typedef typename deref<container1_ref>::minus_ref container1;
   typedef typename deref<container2_ref>::minus_ref container2;
   typedef typename temp_ref<container1_ref>::type container1_temp_ref;
   typedef typename temp_ref<container2_ref>::type container2_temp_ref;

   typedef typename extract_type_param<Params, IteratorCoupler, pair_coupler<> >::type it_coupler;
   typedef typename it_coupler::template defs<typename container_traits<container1>::iterator,
                                              typename container_traits<container2>::iterator,
                                              typename _super::expected_features>::needed_features1
      needed_features1;
   typedef typename it_coupler::template defs<typename container_traits<container1>::iterator,
                                              typename container_traits<container2>::iterator,
                                              typename _super::expected_features>::needed_features2
      needed_features2;
   typedef typename it_coupler::template defs<typename ensure_features<container1,needed_features1>::iterator,
                                              typename ensure_features<container2,needed_features2>::iterator,
                                              typename _super::expected_features>::iterator
      iterator;
   typedef typename it_coupler:: template defs<typename ensure_features<container1,needed_features1>::const_iterator,
                                               typename ensure_features<container2,needed_features2>::const_iterator,
                                               typename _super::expected_features>::iterator
      const_iterator;

   typedef typename least_derived< cons<typename container_traits<container1>::category,
                                        typename container_traits<container2>::category> >::type
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

template <typename IteratorCoupler, typename Params, bool reverse=list_contains<Params, _reversed>::value>
struct reverse_coupler_helper {
   typedef IteratorCoupler type;
};

template <typename IteratorCoupler, typename Params>
struct reverse_coupler_helper<IteratorCoupler, Params, true> : reverse_coupler<IteratorCoupler> {};

template <typename Top, typename Params>
class reverse_container_pair_typebase {
   typedef container_pair_typebase<Top, Params> _base;
   typedef typename reverse_coupler<typename _base::it_coupler>::type rev_it_coupler;
public:
   typedef typename rev_it_coupler::template defs<typename ensure_features<typename _base::container1,
                                                                           typename _base::needed_features1>::reverse_iterator,
                                                  typename ensure_features<typename _base::container2,
                                                                           typename _base::needed_features2>::reverse_iterator,
                                                  typename _base::expected_features>::iterator
      reverse_iterator;
   typedef typename rev_it_coupler::template defs<typename ensure_features<typename _base::container1,
                                                                           typename _base::needed_features1>::const_reverse_iterator,
                                                  typename ensure_features<typename _base::container2,
                                                                           typename _base::needed_features2>::const_reverse_iterator,
                                                  typename _base::expected_features>::iterator
      const_reverse_iterator;
};

template <typename Top, typename Params=typename Top::manipulator_params,
          typename Category=typename container_pair_typebase<Top,Params>::container_category>
class container_pair_impl
   : public container_pair_typebase<Top,Params> {
   typedef container_pair_typebase<Top,Params> _super;
   int _size(False) const { return this->manip_top().get_container1().size(); }
   int _size(True) const { return this->manip_top().get_container2().size(); }
   int _dim(False) const { return get_dim(this->manip_top().get_container1()); }
   int _dim(True) const { return get_dim(this->manip_top().get_container2()); }
   bool _empty(False) const { return this->manip_top().get_container1().empty(); }
   bool _empty(True) const { return this->manip_top().get_container2().empty(); }

   typedef bool2type<object_classifier::what_is<typename deref<typename _super::container1>::type>::value
                     == object_classifier::is_constant>
      discr;
public:
   typedef container_pair_impl<Top,Params> manipulator_impl;
   typedef Params manipulator_params;
   typedef typename _super::iterator iterator;
   typedef typename _super::const_iterator const_iterator;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef container_pair_impl<FeatureCollector,Params> type;
   };

   iterator begin()
   {
      typename _super::container1_temp_ref c1=this->manip_top().get_container1();
      typename _super::container2_temp_ref c2=this->manip_top().get_container2();
      return iterator(ensure(c1, (typename _super::needed_features1*)0).begin(),
                      ensure(c2, (typename _super::needed_features2*)0).begin());
   }
   iterator end()
   {
      typename _super::container1_temp_ref c1=this->manip_top().get_container1();
      typename _super::container2_temp_ref c2=this->manip_top().get_container2();
      return iterator(ensure(c1, (typename _super::needed_features1*)0).end(),
                      ensure(c2, (typename _super::needed_features2*)0).end());
   }
   const_iterator begin() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), (typename _super::needed_features1*)0).begin(),
                            ensure(this->manip_top().get_container2(), (typename _super::needed_features2*)0).begin());
   }
   const_iterator end() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), (typename _super::needed_features1*)0).end(),
                            ensure(this->manip_top().get_container2(), (typename _super::needed_features2*)0).end());
   }

   int size() const { return _size(discr()); }
   int dim() const { return _dim(discr()); }
   bool empty() const { return _empty(discr()); }
};

template <typename Top, typename Params>
class container_pair_impl<Top, Params, forward_iterator_tag>
   : public container_pair_impl<Top, Params, input_iterator_tag> {
   typedef container_pair_impl<Top, Params, input_iterator_tag> _super;
public:
   typename _super::reference front()
   {
      return this->manip_top().get_container1().front();
   }
   typename _super::const_reference front() const
   {
      return this->manip_top().get_container1().front();
   }
};

template <typename Top, typename Params>
class container_pair_impl<Top, Params, bidirectional_iterator_tag>
   : public container_pair_impl<Top, Params, forward_iterator_tag>,
     public reverse_container_pair_typebase<Top, Params> {
   typedef container_pair_impl<Top, Params, forward_iterator_tag> _super;
   typedef reverse_container_pair_typebase<Top, Params> _rbase;
public:
   typename _rbase::reverse_iterator rbegin()
   {
      typename _super::container1_temp_ref c1=this->manip_top().get_container1();
      typename _super::container2_temp_ref c2=this->manip_top().get_container2();
      return typename _rbase::reverse_iterator(ensure(c1, (typename _super::needed_features1*)0).rbegin(),
                                               ensure(c2, (typename _super::needed_features2*)0).rbegin());
   }
   typename _rbase::reverse_iterator rend()
   {
      typename _super::container1_temp_ref c1=this->manip_top().get_container1();
      typename _super::container2_temp_ref c2=this->manip_top().get_container2();
      return typename _rbase::reverse_iterator(ensure(c1, (typename _super::needed_features1*)0).rend(),
                                               ensure(c2, (typename _super::needed_features2*)0).rend());
   }
   typename _rbase::const_reverse_iterator rbegin() const
   {
      return typename _rbase::const_reverse_iterator(ensure(this->manip_top().get_container1(),
                                                            (typename _super::needed_features1*)0).rbegin(),
                                                     ensure(this->manip_top().get_container2(),
                                                            (typename _super::needed_features2*)0).rbegin());
   }
   typename _rbase::const_reverse_iterator rend() const
   {
      return typename _rbase::const_reverse_iterator(ensure(this->manip_top().get_container1(),
                                                            (typename _super::needed_features1*)0).rend(),
                                                     ensure(this->manip_top().get_container2(),
                                                            (typename _super::needed_features2*)0).rend());
   }

   typename _super::reference back()
   {
      return this->manip_top().get_container1().back();
   }
   typename _super::const_reference back() const
   {
      return this->manip_top().get_container1().back();
   }
};

template <typename Top, typename Params>
class container_pair_impl<Top, Params, random_access_iterator_tag>
   : public container_pair_impl<Top, Params, bidirectional_iterator_tag> {
   typedef container_pair_impl<Top, Params, bidirectional_iterator_tag> _super;
public:
   typename _super::reference operator[] (int i)
   {
      return this->manip_top().get_container1()[i];
   }
   typename _super::const_reference operator[] (int i) const
   {
      return this->manip_top().get_container1()[i];
   }
};

template <typename Top, typename Params>
class modified_container_pair_typebase
   : public manip_container_top<Top, Params> {
   typedef manip_container_top<Top, Params> _super;
public:
   typedef typename extract_type_param<Params, Container1>::type container1_ref;
   typedef typename extract_type_param<Params, Container2>::type container2_ref;
   typedef typename deref<container1_ref>::minus_ref container1;
   typedef typename deref<container2_ref>::minus_ref container2;
   typedef typename temp_ref<container1_ref>::type container1_temp_ref;
   typedef typename temp_ref<container2_ref>::type container2_temp_ref;

   typedef typename extract_type_param<Params, Operation>::type operation;
   typedef typename operation_cross_const_helper<operation>::const_operation const_operation;
   typedef typename extract_type_param<Params, IteratorCoupler, pair_coupler<> >::type it_coupler;
   typedef typename it_coupler::template defs<typename container_traits<container1>::iterator,
                                              typename container_traits<container2>::iterator, void>
      coupler_defs;
   typedef typename extract_type_param<Params, IteratorConstructor, binary_transform_constructor<> >::type it_constructor;

   typedef typename it_constructor::template defs<typename coupler_defs::iterator, operation, typename _super::expected_features>
      first_try_defs;
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
   typedef typename it_coupler::template defs<typename ensure_features<container1,needed_features1>::iterator,
                                              typename ensure_features<container2,needed_features2>::iterator,
                                              needed_pair_features>::iterator
      it_pair;
   typedef typename it_coupler::template defs<typename ensure_features<container1,needed_features1>::const_iterator,
                                              typename ensure_features<container2,needed_features2>::const_iterator,
                                              needed_pair_features>::iterator
      const_it_pair;
   typedef typename it_constructor::template defs<it_pair, operation, typename _super::expected_features>::iterator
      iterator;
   typedef typename it_constructor::template defs<const_it_pair, const_operation, typename _super::expected_features>::iterator
      const_iterator;

   typedef cons<typename container_traits<container1>::category,
                typename container_traits<container2>::category> category_list;
   typedef typename least_derived< typename if_else<is_bijective<it_constructor>::value,
                                                    category_list, cons<bidirectional_iterator_tag, category_list> >::type >::type
      container_category;

   typedef typename mix_features<typename enforce_feature_helper<typename deref<container1>::type>::must_enforce_features,
                                 typename enforce_feature_helper<typename deref<container2>::type>::must_enforce_features>::type
      must_enforce_features;

   typedef typename iterator_traits<iterator>::value_type value_type;
   typedef typename iterator_traits<iterator>::reference reference;
   typedef typename iterator_traits<const_iterator>::reference const_reference;
};

template <typename Top, typename Params>
class reverse_modified_container_pair_typebase {
   typedef modified_container_pair_typebase<Top, Params> _base;
   typedef typename reverse_coupler<typename _base::it_coupler>::type rev_it_coupler;
public:
   typedef typename rev_it_coupler::template defs<typename ensure_features<typename _base::container1,
                                                                           typename _base::needed_features1>::reverse_iterator,
                                                  typename ensure_features<typename _base::container2,
                                                                           typename _base::needed_features2>::reverse_iterator,
                                                  typename _base::needed_pair_features>::iterator
      reverse_it_pair;
   typedef typename rev_it_coupler::template defs<typename ensure_features<typename _base::container1,
                                                                           typename _base::needed_features1>::const_reverse_iterator,
                                                  typename ensure_features<typename _base::container2,
                                                                           typename _base::needed_features2>::const_reverse_iterator,
                                                  typename _base::needed_pair_features>::iterator
      const_reverse_it_pair;
   typedef typename _base::it_constructor::template defs<reverse_it_pair, typename _base::operation,
                                                         typename _base::expected_features>::iterator
      reverse_iterator;
   typedef typename _base::it_constructor::template defs<const_reverse_it_pair, typename _base::const_operation,
                                                         typename _base::expected_features>::iterator
      const_reverse_iterator;
};

template <typename Top, typename Params,
          typename Category=typename modified_container_pair_typebase<Top,Params>::container_category,
          bool _bijective=is_bijective<typename modified_container_pair_typebase<Top,Params>::it_constructor>::value,
          bool _identity=is_identity_transform<typename modified_container_pair_typebase<Top,Params>::operation>::value>
class modified_container_pair_elem_access;

template <typename Top, typename Params=typename Top::manipulator_params,
          bool _reversible=derived_from<typename modified_container_pair_typebase<Top,Params>::container_category,
                                        bidirectional_iterator_tag>::value>
class modified_container_pair_impl
   : public modified_container_pair_typebase<Top,Params>,
     public modified_container_pair_elem_access<Top,Params> {
   typedef modified_container_pair_typebase<Top,Params> _super;
public:
   typedef modified_container_pair_impl<Top,Params> manipulator_impl;
   typedef Params manipulator_params;
   typedef typename _super::iterator iterator;
   typedef typename _super::const_iterator const_iterator;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef modified_container_pair_impl<FeatureCollector,Params> type;
   };

   typename is_identity_transform<typename _super::operation>::type get_operation() const
   {
      return typename is_identity_transform<typename _super::operation>::type();
   }

   iterator begin()
   {
      typename _super::container1_temp_ref c1=this->manip_top().get_container1();
      typename _super::container2_temp_ref c2=this->manip_top().get_container2();
      return iterator(ensure(c1, (typename _super::needed_features1*)0).begin(),
                      ensure(c2, (typename _super::needed_features2*)0).begin(),
                      this->manip_top().get_operation());
   }
   iterator end()
   {
      typename _super::container1_temp_ref c1=this->manip_top().get_container1();
      typename _super::container2_temp_ref c2=this->manip_top().get_container2();
      return iterator(ensure(c1, (typename _super::needed_features1*)0).end(),
                      ensure(c2, (typename _super::needed_features2*)0).end(),
                      this->manip_top().get_operation());
   }
   const_iterator begin() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), (typename _super::needed_features1*)0).begin(),
                            ensure(this->manip_top().get_container2(), (typename _super::needed_features2*)0).begin(),
                            this->manip_top().get_operation());
   }
   const_iterator end() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), (typename _super::needed_features1*)0).end(),
                            ensure(this->manip_top().get_container2(), (typename _super::needed_features2*)0).end(),
                            this->manip_top().get_operation());
   }
};

template <typename Top, typename Params>
class modified_container_pair_impl<Top, Params, true>
   : public modified_container_pair_impl<Top, Params, false>,
     public reverse_modified_container_pair_typebase<Top, Params> {
   typedef modified_container_pair_impl<Top, Params, false> _super;
   typedef reverse_modified_container_pair_typebase<Top, Params> _rbase;
public:
   typename _rbase::reverse_iterator rbegin()
   {
      typename _super::container1_temp_ref c1=this->manip_top().get_container1();
      typename _super::container2_temp_ref c2=this->manip_top().get_container2();
      return typename _rbase::reverse_iterator(ensure(c1, (typename _super::needed_features1*)0).rbegin(),
                                               ensure(c2, (typename _super::needed_features2*)0).rbegin(),
                                               this->manip_top().get_operation());
   }
   typename _rbase::reverse_iterator rend()
   {
      typename _super::container1_temp_ref c1=this->manip_top().get_container1();
      typename _super::container2_temp_ref c2=this->manip_top().get_container2();
      return typename _rbase::reverse_iterator(ensure(c1, (typename _super::needed_features1*)0).rend(),
                                               ensure(c2, (typename _super::needed_features2*)0).rend(),
                                               this->manip_top().get_operation());
   }
   typename _rbase::const_reverse_iterator rbegin() const
   {
      return typename _rbase::const_reverse_iterator(ensure(this->manip_top().get_container1(),
                                                            (typename _super::needed_features1*)0).rbegin(),
                                                     ensure(this->manip_top().get_container2(),
                                                            (typename _super::needed_features2*)0).rbegin(),
                                                     this->manip_top().get_operation());
   }
   typename _rbase::const_reverse_iterator rend() const
   {
      return typename _rbase::const_reverse_iterator(ensure(this->manip_top().get_container1(),
                                                            (typename _super::needed_features1*)0).rend(),
                                                     ensure(this->manip_top().get_container2(),
                                                            (typename _super::needed_features2*)0).rend(),
                                                     this->manip_top().get_operation());
   }
};

template <typename Top, typename Params, typename Category, bool _bijective, bool _identity>
class modified_container_pair_elem_access {
   typedef modified_container_pair_typebase<Top,Params> _base;
protected:
   typename _base::manip_top_type& _top()
   {
      return static_cast<modified_container_pair_impl<Top,Params>*>(this)->manip_top();
   }
   const typename _base::manip_top_type& _top() const
   {
      return static_cast<const modified_container_pair_impl<Top,Params>*>(this)->manip_top();
   }
private:
   int _size(False) const { return _top().get_container1().size(); }
   int _size(True) const { return _top().get_container2().size(); }
   int _dim(False) const { return get_dim(_top().get_container1()); }
   int _dim(True) const { return get_dim(_top().get_container2()); }
   bool _empty(False) const { return _top().get_container1().empty(); }
   bool _empty(True) const { return _top().get_container2().empty(); }

   typedef bool2type<object_classifier::what_is<typename deref<typename _base::container1>::type>::value
                     == object_classifier::is_constant>
      discr;
public:
   int size() const { return _size(discr()); }
   int dim() const { return _dim(discr()); }
   bool empty() const { return _empty(discr()); }
};

template <typename Top, typename Params>
class modified_container_pair_elem_access<Top, Params, forward_iterator_tag, true, false>
   : public modified_container_pair_elem_access<Top, Params, input_iterator_tag, true, false> {
   typedef modified_container_pair_typebase<Top, Params> _base;

   typename _base::reference _front(const typename _base::iterator::operation& op, cons<True, True>)
   {
      return op(this->_top().get_container1().front(),
                this->_top().get_container2().front());
   }
   typename _base::const_reference _front(const typename _base::const_iterator::operation& op, cons<True, True>) const
   {
      return op(this->_top().get_container1().front(),
                this->_top().get_container2().front());
   }
   typename _base::reference _front(const typename _base::iterator::operation& op, cons<False, True>)
   {
      return op(this->_top().get_container1().begin(),
                this->_top().get_container2().front());
   }
   typename _base::const_reference _front(const typename _base::const_iterator::operation& op, cons<False, True>) const
   {
      return op(this->_top().get_container1().begin(),
                this->_top().get_container2().front());
   }
   typename _base::reference _front(const typename _base::iterator::operation& op, cons<True, False>)
   {
      return op(this->_top().get_container1().front(),
                this->_top().get_container2().begin());
   }
   typename _base::const_reference _front(const typename _base::const_iterator::operation& op, cons<True, False>) const
   {
      return op(this->_top().get_container1().front(),
                this->_top().get_container2().begin());
   }
   typename _base::reference _front(const typename _base::iterator::operation& op, cons<False, False>)
   {
      return op(this->_top().get_container1().begin(),
                this->_top().get_container2().begin());
   }
   typename _base::const_reference _front(const typename _base::const_iterator::operation& op, cons<False, False>) const
   {
      return op(this->_top().get_container1().begin(),
                this->_top().get_container2().begin());
   }
public:
   typename _base::reference front()
   {
      typedef typename _base::iterator::helper opb;
      return _front(opb::create(this->_top().get_operation()),
                    cons< bool2type<opb::first_data_arg>, bool2type<opb::second_data_arg> >());
   }
   typename _base::const_reference front() const
   {
      typedef typename _base::const_iterator::helper opb;
      return _front(opb::create(this->_top().get_operation()),
                    cons< bool2type<opb::first_data_arg>, bool2type<opb::second_data_arg> >());
   }
};

template <typename Top, typename Params>
class modified_container_pair_elem_access<Top, Params, forward_iterator_tag, true, true>
   : public modified_container_pair_elem_access<Top, Params, input_iterator_tag, true, true> {
   typedef modified_container_pair_typebase<Top, Params> _base;
public:
   typename _base::reference front()
   {
      return this->_top().get_container1().front();
   }
   typename _base::const_reference front() const
   {
      return this->_top().get_container1().front();
   }
};

template <typename Top, typename Params>
class modified_container_pair_elem_access<Top, Params, bidirectional_iterator_tag, true, false>
   : public modified_container_pair_elem_access<Top, Params, forward_iterator_tag, true, false> {
   typedef modified_container_pair_typebase<Top, Params> _base;
   typedef reverse_modified_container_pair_typebase<Top, Params> _rbase;

   typename _base::reference _back(const typename _rbase::reverse_iterator::operation& op, cons<True, True>)
   {
      return op(this->_top().get_container1().back(),
                this->_top().get_container2().back());
   }
   typename _base::const_reference _back(const typename _rbase::const_reverse_iterator::operation& op, cons<True, True>) const
   {
      return op(this->_top().get_container1().back(),
                this->_top().get_container2().back());
   }
   typename _base::reference _back(const typename _rbase::reverse_iterator::operation& op, cons<False, True>)
   {
      return op(this->_top().get_container1().rbegin(),
                this->_top().get_container2().back());
   }
   typename _base::const_reference _back(const typename _rbase::const_reverse_iterator::operation& op, cons<False, True>) const
   {
      return op(this->_top().get_container1().rbegin(),
                this->_top().get_container2().back());
   }
   typename _base::reference _back(const typename _rbase::reverse_iterator::operation& op, cons<True, False>)
   {
      return op(this->_top().get_container1().back(),
                this->_top().get_container2().rbegin());
   }
   typename _base::const_reference _back(const typename _rbase::const_reverse_iterator::operation& op, cons<True, False>) const
   {
      return op(this->_top().get_container1().back(),
                this->_top().get_container2().rbegin());
   }
   typename _base::reference _back(const typename _rbase::reverse_iterator::operation& op, cons<False, False>)
   {
      return op(this->_top().get_container1().rbegin(),
                this->_top().get_container2().rbegin());
   }
   typename _base::const_reference _back(const typename _rbase::const_reverse_iterator::operation& op, cons<False, False>) const
   {
      return op(this->_top().get_container1().rbegin(),
                this->_top().get_container2().rbegin());
   }
public:
   typename _base::reference back()
   {
      typedef typename _rbase::reverse_iterator::helper opb;
      return _back(opb::create(this->_top().get_operation()),
                   cons< bool2type<opb::first_data_arg>, bool2type<opb::second_data_arg> >());
   }
   typename _base::const_reference back() const
   {
      typedef typename _rbase::const_reverse_iterator::helper opb;
      return _back(opb::create(this->_top().get_operation()),
                   cons< bool2type<opb::first_data_arg>, bool2type<opb::second_data_arg> >());
   }
};

template <typename Top, typename Params>
class modified_container_pair_elem_access<Top, Params, bidirectional_iterator_tag, true, true>
   : public modified_container_pair_elem_access<Top, Params, forward_iterator_tag, true, true> {
   typedef modified_container_pair_typebase<Top, Params> _base;
public:
   typename _base::reference back()
   {
      return this->_top().get_container1().back();
   }
   typename _base::const_reference back() const
   {
      return this->_top().get_container1().back();
   }
};

template <typename Top, typename Params>
class modified_container_pair_elem_access<Top, Params, random_access_iterator_tag, true, false>
   : public modified_container_pair_elem_access<Top, Params, bidirectional_iterator_tag, true, false> {
   typedef modified_container_pair_typebase<Top, Params> _base;

   typename _base::reference _random(int i, const typename _base::iterator::operation& op, cons<True, True>)
   {
      return op(this->_top().get_container1()[i],
                this->_top().get_container2()[i]);
   }
   typename _base::const_reference _random(int i, const typename _base::const_iterator::operation& op, cons<True, True>) const
   {
      return op(this->_top().get_container1()[i],
                this->_top().get_container2()[i]);
   }
   typename _base::reference _random(int i, const typename _base::iterator::operation& op, cons<False, True>)
   {
      return op(this->_top().get_container1().begin()+i,
                this->_top().get_container2()[i]);
   }
   typename _base::const_reference _random(int i, const typename _base::const_iterator::operation& op, cons<False, True>) const
   {
      return op(this->_top().get_container1().begin()+i,
                this->_top().get_container2()[i]);
   }
   typename _base::reference _random(int i, const typename _base::iterator::operation& op, cons<True, False>)
   {
      return op(this->_top().get_container1()[i],
                this->_top().get_container2().begin()+i);
   }
   typename _base::const_reference _random(int i, const typename _base::const_iterator::operation& op, cons<True, False>) const
   {
      return op(this->_top().get_container1()[i],
                this->_top().get_container2().begin()+i);
   }
   typename _base::reference _random(int i, const typename _base::iterator::operation& op, cons<False, False>)
   {
      return op(this->_top().get_container1().begin()+i,
                this->_top().get_container2().begin()+i);
   }
   typename _base::const_reference _random(int i, const typename _base::const_iterator::operation& op, cons<False, False>) const
   {
      return op(this->_top().get_container1().begin()+i,
                this->_top().get_container2().begin()+i);
   }
public:
   typename _base::reference operator[] (int i)
   {
      typedef typename _base::iterator::helper opb;
      const bool via_container1=opb::first_data_arg || !iterator_traits<typename _base::container1::iterator>::is_random,
                 via_container2=opb::second_data_arg || !iterator_traits<typename _base::container2::iterator>::is_random;
      return _random(i, opb::create(this->_top().get_operation()),
                     cons< bool2type<via_container1>, bool2type<via_container2> >());
   }
   typename _base::const_reference operator[] (int i) const
   {
      typedef typename _base::const_iterator::helper opb;
      const bool via_container1=opb::first_data_arg || !iterator_traits<typename _base::container1::const_iterator>::is_random,
                 via_container2=opb::second_data_arg || !iterator_traits<typename _base::container2::const_iterator>::is_random;
      return _random(i, opb::create(this->_top().get_operation()),
                     cons< bool2type<via_container1>, bool2type<via_container2> >());
   }
};

template <typename Top, typename Params>
class modified_container_pair_elem_access<Top, Params, random_access_iterator_tag, true, true>
   : public modified_container_pair_elem_access<Top, Params, bidirectional_iterator_tag, true, true> {
   typedef modified_container_pair_typebase<Top, Params> _base;
public:
   typename _base::reference operator[] (int i)
   {
      return this->_top().get_container1()[i];
   }
   typename _base::const_reference operator[] (int i) const
   {
      return this->_top().get_container1()[i];
   }
};

template <typename Top, typename Params, typename Category>
class modified_container_pair_elem_access<Top, Params, Category, false, false>
   : public modified_container_non_bijective_elem_access<Top, modified_container_pair_typebase<Top,Params>,
                                                         derived_from<Category, bidirectional_iterator_tag>::value> {};

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

template <typename Top, typename T=typename Top::element_reference, typename Params=void>
class repeated_value_container_impl
   : public modified_container_pair_impl< Top,
                                          typename concat_list<
                                             cons< Container1< constant_value_container<T> >,
                                             cons< Container2< sequence >,
                                                   Operation< pair<nothing,
                                                                   operations::apply2< BuildUnaryIt<operations::dereference> > > > > >,
                                             Params >::type > {
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
                                          list( Container1< Container >,
                                                Container2< sequence >,
                                                Operation< pair<nothing, operations::apply2< BuildUnaryIt<operations::dereference> > > >,
                                                Hidden< Container > ) > {
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
   typedef typename if_else<identical<typename iterator_traits<typename container_traits<Container>::iterator>::iterator_category,
                                      random_access_iterator_tag>::value,
                            construct_random_indexed<Container>, construct_sequence_indexed<Container>
                           >::type
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
