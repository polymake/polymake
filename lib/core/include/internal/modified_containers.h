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

#include "polymake/internal/singular_containers.h"
#include "polymake/internal/SeriesRaw.h"

namespace pm {

template <typename T>
class prvalue_holder {
public:
   using value_t = pure_type_t<T>;
   using effective_const_value_t = std::conditional_t<object_traits<value_t>::is_always_const, std::add_const_t<T>, T>;
   using ref_t = effective_const_value_t&;

   prvalue_holder() : init(false) {}

   prvalue_holder(const value_t&) = delete;

   prvalue_holder(value_t&& val) : init(true)
   {
      // if the constructor throws an exception, alias' destructor won't be called, hence it's safe to set init=true up front
      new(allocate()) value_t(std::move(val));
   }

   prvalue_holder(const prvalue_holder& other) = delete;

   prvalue_holder(prvalue_holder&& other) : init(other.init)
   {
      if (init) new(allocate()) value_t(std::move(*other.ptr()));
   }

   void reset(value_t&& val)
   {
      if (init) {
         destroy_at(ptr());
         init=false;
      }
      new(allocate()) value_t(std::move(val));
      init=true;
   }

   prvalue_holder& operator= (const prvalue_holder& other) = delete;
   prvalue_holder& operator= (prvalue_holder&& other) = delete;

   ~prvalue_holder()
   {
      if (init) destroy_at(ptr());
   }

   bool is_valid() const { return init; }

   ref_t get_val() { return *ptr(); }
   const value_t& get_val() const { return *ptr(); }

protected:
   alignas(value_t) char area[sizeof(value_t)];
   bool init;

   void* allocate() { return area; }
   value_t*       ptr()       { return reinterpret_cast<value_t*>(area); }
   const value_t* ptr() const { return reinterpret_cast<const value_t*>(area); }
};

template <typename Container, typename FeatureList>
class iterator_over_prvalue
   : private prvalue_holder<Container>
   , public ensure_features<typename prvalue_holder<Container>::effective_const_value_t, FeatureList>::iterator {
   using base_t = prvalue_holder<Container>;
public:
   using iterator_t = typename ensure_features<typename base_t::effective_const_value_t, FeatureList>::iterator;

   iterator_over_prvalue() = default;

   iterator_over_prvalue(Container&& c)
      : base_t(std::move(c))
      , iterator_t(ensure(base_t::get_val(), FeatureList()).begin()) {}

   iterator_over_prvalue(const iterator_over_prvalue&) = delete;

private:
   iterator_over_prvalue(base_t&& other, std::true_type)
      : base_t(std::move(other))
      , iterator_t(base_t::is_valid() ? ensure(base_t::get_val(), FeatureList()).begin() : iterator_t()) {}

   iterator_over_prvalue(base_t&& other, std::false_type)
      : base_t(std::move(other))
      , iterator_t(ensure(base_t::get_val(), FeatureList()).begin()) {}

public:
   iterator_over_prvalue(iterator_over_prvalue&& other)
      : iterator_over_prvalue(std::move(other),
                              std::is_default_constructible<iterator_t>()) {}

   void reset(Container&& c)
   {
      base_t::reset(std::move(c));
      iterator_t::operator=(ensure(base_t::get_val(), FeatureList()).begin());
   }

   iterator_over_prvalue& operator= (const iterator_over_prvalue&) = delete;
   iterator_over_prvalue& operator= (iterator_over_prvalue&&) = delete;

   using base_t::is_valid;
};

// TODO: remove these specializations when entire() is no longer used to produce a temporary iterator passed to copy, fill, constructors, etc.
template <typename ContainerRef, typename FeatureList, typename Feature>
struct check_iterator_feature<iterator_over_prvalue<ContainerRef, FeatureList>, Feature>
   : check_iterator_feature<typename iterator_over_prvalue<ContainerRef, FeatureList>::iterator_t, Feature> {};

template <typename ContainerRef, typename FeatureList>
struct iterator_traits<iterator_over_prvalue<ContainerRef, FeatureList>>
   : iterator_traits<typename iterator_over_prvalue<ContainerRef, FeatureList>::iterator_t> {};


template <typename... MoreFeatures, typename Container>
iterator_over_prvalue<Container, typename mix_features<end_sensitive, typename mlist_wrap<MoreFeatures...>::type>::type>
entire(Container&& c, std::enable_if_t<std::is_rvalue_reference<Container&&>::value, void**> =nullptr)
{
   return std::forward<Container>(c);
}

template <typename... MoreFeatures, typename Container>
iterator_over_prvalue<std::add_const_t<Container>, typename mix_features<end_sensitive, typename mlist_wrap<MoreFeatures...>::type>::type>
entire_const(Container&& c, std::enable_if_t<std::is_rvalue_reference<Container&&>::value, void**> =nullptr)
{
   return std::forward<Container>(c);
}

template <typename... MoreFeatures, typename Container>
auto
entire(Container&& c, std::enable_if_t<std::is_lvalue_reference<Container&&>::value, void**> =nullptr)
{
   return ensure(c, typename mix_features<end_sensitive, typename mlist_wrap<MoreFeatures...>::type>::type()).begin();
}

template <typename... MoreFeatures, typename Container>
auto
entire_const(Container&& c, std::enable_if_t<std::is_lvalue_reference<Container&&>::value, void**> =nullptr)
{
   return ensure(static_cast<std::add_const_t<std::remove_reference_t<Container>>&>(c),
                 typename mix_features<end_sensitive, typename mlist_wrap<MoreFeatures...>::type>::type()).begin();
}

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

template <typename Top, typename Params>
class redirected_container_typebase : public manip_container_top<Top, Params> {
   typedef manip_container_top<Top, Params> base_t;
public:
   using container_ref_raw = typename extract_container_ref<Params, ContainerRefTag, ContainerTag, typename base_t::hidden_type>::type;
   typedef effectively_const_t<container_ref_raw> container_ref;
   typedef typename deref<container_ref>::minus_ref container;
   typedef typename base_t::expected_features needed_features;
   typedef typename ensure_features<container, needed_features>::iterator iterator;
   typedef typename ensure_features<container, needed_features>::const_iterator const_iterator;
   typedef typename enforce_feature_helper<container>::must_enforce_features must_enforce_features;
   typedef typename enforce_feature_helper<container>::can_enforce_features can_enforce_features;
   typedef typename enforce_feature_helper<container>::cannot_enforce_features cannot_enforce_features;
   typedef typename container_traits<container>::category container_category;
   typedef typename container_traits<container>::value_type value_type;
   typedef typename container_traits<container>::reference reference;
   typedef typename container_traits<container>::const_reference const_reference;
   static constexpr int is_resizeable = object_traits<typename deref<container>::type>::is_resizeable;
};

template <typename Top, typename Params,
          bool _enable=redirected_container_typebase<Top,Params>::is_resizeable==1>
class redirected_container_resize {};

template <typename Top, typename Params=typename Top::manipulator_params,
          typename Category=typename redirected_container_typebase<Top,Params>::container_category>
class redirected_container 
   : public redirected_container_typebase<Top, Params>
   , public redirected_container_resize<Top, Params> {
   using base_t = redirected_container_typebase<Top, Params>;
public:
   using manipulator_impl = redirected_container<Top,Params>;
   using manipulator_params = Params;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      using type = redirected_container<FeatureCollector,Params>;
   };

