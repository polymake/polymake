/* Copyright (c) 1997-2018
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

#ifndef POLYMAKE_INDEXED_SUBSET_H
#define POLYMAKE_INDEXED_SUBSET_H

#include "polymake/GenericSet.h"
#include "polymake/internal/Wary.h"

namespace pm {

template <typename Iterator1, typename Iterator2, bool UseIndex1, bool Renumber, bool Reversed=false>
class indexed_selector
   : public Iterator1 {
public:
   using first_type = Iterator1;
   using second_type = Iterator2;

   Iterator2 second;
protected:
   typedef bool_constant<UseIndex1> pos_discr;

   // 0 - general case
   // 1 - sequence::iterator
   // 2 - series::iterator
   static constexpr int step_kind= UseIndex1 ? 0 :
                                   (is_derived_from<Iterator2, sequence::iterator>::value +
                                    is_derived_from<Iterator2, series::iterator>::value) *
                                   std::is_same<typename accompanying_iterator<Iterator2>::type, sequence::iterator>::value;
   using step_discr = int_constant<step_kind>;
private:
   int get_pos1(std::false_type) const
   {
      return *second;
   }
   static
   int get_pos1(std::false_type, int expected)
   {
      return expected;
   }
   int get_pos1(std::true_type, int expected=0) const
   {
      return static_cast<const first_type&>(*this).index();
   }
   void forw_impl(int_constant<0>)
   {
      const int pos=get_pos1(pos_discr());
      ++second;
      if (!at_end())
         std::advance(static_cast<first_type&>(*this), Reversed ? pos-*second : *second-pos);
   }
   void forw_impl(int_constant<1>)
   {
      ++second;
      first_type::operator++();
   }
   void forw_impl(int_constant<2>)
   {
      ++second;
      if (!at_end())
         std::advance(static_cast<first_type&>(*this), second.step());
   }
   void back_impl(int_constant<0>)
   {
      if (second.at_end()) {
         --second;
      } else {
         const int pos=get_pos1(pos_discr());
         --second;
         std::advance(static_cast<first_type&>(*this), Reversed ? pos-*second : *second-pos);
      }
   }
   void back_impl(int_constant<1>)
   {
      --second;
      first_type::operator--();
   }
   void back_impl(int_constant<2>)
   {
      if (second.at_end()) {
         --second;
      } else {
         --second;
         std::advance(static_cast<first_type&>(*this), -second.step());
      }
   }
   int step_impl(int i, int_constant<0>)
   {
      const int pos=second.at_end() ? second[-1] : get_pos1(pos_discr());
      second += i;
      return Reversed ? pos-(second.at_end() ? second[-1] : *second) : (second.at_end() ? second[-1] : *second)-pos;
   }
   int step_impl(int i, int_constant<1>)
   {
      second+=i;
      return i;
   }
   int step_impl(int i, int_constant<2>)
   {
      second+=i;
      return second.step()*i;
   }

   void contract1(int i, bool, std::false_type)
   {
      std::advance(static_cast<first_type&>(*this), i);
   }
   void contract1(int i, bool renumber, std::true_type)
   {
      first_type::contract(renumber, i);
   }
   void contract1(int i, bool renumber)
   {
      contract1(Reversed ? -i : i, renumber, bool_constant<check_iterator_feature<Iterator1, contractable>::value>());
   }
public:
   using iterator_category = typename least_derived_class<typename iterator_traits<Iterator1>::iterator_category,
                                                          typename iterator_traits<Iterator2>::iterator_category>::type;
   using difference_type = typename iterator_traits<Iterator2>::difference_type;
   using iterator = indexed_selector<typename iterator_traits<Iterator1>::iterator, Iterator2, UseIndex1, Renumber, Reversed>;
   using const_iterator = indexed_selector<typename iterator_traits<Iterator1>::const_iterator, Iterator2, UseIndex1, Renumber, Reversed>;

   indexed_selector() = default;

   indexed_selector(const iterator& it)
      : first_type(static_cast<const typename iterator::first_type&>(it))
      , second(it.second) {}

   template <typename SourceIterator1, typename SourceIterator2,
             typename suitable1=typename suitable_arg_for_iterator<SourceIterator1, Iterator1>::type,
             typename suitable2=typename suitable_arg_for_iterator<SourceIterator2, Iterator2>::type>
   indexed_selector(const SourceIterator1& first_arg, const SourceIterator2& second_arg, bool adjust=false, int expected_pos1=0)
      : first_type(prepare_iterator_arg<Iterator1>(first_arg))
      , second(prepare_iterator_arg<Iterator2>(second_arg))
   {
      if (adjust && !at_end()) contract1(*second-get_pos1(pos_discr(), expected_pos1), !Renumber);
   }

   template <typename SourceIterator1, typename SourceIterator2,
             typename suitable1=typename suitable_arg_for_iterator<SourceIterator1, Iterator1>::type,
             typename suitable2=typename suitable_arg_for_iterator<SourceIterator2, Iterator2>::type>
   indexed_selector(const SourceIterator1& first_arg, const SourceIterator2& second_arg, int offset)
      : first_type(prepare_iterator_arg<Iterator1>(first_arg))
      , second(prepare_iterator_arg<Iterator2>(second_arg))
   {
      if (offset) contract1(offset, !Renumber);
   }

   indexed_selector& operator++ ()
   {
      forw_impl(step_discr());
      return *this;
   }
   const indexed_selector operator++ (int) { indexed_selector copy=*this; operator++(); return copy; }

   indexed_selector& operator--()
   {
      static_assert(iterator_pair_traits<Iterator1, Iterator2>::is_bidirectional, "iterator is not bidirectional");
      back_impl(step_discr());
      return *this;
   }
   const indexed_selector operator-- (int) { indexed_selector copy=*this;  operator--();  return copy; }

   indexed_selector& operator+= (int i)
   {
      static_assert(iterator_pair_traits<Iterator1, Iterator2>::is_random, "iterator is not random-access");
      static_cast<first_type&>(*this) += step_impl(i, step_discr());
      return *this;
   }
   indexed_selector& operator-= (int i)
   {
      static_assert(iterator_pair_traits<Iterator1, Iterator2>::is_random, "iterator is not random-access");
      static_cast<first_type&>(*this) += step_impl(-i, step_discr());
      return *this;
   }

   indexed_selector operator+ (int i) const { indexed_selector copy=*this; return copy+=i; }
   indexed_selector operator- (int i) const { indexed_selector copy=*this; return copy-=i; }
   friend indexed_selector operator+ (int i, const indexed_selector& it) { return it+i; }

   template <typename Other>
   std::enable_if_t<is_derived_from_any<Other, iterator, const_iterator>::value, bool>
   operator- (const Other& it) const
   {
      static_assert(iterator_traits<second_type>::is_random, "iterator is not random-access");
      return second - it.second;
   }

   typename first_type::reference operator[] (int i) const
   {
      static_assert(iterator_pair_traits<Iterator1, Iterator2>::is_random, "iterator is not random-access");
      return static_cast<const first_type&>(*this)[ second[i] - (second.at_end() ? second[-1] : get_pos1(pos_discr())) ];
   }

   template <typename Other>
   std::enable_if_t<is_derived_from_any<Other, iterator, const_iterator>::value, bool>
   operator== (const Other& it) const
   {
      return second==it.second;
   }
   template <typename Other>
   std::enable_if_t<is_derived_from_any<Other, iterator, const_iterator>::value, bool>
   operator!= (const Other& it) const
   {
      return !operator==(it);
   }

   bool at_end() const
   {
      return second.at_end();
   }
private:
   int index_impl(std::false_type) const { return *second; }
   int index_impl(std::true_type) const { return second.index(); }
public:
   int index() const
   {
      return index_impl(bool_constant<Renumber>());
   }

   void rewind()
   {
      static_assert(check_iterator_feature<Iterator1, rewindable>::value && check_iterator_feature<Iterator2, rewindable>::value,
                    "iterator is not rewindable");
      first_type::rewind();
      second.rewind();
   }

   void contract(bool renumber, int distance_front, int distance_back=0)
   {
      static_assert(check_iterator_feature<Iterator2, contractable>::value, "iterator is not contractable");
      const int pos=get_pos1(pos_discr());
      second.contract(Renumber && renumber, distance_front, distance_back);
      contract1(*second-pos, !Renumber && renumber);
   }
};

template <typename Iterator1, typename Iterator2, bool UseIndex1, bool Renumber, bool Reversed, typename Feature>
struct check_iterator_feature<indexed_selector<Iterator1, Iterator2, UseIndex1, Renumber, Reversed>, Feature>
   : bool_constant<((is_among<Feature, end_sensitive, contractable>::value ||
                     check_iterator_feature<Iterator1, Feature>::value) &&
                    check_iterator_feature<Iterator2, Feature>::value)> {};

template <typename Iterator1, typename Iterator2, bool UseIndex1, bool Renumber, bool Reversed>
struct check_iterator_feature<indexed_selector<Iterator1, Iterator2, UseIndex1, Renumber, Reversed>, indexed>
   : bool_constant<(!Renumber || check_iterator_feature<Iterator2, indexed>::value)> {};

template <bool Renumber, bool Reverse>
struct sparse_indexed_selector_coupler {
   using Controller = std::conditional_t<Reverse, reverse_zipper<set_intersection_zipper>, set_intersection_zipper>;
   template <typename Iterator1, typename Iterator2, typename ExpectedFeatures>
   struct defs {
      typedef iterator_zipper<Iterator1, Iterator2, operations::cmp, Controller, true, false> iterator;
      using needed_features1 = ExpectedFeatures;  // is already sparse
      using needed_features2 = typename mix_features<needed_features1,
                                 typename mlist_prepend_if<Renumber, provide_construction<indexed>, end_sensitive>::type>::type;
   };
};

template <bool Renumber, bool Reverse>
struct reverse_coupler< sparse_indexed_selector_coupler<Renumber, Reverse> > {
   using type = sparse_indexed_selector_coupler<Renumber, !Reverse>;
};

template <typename Top, typename Params> class indexed_subset_typebase;

namespace subset_classifier {
   enum kind { generic, sparse, plain, contiguous, range };

   template <typename ContainerRef,
             bool maybe=is_derived_from_instance_of<typename deref<ContainerRef>::type, modified_container_typebase>::value>
   struct detect_set_of_indices : std::false_type {};

   template <typename ContainerRef>
   struct detect_set_of_indices<ContainerRef, true>
      : std::is_same<typename deref<ContainerRef>::type::operation, BuildUnaryIt<operations::index2element> > {};

   template <typename ContainerRef,
             bool maybe=is_derived_from_instance_of<typename deref<ContainerRef>::type, indexed_subset_typebase>::value>
   struct detect_indexed_slice : std::false_type {};

   template <typename ContainerRef>
   struct detect_indexed_slice<ContainerRef, true>
      : is_among< typename deref<ContainerRef>::type::operation,
                  operations::apply2< BuildUnaryIt<operations::index2element> >,
                  pair< nothing, operations::apply2< BuildUnaryIt<operations::index2element> > >,
                  pair< operations::apply2< BuildUnaryIt<operations::index2element> >,
                        operations::apply2< BuildUnaryIt<operations::index2element> > >
                > {};

   template <typename ContainerRef,
             bool Renumber=true,
             typename TTag=typename object_traits<typename deref<ContainerRef>::type>::generic_tag>
   struct index2element : std::false_type {};

   template <typename ContainerRef>
   struct index2element<ContainerRef, true, is_set> {
      static const bool value = detect_set_of_indices<ContainerRef>::value || detect_indexed_slice<ContainerRef>::value;
   };

   template <typename ContainerRef1, typename ContainerRef2, bool Renumber>
   struct index_helper {
      static constexpr bool random=iterator_traits<typename container_traits<ContainerRef1>::iterator>::is_random;
      static constexpr bool use_index1= Renumber && check_container_feature<typename deref<ContainerRef1>::type, sparse_compatible>::value;
      static constexpr kind
      value = check_container_feature<typename deref<ContainerRef1>::type, pm::sparse>::value ||
                 index2element<ContainerRef1, Renumber>::value
                 ? sparse :
                 std::is_same<typename container_traits<ContainerRef2>::iterator, sequence::iterator>::value && !use_index1
                 ? plain
                 : generic;
   };

   template <typename IteratorPair, typename Operation>
   struct iterator_helper {
      using iterator = binary_transform_iterator<IteratorPair, Operation>;
   };
   template <typename IteratorPair>
   struct iterator_helper<IteratorPair, void> {
      using iterator = IteratorPair;
   };
}

template <typename> class RenumberTag {};
template <typename> class HintTag {};

template <typename Top, typename Params>
class indexed_subset_typebase : public manip_container_top<Top, Params> {
public:
   using container1_ref_raw = typename extract_container_ref<Params, Container1RefTag, Container1Tag>::type;
   using container2_ref = typename extract_container_ref<Params, Container2RefTag, Container2Tag>::type;
   typedef effectively_const_t<container1_ref_raw> container1_ref;
   typedef typename deref<container1_ref>::minus_ref container1;
   typedef typename deref<container2_ref>::minus_ref container2;

   static constexpr bool renumber=tagged_list_extract_integral<Params, RenumberTag>(false);

   using index_helper = subset_classifier::index_helper<container1_ref, container2_ref, renumber>;

   using data_container = container1;
   using index_container = container2;
   using reference = typename container_traits<data_container>::reference;
   using const_reference = typename container_traits<data_container>::const_reference;
   using value_type = typename container_traits<data_container>::value_type;
   using container_category = typename least_derived_class<typename container_traits<data_container>::category,
                                                           typename container_traits<index_container>::category>::type;

   using operation = typename mselect<std::enable_if< mtagged_list_extract<Params, OperationTag>::is_specified,
                                                      typename mtagged_list_extract<Params, OperationTag>::type >,
                                      std::enable_if< subset_classifier::index2element<container1_ref, renumber>::value,
                                                      pair< operations::apply2<BuildUnaryIt<operations::index2element>>,
                                                            operations::apply2<BuildUnaryIt<operations::index2element>> > >,
                                      std::enable_if< renumber && index_helper::value == subset_classifier::sparse,
                                                      pair< nothing,
                                                            operations::apply2<BuildUnaryIt<operations::index2element>> > >,
                                      void>::type;

   using must_enforce_features = typename mlist_prepend_if<renumber, indexed, mlist<end_sensitive, rewindable>>::type;
   using expected_features = typename manip_container_top<Top, Params>::expected_features;

protected:
   static constexpr bool at_end_required = mlist_contains<expected_features, end_sensitive, absorbing_feature>::value,
                         rewind_required = mlist_contains<expected_features, rewindable, absorbing_feature>::value,
                          index_required = renumber && (mlist_contains<expected_features, indexed, absorbing_feature>::value ||
                                                        check_container_feature<data_container, sparse_compatible>::value);

   static constexpr subset_classifier::kind suggested_kind=
      subset_classifier::kind(std::is_same<typename mtagged_list_extract<Params, HintTag>::type, sparse>::value
                              ? subset_classifier::sparse : index_helper::value);
public:
   static constexpr subset_classifier::kind kind=subset_classifier::kind(
      suggested_kind == subset_classifier::plain
      ? ( at_end_required
          ? (index_helper::random
             ? subset_classifier::range : subset_classifier::generic) :
          index_required
          ? (index_helper::random
             ? subset_classifier::contiguous : subset_classifier::generic) :
          rewind_required
          ? subset_classifier::contiguous : subset_classifier::plain )
      : suggested_kind);
protected:
   using expected_features1 = typename mlist_difference<expected_features, typename mlist_prepend_if<renumber, indexed, end_sensitive>::type>::type;

   using needed_features1 = std::conditional_t<rewind_required,
                                               typename mlist_replace<expected_features1, rewindable, provide_construction<rewindable,false> >::type,
                                               expected_features1>;

   using needed_features2 = std::conditional_t<index_required,
                                               typename mix_features<expected_features,
                                                                     mlist<provide_construction<indexed>, end_sensitive> >::type,
                                               typename mix_features<typename mlist_match_all<expected_features, indexed, absorbing_feature>::complement,
                                                                     end_sensitive >::type >;

   static bool constexpr use_index1=index_helper::use_index1;
public:
   int size() const
   {
      return this->manip_top().get_container2().size();
   }
   int max_size() const
   {
      return size();
   }
   bool empty() const
   {
      return this->manip_top().get_container2().empty();
   }
};

template <typename Top, typename Params,
          subset_classifier::kind Kind=indexed_subset_typebase<Top, Params>::kind,
          typename Category=typename indexed_subset_typebase<Top, Params>::container_category>
class indexed_subset_elem_access
   : public indexed_subset_typebase<Top, Params> {
   using base_t = indexed_subset_typebase<Top, Params>;
public:
   using iterator = indexed_selector<typename ensure_features<typename base_t::data_container, typename base_t::needed_features1>::iterator,
                                     typename ensure_features<typename base_t::index_container, typename base_t::needed_features2>::const_iterator,
                                     base_t::use_index1, base_t::renumber>;
   using const_iterator = indexed_selector<typename ensure_features<typename base_t::data_container, typename base_t::needed_features1>::const_iterator,
                                           typename ensure_features<typename base_t::index_container, typename base_t::needed_features2>::const_iterator,
                                           base_t::use_index1, base_t::renumber>;

   iterator begin()
   {
      auto&& c1=this->manip_top().get_container1();
      return iterator(ensure(c1, typename base_t::needed_features1()).begin(),
                      ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).begin(),
                      true, 0);
   }
   iterator end()
   {
      auto&& c1=this->manip_top().get_container1();
      const auto& indices=this->manip_top().get_container2();
      return iterator(ensure(c1, typename base_t::needed_features1()).end(),
                      ensure(indices, typename base_t::needed_features2()).end(),
                      !iterator_traits<typename iterator::first_type>::is_bidirectional || indices.empty() ? 0 : indices.back()-int(c1.size()));
   }
   const_iterator begin() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).begin(),
                            ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).begin(),
                            true, 0);
   }
   const_iterator end() const
   {
      const auto& c1=this->manip_top().get_container1();
      const auto& indices=this->manip_top().get_container2();
      return const_iterator(ensure(c1, typename base_t::needed_features1()).end(),
                            ensure(indices, typename base_t::needed_features2()).end(),
                            !iterator_traits<typename iterator::first_type>::is_bidirectional || indices.empty() ? 0 : indices.back()-int(c1.size()));
   }
};

template <typename Top, typename Params, subset_classifier::kind Kind>
class indexed_subset_elem_access<Top, Params, Kind, forward_iterator_tag>
   : public indexed_subset_elem_access<Top, Params, Kind, input_iterator_tag> {
public:
   decltype(auto) front() { return *(this->begin()); }
   decltype(auto) front() const { return *(this->begin()); }
};

template <typename Top, typename Params, subset_classifier::kind Kind>
class indexed_subset_rev_elem_access
   : public indexed_subset_elem_access<Top, Params, Kind, forward_iterator_tag> {
   using base_t = indexed_subset_elem_access<Top, Params, Kind, forward_iterator_tag>;
public:
   using reverse_iterator = indexed_selector<typename ensure_features<typename base_t::data_container, typename base_t::needed_features1>::reverse_iterator,
                                             typename ensure_features<typename base_t::index_container, typename base_t::needed_features2>::const_reverse_iterator,
                                             base_t::use_index1, base_t::renumber, true>;
   using const_reverse_iterator = indexed_selector<typename ensure_features<typename base_t::data_container, typename base_t::needed_features1>::const_reverse_iterator,
                                                   typename ensure_features<typename base_t::index_container, typename base_t::needed_features2>::const_reverse_iterator,
                                                   base_t::use_index1, base_t::renumber, true>;

   reverse_iterator rbegin()
   {
      auto&& c1=this->manip_top().get_container1();
      return reverse_iterator(ensure(c1, typename base_t::needed_features1()).rbegin(),
                              ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).rbegin(),
                              true, int(c1.size())-1);
   }
   reverse_iterator rend()
   {
      auto&& c1=this->manip_top().get_container1();
      const auto& indices=this->manip_top().get_container2();
      return reverse_iterator(ensure(c1, typename base_t::needed_features1()).rend(),
                              ensure(indices, typename base_t::needed_features2()).rend(),
                              !iterator_traits<typename reverse_iterator::first_type>::is_bidirectional || indices.empty() ? 0 : indices.front()+1);
   }
   const_reverse_iterator rbegin() const
   {
      const auto& c1=this->manip_top().get_container1();
      return const_reverse_iterator(ensure(c1, typename base_t::needed_features1()).rbegin(),
                                    ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).rbegin(),
                                    true, int(c1.size())-1);
   }
   const_reverse_iterator rend() const
   {
      const auto& indices=this->manip_top().get_container2();
      return const_reverse_iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).rend(),
                                    ensure(indices, typename base_t::needed_features2()).rend(),
                                    !iterator_traits<typename reverse_iterator::first_type>::is_bidirectional || indices.empty() ? 0 : indices.front()+1);
   }
};

template <typename Top, typename Params, subset_classifier::kind Kind>
class indexed_subset_elem_access<Top, Params, Kind, bidirectional_iterator_tag>
   : public indexed_subset_rev_elem_access<Top, Params, Kind> {
public:
   decltype(auto) back() { return *(this->rbegin()); }
   decltype(auto) back() const { return *(this->rbegin()); }
};

template <typename Top, typename Params>
class indexed_subset_elem_access<Top, Params, subset_classifier::plain, input_iterator_tag>
   : public indexed_subset_typebase<Top, Params> {
   using base_t = indexed_subset_typebase<Top, Params>;
protected:
   using needed_features1 = typename base_t::expected_features;
public:
   using iterator = typename ensure_features<typename base_t::container1, needed_features1>::iterator;
   using const_iterator = typename ensure_features<typename base_t::container1, needed_features1>::const_iterator;

   iterator begin()
   {
      auto&& c1=this->manip_top().get_container1();
      iterator b=ensure(c1, needed_features1()).begin();
      std::advance(b, this->manip_top().get_container2().front());
      return b;
   }
   iterator end()
   {
      auto&& c1=this->manip_top().get_container1();
      if (iterator_traits<iterator>::is_bidirectional) {
         iterator e=ensure(c1, needed_features1()).end();
         std::advance(e, this->manip_top().get_container2().back()+1-int(c1.size()));
         return e;
      } else {
         iterator b=ensure(c1, needed_features1()).begin();
         std::advance(b, this->manip_top().get_container2().back()+1);
         return b;
      }
   }
   const_iterator begin() const
   {
      const_iterator b=ensure(this->manip_top().get_container1(), needed_features1()).begin();
      std::advance(b, this->manip_top().get_container2().front());
      return b;
   }
   const_iterator end() const
   {
      const auto& c1=this->manip_top().get_container1();
      if (iterator_traits<const_iterator>::is_bidirectional) {
         const_iterator e=ensure(c1, needed_features1()).end();
         std::advance(e, this->manip_top().get_container2().back()+1-int(c1.size()));
         return e;
      } else {
         const_iterator b=ensure(c1, needed_features1()).begin();
         std::advance(b, this->manip_top().get_container2().back()+1);
         return b;
      }
   }
};

template <typename Top, typename Params>
class indexed_subset_rev_elem_access<Top, Params, subset_classifier::plain>
   : public indexed_subset_elem_access<Top, Params, subset_classifier::plain, forward_iterator_tag> {
   using base_t = indexed_subset_elem_access<Top, Params, subset_classifier::plain, forward_iterator_tag>;
public:
   using reverse_iterator = typename ensure_features<typename base_t::container1, typename base_t::needed_features1>::reverse_iterator;
   using const_reverse_iterator = typename ensure_features<typename base_t::container1, typename base_t::needed_features1>::const_reverse_iterator;

   reverse_iterator rbegin()
   {
      auto&& c1=this->manip_top().get_container1();
      reverse_iterator rb=ensure(c1, typename base_t::needed_features1()).rbegin();
      std::advance(rb, int(c1.size())-1 - this->manip_top().get_container2().back());
      return rb;
   }
   reverse_iterator rend()
   {
      auto&& c1=this->manip_top().get_container1();
      if (iterator_traits<reverse_iterator>::is_bidirectional) {
         reverse_iterator re=ensure(c1, typename base_t::needed_features1()).rend();
         std::advance(re, -this->manip_top().get_container2().front());
         return re;
      } else {
         reverse_iterator rb=ensure(c1, typename base_t::needed_features1()).rbegin();
         std::advance(rb, int(c1.size()) - this->manip_top().get_container2().front());
         return rb;
      }
   }
   const_reverse_iterator rbegin() const
   {
      const auto& c1=this->manip_top().get_container1();
      const_reverse_iterator rb=ensure(c1, typename base_t::needed_features1()).rbegin();
      std::advance(rb, int(c1.size())-1 - this->manip_top().get_container2().back());
      return rb;
   }
   const_reverse_iterator rend() const
   {
      if (iterator_traits<reverse_iterator>::is_bidirectional) {
         const_reverse_iterator re=ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).rend();
         std::advance(re, -this->manip_top().get_container2().front());
         return re;
      } else {
         const_reverse_iterator rb=ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).rbegin();
         std::advance(rb, int(this->manip_top().get_container1().size()) - this->manip_top().get_container2().front());
         return rb;
      }
   }
};

template <typename Top, typename Params>
class indexed_subset_elem_access<Top, Params, subset_classifier::contiguous, input_iterator_tag>
   : public indexed_subset_typebase<Top, Params> {
   using base_t = indexed_subset_typebase<Top, Params>;
protected:
   using enforce_features1 = typename mlist_prepend_if<mlist_contains<typename base_t::expected_features, indexed, absorbing_feature>::value,
                                                       provide_construction<indexed, false>,
                             typename mlist_prepend_if<mlist_contains<typename base_t::expected_features, rewindable, absorbing_feature>::value,
                                                       provide_construction<rewindable, false>,
                                                       mlist<>>::type >::type;
   using needed_features1 = typename mix_features<typename base_t::expected_features, enforce_features1>::type;
public:
   using iterator = typename ensure_features<typename base_t::container1, needed_features1>::iterator;
   using const_iterator = typename ensure_features<typename base_t::container1, needed_features1>::const_iterator;

   iterator begin()
   {
      auto&& c1=this->manip_top().get_container1();
      iterator b=ensure(c1, needed_features1()).begin();
      b.contract(base_t::renumber, this->manip_top().get_container2().front());
      return b;
   }
   iterator end()
   {
      iterator b=begin();
      std::advance(b, this->size());
      return b;
   }
   const_iterator begin() const
   {
      const_iterator b=ensure(this->manip_top().get_container1(), needed_features1()).begin();
      b.contract(base_t::renumber, this->manip_top().get_container2().front());
      return b;
   }
   const_iterator end() const
   {
      const_iterator b=begin();
      std::advance(b, this->size());
      return b;
   }
};

template <typename Top, typename Params>
class indexed_subset_rev_elem_access<Top, Params, subset_classifier::contiguous>
   : public indexed_subset_elem_access<Top, Params, subset_classifier::contiguous, forward_iterator_tag> {
   using base_t = indexed_subset_elem_access<Top, Params, subset_classifier::contiguous, forward_iterator_tag>;
public:
   using reverse_iterator = typename ensure_features<typename base_t::container1, typename base_t::needed_features1>::reverse_iterator;
   using const_reverse_iterator = typename ensure_features<typename base_t::container1, typename base_t::needed_features1>::const_reverse_iterator;

   reverse_iterator rbegin()
   {
      auto&& c1=this->manip_top().get_container1();
      const auto& indices=this->manip_top().get_container2();
      reverse_iterator rb=ensure(c1, typename base_t::needed_features1()).rbegin();
      rb.contract(base_t::renumber,  int(c1.size())-1 - indices.back(),  base_t::index_required ? indices.front() : 0);
      return rb;
   }
   reverse_iterator rend()
   {
      reverse_iterator rb=rbegin();
      std::advance(rb, this->size());
      return rb;
   }
   const_reverse_iterator rbegin() const
   {
      const auto& c1=this->manip_top().get_container1();
      const auto& indices=this->manip_top().get_container2();
      const_reverse_iterator rb=ensure(c1, typename base_t::needed_features1()).rbegin();
      rb.contract(base_t::renumber,  int(c1.size())-1 - indices.back(),  base_t::index_required ? indices.front() : 0);
      return rb;
   }
   const_reverse_iterator rend() const
   {
      const_reverse_iterator rb=rbegin();
      std::advance(rb, this->size());
      return rb;
   }
};

template <typename Top, typename Params>
class indexed_subset_elem_access<Top, Params, subset_classifier::range, input_iterator_tag>
   : public indexed_subset_typebase<Top, Params> {
   using base_t = indexed_subset_typebase<Top, Params>;
protected:
   using enforce_features1 = typename mlist_prepend_if<mlist_contains<typename base_t::expected_features, indexed, absorbing_feature>::value,
                                                       provide_construction<indexed, false>,
                             typename mlist_prepend_if<mlist_contains<typename base_t::expected_features, rewindable, absorbing_feature>::value,
                                                       provide_construction<rewindable, false>,
                                                       provide_construction<end_sensitive, false> >::type >::type;
   using needed_features1 = typename mix_features<typename base_t::expected_features, enforce_features1>::type;
public:
   using iterator = typename ensure_features<typename base_t::container1, needed_features1>::iterator;
   using const_iterator = typename ensure_features<typename base_t::container1, needed_features1>::const_iterator;

   iterator begin()
   {
      auto&& c1=this->manip_top().get_container1();
      const auto& indices=this->manip_top().get_container2();
      iterator b=ensure(c1, needed_features1()).begin();
      b.contract(base_t::renumber, indices.front(), int(c1.size())-1-indices.back());
      return b;
   }
   iterator end()
   {
      iterator b=begin();
      b += this->size();
      return b;
   }
   const_iterator begin() const
   {
      const auto& c1=this->manip_top().get_container1();
      const auto& indices=this->manip_top().get_container2();
      const_iterator b=ensure(c1, needed_features1()).begin();
      b.contract(base_t::renumber, indices.front(), int(c1.size())-1-indices.back());
      return b;
   }
   const_iterator end() const
   {
      const_iterator b=begin();
      b += this->size();
      return b;
   }
};

template <typename Top, typename Params>
class indexed_subset_rev_elem_access<Top, Params, subset_classifier::range>
   : public indexed_subset_elem_access<Top, Params, subset_classifier::range, forward_iterator_tag> {
   using base_t = indexed_subset_elem_access<Top, Params, subset_classifier::range, forward_iterator_tag>;
public:
   using reverse_iterator = typename ensure_features<typename base_t::container1, typename base_t::needed_features1>::reverse_iterator;
   using const_reverse_iterator = typename ensure_features<typename base_t::container1, typename base_t::needed_features1>::const_reverse_iterator;

   reverse_iterator rbegin()
   {
      auto&& c1=this->manip_top().get_container1();
      const auto& indices=this->manip_top().get_container2();
      reverse_iterator rb=ensure(c1, typename base_t::needed_features1()).rbegin();
      rb.contract(base_t::renumber, int(c1.size())-1 - indices.back(), indices.front());
      return rb;
   }
   reverse_iterator rend()
   {
      reverse_iterator rb=rbegin();
      rb += this->size();
      return rb;
   }
   const_reverse_iterator rbegin() const
   {
      const auto& c1=this->manip_top().get_container1();
      const auto& indices=this->manip_top().get_container2();
      const_reverse_iterator rb=ensure(c1, typename base_t::needed_features1()).rbegin();
      rb.contract(base_t::renumber, int(c1.size())-1 - indices.back(), indices.front());
      return rb;
   }
   const_reverse_iterator rend() const
   {
      const_reverse_iterator rb=rbegin();
      rb += this->size();
      return rb;
   }
};

template <typename Top, typename Params, subset_classifier::kind Kind>
class indexed_subset_elem_access<Top, Params, Kind, random_access_iterator_tag>
   : public indexed_subset_elem_access<Top, Params, Kind, bidirectional_iterator_tag> {
public:
   decltype(auto) front()
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2().front() ];
   }
   decltype(auto) front() const
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2().front() ];
   }
   decltype(auto) back()
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2().back() ];
   }
   decltype(auto) back() const
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2().back() ];
   }
   decltype(auto) operator[] (int i)
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2()[i] ];
   }
   decltype(auto) operator[] (int i) const
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2()[i] ];
   }
};

template <typename Top, typename Params>
class indexed_subset_elem_access<Top, Params, subset_classifier::sparse, forward_iterator_tag>
   : public indexed_subset_typebase<Top, Params> {
   using base_t = indexed_subset_typebase<Top, Params>;
protected:
   using it_coupler = typename mtagged_list_extract<Params, IteratorCouplerTag, sparse_indexed_selector_coupler<base_t::renumber, false> >::type;
   using needed_features1 = typename it_coupler::template defs<typename container_traits<typename base_t::data_container>::iterator,
                                                               typename container_traits<typename base_t::index_container>::const_iterator,
                                                               typename base_t::expected_features>::needed_features1;
   using needed_features2 = typename it_coupler::template defs<typename container_traits<typename base_t::data_container>::const_iterator,
                                                               typename container_traits<typename base_t::index_container>::const_iterator,
                                                               typename base_t::expected_features>::needed_features2;

   using iterator_pair = typename it_coupler::template defs<typename ensure_features<typename base_t::data_container, needed_features1>::iterator,
                                                            typename ensure_features<typename base_t::index_container, needed_features2>::const_iterator,
                                                            typename base_t::expected_features>::iterator;
   using const_iterator_pair = typename it_coupler::template defs<typename ensure_features<typename base_t::data_container, needed_features1>::const_iterator,
                                                                  typename ensure_features<typename base_t::index_container, needed_features2>::const_iterator,
                                                                  typename base_t::expected_features>::iterator;

public:
   using iterator = typename subset_classifier::iterator_helper<iterator_pair, typename base_t::operation>::iterator;
   using const_iterator = typename subset_classifier::iterator_helper<const_iterator_pair, typename base_t::operation>::iterator;

   iterator begin()
   {
      auto&& c1=this->manip_top().get_container1();
      return iterator(ensure(c1, needed_features1()).begin(),
                      ensure(this->manip_top().get_container2(), needed_features2()).begin());
   }
   iterator end()
   {
      auto&& c1=this->manip_top().get_container1();
      return iterator(ensure(c1, needed_features1()).end(),
                      ensure(this->manip_top().get_container2(), needed_features2()).end());
   }
   const_iterator begin() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), needed_features1()).begin(),
                            ensure(this->manip_top().get_container2(), needed_features2()).begin());
   }
   const_iterator end() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), needed_features1()).end(),
                            ensure(this->manip_top().get_container2(), needed_features2()).end());
   }

   int size() const { return count_it(begin()); }
   bool empty() const { return begin().at_end(); }

   decltype(auto) front() { return *begin(); }
   decltype(auto) front() const { return *begin(); }
};

template <typename Top, typename Params>
class indexed_subset_elem_access<Top, Params, subset_classifier::sparse, bidirectional_iterator_tag>
   : public indexed_subset_elem_access<Top, Params, subset_classifier::sparse, forward_iterator_tag> {
   using base_t = indexed_subset_elem_access<Top, Params, subset_classifier::sparse, forward_iterator_tag>;
protected:
   using rev_coupler = typename reverse_coupler<typename base_t::it_coupler>::type;
public:
   using reverse_iterator_pair = typename rev_coupler::template defs<typename ensure_features<typename base_t::data_container, typename base_t::needed_features1>::reverse_iterator,
                                                                     typename ensure_features<typename base_t::index_container, typename base_t::needed_features2>::const_reverse_iterator,
                                                                     typename base_t::expected_features>::iterator;
   using const_reverse_iterator_pair = typename rev_coupler::template defs<typename ensure_features<typename base_t::data_container, typename base_t::needed_features1>::const_reverse_iterator,
                                                                           typename ensure_features<typename base_t::index_container, typename base_t::needed_features2>::const_reverse_iterator,
                                                                           typename base_t::expected_features>::iterator;
   using reverse_iterator = typename subset_classifier::iterator_helper<reverse_iterator_pair, typename base_t::operation>::iterator;
   using const_reverse_iterator = typename subset_classifier::iterator_helper<const_reverse_iterator_pair, typename base_t::operation>::iterator;

   reverse_iterator rbegin()
   {
      auto&& c1=this->manip_top().get_container1();
      return reverse_iterator(ensure(c1, typename base_t::needed_features1()).rbegin(),
                              ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).rbegin());
   }
   reverse_iterator rend()
   {
      auto&& c1=this->manip_top().get_container1();
      return reverse_iterator(ensure(c1, typename base_t::needed_features1()).rend(),
                              ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).rend());
   }
   const_reverse_iterator rbegin() const
   {
      return const_reverse_iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).rbegin(),
                                    ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).rbegin());
   }
   const_reverse_iterator rend() const
   {
      return const_reverse_iterator(ensure(this->manip_top().get_container1(), typename base_t::needed_features1()).rend(),
                                    ensure(this->manip_top().get_container2(), typename base_t::needed_features2()).rend());
   }

   decltype(auto) back() { return *rbegin(); }
   decltype(auto) back() const { return *rbegin(); }
};

template <typename Top, typename Params=typename Top::manipulator_params>
class indexed_subset_impl
   : public indexed_subset_elem_access<Top, Params> {
public:
   typedef indexed_subset_impl<Top, Params> manipulator_impl;
   typedef Params manipulator_params;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef indexed_subset_impl<FeatureCollector, Params> type;
   };
};

template <typename ContainerRef1, typename ContainerRef2, typename Params=mlist<>> class IndexedSubset;

template <typename ContainerRef1, typename ContainerRef2, typename Params,
          typename Generic1=typename object_traits<typename deref<ContainerRef1>::type>::generic_type,
          typename Generic2=typename object_traits<typename deref<ContainerRef2>::type>::generic_type>
class generic_of_indexed_subset {};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
class IndexedSubset
   : public container_pair_base<ContainerRef1, ContainerRef2>
   , public indexed_subset_impl< IndexedSubset<ContainerRef1, ContainerRef2, Params>,
                                 typename mlist_concat< Container1RefTag<ContainerRef1>, Container2RefTag<ContainerRef2>, Params >::type >
   , public generic_of_indexed_subset<ContainerRef1, ContainerRef2, Params> {
public:
   using container_pair_base<ContainerRef1, ContainerRef2>::container_pair_base;

   int dim() const { return get_dim(this->get_container1()); }
};

template <typename ContainerRef1, typename ContainerRef2, typename Params,
          typename Set1, typename E, typename Comparator, typename Set2>
class generic_of_indexed_subset<ContainerRef1, ContainerRef2, Params,
                                GenericSet<Set1, E, Comparator>, GenericSet<Set2, int, operations::cmp> >
   : public GenericSet<IndexedSubset<ContainerRef1, ContainerRef2, Params>, E, Comparator> {
public:
   decltype(auto) get_comparator() const
   {
      return this->manip_top().get_container1().get_comparator();
   }
};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct spec_object_traits< IndexedSubset<ContainerRef1, ContainerRef2, Params> >
   : spec_object_traits<is_container> {
   static constexpr bool
      is_temporary = true,
      is_lazy = object_traits<typename deref<ContainerRef1>::type>::is_lazy,
      is_always_const = is_effectively_const<ContainerRef1>::value || is_generic_set<ContainerRef1>::value;
};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct check_container_feature<IndexedSubset<ContainerRef1, ContainerRef2, Params>, sparse_compatible>
   : check_container_feature<typename deref<ContainerRef1>::type, sparse_compatible> {};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct check_container_feature<IndexedSubset<ContainerRef1, ContainerRef2, Params>, sparse>
   : check_container_feature<typename deref<ContainerRef1>::type, sparse> {};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct check_container_feature<IndexedSubset<ContainerRef1, ContainerRef2, Params>, pure_sparse>
   : check_container_feature<typename deref<ContainerRef1>::type, pure_sparse> {};

// for index sets of slices, minors, and similar contexts with known full range
class OpenRange
   : public sequence {
public:
   explicit OpenRange(int start)
      : sequence(start, 0) {}

   sequence stretch_dim(int d) const
   {
      // for empty containers, we ignore the start value,
      // because most of the time OpenRange is used instead of a complement ~[0]
      return d ? sequence(start_, d-start_) : sequence(0, 0);
   }
};

template <>
struct spec_object_traits< OpenRange >
   : spec_object_traits<sequence> {};

inline
OpenRange range_from(int start)
{
   return OpenRange(start);
}

template <typename IndexSet>
std::enable_if_t<!check_container_feature<typename Concrete<IndexSet>::type, sparse_compatible>::value, bool>
set_within_range(const IndexSet& s, int d)
{
   const auto& ss = unwary(concrete(s));
   return ss.empty() || (ss.front()>=0 && ss.back()<d);
}

template <typename IndexSet>
std::enable_if_t<check_container_feature<typename Concrete<IndexSet>::type, sparse_compatible>::value, bool>
set_within_range(const IndexSet& s, int d)
{
   return unwary(concrete(s)).dim()<=d;
}

template <typename IndexSet>
bool set_within_range(const Complement<IndexSet>& s, int d)
{
   // as a special case we allow a complement-based slice or minor of an empty vector resp. matrix
   return d==0 || set_within_range(s.base(), d);
}

template <typename IndexSetRef, typename=void>
struct final_index_set {
   using type = add_const_t<unwary_t<IndexSetRef>>;
};

template <typename IndexSet, typename GetDim>
decltype(auto) prepare_index_set(IndexSet&& indices, const GetDim&,
                                 std::enable_if_t<!is_instance_of<pure_type_t<unwary_t<IndexSet>>, Complement>::value &&
                                 !std::is_same<pure_type_t<unwary_t<IndexSet>>, OpenRange>::value, void**> =nullptr)
{
   return unwary(std::forward<IndexSet>(indices));
}

template <typename IndexSet, typename GetDim>
auto prepare_index_set(IndexSet&& indices, const GetDim& d,
                       std::enable_if_t<is_instance_of<pure_type_t<unwary_t<IndexSet>>, Complement>::value, void**> =nullptr)
{
   return pure_type_t<unwary_t<IndexSet>>(unwary(std::forward<IndexSet>(indices)), d());
}

template <typename IndexSetRef>
struct final_index_set<IndexSetRef, std::enable_if_t<std::is_same<pure_type_t<IndexSetRef>, OpenRange>::value>> {
   using type = const sequence;
};

template <typename GetDim>
sequence prepare_index_set(const OpenRange& indices, const GetDim& d)
{
   return indices.stretch_dim(d());
}

template <typename Container, typename IndexSet>
auto select(Container&& c, IndexSet&& indices)
{
   if (POLYMAKE_DEBUG || is_wary<Container>() || is_wary<IndexSet>()) {
      if (!set_within_range(indices, get_dim(unwary(c))))
         throw std::runtime_error("select - indices out of range");
   }
   using result_type = IndexedSubset<unwary_t<Container>, add_const_t<unwary_t<IndexSet>>>;
   return result_type(unwary(std::forward<Container>(c)),
                      prepare_index_set(std::forward<IndexSet>(indices), [&](){ return get_dim(unwary(c)); }));
}

template <typename ContainerRef1, typename ContainerRef2, typename Params=mlist<>>
class IndexedSlice;

template <typename ContainerRef1, typename ContainerRef2, typename Params,
          typename Generic1=typename object_traits<typename deref<ContainerRef1>::type>::generic_type,
          typename Generic2=typename object_traits<typename deref<ContainerRef2>::type>::generic_type>
class generic_of_indexed_slice
   : public inherit_generic< IndexedSlice<ContainerRef1, ContainerRef2, Params>, typename deref<ContainerRef1>::type>::type {};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct IndexedSlice_impl {
   using type = indexed_subset_impl< IndexedSlice<ContainerRef1, ContainerRef2, Params>,
                                     typename mlist_concat< Container1RefTag<ContainerRef1>, Container2RefTag<ContainerRef2>,
                                                            RenumberTag<std::true_type>, Params >::type >;
};

template <typename ContainerRef1, typename ContainerRef2, typename Params,
          bool is_immutable=is_effectively_const<ContainerRef1>::value,
          bool is_sparse=check_container_ref_feature<ContainerRef1, sparse>::value,
          typename tag=typename object_traits<typename deref<ContainerRef1>::type>::generic_tag,
          bool is_bidir=is_derived_from<typename IndexedSlice_impl<ContainerRef1, ContainerRef2, Params>::type::container_category,
                                        bidirectional_iterator_tag>::value>
class IndexedSlice_mod {};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
class IndexedSlice
   : public container_pair_base<ContainerRef1, ContainerRef2>
   , public IndexedSlice_impl<ContainerRef1, ContainerRef2, Params>::type
   , public IndexedSlice_mod<ContainerRef1, ContainerRef2, Params>
   , public generic_of_indexed_slice<ContainerRef1, ContainerRef2, Params> {
   using base_t = container_pair_base<ContainerRef1, ContainerRef2>;
public:
   using generic_mutable_type = typename inherit_generic<IndexedSlice, typename deref<ContainerRef1>::type>::type;

   using container_pair_base<ContainerRef1, ContainerRef2>::container_pair_base;

   IndexedSlice& operator= (const IndexedSlice& other) { return generic_mutable_type::operator=(other); }
   using generic_mutable_type::operator=;

public:
   int dim() const
   {
      return this->get_container2().size();
   }

   template <typename, typename, typename, bool, bool, typename, bool> friend class IndexedSlice_mod;
};

template <typename ContainerRef1, typename ContainerRef2, typename Params,
          typename Set1, typename E, typename Comparator, typename Set2>
class generic_of_indexed_slice<ContainerRef1, ContainerRef2, Params,
                               GenericSet<Set1, E, Comparator>, GenericSet<Set2, int, operations::cmp> >
   : public inherit_generic< IndexedSlice<ContainerRef1, ContainerRef2, Params>, typename deref<ContainerRef1>::type>::type {
public:
   decltype(auto) get_comparator() const
   {
      return this->top().get_container1().get_comparator();
   }
};

// set, forward category
template <typename ContainerRef1, typename ContainerRef2, typename Params>
class IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, false, is_set, false> {
   using master = IndexedSlice<ContainerRef1, ContainerRef2, Params>;
protected:
   using impl_t = typename IndexedSlice_impl<ContainerRef1, ContainerRef2, Params>::type;
private:
   typename impl_t::iterator::second_type rewind_index_impl(const typename impl_t::iterator::second_type&, int i, forward_iterator_tag)
   {
      master& me=static_cast<master&>(*this);
      typename impl_t::iterator::second_type iit=ensure(me.manip_top().get_container2(), typename master::needed_features2()).begin();
      while (iit.index() < i) ++iit;
      return iit;
   }

   typename impl_t::iterator::second_type rewind_index_impl(typename impl_t::iterator::second_type iit, int i, bidirectional_iterator_tag)
   {
      if (iit.at_end()) --iit;
      std::advance(iit, i-iit.index());
      return iit;
   }
protected:
   typename impl_t::iterator::second_type rewind_index(const typename impl_t::iterator::second_type& iit, int i)
   {
      return rewind_index_impl(iit, i, typename iterator_traits<typename impl_t::iterator::second_type>::iterator_category());
   }
public:
   void clear()
   {
      master& me=static_cast<master&>(*this);
      for (typename impl_t::iterator it=me.begin(); !it.at_end(); )
         me.get_container1().erase(it++);
   }

   typename impl_t::iterator insert(const typename impl_t::iterator& pos, int i)
   {
      master& me=static_cast<master&>(*this);
      typename impl_t::iterator::second_type iit=rewind_index(pos.second, i);
      return typename impl_t::iterator(me.get_container1().insert(pos, *iit), iit);
   }

   typename impl_t::iterator insert(int i)
   {
      master& me=static_cast<master&>(*this);
      typename impl_t::iterator::second_type iit=rewind_index(ensure(me.manip_top().get_container2(), typename master::needed_features2()).end(), i);
      return typename impl_t::iterator(me.get_container1().insert(*iit), iit);
   }

   void erase(const typename impl_t::iterator& pos)
   {
      master& me=static_cast<master&>(*this);
      me.get_container1().erase(pos);
   }
};

// set, bidirectional category
template <typename ContainerRef1, typename ContainerRef2, typename Params>
class IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, false, is_set, true>
   : public IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, false, is_set, false> {
   using base_t = IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, false, is_set, false>;
   using master = IndexedSlice<ContainerRef1, ContainerRef2, Params>;
protected:
   using impl_t = typename base_t::impl_t;
private:
   typename impl_t::reverse_iterator::second_type rewind_index_impl(const typename impl_t::reverse_iterator::second_type&, int i, forward_iterator_tag)
   {
      master& me=static_cast<master&>(*this);
      typename impl_t::reverse_iterator::second_type iit=ensure(me.manip_top().get_container2(), typename master::needed_features2()).rbegin();
      while (iit.index() > i) ++iit;
      return iit;
   }

   typename impl_t::reverse_iterator::second_type rewind_index_impl(typename impl_t::reverse_iterator::second_type iit, int i, bidirectional_iterator_tag)
   {
      if (iit.at_end()) --iit;
      std::advance(iit, iit.index()-i);
      return iit;
   }
protected:
   typename impl_t::reverse_iterator::second_type rewind_index(const typename impl_t::reverse_iterator::second_type& iit, int i)
   {
      return rewind_index_impl(iit, i, typename iterator_traits<typename impl_t::reverse_iterator::second_type>::iterator_category());
   }
   using base_t::rewind_index;
public:
   typename impl_t::reverse_iterator insert(const typename impl_t::reverse_iterator& pos, int i)
   {
      master& me=static_cast<master&>(*this);
      typename impl_t::reverse_iterator::second_type iit=rewind_index(pos.second, i);
      return typename impl_t::reverse_iterator(me.get_container1().insert(pos, *iit), iit);
   }
   using base_t::insert;

   void erase(const typename impl_t::reverse_iterator& pos)
   {
      master& me=static_cast<master&>(*this);
      me.get_container1().erase(pos);
   }
   using base_t::erase;
};

// sparse vector, forward category
template <typename ContainerRef1, typename ContainerRef2, typename Params, typename Tag>
class IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, true, Tag, false>
   : public IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, false, is_set> {
   using base_t = IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, false, is_set>;
   using master = IndexedSlice<ContainerRef1, ContainerRef2, Params>;
protected:
   using impl_t = typename base_t::impl_t;
public:
   template <typename Data>
   typename impl_t::iterator insert(const typename impl_t::iterator& pos, int i, const Data& d)
   {
      master& me=static_cast<master&>(*this);
      typename impl_t::iterator::second_type iit=this->rewind_index(pos.second, i);
      return typename impl_t::iterator(me.get_container1().insert(pos, *iit, d), iit);
   }

   template <typename Data>
   typename impl_t::iterator insert(int i, const Data& d)
   {
      master& me=static_cast<master&>(*this);
      typename impl_t::iterator::second_type iit=this->rewind_index(ensure(me.manip_top().get_container2(), typename master::needed_features2()).end(), i);
      return typename impl_t::iterator(me.get_container1().insert(*iit, d), iit);
   }

   using base_t::insert;
};

// sparse vector, bidirectional category
template <typename ContainerRef1, typename ContainerRef2, typename Params, typename Tag>
class IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, true, Tag, true>
   : public IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, true, Tag, false> {
   using base_t = IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, true, Tag, false>;
   using master = IndexedSlice<ContainerRef1, ContainerRef2, Params>;
protected:
   using impl_t = typename base_t::impl_t;
public:
   template <typename Data>
   typename impl_t::reverse_iterator insert(const typename impl_t::reverse_iterator& pos, int i, const Data& d)
   {
      master& me=static_cast<master&>(*this);
      typename impl_t::reverse_iterator::second_type iit=this->rewind_index(pos.second, i);
      return typename impl_t::reverse_iterator(me.get_container1().insert(pos, *iit, d), iit);
   }
   using base_t::insert;
};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct spec_object_traits< IndexedSlice<ContainerRef1, ContainerRef2, Params> >
   : spec_object_traits<is_container> {
   static constexpr bool
      is_temporary = true,
      is_lazy = object_traits<typename deref<ContainerRef1>::type>::is_lazy,
      is_always_const = is_effectively_const<ContainerRef1>::value;
};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct check_container_feature<IndexedSlice<ContainerRef1, ContainerRef2, Params>, sparse_compatible>
   : check_container_ref_feature<ContainerRef1, sparse_compatible> {};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct check_container_feature<IndexedSlice<ContainerRef1, ContainerRef2, Params>, sparse>
   : check_container_ref_feature<ContainerRef1, sparse> {};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct check_container_feature<IndexedSlice<ContainerRef1, ContainerRef2, Params>, pure_sparse>
   : check_container_ref_feature<ContainerRef1, pure_sparse> {};

} // end namespace pm

namespace polymake {

using pm::range_from;
using pm::select;
using pm::IndexedSubset;
using pm::IndexedSlice;

}

namespace std {

// due to silly overloading rules
template <typename ContainerRef1, typename ContainerRef2, typename Params>
void swap(pm::IndexedSlice<ContainerRef1, ContainerRef2, Params>& s1,
          pm::IndexedSlice<ContainerRef1, ContainerRef2, Params>& s2) { s1.swap(s2); }

}

#endif // POLYMAKE_INDEXED_SUBSET_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
