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

#ifndef POLYMAKE_CONTAINER_CHAIN_H
#define POLYMAKE_CONTAINER_CHAIN_H

#include "polymake/internal/operations.h"
#include "polymake/ContainerUnion.h"
#include <array>

namespace pm {
namespace chains {

template <typename Indexes, typename Operation>
class Function;

template <size_t... Index, typename Operation>
class Function<std::index_sequence<Index...>, Operation> {
   using fpointer = typename Operation::fpointer;
   static constexpr size_t length = sizeof...(Index);

   static const fpointer table[length];
public:
   static fpointer get(int i) { return table[i]; }
};

template <size_t... Index, typename Operation>
const typename Function<std::index_sequence<Index...>, Operation>::fpointer
Function<std::index_sequence<Index...>, Operation>::table[] = { &Operation::template execute<Index>... };

template <typename IteratorList>
class Operations {
public:
   using it_tuple = typename mlist2tuple<IteratorList>::type;
   using reference = typename union_iterator_traits<IteratorList>::reference;
   using pointer = typename union_iterator_traits<IteratorList>::pointer;

   struct star {
      using fpointer = reference (*)(const it_tuple&);
      template <size_t i>
      static reference execute(const it_tuple& its)
      {
         return *std::get<i>(its);
      }
   };
   struct arrow {
      using fpointer = pointer (*)(const it_tuple&);
      template <size_t i>
      static pointer execute(const it_tuple& its)
      {
         return std::get<i>(its).operator->();
      }
   };
   struct incr {
      using fpointer = bool (*)(it_tuple&);
      template <size_t i>
      static bool execute(it_tuple& its)
      {
         return (++std::get<i>(its)).at_end();
      }
   };
   struct eq {
      using fpointer = bool (*)(const it_tuple&, const it_tuple&);
      template <size_t i>
      static bool execute(const it_tuple& its, const it_tuple& other)
      {
         return std::get<i>(its) == std::get<i>(other);
      }
   };
   struct at_end {
      using fpointer = bool (*)(const it_tuple&);
      template <size_t i>
      static bool execute(const it_tuple& its)
      {
         return std::get<i>(its).at_end();
      }
   };
   struct index {
      using fpointer = Int (*)(const it_tuple&);
      template <size_t i>
      static Int execute(const it_tuple& its)
      {
         return std::get<i>(its).index();
      }
   };
};

template <typename IteratorList,
          bool is_homogeneous=(mlist_length<typename mlist_remove_duplicates<IteratorList>::type>::value == 1)>
class iterator_store {
public:
   using iterator_t = typename mlist_head<IteratorList>::type;
   static constexpr size_t N = mlist_length<IteratorList>::value;
   using it_tuple = std::array<iterator_t, N>;
   it_tuple its;

   using reference = typename iterator_traits<iterator_t>::reference;
   using pointer = typename iterator_traits<iterator_t>::pointer;
   using value_type = typename iterator_traits<iterator_t>::value_type;

   template <typename... Iterator, typename=std::enable_if_t<sizeof...(Iterator)==N>>
   explicit iterator_store(Iterator&&... it)
      : its{ { std::forward<Iterator>(it)... } }
   {}

   iterator_store() = default;
   iterator_store(const iterator_store&) = default;
   iterator_store(iterator_store&&) = default;
   iterator_store& operator= (const iterator_store&) = default;

   template <typename IteratorList2, typename=std::enable_if_t<std::is_constructible<iterator_t, const typename mlist_head<IteratorList2>::type&>::value>>
   iterator_store(const iterator_store<IteratorList2, is_homogeneous>& other)
      : iterator_store(other, std::make_index_sequence<N>()) {}

   template <typename IteratorList2, size_t... Index>
   iterator_store(const iterator_store<IteratorList2, is_homogeneous>& other, std::index_sequence<Index...>)
      : its{ other.its[Index]... }
   {}

   template <typename IteratorList2, typename=std::enable_if_t<std::is_assignable<iterator_t&, const typename mlist_head<IteratorList2>::type&>::value>>
   iterator_store& operator= (const iterator_store<IteratorList2, is_homogeneous>& other)
   {
      std::copy(other.its.begin(), other.its.end(), its.begin());
      return *this;
   }

   it_tuple& get_it_tuple() { return its; }
   const it_tuple& get_it_tuple() const { return its; }

   reference star(int i) const
   {
      return *its[i];
   }

   pointer arrow(int i) const
   {
      return its[i].operator->();
   }

   Int index(int i) const
   {
      return its[i].index();
   }

   bool incr(int i)
   {
      return (++its[i]).at_end();
   }

   bool eq(int i, const iterator_store& other) const
   {
      return its[i] == other.its[i];
   }

   bool at_end(int i) const
   {
      return its[i].at_end();
   }

   void rewind()
   {
      for (iterator_t& it : its) it.rewind();
   }
};


template <typename IteratorList>
class iterator_store<IteratorList, false> {
public:
   using it_tuple = typename mlist2tuple<IteratorList>::type;
   it_tuple its;
   static constexpr size_t N = mlist_length<IteratorList>::value;

   using ops = Operations<IteratorList>;
   template <typename Operation>
   using functions = Function<std::make_index_sequence<N>, Operation>;

   using reference = typename union_iterator_traits<IteratorList>::reference;
   using pointer = typename union_iterator_traits<IteratorList>::pointer;
   using value_type = typename union_iterator_traits<IteratorList>::value_type;

   template <typename... Iterator, typename=std::enable_if_t<sizeof...(Iterator)==N>>
   explicit iterator_store(Iterator&&... it)
      : its(std::forward<Iterator>(it)...)
   {}

   iterator_store() = default;
   iterator_store(const iterator_store&) = default;
   iterator_store(iterator_store&&) = default;
   iterator_store& operator= (const iterator_store&) = default;

   template <typename IteratorList2, typename=std::enable_if_t<std::is_constructible<it_tuple, const typename mlist2tuple<IteratorList2>::type&>::value>>
   iterator_store(const iterator_store<IteratorList2, false>& other)
      : its(other.its) {}