   typename base_t::iterator begin()
   {
      return ensure(this->manip_top().get_container(), typename base_t::needed_features()).begin();
   }
   typename base_t::iterator end()
   {
      return ensure(this->manip_top().get_container(), typename base_t::needed_features()).end();
   }
   typename base_t::const_iterator begin() const
   {
      return ensure(this->manip_top().get_container(), typename base_t::needed_features()).begin();
   }
   typename base_t::const_iterator end() const
   {
      return ensure(this->manip_top().get_container(), typename base_t::needed_features()).end();
   }

   Int size() const { return this->manip_top().get_container().size(); }
   bool empty() const { return this->manip_top().get_container().empty(); }
};

template <typename Top, typename Params>
class redirected_container<Top, Params, forward_iterator_tag>
   : public redirected_container<Top, Params, input_iterator_tag> {
public:
   decltype(auto) front()
   {
      return this->manip_top().get_container().front();
   }
   decltype(auto) front() const
   {
      return this->manip_top().get_container().front();
   }
};

template <typename Top, typename Params>
class redirected_container<Top, Params, bidirectional_iterator_tag>
   : public redirected_container<Top, Params, forward_iterator_tag> {
   using base_t = redirected_container<Top, Params, forward_iterator_tag>;
public:
   using reverse_iterator = typename ensure_features<typename base_t::container, typename base_t::needed_features>::reverse_iterator;
   using const_reverse_iterator = typename ensure_features<typename base_t::container, typename base_t::needed_features>::const_reverse_iterator;

   reverse_iterator rbegin()
   {
      return ensure(this->manip_top().get_container(), typename base_t::needed_features()).rbegin();
   }
   reverse_iterator rend()
   {
      return ensure(this->manip_top().get_container(), typename base_t::needed_features()).rend();
   }
   const_reverse_iterator rbegin() const
   {
      return ensure(this->manip_top().get_container(), typename base_t::needed_features()).rbegin();
   }
   const_reverse_iterator rend() const
   {
      return ensure(this->manip_top().get_container(), typename base_t::needed_features()).rend();
   }

   decltype(auto) back()
   {
      return this->manip_top().get_container().back();
   }
   decltype(auto) back() const
   {
      return this->manip_top().get_container().back();
   }
};

template <typename Top, typename Params>
class redirected_container<Top, Params, random_access_iterator_tag>
   : public redirected_container<Top, Params, bidirectional_iterator_tag> {
public:
   decltype(auto) operator[] (Int i)
   {
      return this->manip_top().get_container()[i];
   }
   decltype(auto) operator[] (Int i) const
   {
      return this->manip_top().get_container()[i];
   }
};

template <typename Top, typename Params>
class redirected_container_resize<Top, Params, true> {
   using master_t = redirected_container<Top, Params>;
public:
   void resize(Int n)
   {
      static_cast<master_t*>(this)->manip_top().get_container().resize(n);
   }
};

template <typename Top, bool is_bidir>
class modified_container_non_bijective_elem_access {
public:
   decltype(auto) front()
   {
      return *static_cast<Top&>(*this).begin();
   }
   decltype(auto) front() const
   {
      return *static_cast<const Top&>(*this).begin();
   }

   Int size() const
   {
      return count_it(static_cast<const Top&>(*this).begin());
   }
   bool empty() const
   {
      return static_cast<const Top&>(*this).begin().at_end();
   }
};

template <typename Top>
class modified_container_non_bijective_elem_access<Top, true>
   : public modified_container_non_bijective_elem_access<Top, false> {
public:
   decltype(auto) back()
   {
      return *static_cast<Top&>(*this).rbegin();
   }
   decltype(auto) back() const
   {
      return *static_cast<const Top&>(*this).rbegin();
   }
};

template <typename Top, typename Params>
class modified_container_typebase
   : public manip_container_top<Top, Params> {
   using base_t = manip_container_top<Top, Params>;
public:
   using container_ref_raw = typename extract_container_ref<Params, ContainerRefTag, ContainerTag, typename base_t::hidden_type>::type;
   typedef effectively_const_t<container_ref_raw> container_ref;
   typedef typename deref<container_ref>::minus_ref container;
   typedef typename mtagged_list_extract<Params, OperationTag>::type operation;
   typedef typename operation_cross_const_helper<operation>::const_operation const_operation;
   typedef typename mtagged_list_extract<Params, IteratorConstructorTag, unary_transform_constructor<> >::type it_constructor;

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

   typedef typename enforce_feature_helper<container>::must_enforce_features must_enforce_features;

   typedef typename iterator_traits<iterator>::value_type value_type;
   typedef typename iterator_traits<iterator>::reference reference;
   typedef typename iterator_traits<const_iterator>::reference const_reference;
};

