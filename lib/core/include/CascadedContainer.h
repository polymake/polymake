/* Copyright (c) 1997-2022
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

#include "polymake/internal/iterators.h"
#include "polymake/internal/modified_containers.h"

namespace pm {

template <typename Iterator, typename ExpectedFeatures, int depth> class cascaded_iterator;

template <typename Iterator, typename ExpectedFeatures, int depth>
struct cascaded_iterator_traits {
   using next_level_iterator = typename ensure_features<std::remove_reference_t<typename iterator_traits<Iterator>::reference>,
                                                        ExpectedFeatures>::iterator;
   using base_t = cascaded_iterator<next_level_iterator, ExpectedFeatures, depth-1>;

   template <typename Container>
   static bool base_init(base_t& it, Container&& c)
   {
      // TODO: std::forward<Container> wheh ensure stops being a reinterpret_cast
      it.cur=ensure(c, ExpectedFeatures()).begin();
      return it.init();
   }

   static bool base_incr(base_t& it)
   {
      return it.incr();
   }
};

template <typename Iterator, typename ExpectedFeatures>
struct cascaded_iterator_traits<Iterator, ExpectedFeatures, 2> {
   using next_level_iterator = typename ensure_features<std::remove_reference_t<typename iterator_traits<Iterator>::reference>,
                                                        ExpectedFeatures>::iterator;

   static constexpr bool accumulate_offset= check_container_feature<typename iterator_traits<Iterator>::value_type, sparse>::value ||
                                            mlist_contains<ExpectedFeatures, indexed, absorbing_feature>::value;
   using base_features = std::conditional_t<accumulate_offset, typename mix_features<ExpectedFeatures, indexed>::type, ExpectedFeatures>;
   using base_t = cascaded_iterator<next_level_iterator, base_features, 1>;

   template <typename Container>
   static bool base_init(base_t& it, Container&& c)
   {
      it.index_store.store_dim(c);
      // TODO: std::forward<Container> wheh ensure stops being a reinterpret_cast
      static_cast<next_level_iterator&>(it)=ensure(c, base_features()).begin();
      return !it.at_end() || (it.index_store.adjust_offset(), false);
   }

   static bool base_incr(base_t& it)
   {
      ++it;
      if (!it.at_end()) return true;
      it.index_store.adjust_offset();
      return false;
   }
};

template <typename Iterator, typename ExpectedFeatures, int depth>
class cascaded_iterator
   : public cascaded_iterator_traits<Iterator, ExpectedFeatures, depth>::base_t {
   using traits = cascaded_iterator_traits<Iterator, ExpectedFeatures, depth>;
public:
   using iterator = cascaded_iterator<typename iterator_traits<Iterator>::iterator, ExpectedFeatures, depth>;
   using const_iterator = cascaded_iterator<typename iterator_traits<Iterator>::const_iterator, ExpectedFeatures, depth>;
   using base_t = typename traits::base_t;
private:
   template <typename, typename, int> friend struct cascaded_iterator_traits;
   template <typename, typename, int> friend class cascaded_iterator;

   Iterator cur;

   bool at_end_impl(std::false_type) const { return false; }
   bool at_end_impl(std::true_type) const { return cur.at_end(); }

   bool init()
   {
      while (!at_end()) {
         if (traits::base_init(*this, *cur)) return true;
         ++cur;
      }
      return false;
   }

   bool incr()
   {
      return traits::base_incr(*this) || (++cur, init());
   }

public:
   cascaded_iterator() = default;

   template <typename SourceIterator, typename=std::enable_if_t<is_const_compatible_with<pure_type_t<SourceIterator>, Iterator>::value>>
   cascaded_iterator(SourceIterator&& cur_arg)
      : cur(std::forward<SourceIterator>(cur_arg))
   {
      init();
   }

   cascaded_iterator(const iterator& it)
      : base_t(it)
      , cur(it.cur) {}

   cascaded_iterator& operator= (const iterator& it)
   {
      cur=it.cur;
      base_t::operator=(it);
      return *this;
   }

   cascaded_iterator& operator++ () { incr(); return *this; }
   cascaded_iterator operator++ (int) = delete;

   template <typename Other>
   std::enable_if_t<is_among<Other, iterator, const_iterator>::value, bool>
   operator== (const Other& it) const
   {
      return cur==it.cur && base_t::operator==(it);
   }

   template <typename Other>
   std::enable_if_t<is_among<Other, iterator, const_iterator>::value, bool>
   operator!= (const iterator& it) const { return !operator==(it); }

   bool at_end() const
   {
      return at_end_impl(bool_constant<check_iterator_feature<Iterator, end_sensitive>::value>());
   }

   void rewind()
   {
      static_assert(check_iterator_feature<Iterator, rewindable>::value, "iterator is not rewindable");
      cur.rewind();
      init();
   }
};

template <bool provide>
class cascaded_iterator_index_store {
public:
   template <typename Container>
   void store_dim(const Container&) {}
   void adjust_offset() {}
   Int adjust_index(Int i) const { return i; }
};

template <>
class cascaded_iterator_index_store<true> {
public:
   cascaded_iterator_index_store() : offset(0) {}

   template <typename Container>
   void store_dim(const Container& c) { dim=get_dim(c); }
   void adjust_offset() { offset+=dim; }
   Int adjust_index(Int i) const { return i+offset; }
private:
   Int offset, dim;
};

template <typename Iterator, typename ExpectedFeatures>
class cascaded_iterator<Iterator, ExpectedFeatures, 1>
   : public Iterator {
   template <typename, typename, int> friend struct cascaded_iterator_traits;
   template <typename, typename, int> friend class cascaded_iterator;
public:
   using base_t = Iterator;
   static constexpr bool provide_index=mlist_contains<ExpectedFeatures, indexed, absorbing_feature>::value;
   using iterator_category = forward_iterator_tag;

   cascaded_iterator() = default;

   cascaded_iterator(const cascaded_iterator<typename iterator_traits<Iterator>::iterator, ExpectedFeatures, 1>& it)
      : base_t(it)
      , index_store(it.index_store) {}

   cascaded_iterator& operator= (const cascaded_iterator<typename iterator_traits<Iterator>::iterator, ExpectedFeatures, 1>& it)
   {
      base_t::operator=(it);
      index_store=it.index_store;
      return *this;
   }

   Int index() const { return index_store.adjust_index(base_t::index()); }
protected:
   cascaded_iterator_index_store<provide_index> index_store;
};

template <typename Iterator, typename ExpectedFeatures, int depth, typename Feature>
struct check_iterator_feature<cascaded_iterator<Iterator, ExpectedFeatures, depth>, Feature> {
   using check_iterator = std::conditional_t<is_among<Feature, end_sensitive, rewindable, unlimited>::value,
                                             Iterator,
                                             typename cascaded_iterator<Iterator, ExpectedFeatures, depth>::base_t>;
   static constexpr bool value = check_iterator_feature<check_iterator, Feature>::value &&
                                 (!absorbing_feature<Feature, indexed>::value || cascaded_iterator<Iterator, ExpectedFeatures, depth>::provide_index);
   using type = bool_constant<value>;
   using value_type = bool;
};

template <typename> class CascadeDepth {};

template <typename Container, int depth>
struct cascade_traits : cascade_traits<typename container_traits<Container>::value_type, depth-1> {
   using category = typename least_derived_class<typename container_traits<Container>::category,
                                                 typename cascade_traits<typename container_traits<Container>::value_type, depth-1>::category>::type;
};

template <typename Container>
struct cascade_traits<Container, 1> : container_traits<Container> {
   using category = typename least_derived_class<typename container_traits<Container>::category, bidirectional_iterator_tag >::type;
   using base_container = Container;
};

template <typename Top, typename Params>
class cascade_typebase : public manip_container_top<Top, Params> {
   using base_t = manip_container_top<Top, Params>;
public:
   using container_ref = typename extract_container_ref<Params, ContainerRefTag, ContainerTag, typename base_t::hidden_type>::type;
   using container = typename deref<container_ref>::minus_ref;

   static constexpr int depth = tagged_list_extract_integral<Params, CascadeDepth>(1);
   using can_enforce_features = dense;
   using needed_features = typename mix_features<typename base_t::expected_features, end_sensitive>::type;
   using needed_features_next_level = typename mlist_match_all<rewindable, needed_features, absorbing_feature>::complement2;
   using iterator = cascaded_iterator<typename ensure_features<container, needed_features>::iterator, needed_features_next_level, depth>;
   using const_iterator = cascaded_iterator<typename ensure_features<container, needed_features>::const_iterator, needed_features_next_level, depth>;
   using reference = typename iterator::reference;
   using const_reference = typename const_iterator::reference;
   using value_type = typename iterator::value_type;

   using base_traits = cascade_traits<container, depth>;
   using container_category = typename base_traits::category;
};

template <typename Top, typename Params = typename Top::manipulator_params,
          typename Category = typename cascade_typebase<Top, Params>::container_category>
class cascade_impl
   : public cascade_typebase<Top, Params> {
   using base_t = cascade_typebase<Top, Params>;
public:
   typedef Params manipulator_params;
   typedef cascade_impl<Top, Params> manipulator_impl;
   typedef typename base_t::iterator iterator;
   typedef typename base_t::const_iterator const_iterator;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      using type = cascade_impl<FeatureCollector, Params>;
   };

   iterator begin()
   {
      auto&& c=this->manip_top().get_container();
      return iterator(ensure(c, typename base_t::needed_features()).begin());
   }
   iterator end()
   {
      auto&& c=this->manip_top().get_container();
      return iterator(ensure(c, typename base_t::needed_features()).end());
   }
   const_iterator begin() const
   {
      return const_iterator(ensure(this->manip_top().get_container(), typename base_t::needed_features()).begin());
   }
   const_iterator end() const
   {
      return const_iterator(ensure(this->manip_top().get_container(), typename base_t::needed_features()).end());
   }

   Int size() const
   {
      return cascade_size(this->manip_top().get_container(), int_constant<base_t::depth>());
   }
   bool empty() const
   {
      return cascade_empty(this->manip_top().get_container(), int_constant<base_t::depth>());
   }
   Int dim() const
   {
      return cascade_dim(this->manip_top().get_container(), int_constant<base_t::depth>());
   }
};

template <typename Top, typename Params>
class cascade_impl<Top, Params, forward_iterator_tag>
   : public cascade_impl<Top, Params, input_iterator_tag> {
public:
   decltype(auto) front() { return *this->begin(); }
   decltype(auto) front() const { return *this->begin(); }
};

template <typename Top, typename Params>
class cascade_impl<Top, Params, bidirectional_iterator_tag>
   : public cascade_impl<Top, Params, forward_iterator_tag> {
   using base_t = cascade_impl<Top, Params, forward_iterator_tag>;
public:
   using needed_reverse_features = typename toggle_features<typename base_t::needed_features, reversed>::type;
   using needed_reverse_features_next_level = typename toggle_features<typename base_t::needed_features_next_level, reversed>::type;
   using reverse_iterator = cascaded_iterator<typename ensure_features<typename base_t::container, needed_reverse_features>::iterator,
                                              needed_reverse_features_next_level, cascade_typebase<Top, Params>::depth>;
   using const_reverse_iterator = cascaded_iterator<typename ensure_features<typename base_t::container, needed_reverse_features>::const_iterator,
                                                    needed_reverse_features_next_level, cascade_typebase<Top, Params>::depth>;

   reverse_iterator rbegin()
   {
      auto&& c=this->manip_top().get_container();
      return reverse_iterator(ensure(c, needed_reverse_features()).begin());
   }
   reverse_iterator rend()
   {
      auto&& c=this->manip_top().get_container();
      return reverse_iterator(ensure(c, needed_reverse_features()).end());
   }
   const_reverse_iterator rbegin() const
   {
      return const_reverse_iterator(ensure(this->manip_top().get_container(), needed_reverse_features()).begin());
   }
   const_reverse_iterator rend() const
   {
      return const_reverse_iterator(ensure(this->manip_top().get_container(), needed_reverse_features()).end());
   }

   decltype(auto) back() { return *rbegin(); }
   decltype(auto) back() const { return *rbegin(); }
};

template <typename Container>
Int cascade_size(const Container& c, int_constant<1>)
{
   return c.size();
}

template <typename Container, int depth>
Int cascade_size(const Container& c, int_constant<depth>)
{
   Int size = 0;
   for (auto i = entire(c); !i.at_end(); ++i)
      size += cascade_size(*i, int_constant<depth-1>());
   return size;
}

template <typename Container>
Int cascade_dim(const Container& c, int_constant<1>)
{
   return get_dim(c);
}

template <typename Container, int depth>
Int cascade_dim(const Container& c, int_constant<depth>)
{
   Int d = 0;
   for (auto i = entire(c); !i.at_end(); ++i)
      d += cascade_dim(*i, int_constant<depth-1>());
   return d;
}

template <typename Container>
bool cascade_empty(const Container& c, int_constant<1>)
{
   return c.empty();
}

template <typename Container, int depth>
bool cascade_empty(const Container& c, int_constant<depth>)
{
   for (auto i=entire(c); !i.at_end(); ++i)
      if (!cascade_empty(*i, int_constant<depth-1>())) return false;
   return true;
}

template <typename ContainerRef, int depth=object_traits<typename deref<ContainerRef>::type>::nesting_level>
class CascadedContainer
   : public cascade_impl< CascadedContainer<ContainerRef, depth>,
                          mlist< ContainerRefTag< ContainerRef >,
                                 CascadeDepth< int_constant<depth> > > > {
protected:
   using alias_t = alias<ContainerRef>;
   alias_t src;
public:
   template <typename Arg, typename=std::enable_if_t<std::is_constructible<alias_t, Arg>::value>>
   explicit CascadedContainer(Arg&& src_arg)
      : src(std::forward<Arg>(src_arg)) {}

   decltype(auto) get_container() { return *src; }
   decltype(auto) get_container() const { return *src; }
};

template <typename ContainerRef, int depth>
struct check_container_feature<CascadedContainer<ContainerRef, depth>, sparse>
   : check_container_feature<typename CascadedContainer<ContainerRef, depth>::base_traits::base_container, sparse> {};

template <typename ContainerRef, int depth>
struct check_container_feature<CascadedContainer<ContainerRef, depth>, pure_sparse>
   : check_container_feature<typename CascadedContainer<ContainerRef, depth>::base_traits::base_container, pure_sparse> {};

template <typename ContainerRef, int depth>
struct spec_object_traits< CascadedContainer<ContainerRef, depth> >
   : spec_object_traits<is_container> {
   using base_traits = typename CascadedContainer<ContainerRef, depth>::base_traits;
   static constexpr bool
      is_temporary = true,
      is_always_const = is_effectively_const<ContainerRef>::value || object_traits<typename base_traits::base_container>::is_always_const;
};

template <typename Container>
auto cascade(Container&& c)
{
   return CascadedContainer<Container>(std::forward<Container>(c));
}

template <int depth, typename Container>
auto cascade(Container&& c, int_constant<depth>)
{
   return CascadedContainer<Container, depth>(std::forward<Container>(c));
}

} // end namespace pm

namespace polymake {
   using pm::cascade;
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