   template <typename IteratorList2, typename=std::enable_if_t<std::is_assignable<it_tuple&, const typename mlist2tuple<IteratorList2>::type&>::value>>
   iterator_store& operator= (const iterator_store<IteratorList2, false>& other)
   {
      its = other.its;
      return *this;
   }

   it_tuple& get_it_tuple() { return its; }
   const it_tuple& get_it_tuple() const { return its; }

   reference star(int i) const
   {
      return functions<typename ops::star>::get(i)(its);
   }

   pointer arrow(int i) const
   {
      return functions<typename ops::arrow>::get(i)(its);
   }

   Int index(int i) const
   {
      return functions<typename ops::index>::get(i)(its);
   }

   bool incr(int i)
   {
      return functions<typename ops::incr>::get(i)(its);
   }

   bool eq(int i, const iterator_store& other) const
   {
      return functions<typename ops::eq>::get(i)(its, other.its);
   }

   bool at_end(int i) const
   {
      return functions<typename ops::at_end>::get(i)(its);
   }

   void rewind()
   {
      foreach_in_tuple(its, [](auto& it) -> void { it.rewind(); });
   }
};

template <typename T>
struct get_iterator {
   using type = typename iterator_traits<T>::iterator;
};
template <typename T>
struct get_const_iterator {
   using type = typename iterator_traits<T>::const_iterator;
};

}

template <typename IteratorList, bool is_indexed>
class iterator_chain
   : protected chains::iterator_store<IteratorList> {
   using base_t = chains::iterator_store<IteratorList>;
   template <typename, bool> friend class iterator_chain;
protected:
   int leg;

   static constexpr size_t n_off = is_indexed ? mlist_length<IteratorList>::value : 0;
   using offsets_t = std::array<Int, n_off>;
   offsets_t index_offsets;

   void valid_position()
   {
      while (!at_end() && base_t::at_end(leg)) ++leg;
   }
public:
   using iterator_category = forward_iterator_tag;
   using typename base_t::value_type;
   using typename base_t::reference;
   using typename base_t::pointer;
   using difference_type = ptrdiff_t;

   using iterator = iterator_chain<typename mlist_transform_unary<IteratorList, chains::get_iterator>::type, is_indexed>;
   using const_iterator = iterator_chain<typename mlist_transform_unary<IteratorList, chains::get_const_iterator>::type, is_indexed>;

   iterator_chain()
      : leg(mlist_length<IteratorList>::value) {}

   template <typename... Iterator>
   iterator_chain(int leg_arg, offsets_t&& offsets_arg, Iterator&&... it)
      : base_t(std::forward<Iterator>(it)...)
      , leg(leg_arg)
      , index_offsets(offsets_arg)
   {
      valid_position();
   }

   template <typename... Iterator>
   iterator_chain(int leg_arg, std::nullptr_t, Iterator&&... it)
      : base_t(std::forward<Iterator>(it)...)
      , leg(leg_arg)
   {
      valid_position();
   }

   iterator_chain(const iterator& other)
      : base_t(other)
      , leg(other.leg)
      , index_offsets(other.index_offsets) {}

   iterator_chain& operator= (const iterator& other)
   {
      base_t::operator=(other);
      leg=other.leg;
      index_offsets=other.index_offsets;
      return *this;
   }

   typename base_t::reference operator* () const
   {
      return base_t::star(leg);
   }

   typename base_t::pointer operator-> () const
   {
      return base_t::arrow(leg);
   }

   iterator_chain& operator++ ()
   {
      if (base_t::incr(leg)) {
         ++leg;
         valid_position();
      }
      return *this;
   }
   const iterator_chain operator++ (int) { iterator_chain copy=*this; operator++(); return copy; }

   bool operator== (const iterator_chain& other) const
   {
      return leg==other.leg && (at_end() || base_t::eq(leg, other));
   }

   bool operator!= (const iterator_chain& other) const
   {
      return !operator==(other);
   }

   bool at_end() const
   {
      return leg == mlist_length<IteratorList>::value;
   }

   void rewind()
   {
      static_assert(check_iterator_feature<iterator_chain, rewindable>::value, "iterator is not rewindable");
      base_t::rewind();
      leg=0;
      valid_position();
   }

   Int index() const
   {
      static_assert(is_indexed, "iterator is not indexed");
      return base_t::index(leg) + index_offsets[leg];
   }

   using typename base_t::it_tuple;
   int get_leg() const { return leg; }
   const it_tuple& get_it_tuple() const { return base_t::get_it_tuple(); }
};

template <typename IteratorList, bool is_indexed, typename Feature>
struct check_iterator_feature<iterator_chain<IteratorList, is_indexed>, Feature>
   : mlist_and<typename mlist_transform_binary<IteratorList, mrepeat<Feature>, check_iterator_feature>::type> {};

template <typename IteratorList, bool is_indexed>
struct check_iterator_feature<iterator_chain<IteratorList, is_indexed>, indexed>
   : bool_constant<is_indexed> {};

template <typename IteratorList, bool is_indexed>
struct check_iterator_feature<iterator_chain<IteratorList, is_indexed>, contractable>
   : std::false_type {};


template <typename ContainerList, bool enforce_const=false>
struct chain_const_helper {
   static constexpr bool chain_is_const = enforce_const || mlist_or<typename mlist_transform_unary<ContainerList, is_effectively_const>::type>::value;
   using type = std::conditional_t<chain_is_const, typename mlist_transform_unary<ContainerList, add_const>::type, ContainerList>;
};

template <typename Top, typename Params>
class container_chain_typebase : public manip_container_top<Top, Params> {
   using base_t = manip_container_top<Top, Params>;
public:
   using container_ref_list = typename mtagged_list_extract<Params, ContainerRefTag>::type;
   using container_list = typename mlist_transform_unary<container_ref_list, deref>::type;