template <typename Top, typename Params>
class reverse_modified_container_typebase {
   typedef modified_container_typebase<Top, Params> base_t;
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

template <typename Top, typename Params,
          typename Category=typename modified_container_typebase<Top, Params>::container_category,
          bool TBijective=is_bijective<typename modified_container_typebase<Top, Params>::it_constructor>::value,
          bool TIdentity=is_identity_transform<typename modified_container_typebase<Top, Params>::operation>::value>
class modified_container_elem_access;

template <typename Top, typename Params=typename Top::manipulator_params,
          bool TReversible=is_derived_from<typename modified_container_typebase<Top, Params>::container_category,
                                           bidirectional_iterator_tag>::value>
class modified_container_impl
   : public modified_container_typebase<Top, Params>
   , public modified_container_elem_access<Top, Params> {
   typedef modified_container_typebase<Top, Params> base_t;
public:
   typedef modified_container_impl<Top, Params> manipulator_impl;
   typedef Params manipulator_params;
   typedef typename base_t::iterator iterator;
   typedef typename base_t::const_iterator const_iterator;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef modified_container_impl<FeatureCollector, Params> type;
   };

   typename is_identity_transform<typename base_t::operation>::type get_operation() const
   {
      return typename is_identity_transform<typename base_t::operation>::type();
   }

   iterator begin()
   {
      return iterator(ensure(this->manip_top().get_container(), typename base_t::needed_features()).begin(),
                      this->manip_top().get_operation());
   }
   iterator end()
   {
      return iterator(ensure(this->manip_top().get_container(), typename base_t::needed_features()).end(),
                      this->manip_top().get_operation());
   }
   const_iterator begin() const
   {
      return const_iterator(ensure(this->manip_top().get_container(), typename base_t::needed_features()).begin(),
                            this->manip_top().get_operation());
   }
   const_iterator end() const
   {
      return const_iterator(ensure(this->manip_top().get_container(), typename base_t::needed_features()).end(),
                            this->manip_top().get_operation());
   }
};

template <typename Top, typename Params>
class modified_container_impl<Top, Params, true>
   : public modified_container_impl<Top, Params, false>,
     public reverse_modified_container_typebase<Top, Params> {
   typedef modified_container_impl<Top, Params, false> base_t;
   typedef reverse_modified_container_typebase<Top, Params> rbase_t;
public:
   typename rbase_t::reverse_iterator rbegin()
   {
      return typename rbase_t::reverse_iterator(ensure(this->manip_top().get_container(), typename base_t::needed_features()).rbegin(),
                                                this->manip_top().get_operation());
   }
   typename rbase_t::reverse_iterator rend()
   {
      return typename rbase_t::reverse_iterator(ensure(this->manip_top().get_container(), typename base_t::needed_features()).rend(),
                                                this->manip_top().get_operation());
   }
   typename rbase_t::const_reverse_iterator rbegin() const
   {
      return typename rbase_t::const_reverse_iterator(ensure(this->manip_top().get_container(), typename base_t::needed_features()).rbegin(),
                                                      this->manip_top().get_operation());
   }
   typename rbase_t::const_reverse_iterator rend() const
   {
      return typename rbase_t::const_reverse_iterator(ensure(this->manip_top().get_container(), typename base_t::needed_features()).rend(),
                                                      this->manip_top().get_operation());
   }
};

template <typename Top, typename Params, typename Category, bool is_bijective, bool is_identity>
class modified_container_elem_access {
   typedef modified_container_typebase<Top, Params> base_t;
protected:
   typename base_t::manip_top_type& _top()
   {
      return static_cast<modified_container_impl<Top, Params>*>(this)->manip_top();
   }
   const typename base_t::manip_top_type& _top() const
   {
      return static_cast<const modified_container_impl<Top, Params>*>(this)->manip_top();
   }
public:
   Int size() const
   {
      return _top().get_container().size();
   }
   Int dim() const
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
   typedef modified_container_typebase<Top, Params> base_t;

   decltype(auto) front_impl(const typename base_t::iterator::operation& op, std::true_type)
   {
      return op(this->_top().get_container().front());
   }
   decltype(auto) front_impl(const typename base_t::const_iterator::operation& op, std::true_type) const
   {
      return op(this->_top().get_container().front());
   }
   decltype(auto) front_impl(const typename base_t::iterator::operation& op, std::false_type)
   {
      return op(this->_top().get_container().begin());
   }
   decltype(auto) front_impl(const typename base_t::const_iterator::operation& op, std::false_type) const
   {
      return op(this->_top().get_container().begin());
   }
public:
   decltype(auto) front()
   {
      typedef typename base_t::iterator::helper opb;
      return front_impl(opb::create(this->_top().get_operation()), bool_constant<opb::data_arg>());
   }
   decltype(auto) front() const
   {
      typedef typename base_t::const_iterator::helper opb;
      return front_impl(opb::create(this->_top().get_operation()), bool_constant<opb::data_arg>());
   }
};

template <typename Top, typename Params>
class modified_container_elem_access<Top, Params, forward_iterator_tag, true, true>
   : public modified_container_elem_access<Top, Params, input_iterator_tag, true, true> {
public:
   decltype(auto) front()
   {
      return this->_top().get_container().front();
   }
   decltype(auto) front() const
   {
      return this->_top().get_container().front();
   }
};

template <typename Top, typename Params>
class modified_container_elem_access<Top, Params, bidirectional_iterator_tag, true, false>
   : public modified_container_elem_access<Top, Params, forward_iterator_tag, true, false> {
   typedef modified_container_typebase<Top, Params> base_t;
   typedef reverse_modified_container_typebase<Top, Params> rbase_t;

   decltype(auto) back_impl(const typename rbase_t::reverse_iterator::operation& op, std::true_type)
   {
      return op(this->_top().get_container().back());
   }
   decltype(auto) back_impl(const typename rbase_t::const_reverse_iterator::operation& op, std::true_type) const
   {
      return op(this->_top().get_container().back());
   }
   decltype(auto) back_impl(const typename rbase_t::reverse_iterator::operation& op, std::false_type)
   {
      return op(this->_top().get_container().rbegin());
   }
   decltype(auto) back_impl(const typename rbase_t::const_reverse_iterator::operation& op, std::false_type) const
   {
      return op(this->_top().get_container().rbegin());
   }
public:
   decltype(auto) back()
   {
      typedef typename rbase_t::reverse_iterator::helper opb;
      return back_impl(opb::create(this->_top().get_operation()), bool_constant<opb::data_arg>());
   }
   decltype(auto) back() const
   {
      typedef typename rbase_t::const_reverse_iterator::helper opb;
      return back_impl(opb::create(this->_top().get_operation()), bool_constant<opb::data_arg>());
   }
};