   static constexpr bool
      is_const       = chain_const_helper<container_ref_list>::chain_is_const,
      is_sparse      = mlist_or<typename mlist_transform_binary<container_list, mrepeat<sparse>, check_container_feature>::type>::value,
      all_sparse_compatible = mlist_and<typename mlist_transform_binary<container_list, mrepeat<sparse_compatible>, check_container_feature>::type>::value,
      is_pure_sparse = mlist_and<typename mlist_transform_binary<container_list, mrepeat<pure_sparse>, check_container_feature>::type>::value;

   using can_enforce_features = dense;
   // either some containers are sparse (then the resulting iterator_chain is automatically indexed),
   // or the whole chain should be made indexed (it is cheaper)
   using cannot_enforce_features = mlist<indexed, provide_construction<rewindable, false>, provide_construction<end_sensitive, false> >;

   using needed_features = typename mix_features<
      typename base_t::expected_features,
      std::conditional_t<is_sparse,
                         std::conditional_t<mlist_contains<typename base_t::expected_features, dense>::value,
                                            mlist<indexed, end_sensitive>, sparse_compatible>,
                         end_sensitive>>::type;

   static constexpr bool needs_indexed = all_sparse_compatible || mlist_contains<needed_features, indexed, absorbing_feature>::value;

   template <typename T>
   using get_category = typename container_traits<T>::category;

   using iterator_list = typename mlist_transform_binary<container_ref_list, mrepeat<needed_features>, extract_iterator_with_features>::type;
   using const_iterator_list = typename mlist_transform_binary<container_ref_list, mrepeat<needed_features>, extract_const_iterator_with_features>::type;

   using iterator = iterator_chain<std::conditional_t<is_const, const_iterator_list, iterator_list>, needs_indexed>;
   using const_iterator = iterator_chain<const_iterator_list, needs_indexed>;

   using reference = typename iterator::reference;
   using const_reference = typename const_iterator::reference;
   using value_type = typename iterator::value_type;

   using container_category = typename least_derived_class<typename mlist_concat<bidirectional_iterator_tag,
                                 typename mlist_transform_unary<container_list, extract_category>::type>::type>::type;

   static constexpr size_t chain_length = mlist_length<container_list>::value;

   static constexpr auto tuple_indexes()
   {
      return std::make_index_sequence<chain_length>();
   }

   static constexpr auto reverse_tuple_indexes()
   {
      return make_reverse_index_sequence<chain_length>();
   }

   using typename base_t::manip_top_type;

   template <typename Iterator, typename Creator, size_t... Index, typename OffsetsArg>
   Iterator make_iterator(int leg_arg, const Creator& cr, std::index_sequence<Index...>, OffsetsArg&& offsets_arg)
   {
      using me_t = std::conditional_t<is_const, const manip_top_type&, manip_top_type&>;
      me_t& me = this->manip_top();
      return Iterator(leg_arg, std::forward<OffsetsArg>(offsets_arg), cr(me.get_container(size_constant<Index>()))...);
   }

   template <typename Iterator, typename Creator, size_t... Index, typename OffsetsArg>
   Iterator make_iterator(int leg_arg, const Creator& cr, std::index_sequence<Index...>, OffsetsArg&& offsets_arg) const
   {
      const auto& me = this->manip_top();
      return Iterator(leg_arg, std::forward<OffsetsArg>(offsets_arg), cr(me.get_container(size_constant<Index>()))...);
   }

   static constexpr auto make_begin()
   {
      return [](auto&& c) { return ensure(c, needed_features()).begin(); };
   }
   static constexpr auto make_end()
   {
      return [](auto&& c) { return ensure(c, needed_features()).end(); };
   }
   static constexpr auto make_rbegin()
   {
      return [](auto&& c) { return ensure(c, needed_features()).rbegin(); };
   }
   static constexpr auto make_rend()
   {
      return [](auto&& c) { return ensure(c, needed_features()).rend(); };
   }

   template <size_t... Index>
   std::array<Int, chain_length> make_index_offsets(std::true_type, bool is_reverse, std::index_sequence<Index...>) const
   {
      Int off = 0;
      const auto summator = [&off](Int i) ->Int { Int sum = off; off += i; return sum; };

      std::array<Int, chain_length> offsets{ { summator(get_dim(this->manip_top().get_container(size_constant<Index>())))... } };
      if (is_reverse) std::reverse(offsets.begin(), offsets.end());
      return offsets;
   }

   template <typename... Args>
   static constexpr std::nullptr_t make_index_offsets(std::false_type, Args&&... args)
   {
      return nullptr;
   }

   auto make_index_offsets(bool is_reverse) const
   {
      return make_index_offsets(bool_constant<needs_indexed>(), is_reverse, tuple_indexes());
   }
};

template <typename Top, typename Params=typename Top::manipulator_params,
          typename Category = typename container_chain_typebase<Top, Params>::container_category>
class container_chain_impl
   : public container_chain_typebase<Top, Params> {
   using base_t = container_chain_typebase<Top, Params>;
private:
   static constexpr Int size_(size_constant<0>)
   {
      return 0;
   }
   static constexpr Int dim_(size_constant<0>)
   {
      return 0;
   }
   static constexpr bool empty_(size_constant<0>)
   {
      return true;
   }
   template <size_t i>
   Int size_(size_constant<i>) const
   {
      return this->manip_top().get_container(size_constant<i-1>()).size() + size_(size_constant<i-1>());
   }
   template <size_t i>
   Int dim_(size_constant<i>) const
   {
      return get_dim(this->manip_top().get_container(size_constant<i-1>())) + dim_(size_constant<i-1>());
   }
   template <size_t i>
   bool empty_(size_constant<i>) const
   {
      return this->manip_top().get_container(size_constant<i-1>()).empty() && empty_(size_constant<i-1>());
   }

public:
   typedef container_chain_impl<Top, Params> manipulator_impl;
   typedef Params manipulator_params;
   using typename base_t::iterator;
   using typename base_t::const_iterator;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef container_chain_impl<FeatureCollector, Params> type;
   };

   iterator begin()
   {
      return base_t::template make_iterator<iterator>(0, base_t::make_begin(), base_t::tuple_indexes(), base_t::make_index_offsets(false));
   }
   iterator end()
   {
      return base_t::template make_iterator<iterator>(base_t::chain_length, base_t::make_end(), base_t::tuple_indexes(), nullptr);
   }
   const_iterator begin() const
   {
      return base_t::template make_iterator<const_iterator>(0, base_t::make_begin(), base_t::tuple_indexes(), base_t::make_index_offsets(false));
   }
   const_iterator end() const
   {
      return base_t::template make_iterator<const_iterator>(base_t::chain_length, base_t::make_end(), base_t::tuple_indexes(), nullptr);
   }

   Int size() const
   {
      return size_(size_constant<base_t::chain_length>());
   }
   Int dim() const
   {
      return dim_(size_constant<base_t::chain_length>());
   }
   bool empty() const
   {
      return empty_(size_constant<base_t::chain_length>());
   }
};

template <typename Top, typename Params>
class container_chain_impl<Top, Params, forward_iterator_tag>
   : public container_chain_impl<Top, Params, input_iterator_tag> {
   using base_t = container_chain_impl<Top, Params, input_iterator_tag>;
public:
   using typename base_t::reference;
   using typename base_t::const_reference;
private:
   reference front_(size_constant<base_t::chain_length-1>)
   {
      return this->manip_top().get_container(size_constant<base_t::chain_length-1>()).front();
   }
   const_reference front_(size_constant<base_t::chain_length-1>) const
   {
      return this->manip_top().get_container(size_constant<base_t::chain_length-1>()).front();
   }
   template <size_t i>
   reference front_(size_constant<i>)
   {
      auto& c = this->manip_top().get_container(size_constant<i>());
      if (!c.empty()) return c.front();
      return front_(size_constant<i+1>());
   }
   template <size_t i>
   const_reference front_(size_constant<i>) const
   {
      const auto& c = this->manip_top().get_container(size_constant<i>());
      if (!c.empty()) return c.front();
      return front_(size_constant<i+1>());
   }
public:
   reference front()
   {
      return front_(size_constant<0>());
   }
   const_reference front() const
   {
      return front_(size_constant<0>());
   }
};

template <typename Top, typename Params>
class container_chain_impl<Top, Params, bidirectional_iterator_tag>
   : public container_chain_impl<Top, Params, forward_iterator_tag> {
   using base_t = container_chain_impl<Top, Params, forward_iterator_tag>;
public:
   using reverse_container_list = typename mlist_reverse<typename base_t::container_ref_list>::type;
   using reverse_iterator_list = typename mlist_transform_binary<reverse_container_list,
                                   mrepeat<typename base_t::needed_features>, extract_reverse_iterator_with_features>::type;
   using const_reverse_iterator_list = typename mlist_transform_binary<reverse_container_list,
                                   mrepeat<typename base_t::needed_features>, extract_const_reverse_iterator_with_features>::type;
   using reverse_iterator = iterator_chain<std::conditional_t<base_t::is_const, const_reverse_iterator_list, reverse_iterator_list>, base_t::needs_indexed>;
   using const_reverse_iterator = iterator_chain<const_reverse_iterator_list, base_t::needs_indexed>;
   using typename base_t::reference;
   using typename base_t::const_reference;
private:
   reference back_(size_constant<0>)
   {
      return this->manip_top().get_container(size_constant<0>()).back();
   }
   const_reference back_(size_constant<0>) const
   {
      return this->manip_top().get_container(size_constant<0>()).back();
   }
   template <size_t i>
   reference back_(size_constant<i>)
   {
      auto& c = this->manip_top().get_container(size_constant<i>());
      if (!c.empty()) return c.back();
      return back_(size_constant<i-1>());
   }
   template <size_t i>
   const_reference back_(size_constant<i>) const
   {
      const auto& c = this->manip_top().get_container(size_constant<i>());
      if (!c.empty()) return c.back();
      return back_(size_constant<i-1>());
   }
public:
   reverse_iterator rbegin()
   {
      return base_t::template make_iterator<reverse_iterator>(0, base_t::make_rbegin(), base_t::reverse_tuple_indexes(), base_t::make_index_offsets(true));
   }
   reverse_iterator rend()
   {
      return base_t::template make_iterator<reverse_iterator>(base_t::chain_length, base_t::make_rend(), base_t::reverse_tuple_indexes(), nullptr);
   }
   const_reverse_iterator rbegin() const
   {
      return base_t::template make_iterator<const_reverse_iterator>(0, base_t::make_rbegin(), base_t::reverse_tuple_indexes(), base_t::make_index_offsets(true));
   }
   const_reverse_iterator rend() const
   {
      return base_t::template make_iterator<const_reverse_iterator>(base_t::chain_length, base_t::make_rend(), base_t::reverse_tuple_indexes(), nullptr);
   }

   reference back()
   {
      return back_(size_constant<base_t::chain_length-1>());
   }
   const_reference back() const
   {
      return back_(size_constant<base_t::chain_length-1>());
   }
};

template <template <typename...> class Owner, typename... TailParams>
struct chain_arg_helper {
   template <typename T>
   using recognize = is_instance_of<pure_type_t<T>, Owner>;

   template <typename T, typename=void>
   struct extract_impl {
      using type = T;
   };
   template <typename T>
   struct extract_impl<T, std::enable_if_t<recognize<T>::value &&
            std::is_same<typename mlist_tail<typename recognize<T>::params>::type, mlist<TailParams...>>::value>> {
      using aliases = typename mlist_transform_unary<typename mget_template_parameter<pure_type_t<T>, 0>::type, make_alias>::type;
      using type = typename mlist_transform_binary<aliases, mrepeat<T>, inherit_reference>::type;
   };

   template <typename T>
   using extract = extract_impl<T>;

   template <typename ContainerList, typename ArgList>
   static constexpr bool allow(ContainerList, ArgList)
   {
      using flattened_args = typename mlist_flatten<typename mlist_transform_unary<ArgList, extract>::type>::type;
      using alias_list = typename mlist_transform_unary<ContainerList, make_alias>::type;
      return allow_args(alias_list(), flattened_args());
   }