template <typename Top, typename Params>
class modified_container_elem_access<Top, Params, bidirectional_iterator_tag, true, true>
   : public modified_container_elem_access<Top, Params, forward_iterator_tag, true, true> {
public:
   decltype(auto) back()
   {
      return this->_top().get_container().back();
   }
   decltype(auto) back() const
   {
      return this->_top().get_container().back();
   }
};

template <typename Top, typename Params>
class modified_container_elem_access<Top, Params, random_access_iterator_tag, true, false>
   : public modified_container_elem_access<Top, Params, bidirectional_iterator_tag, true, false> {
   using base_t = modified_container_typebase<Top, Params>;

   decltype(auto) elem_by_index(Int i, const typename base_t::iterator::operation& op, std::true_type)
   {
      return op(this->_top().get_container()[i]);
   }
   decltype(auto) elem_by_index(Int i, const typename base_t::const_iterator::operation& op, std::true_type) const
   {
      return op(this->_top().get_container()[i]);
   }
   decltype(auto) elem_by_index(Int i, const typename base_t::iterator::operation& op, std::false_type)
   {
      return op(this->_top().get_container().begin() + i);
   }
   decltype(auto) elem_by_index(Int i, const typename base_t::const_iterator::operation& op, std::false_type) const
   {
      return op(this->_top().get_container().begin() + i);
   }
public:
   decltype(auto) operator[] (Int i)
   {
      typedef typename base_t::iterator::helper opb;
      const bool via_container=opb::data_arg || !iterator_traits<typename base_t::container::iterator>::is_random;
      return elem_by_index(i, opb::create(this->_top().get_operation()), bool_constant<via_container>());
   }
   decltype(auto) operator[] (Int i) const
   {
      typedef typename base_t::const_iterator::helper opb;
      const bool via_container=opb::data_arg || !iterator_traits<typename base_t::container::const_iterator>::is_random;
      return elem_by_index(i, opb::create(this->_top().get_operation()), bool_constant<via_container>());
   }
};

template <typename Top, typename Params>
class modified_container_elem_access<Top, Params, random_access_iterator_tag, true, true>
   : public modified_container_elem_access<Top, Params, bidirectional_iterator_tag, true, true> {
public:
   decltype(auto) operator[] (Int i)
   {
      return this->_top().get_container()[i];
   }
   decltype(auto) operator[] (Int i) const
   {
      return this->_top().get_container()[i];
   }
};

template <typename Top, typename Params, typename Category>
class modified_container_elem_access<Top, Params, Category, false, false>
   : public modified_container_non_bijective_elem_access<Top, is_derived_from<Category, bidirectional_iterator_tag>::value> {};

template <typename Top, typename Params>
class container_pair_typebase : public manip_container_top<Top, Params> {
   typedef manip_container_top<Top, Params> base_t;
public:
   using container1_ref_raw = typename extract_container_ref<Params, Container1RefTag, Container1Tag>::type;
   using container2_ref_raw = typename extract_container_ref<Params, Container2RefTag, Container2Tag>::type;
   typedef effectively_const_t<container1_ref_raw> container1_ref;
   typedef effectively_const_t<container2_ref_raw> container2_ref;
   typedef typename deref<container1_ref>::minus_ref container1;
   typedef typename deref<container2_ref>::minus_ref container2;

   typedef typename mtagged_list_extract<Params, IteratorCouplerTag, pair_coupler<> >::type it_coupler;
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
   typedef typename mix_features<typename enforce_feature_helper<container1>::must_enforce_features,
                                 typename enforce_feature_helper<container2>::must_enforce_features>::type
      must_enforce_features;

   typedef typename iterator_traits<iterator>::value_type value_type;
   typedef typename iterator_traits<iterator>::reference reference;
   typedef typename iterator_traits<const_iterator>::reference const_reference;
};

template <typename IteratorCoupler>
struct reverse_coupler {
   using type = IteratorCoupler;
};

template <typename IteratorCoupler, typename Params, bool is_reversed=mlist_contains<Params, reversed>::value>
struct reverse_coupler_helper {
   using type = IteratorCoupler;
};

template <typename IteratorCoupler, typename Params>
struct reverse_coupler_helper<IteratorCoupler, Params, true> : reverse_coupler<IteratorCoupler> {};

template <typename Top, typename Params>
class reverse_container_pair_typebase {
   typedef container_pair_typebase<Top, Params> base_t;
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

template <typename Top, typename Params=typename Top::manipulator_params,
          typename Category=typename container_pair_typebase<Top, Params>::container_category>
class container_pair_impl
   : public container_pair_typebase<Top, Params> {
   typedef container_pair_typebase<Top, Params> base_t;
   Int size_impl(std::false_type) const { return this->manip_top().get_container1().size(); }
   Int size_impl(std::true_type) const { return this->manip_top().get_container2().size(); }
   Int dim_impl(std::false_type) const { return get_dim(this->manip_top().get_container1()); }
   Int dim_impl(std::true_type) const { return get_dim(this->manip_top().get_container2()); }
   bool empty_impl(std::false_type) const { return this->manip_top().get_container1().empty(); }
   bool empty_impl(std::true_type) const { return this->manip_top().get_container2().empty(); }

   typedef bool_constant<object_classifier::what_is<typename deref<typename base_t::container1>::type>::value
                         == object_classifier::is_constant> unlimited1;
public:
   typedef container_pair_impl<Top, Params> manipulator_impl;
   typedef Params manipulator_params;
   using typename base_t::iterator;
   using typename base_t::const_iterator;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef container_pair_impl<FeatureCollector, Params> type;
   };

   iterator begin()
   {
      return iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).begin(),
                      ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).begin());
   }
   iterator end()
   {
      return iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).end(),
                      ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).end());
   }
   const_iterator begin() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).begin(),
                            ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).begin());
   }
   const_iterator end() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).end(),
                            ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).end());
   }

   Int size() const { return size_impl(unlimited1()); }
   Int dim() const { return dim_impl(unlimited1()); }
   bool empty() const { return empty_impl(unlimited1()); }
};