   template <typename AliasList, typename ArgList>
   static constexpr std::enable_if_t<mlist_length<AliasList>::value == mlist_length<ArgList>::value, bool>
   allow_args(AliasList, ArgList)
   {
      return mlist_and< typename mlist_transform_binary<AliasList, ArgList, is_direct_constructible>::type >::value;
   }

   template <typename AliasList, typename ArgList>
   static constexpr std::enable_if_t<mlist_length<AliasList>::value != mlist_length<ArgList>::value, bool>
   allow_args(AliasList, ArgList)
   {
      return false;
   }

   template <typename T>
   using count = int_constant<mlist_length<typename extract_impl<T>::type>::value>;

   template <typename Count, typename CountList>
   using add_count = mlist_concat<Count, typename mlist_transform_binary<CountList, mrepeat<Count>, madd>::type>;

   template <typename ArgList, bool no_tuples>
   struct matcher {
      using type = std::true_type;
   };

   template <typename ArgList>
   struct matcher<ArgList, false> {
      using type = typename mlist_wrap<typename mlist_fold_transform<ArgList, count, add_count>::type>::type;
   };

   template <int length, typename ArgList>
   static constexpr auto match(ArgList)
   {
      return typename matcher<ArgList, (length==mlist_length<ArgList>::value)>::type();
   }
};

template <typename Elements>
class alias_tuple {
protected:
   using alias_list = typename mlist_transform_unary<Elements, make_alias>::type;
   using alias_store = typename mlist2tuple<alias_list>::type;

   alias_store aliases;

   // TODO: =delete
   alias_tuple() = default;

   alias_tuple(alias_tuple&&) = default;
   alias_tuple(const alias_tuple&) = default;

   template <template <typename...> class Owner, typename... TailParams, typename... Args>
   alias_tuple(chain_arg_helper<Owner, TailParams...> helper, Args&&... args)
      : alias_tuple(helper.template match<mlist_length<Elements>::value>(mlist<Args...>()), std::forward<Args>(args)...) {}

private:
   template <typename... Args>
   alias_tuple(std::true_type, Args&&... args)
      : aliases(std::forward<Args>(args)...) {}

   template <typename... Counts, typename... Args>
   alias_tuple(mlist<Counts...>, Args&&... args)
      : alias_tuple(typename index_sequence_for<Elements>::type(),
                    mlist<int_constant<0>, Counts...>(),
                    std::forward_as_tuple(std::forward<Args>(args)...)) {}

   template <size_t... Index, typename Counts, typename... Args>
   alias_tuple(std::index_sequence<Index...>, Counts, std::tuple<Args...>&& args)
      : aliases(pick_arg<Index, Counts>(args)...) {}

   template <int i, typename Counts, typename... Args>
   static constexpr decltype(auto) pick_arg(std::tuple<Args...>& args)
   {
      using arg_finder = mlist_find_if<Counts, mis_greater, int_constant<i>>;
      constexpr int offset = mlist_at<Counts, arg_finder::pos-1>::type::value;
      constexpr bool arg_is_tuple = arg_finder::match::value > offset+1;
      return pick_arg(std::get<arg_finder::pos-1>(std::move(args)), int_constant<(arg_is_tuple ? i-offset : -1)>());
   }

   template <typename Arg>
   static constexpr decltype(auto) pick_arg(Arg&& arg, int_constant<-1>)
   {
      return std::forward<Arg>(arg);
   }

   template <typename Arg, int i>
   static constexpr decltype(auto) pick_arg(Arg&& arg, int_constant<i>)
   {
      return std::forward<Arg>(arg).get_alias(size_constant<i>());
   }

public:
   template <size_t i>
   decltype(auto) get_alias(size_constant<i>) &
   {
      return std::get<i>(aliases);
   }
   template <size_t i>
   decltype(auto) get_alias(size_constant<i>) const &
   {
      return std::get<i>(aliases);
   }
   template <size_t i>
   decltype(auto) get_alias(size_constant<i>) &&
   {
      return std::move(std::get<i>(aliases));
   }
   template <size_t i>
   decltype(auto) get_container(size_constant<i>)
   {
      return *get_alias(size_constant<i>());
   }
   template <size_t i>
   decltype(auto) get_container(size_constant<i>) const
   {
      return *get_alias(size_constant<i>());
   }

   template <typename T>
   using get_lazy = bool_constant<object_traits<typename deref<T>::type>::is_lazy>;

   static constexpr bool
      is_lazy = mlist_or<typename mlist_transform_unary<Elements, get_lazy>::type>::value,
      is_always_const = mlist_or<typename mlist_transform_unary<Elements, is_effectively_const>::type>::value;
};

template <template <typename...> class Owner, bool enforce_const=false, typename... TailParams>
struct chain_compose {
   template <typename T>
   using recognize = is_instance_of<pure_type_t<T>, Owner>;

   template <typename T, typename=void>
   struct extract_impl {
      using type = T;
   };
   template <typename T>
   struct extract_impl<T, std::enable_if_t<recognize<T>::value &&
            std::is_same<typename mlist_tail<typename recognize<T>::params>::type, mlist<TailParams...>>::value>> {
      using type = typename mlist_head<typename recognize<T>::params>::type;
   };
   template <typename T>
   using extract = extract_impl<T>;

   template <typename... T>
   using component_list = typename mlist_flatten<typename mlist_transform_unary<typename mlist_wrap<T...>::type, extract>::type>::type;

   template <typename... T>
   using list = typename chain_const_helper<component_list<T...>, enforce_const>::type;

   template <typename... T>
   using with = Owner<list<T...>, TailParams...>;
};

template <typename ContainerList>
class ContainerChain
   : public alias_tuple<ContainerList>
   , public container_chain_impl< ContainerChain<ContainerList>,
                                  mlist< ContainerRefTag<ContainerList> > > {
   using arg_helper = chain_arg_helper<pm::ContainerChain>;
protected:
   using alias_tuple<ContainerList>::alias_tuple;
public:
   template <typename... Args, typename=std::enable_if_t<arg_helper::allow(ContainerList(), mlist<Args...>())>>
   explicit ContainerChain(Args&&... args)
      : alias_tuple<ContainerList>(arg_helper(), std::forward<Args>(args)...) {}
};