template <typename Top, typename Params>
class container_pair_impl<Top, Params, forward_iterator_tag>
   : public container_pair_impl<Top, Params, input_iterator_tag> {
public:
   decltype(auto) front()
   {
      return this->manip_top().get_container1().front();
   }
   decltype(auto) front() const
   {
      return this->manip_top().get_container1().front();
   }
};

template <typename Top, typename Params>
class container_pair_impl<Top, Params, bidirectional_iterator_tag>
   : public container_pair_impl<Top, Params, forward_iterator_tag>,
     public reverse_container_pair_typebase<Top, Params> {
   typedef container_pair_impl<Top, Params, forward_iterator_tag> base_t;
   typedef reverse_container_pair_typebase<Top, Params> rbase_t;
public:
   typename rbase_t::reverse_iterator rbegin()
   {
      return typename rbase_t::reverse_iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).rbegin(),
                                                ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).rbegin());
   }
   typename rbase_t::reverse_iterator rend()
   {
      return typename rbase_t::reverse_iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).rend(),
                                                ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).rend());
   }
   typename rbase_t::const_reverse_iterator rbegin() const
   {
      return typename rbase_t::const_reverse_iterator(ensure(this->manip_top().get_container1(),
                                                             typename base_t::needed_features1()).rbegin(),
                                                      ensure(this->manip_top().get_container2(),
                                                             typename base_t::needed_features2()).rbegin());
   }
   typename rbase_t::const_reverse_iterator rend() const
   {
      return typename rbase_t::const_reverse_iterator(ensure(this->manip_top().get_container1(),
                                                             typename base_t::needed_features1()).rend(),
                                                      ensure(this->manip_top().get_container2(),
                                                             typename base_t::needed_features2()).rend());
   }

   decltype(auto) back()
   {
      return this->manip_top().get_container1().back();
   }
   decltype(auto) back() const
   {
      return this->manip_top().get_container1().back();
   }
};

template <typename Top, typename Params>
class container_pair_impl<Top, Params, random_access_iterator_tag>
   : public container_pair_impl<Top, Params, bidirectional_iterator_tag> {
public:
   decltype(auto) operator[] (Int i)
   {
      return this->manip_top().get_container1()[i];
   }
   decltype(auto) operator[] (Int i) const
   {
      return this->manip_top().get_container1()[i];
   }
};

template <typename Top, typename Params>
class modified_container_pair_typebase
   : public manip_container_top<Top, Params> {
   typedef manip_container_top<Top, Params> base_t;
public:
   using container1_ref_raw = typename extract_container_ref<Params, Container1RefTag, Container1Tag>::type;
   using container2_ref_raw = typename extract_container_ref<Params, Container2RefTag, Container2Tag>::type;
   typedef effectively_const_t<container1_ref_raw> container1_ref;
   typedef effectively_const_t<container2_ref_raw> container2_ref;
   typedef typename deref<container1_ref>::minus_ref container1;
   typedef typename deref<container2_ref>::minus_ref container2;

   typedef typename mtagged_list_extract<Params, OperationTag>::type operation;
   typedef typename operation_cross_const_helper<operation>::const_operation const_operation;
   typedef typename mtagged_list_extract<Params, IteratorCouplerTag, pair_coupler<> >::type it_coupler;
   typedef typename it_coupler::template defs<typename container_traits<container1>::iterator,
                                              typename container_traits<container2>::iterator>
      coupler_defs;
   typedef typename mtagged_list_extract<Params, IteratorConstructorTag, binary_transform_constructor<> >::type it_constructor;

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

   typedef typename mix_features<typename enforce_feature_helper<container1>::must_enforce_features,
                                 typename enforce_feature_helper<container2>::must_enforce_features>::type
      must_enforce_features;

   typedef typename iterator_traits<iterator>::value_type value_type;
   typedef typename iterator_traits<iterator>::reference reference;
   typedef typename iterator_traits<const_iterator>::reference const_reference;
};

template <typename Top, typename Params>
class reverse_modified_container_pair_typebase {
   typedef modified_container_pair_typebase<Top, Params> base_t;
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

template <typename Top, typename Params,
          typename Category=typename modified_container_pair_typebase<Top, Params>::container_category,
          bool is_bijective=is_bijective<typename modified_container_pair_typebase<Top, Params>::it_constructor>::value,
          bool is_identity=is_identity_transform<typename modified_container_pair_typebase<Top, Params>::operation>::value>
class modified_container_pair_elem_access;

template <typename Top, typename Params=typename Top::manipulator_params,
          bool is_bidir=is_derived_from<typename modified_container_pair_typebase<Top, Params>::container_category,
                                        bidirectional_iterator_tag>::value>
class modified_container_pair_impl
   : public modified_container_pair_typebase<Top, Params>
   , public modified_container_pair_elem_access<Top, Params> {
   using base_t = modified_container_pair_typebase<Top, Params>;
public:
   typedef modified_container_pair_impl<Top, Params> manipulator_impl;
   typedef Params manipulator_params;
   using typename base_t::iterator;
   using typename base_t::const_iterator;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef modified_container_pair_impl<FeatureCollector, Params> type;
   };

   typename is_identity_transform<typename base_t::operation>::type get_operation() const
   {
      return typename is_identity_transform<typename base_t::operation>::type();
   }

   iterator begin()
   {
      return iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).begin(),
                      ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).begin(),
                      this->manip_top().get_operation());
   }
   iterator end()
   {
      return iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).end(),
                      ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).end(),
                      this->manip_top().get_operation());
   }
   const_iterator begin() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).begin(),
                            ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).begin(),
                            this->manip_top().get_operation());
   }
   const_iterator end() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).end(),
                            ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).end(),
                            this->manip_top().get_operation());
   }
};

template <typename Top, typename Params>
class modified_container_pair_impl<Top, Params, true>
   : public modified_container_pair_impl<Top, Params, false>,
     public reverse_modified_container_pair_typebase<Top, Params> {
   typedef modified_container_pair_impl<Top, Params, false> base_t;
   typedef reverse_modified_container_pair_typebase<Top, Params> rbase_t;
public:
   typename rbase_t::reverse_iterator rbegin()
   {
      return typename rbase_t::reverse_iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).rbegin(),
                                                ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).rbegin(),
                                                this->manip_top().get_operation());
   }
   typename rbase_t::reverse_iterator rend()
   {
      return typename rbase_t::reverse_iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).rend(),
                                                ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).rend(),
                                                this->manip_top().get_operation());
   }
   typename rbase_t::const_reverse_iterator rbegin() const
   {
      return typename rbase_t::const_reverse_iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).rbegin(),
                                                      ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).rbegin(),
                                                      this->manip_top().get_operation());
   }
   typename rbase_t::const_reverse_iterator rend() const
   {
      return typename rbase_t::const_reverse_iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).rend(),
                                                      ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).rend(),
                                                      this->manip_top().get_operation());
   }
};

template <typename Top, typename Params, typename Category, bool is_bijective, bool is_identity>
class modified_container_pair_elem_access {
   typedef modified_container_pair_typebase<Top, Params> base_t;
protected:
   typename base_t::manip_top_type& _top()
   {
      return static_cast<modified_container_pair_impl<Top, Params>*>(this)->manip_top();
   }
   const typename base_t::manip_top_type& _top() const
   {
      return static_cast<const modified_container_pair_impl<Top, Params>*>(this)->manip_top();
   }
private:
   Int size_impl(std::false_type) const { return _top().get_container1().size(); }
   Int size_impl(std::true_type) const { return _top().get_container2().size(); }
   Int dim_impl(std::false_type) const { return get_dim(_top().get_container1()); }
   Int dim_impl(std::true_type) const { return get_dim(_top().get_container2()); }
   bool empty_impl(std::false_type) const { return _top().get_container1().empty(); }
   bool empty_impl(std::true_type) const { return _top().get_container2().empty(); }

   typedef bool_constant<(object_classifier::what_is<typename deref<typename base_t::container1>::type>::value
                          == object_classifier::is_constant)> unlimited1;
public:
   Int size() const { return size_impl(unlimited1()); }
   Int dim() const { return dim_impl(unlimited1()); }
   bool empty() const { return empty_impl(unlimited1()); }
};

template <typename Top, typename Params>
class modified_container_pair_elem_access<Top, Params, forward_iterator_tag, true, false>
   : public modified_container_pair_elem_access<Top, Params, input_iterator_tag, true, false> {
   typedef modified_container_pair_typebase<Top, Params> base_t;

   decltype(auto) front_impl(const typename base_t::iterator::operation& op, std::true_type, std::true_type)
   {
      return op(this->_top().get_container1().front(),
                this->_top().get_container2().front());
   }
   decltype(auto) front_impl(const typename base_t::const_iterator::operation& op, std::true_type, std::true_type) const
   {
      return op(this->_top().get_container1().front(),
                this->_top().get_container2().front());
   }
   decltype(auto) front_impl(const typename base_t::iterator::operation& op, std::false_type, std::true_type)
   {
      return op(this->_top().get_container1().begin(),
                this->_top().get_container2().front());
   }
   decltype(auto) front_impl(const typename base_t::const_iterator::operation& op, std::false_type, std::true_type) const
   {
      return op(this->_top().get_container1().begin(),
                this->_top().get_container2().front());
   }
   decltype(auto) front_impl(const typename base_t::iterator::operation& op, std::true_type, std::false_type)
   {
      return op(this->_top().get_container1().front(),
                this->_top().get_container2().begin());
   }
   decltype(auto) front_impl(const typename base_t::const_iterator::operation& op, std::true_type, std::false_type) const
   {
      return op(this->_top().get_container1().front(),
                this->_top().get_container2().begin());
   }
   decltype(auto) front_impl(const typename base_t::iterator::operation& op, std::false_type, std::false_type)
   {
      return op(this->_top().get_container1().begin(),
                this->_top().get_container2().begin());
   }
   decltype(auto) front_impl(const typename base_t::const_iterator::operation& op, std::false_type, std::false_type) const
   {
      return op(this->_top().get_container1().begin(),
                this->_top().get_container2().begin());
   }
public:
   decltype(auto) front()
   {
      typedef typename base_t::iterator::helper opb;
      return front_impl(opb::create(this->_top().get_operation()),
                        bool_constant<opb::first_data_arg>(), bool_constant<opb::second_data_arg>());
   }
   decltype(auto) front() const
   {
      typedef typename base_t::const_iterator::helper opb;
      return front_impl(opb::create(this->_top().get_operation()),
                        bool_constant<opb::first_data_arg>(), bool_constant<opb::second_data_arg>());
   }
};

template <typename Top, typename Params>
class modified_container_pair_elem_access<Top, Params, forward_iterator_tag, true, true>
   : public modified_container_pair_elem_access<Top, Params, input_iterator_tag, true, true> {
public:
   decltype(auto) front()
   {
      return this->_top().get_container1().front();
   }
   decltype(auto) front() const
   {
      return this->_top().get_container1().front();
   }
};