template <typename ContainerList>
struct spec_object_traits< ContainerChain<ContainerList> >
   : spec_object_traits<is_container> {
   static constexpr bool
      is_temporary = true,
      is_lazy = alias_tuple<ContainerList>::is_lazy,
      is_always_const = alias_tuple<ContainerList>::is_always_const;
};

template <typename ContainerList>
struct check_container_feature<ContainerChain<ContainerList>, sparse>
   : mlist_or<typename mlist_transform_binary<ContainerList, mrepeat<sparse>, check_container_ref_feature>::type> {};

template <typename ContainerList>
struct check_container_feature<ContainerChain<ContainerList>, pure_sparse>
   : mlist_and<typename mlist_transform_binary<ContainerList, mrepeat<pure_sparse>, check_container_ref_feature>::type> {};

template <typename... Container>
auto concatenate(Container&&... c)
{
   return typename chain_compose<ContainerChain>::template with<Container...>(std::forward<Container>(c)...);
}

template <typename IteratorList, typename Operation>
class tuple_transform_iterator
   : protected chains::iterator_store<IteratorList> {
   using base_t = chains::iterator_store<IteratorList>;
   using typename base_t::it_tuple;
protected:
   Operation op;

   template <size_t... Index>
   static decltype(auto) apply_op(const Operation& op, const it_tuple& t, std::index_sequence<Index...>)
   {
      return op(*(std::get<Index>(t))...);
   }

public:
   using iterator_category = typename union_iterator_traits<IteratorList>::iterator_category;
   using difference_type = ptrdiff_t;

   using iterator = tuple_transform_iterator<typename mlist_transform_unary<IteratorList, chains::get_iterator>::type, Operation>;
   using const_iterator = tuple_transform_iterator<typename mlist_transform_unary<IteratorList, chains::get_const_iterator>::type, Operation>;

   tuple_transform_iterator() = default;

   template <typename... Iterator, typename=std::enable_if_t<std::is_constructible<base_t, Iterator...>::value>>
   explicit tuple_transform_iterator(Iterator&&... args)
      : base_t(std::forward<Iterator>(args)...) {}

   template <typename... Iterator, typename=std::enable_if_t<std::is_constructible<base_t, Iterator...>::value>>
   tuple_transform_iterator(const Operation& op_arg, Iterator&&... args)
      : base_t(std::forward<Iterator>(args)...)
      , op(op_arg) {}

   tuple_transform_iterator(const iterator& other)
      : base_t(other) {}

   tuple_transform_iterator& operator= (const iterator& other)
   {
      base_t::operator=(other);
      return *this;
   }

   using reference = decltype(apply_op(std::declval<const Operation&>(), std::declval<const it_tuple&>(), typename index_sequence_for<IteratorList>::type()));
   using pointer = typename arrow_helper<reference>::pointer;
   using value_type = pure_type_t<reference>;

   reference operator* () const
   {
      return apply_op(op, base_t::get_it_tuple(), typename index_sequence_for<IteratorList>::type());
   }
   pointer operator-> () const
   {
      return arrow_helper<reference>::get(*this);
   }

   tuple_transform_iterator& operator++ ()
   {
      foreach_in_tuple(base_t::its, [](auto& it) ->void { ++it; });
      return *this;
   }
   tuple_transform_iterator operator++ (int) { tuple_transform_iterator copy(*this); operator++(); return copy; }

   tuple_transform_iterator& operator-- ()
   {
      static_assert(is_derived_from<iterator_category, std::bidirectional_iterator_tag>::value,
                    "decrement is not supported by all involved iterators");
      foreach_in_tuple(base_t::get_it_tuple(), [](auto& it) ->void { --it; });
      return *this;
   }
   tuple_transform_iterator operator-- (int) { tuple_transform_iterator copy(*this); operator--(); return copy; }

   tuple_transform_iterator& operator+= (ptrdiff_t i)
   {
      static_assert(std::is_same<iterator_category, std::random_access_iterator_tag>::value,
                    "random access is not supported by all involved iterators");
      foreach_in_tuple(base_t::get_it_tuple(), [i](auto& it) ->void { it+=i; });
      return *this;
   }
   tuple_transform_iterator& operator-= (ptrdiff_t i)
   {
      static_assert(std::is_same<iterator_category, std::random_access_iterator_tag>::value,
                    "random access is not supported by all involved iterators");
      foreach_in_tuple(base_t::get_it_tuple(), [i](auto& it) ->void { it-=i; });
      return *this;
   }

   tuple_transform_iterator operator+ (ptrdiff_t i) const
   {
      tuple_transform_iterator copy(*this);  return copy+=i;
   }
   tuple_transform_iterator operator- (ptrdiff_t i) const
   {
      tuple_transform_iterator copy(*this);  return copy-=i;
   }
   friend tuple_transform_iterator operator+ (ptrdiff_t i, const tuple_transform_iterator& me)
   {
      return me+i;
   }
   ptrdiff_t operator- (const tuple_transform_iterator& other) const
   {
      constexpr int i = mlist_find_if<IteratorList, can_subtract_iterators>::pos;
      static_assert(i>=0, "no random access");
      return std::get<i>(base_t::get_it_tuple()) - std::get<i>(other.get_it_tuple());
   }

   bool at_end() const
   {
      constexpr int i = mlist_find_if<IteratorList, check_iterator_feature, end_sensitive>::pos;
      return std::get<i>(base_t::get_it_tuple()).at_end();
   }

   Int index() const
   {
      constexpr int i = mlist_find_if<IteratorList, check_iterator_feature, indexed>::pos;
      return std::get<i>(base_t::get_it_tuple()).index();
   }

   bool operator== (const tuple_transform_iterator& other) const
   {
      constexpr int i = mlist_find_if<IteratorList, mnegate_binary<check_iterator_feature>::template func, unlimited>::pos;
      return std::get<i>(base_t::get_it_tuple()) == std::get<i>(other.get_it_tuple());
   }
   bool operator!= (const tuple_transform_iterator& other) const
   {
      return !operator==(other);
   }
};

template <typename IteratorList, typename Operation, typename Feature>
struct check_iterator_feature< tuple_transform_iterator<IteratorList, Operation>, Feature>
   : mlist_or< typename mlist_transform_binary<IteratorList, mrepeat<Feature>, check_iterator_feature>::type > {};

template <typename IteratorList, typename Operation>
struct check_iterator_feature< tuple_transform_iterator<IteratorList, Operation>, unlimited>
   : mlist_and< typename mlist_transform_binary<IteratorList, mrepeat<unlimited>, check_iterator_feature>::type > {};

template <typename Top, typename Params>
class modified_container_tuple_typebase : public manip_container_top<Top, Params> {
   using base_t = manip_container_top<Top, Params>;
public:
   using container_ref_list = typename mtagged_list_extract<Params, ContainerRefTag>::type;
   using container_list = typename mlist_transform_unary<container_ref_list, deref>::type;
   using operation = typename mtagged_list_extract<Params, OperationTag>::type;
   using raw_iterator_list = typename mlist_transform_unary<container_ref_list, extract_iterator>::type;

   static constexpr int tuple_size = mlist_length<container_list>::value;
   static constexpr auto tuple_indexes()
   {
      return std::make_index_sequence<tuple_size>();
   }

   using usual_or_features = mlist<end_sensitive, indexed>;
   using typename base_t::expected_features;
   using or_features = typename mlist_match_all<expected_features, usual_or_features, equivalent_features>::type;
   using and_features = typename mlist_match_all<expected_features, usual_or_features, equivalent_features>::complement;
   using missing_or_features = typename mlist_match_all<raw_iterator_list, or_features, check_iterator_feature>::complement2;

   static constexpr size_t normal_it_pos = mlist_find_if<raw_iterator_list, mnegate_binary<check_iterator_feature>::template func, unlimited>::pos;
   using needed_feature_list = typename mlist_concat< typename mreplicate< ExpectedFeaturesTag<and_features>, normal_it_pos>::type,
                                                      ExpectedFeaturesTag< typename mix_features<and_features, missing_or_features>::type >,
                                                      typename mreplicate< ExpectedFeaturesTag<and_features>, tuple_size-normal_it_pos-1>::type >::type;

   using iterator_list = typename mlist_transform_binary<container_ref_list, needed_feature_list, extract_iterator_with_features>::type;
   using const_iterator_list = typename mlist_transform_binary<container_ref_list, needed_feature_list, extract_const_iterator_with_features>::type;

   using iterator = tuple_transform_iterator<iterator_list, operation>;
   using const_iterator = tuple_transform_iterator<const_iterator_list, operation>;
   using container_category = typename least_derived_class< typename mlist_transform_unary<container_list, extract_category>::type >::type;
   using reference = typename iterator::reference;
   using const_reference = typename const_iterator::reference;
   using value_type = typename iterator::value_type;
};

template <typename Top, typename Params>
class reverse_modified_container_tuple_typebase {
   using base_t = modified_container_tuple_typebase<Top, Params>;
public:
   using reverse_iterator_list =
      typename mlist_transform_binary<typename base_t::container_ref_list, typename base_t::needed_feature_list,
                                      extract_reverse_iterator_with_features>::type;
   using const_reverse_iterator_list =
      typename mlist_transform_binary<typename base_t::container_ref_list, typename base_t::needed_feature_list,
                                      extract_const_reverse_iterator_with_features>::type;

   using reverse_iterator = tuple_transform_iterator<reverse_iterator_list, typename base_t::operation>;
   using const_reverse_iterator = tuple_transform_iterator<const_reverse_iterator_list, typename base_t::operation>;
};

template <typename Top, typename Params=typename Top::manipulator_params,
          typename Category=typename modified_container_tuple_typebase<Top, Params>::container_category>
class modified_container_tuple_impl
   : public modified_container_tuple_typebase<Top, Params> {
   using base_t = modified_container_tuple_typebase<Top, Params>;
public:
   typedef modified_container_tuple_impl<Top, Params> manipulator_impl;
   typedef Params manipulator_params;
   using typename base_t::iterator;
   using typename base_t::const_iterator;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef modified_container_tuple_impl<FeatureCollector, Params> type;
   };

   iterator begin()
   {
      return make_begin(base_t::tuple_indexes(), typename base_t::needed_feature_list());
   }
   iterator end()
   {
      return make_end(base_t::tuple_indexes(), typename base_t::needed_feature_list());
   }
   const_iterator begin() const
   {
      return make_begin(base_t::tuple_indexes(), typename base_t::needed_feature_list());
   }
   const_iterator end() const
   {
      return make_end(base_t::tuple_indexes(), typename base_t::needed_feature_list());
   }

   Int size() const
   {
      return this->manip_top().template get_container(size_constant<base_t::normal_it_pos>()).size();
   }
   Int dim() const
   {
      return get_dim(this->manip_top().template get_container(size_constant<base_t::normal_it_pos>()));
   }
   bool empty() const
   {
      return this->manip_top().template get_container(size_constant<base_t::normal_it_pos>()).empty();
   }

   decltype(auto) front()
   {
      return make_front(base_t::tuple_indexes());
   }
   decltype(auto) front() const
   {
      return make_front(base_t::tuple_indexes());
   }