template <typename Top, typename Params>
class modified_container_pair_elem_access<Top, Params, bidirectional_iterator_tag, true, false>
   : public modified_container_pair_elem_access<Top, Params, forward_iterator_tag, true, false> {
   typedef modified_container_pair_typebase<Top, Params> base_t;
   typedef reverse_modified_container_pair_typebase<Top, Params> rbase_t;

   decltype(auto) back_impl(const typename rbase_t::reverse_iterator::operation& op, std::true_type, std::true_type)
   {
      return op(this->_top().get_container1().back(),
                this->_top().get_container2().back());
   }
   decltype(auto) back_impl(const typename rbase_t::const_reverse_iterator::operation& op, std::true_type, std::true_type) const
   {
      return op(this->_top().get_container1().back(),
                this->_top().get_container2().back());
   }
   decltype(auto) back_impl(const typename rbase_t::reverse_iterator::operation& op, std::false_type, std::true_type)
   {
      return op(this->_top().get_container1().rbegin(),
                this->_top().get_container2().back());
   }
   decltype(auto) back_impl(const typename rbase_t::const_reverse_iterator::operation& op, std::false_type, std::true_type) const
   {
      return op(this->_top().get_container1().rbegin(),
                this->_top().get_container2().back());
   }
   decltype(auto) back_impl(const typename rbase_t::reverse_iterator::operation& op, std::true_type, std::false_type)
   {
      return op(this->_top().get_container1().back(),
                this->_top().get_container2().rbegin());
   }
   decltype(auto) back_impl(const typename rbase_t::const_reverse_iterator::operation& op, std::true_type, std::false_type) const
   {
      return op(this->_top().get_container1().back(),
                this->_top().get_container2().rbegin());
   }
   decltype(auto) back_impl(const typename rbase_t::reverse_iterator::operation& op, std::false_type, std::false_type)
   {
      return op(this->_top().get_container1().rbegin(),
                this->_top().get_container2().rbegin());
   }
   decltype(auto) back_impl(const typename rbase_t::const_reverse_iterator::operation& op, std::false_type, std::false_type) const
   {
      return op(this->_top().get_container1().rbegin(),
                this->_top().get_container2().rbegin());
   }
public:
   decltype(auto) back()
   {
      typedef typename rbase_t::reverse_iterator::helper opb;
      return back_impl(opb::create(this->_top().get_operation()),
                       bool_constant<opb::first_data_arg>(), bool_constant<opb::second_data_arg>());
   }
   decltype(auto) back() const
   {
      typedef typename rbase_t::const_reverse_iterator::helper opb;
      return back_impl(opb::create(this->_top().get_operation()),
                       bool_constant<opb::first_data_arg>(), bool_constant<opb::second_data_arg>());
   }
};

template <typename Top, typename Params>
class modified_container_pair_elem_access<Top, Params, bidirectional_iterator_tag, true, true>
   : public modified_container_pair_elem_access<Top, Params, forward_iterator_tag, true, true> {
public:
   decltype(auto) back()
   {
      return this->_top().get_container1().back();
   }
   decltype(auto) back() const
   {
      return this->_top().get_container1().back();
   }
};

template <typename Top, typename Params>
class modified_container_pair_elem_access<Top, Params, random_access_iterator_tag, true, false>
   : public modified_container_pair_elem_access<Top, Params, bidirectional_iterator_tag, true, false> {
   typedef modified_container_pair_typebase<Top, Params> base_t;

   decltype(auto) elem_by_index(Int i, const typename base_t::iterator::operation& op, std::true_type, std::true_type)
   {
      return op(this->_top().get_container1()[i],
                this->_top().get_container2()[i]);
   }
   decltype(auto) elem_by_index(Int i, const typename base_t::const_iterator::operation& op, std::true_type, std::true_type) const
   {
      return op(this->_top().get_container1()[i],
                this->_top().get_container2()[i]);
   }
   decltype(auto) elem_by_index(Int i, const typename base_t::iterator::operation& op, std::false_type, std::true_type)
   {
      return op(this->_top().get_container1().begin()+i,
                this->_top().get_container2()[i]);
   }
   decltype(auto) elem_by_index(Int i, const typename base_t::const_iterator::operation& op, std::false_type, std::true_type) const
   {
      return op(this->_top().get_container1().begin()+i,
                this->_top().get_container2()[i]);
   }
   decltype(auto) elem_by_index(Int i, const typename base_t::iterator::operation& op, std::true_type, std::false_type)
   {
      return op(this->_top().get_container1()[i],
                this->_top().get_container2().begin()+i);
   }
   decltype(auto) elem_by_index(Int i, const typename base_t::const_iterator::operation& op, std::true_type, std::false_type) const
   {
      return op(this->_top().get_container1()[i],
                this->_top().get_container2().begin()+i);
   }
   decltype(auto) elem_by_index(Int i, const typename base_t::iterator::operation& op, std::false_type, std::false_type)
   {
      return op(this->_top().get_container1().begin()+i,
                this->_top().get_container2().begin()+i);
   }
   decltype(auto) elem_by_index(Int i, const typename base_t::const_iterator::operation& op, std::false_type, std::false_type) const
   {
      return op(this->_top().get_container1().begin()+i,
                this->_top().get_container2().begin()+i);
   }
public:
   decltype(auto) operator[] (Int i)
   {
      typedef typename base_t::iterator::helper opb;
      const bool via_container1=opb::first_data_arg || !iterator_traits<typename base_t::container1::iterator>::is_random,
                 via_container2=opb::second_data_arg || !iterator_traits<typename base_t::container2::iterator>::is_random;
      return elem_by_index(i, opb::create(this->_top().get_operation()),
                           bool_constant<via_container1>(), bool_constant<via_container2>());
   }
   decltype(auto) operator[] (Int i) const
   {
      typedef typename base_t::const_iterator::helper opb;
      const bool via_container1=opb::first_data_arg || !iterator_traits<typename base_t::container1::const_iterator>::is_random,
                 via_container2=opb::second_data_arg || !iterator_traits<typename base_t::container2::const_iterator>::is_random;
      return elem_by_index(i, opb::create(this->_top().get_operation()),
                           bool_constant<via_container1>(), bool_constant<via_container2>());
   }
};

template <typename Top, typename Params>
class modified_container_pair_elem_access<Top, Params, random_access_iterator_tag, true, true>
   : public modified_container_pair_elem_access<Top, Params, bidirectional_iterator_tag, true, true> {
public:
   decltype(auto) operator[] (Int i)
   {
      return this->_top().get_container1()[i];
   }
   decltype(auto) operator[] (Int i) const
   {
      return this->_top().get_container1()[i];
   }
};

template <typename Top, typename Params, typename Category>
class modified_container_pair_elem_access<Top, Params, Category, false, false>
   : public modified_container_non_bijective_elem_access<Top, is_derived_from<Category, bidirectional_iterator_tag>::value> {};

template <typename ContainerRef, typename Operation>
class modified_container_base {
protected:
   using alias_t = alias<ContainerRef>;
   alias_t src;
   typedef typename is_identity_transform<Operation>::type operation_type;
   operation_type op;
public:
   modified_container_base() = default;

   template <typename SrcArg, typename... OpArgs,
             typename=std::enable_if_t<std::is_constructible<alias_t, SrcArg>::value &&
                                       std::is_constructible<operation_type, OpArgs...>::value> >
   explicit modified_container_base(SrcArg&& src_arg, OpArgs&&... op_args)
      : src(std::forward<SrcArg>(src_arg))
      , op(std::forward<OpArgs>(op_args)...) {}