private:
   template <size_t... Index, typename... Features>
   iterator make_begin(std::index_sequence<Index...>, mlist<Features...>)
   {
      return iterator(this->manip_top().get_operation(),
                      ensure(this->manip_top().template get_container(size_constant<Index>()), muntag_t<Features>()).begin()...);
   }

   template <size_t... Index, typename... Features>
   iterator make_end(std::index_sequence<Index...>, mlist<Features...>)
   {
      return iterator(this->manip_top().get_operation(),
                      ensure(this->manip_top().template get_container(size_constant<Index>()), muntag_t<Features>()).end()...);
   }

   template <size_t... Index, typename... Features>
   const_iterator make_begin(std::index_sequence<Index...>, mlist<Features...>) const
   {
      return const_iterator(this->manip_top().get_operation(),
                            ensure(this->manip_top().template get_container(size_constant<Index>()), muntag_t<Features>()).begin()...);
   }

   template <size_t... Index, typename... Features>
   const_iterator make_end(std::index_sequence<Index...>, mlist<Features...>) const
   {
      return const_iterator(this->manip_top().get_operation(),
                            ensure(this->manip_top().template get_container(size_constant<Index>()), muntag_t<Features>()).end()...);
   }

   template <size_t... Index>
   decltype(auto) make_front(std::index_sequence<Index...>)
   {
      return this->manip_top().get_operation()( this->manip_top().template get_container(size_constant<Index>()).front()... );
   }

   template <size_t... Index>
   decltype(auto) make_front(std::index_sequence<Index...>) const
   {
      return this->manip_top().get_operation()( this->manip_top().template get_container(size_constant<Index>()).front()... );
   }
};

template <typename Top, typename Params>
class modified_container_tuple_impl<Top, Params, std::bidirectional_iterator_tag>
   : public modified_container_tuple_impl<Top, Params, std::forward_iterator_tag> {
   using base_t = modified_container_tuple_impl<Top, Params, std::forward_iterator_tag>;
   using rev_traits = reverse_modified_container_tuple_typebase<Top, Params>;
public:
   using reverse_iterator = typename rev_traits::reverse_iterator;
   using const_reverse_iterator = typename rev_traits::const_reverse_iterator;

   reverse_iterator rbegin()
   {
      return make_rbegin(base_t::tuple_indexes(), typename base_t::needed_feature_list());
   }
   reverse_iterator rend()
   {
      return make_rend(base_t::tuple_indexes(), typename base_t::needed_feature_list());
   }
   const_reverse_iterator rbegin() const
   {
      return make_rbegin(base_t::tuple_indexes(), typename base_t::needed_feature_list());
   }
   const_reverse_iterator rend() const
   {
      return make_rend(base_t::tuple_indexes(), typename base_t::needed_feature_list());
   }

   decltype(auto) back()
   {
      return make_back(base_t::tuple_indexes());
   }
   decltype(auto) back() const
   {
      return make_back(base_t::tuple_indexes());
   }

private:
   template <size_t... Index, typename... Features>
   reverse_iterator make_rbegin(std::index_sequence<Index...>, mlist<Features...>)
   {
      return reverse_iterator(this->manip_top().get_operation(),
                              ensure(this->manip_top().template get_container(size_constant<Index>()), muntag_t<Features>()).rbegin()...);
   }

   template <size_t... Index, typename... Features>
   reverse_iterator make_rend(std::index_sequence<Index...>, mlist<Features...>)
   {
      return reverse_iterator(this->manip_top().get_operation(),
                              ensure(this->manip_top().template get_container(size_constant<Index>()), muntag_t<Features>()).rend()...);
   }

   template <size_t... Index, typename... Features>
   const_reverse_iterator make_rbegin(std::index_sequence<Index...>, mlist<Features...>) const
   {
      return const_reverse_iterator(this->manip_top().get_operation(),
                                    ensure(this->manip_top().template get_container(size_constant<Index>()), muntag_t<Features>()).rbegin()...);
   }

   template <size_t... Index, typename... Features>
   const_reverse_iterator make_rend(std::index_sequence<Index...>, mlist<Features...>) const
   {
      return const_reverse_iterator(this->manip_top().get_operation(),
                                    ensure(this->manip_top().template get_container(size_constant<Index>()), muntag_t<Features>()).rend()...);
   }

   template <size_t... Index>
   decltype(auto) make_back(std::index_sequence<Index...>)
   {
      return this->manip_top().get_operation()( this->manip_top().template get_container(size_constant<Index>()).back()... );
   }

   template <size_t... Index>
   decltype(auto) make_back(std::index_sequence<Index...>) const
   {
      return this->manip_top().get_operation()( this->manip_top().template get_container(size_constant<Index>()).back()... );
   }
};

template <typename Top, typename Params>
class modified_container_tuple_impl<Top, Params, std::random_access_iterator_tag>
   : public modified_container_tuple_impl<Top, Params, std::bidirectional_iterator_tag> {
   using base_t = modified_container_tuple_impl<Top, Params, std::bidirectional_iterator_tag>;
public:
   decltype(auto) operator[] (Int i)
   {
      return make_random(i, base_t::tuple_indexes());
   }
   decltype(auto) operator[] (Int i) const
   {
      return make_random(i, base_t::tuple_indexes());
   }

private:
   template <size_t... Index>
   decltype(auto) make_random(Int i, std::index_sequence<Index...>)
   {
      return this->manip_top().get_operation()( this->manip_top().template get_container(size_constant<Index>())[i]... );
   }

   template <size_t... Index>
   decltype(auto) make_random(Int i, std::index_sequence<Index...>) const
   {
      return this->manip_top().get_operation()( this->manip_top().template get_container(size_constant<Index>())[i]... );
   }
};

namespace operations {

template <typename LeftRef, typename RightRef,
          typename Discr=typename isomorphic_types<typename deref<LeftRef>::type, typename deref<RightRef>::type>::discriminant>
struct concat_impl;

template <typename LeftRef, typename RightRef>
struct concat : concat_impl<LeftRef, RightRef> {};

} // end namespace operations
} // end namespace pm

namespace polymake {

using pm::concatenate;

namespace operations {

typedef BuildBinary<pm::operations::concat> concat;

// to be used in Rows/Cols of BlockMatrix, where no hidden chains can ocur among the arguments
template <template <typename> class Result>
struct concat_tuple {
   template <typename... Args>
   auto operator() (Args&&... args) const
   {
      using list = typename pm::chain_const_helper<mlist<Args...>>::type;
      return Result<list>(std::forward<Args>(args)...);
   }
};

} }

#endif // POLYMAKE_CONTAINER_CHAIN_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