   decltype(auto) get_container() { return *src; }
   decltype(auto) get_container() const { return *src; }
   const alias_t& get_container_alias() const { return src; }
   const operation_type& get_operation() const { return op; }
};

template <typename ContainerRef1, typename ContainerRef2>
class container_pair_base {
protected:
   using first_alias_t = alias<ContainerRef1>;
   using second_alias_t = alias<ContainerRef2>;
   first_alias_t src1;
   second_alias_t src2;
public:
   container_pair_base() = default;

   template <typename Arg1, typename Arg2,
             typename=std::enable_if_t<std::is_constructible<first_alias_t, Arg1>::value &&
                                       std::is_constructible<second_alias_t, Arg2>::value>>
   container_pair_base(Arg1&& src1_arg, Arg2&& src2_arg)
      : src1(std::forward<Arg1>(src1_arg))
      , src2(std::forward<Arg2>(src2_arg)) {}

   decltype(auto) get_container1() { return *src1; }
   decltype(auto) get_container2() { return *src2; }
   decltype(auto) get_container1() const { return *src1; }
   decltype(auto) get_container2() const { return *src2; }
   const first_alias_t& get_container1_alias() const { return src1; }
   const second_alias_t& get_container2_alias() const { return src2; }
};

template <typename ContainerRef1, typename ContainerRef2, typename Operation>
class modified_container_pair_base
   : public container_pair_base<ContainerRef1, ContainerRef2> {
   using base_t = container_pair_base<ContainerRef1, ContainerRef2>;
protected:
   typedef typename is_identity_transform<Operation>::type operation_type;
   operation_type op;
public:
   modified_container_pair_base() = default;

   template <typename Arg1, typename Arg2, typename... OpArgs,
             typename=std::enable_if_t<std::is_constructible<base_t, Arg1, Arg2>::value &&
                                       std::is_constructible<operation_type, OpArgs...>::value>>
   modified_container_pair_base(Arg1&& src1_arg, Arg2&& src2_arg, OpArgs&&... op_args)
      : base_t(std::forward<Arg1>(src1_arg), std::forward<Arg2>(src2_arg))
      , op(std::forward<OpArgs>(op_args)...) {}

   const operation_type& get_operation() const { return op; }
};

template <typename Top, typename ElemRef=typename Top::element_reference, typename Params=mlist<>>
class repeated_value_container_impl
   : public modified_container_pair_impl< Top,
                                          typename mlist_concat<
                                             Container1RefTag< same_value_container<ElemRef> >,
                                             Container2RefTag< sequence_raw >,
                                             OperationTag< pair<nothing,
                                                                operations::apply2< BuildUnaryIt<operations::dereference> > > >,
                                             Params >::type > {
public:
   using element_reference = ElemRef;

   decltype(auto) get_container1() const
   {
      return as_same_value_container(this->manip_top().get_elem_alias());
   }
   sequence_raw get_container2() const
   {
      return sequence_raw(0, this->manip_top().size());
   }

   decltype(auto) front() const { return this->manip_top().get_container1().front(); }
   decltype(auto) back() const { return front(); }
};

template <typename ElemRef>
class repeated_value_container
   : public repeated_value_container_impl<repeated_value_container<ElemRef>, ElemRef> {
protected:
   using alias_t = alias<ElemRef>;
   alias_t value;
   Int d;
public:
   // TODO: remove this when iterators stop outliving containers
   repeated_value_container()
      : d(0) {}

   template <typename Arg, typename=std::enable_if_t<std::is_constructible<alias_t, Arg>::value>>
   repeated_value_container(Arg&& value_arg, Int dim_arg)
      : value(std::forward<Arg>(value_arg))
      , d(dim_arg)
   {
      if (POLYMAKE_DEBUG && dim_arg<0)
         throw std::runtime_error("repeated_value_container - invalid dimension");
   }

   alias_t& get_elem_alias() { return value; }
   const alias_t& get_elem_alias() const { return value; }

   Int dim() const { return d; }
   Int size() const { return d; }
   bool empty() const { return d==0; }

   void stretch_dim(Int to_dim)
   {
      d = to_dim;
   }
};

template <typename ElemRef>
struct spec_object_traits< repeated_value_container<ElemRef> >
   : spec_object_traits<is_container> {
   static constexpr bool
      is_temporary = true,
      is_always_const = true;
};

template <typename E>
auto repeat_value(E&& value, Int count)
{
   return repeated_value_container<prevent_int_element<E>>(std::forward<E>(value), count);
}

template <typename E>
auto single_value_as_container(E&& value)
{
   return repeated_value_container<prevent_int_element<E>>(std::forward<E>(value), 1);
}

template <typename Container, int kind = object_classifier::what_is<Container>::value>
struct extract_expected_features {
   using type = mlist<>;
};

template <typename Container>
struct extract_expected_features<Container, object_classifier::is_manip> {
   using type = typename Container::expected_features;
};

template <typename Container>
class construct_sequence_indexed
   : public modified_container_pair_impl< construct_sequence_indexed<Container>,
                                          mlist< Container1Tag< Container >,
                                                 Container2Tag< sequence_raw >,
                                                 OperationTag< pair<nothing, operations::apply2< BuildUnaryIt<operations::dereference> > > >,
                                                 ExpectedFeaturesTag< typename extract_expected_features<Container>::type >,
                                                 HiddenTag< Container > > > {
public:
   sequence_raw get_container2() const
   {
      // the size is being determined on the first (main) container unless it is of unlimited-const nature
      return sequence_raw(0, object_classifier::what_is<Container>::value == object_classifier::is_constant ? Int(this->hidden().size()) : 1);
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
   static constexpr bool is_temporary=false;
};

template <typename Container, typename Features>
struct default_enforce_features<Container, Features, object_classifier::is_constant>
   : default_enforce_features<construct_sequence_indexed<Container>, Features, object_classifier::is_manip> {};

template <typename Container>
struct default_enforce_features<Container, reversed, object_classifier::is_constant> {
   using container = Container;
};

} // end namespace pm

namespace polymake {

using pm::entire;
using pm::entire_const;
using pm::repeat_value;
using pm::single_value_as_container;

}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
