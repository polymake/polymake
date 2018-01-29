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

template <typename Iterator1, typename Iterator2, bool TUseIndex1, bool TRenumber, bool TReversed=false>
class indexed_selector
   : public Iterator1 {
public:
   typedef Iterator1 first_type;
   typedef Iterator2 second_type;

   Iterator2 second;
protected:
   typedef bool_constant<TUseIndex1> pos_discr;

   // 0 - general case
   // 1 - sequence::iterator
   // 2 - series::iterator
   static const int step_kind= TUseIndex1 ? 0 :
                               (is_derived_from<Iterator2, sequence::iterator>::value +
				is_derived_from<Iterator2, series::iterator>::value) *
                               std::is_same<typename accompanying_iterator<Iterator2>::type, sequence::iterator>::value;
   typedef int_constant<step_kind> step_discr;
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
	 std::advance(static_cast<first_type&>(*this), TReversed ? pos-*second : *second-pos);
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
	 std::advance(static_cast<first_type&>(*this), TReversed ? pos-*second : *second-pos);
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
      second+=i;
      return TReversed ? pos-(second.at_end() ? second[-1] : *second) : (second.at_end() ? second[-1] : *second)-pos;
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
      contract1(TReversed ? -i : i, renumber, bool_constant<check_iterator_feature<Iterator1, contractable>::value>());
   }
public:
   typedef typename least_derived_class<typename iterator_traits<Iterator1>::iterator_category,
                                        typename iterator_traits<Iterator2>::iterator_category>::type
      iterator_category;
   typedef typename iterator_traits<Iterator2>::difference_type difference_type;
   typedef indexed_selector<typename iterator_traits<Iterator1>::iterator, Iterator2, TUseIndex1, TRenumber, TReversed> iterator;
   typedef indexed_selector<typename iterator_traits<Iterator1>::const_iterator, Iterator2, TUseIndex1, TRenumber, TReversed> const_iterator;

   indexed_selector() {}
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
      if (adjust && !at_end()) contract1(*second-get_pos1(pos_discr(), expected_pos1), !TRenumber);
   }

   template <typename SourceIterator1, typename SourceIterator2,
             typename suitable1=typename suitable_arg_for_iterator<SourceIterator1, Iterator1>::type,
             typename suitable2=typename suitable_arg_for_iterator<SourceIterator2, Iterator2>::type>
   indexed_selector(const SourceIterator1& first_arg, const SourceIterator2& second_arg, int offset)
      : first_type(prepare_iterator_arg<Iterator1>(first_arg))
      , second(prepare_iterator_arg<Iterator2>(second_arg))
   {
      if (offset) contract1(offset, !TRenumber);
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
   typename std::enable_if<is_derived_from_any<Other, iterator, const_iterator>::value, bool>::type
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
   typename std::enable_if<is_derived_from_any<Other, iterator, const_iterator>::value, bool>::type
   operator== (const Other& it) const
   {
      return second==it.second;
   }
   template <typename Other>
   typename std::enable_if<is_derived_from_any<Other, iterator, const_iterator>::value, bool>::type
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
      return index_impl(bool_constant<TRenumber>());
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
      second.contract(TRenumber && renumber, distance_front, distance_back);
      contract1(*second-pos, !TRenumber && renumber);
   }
};

template <typename Iterator1, typename Iterator2, bool TUseIndex1, bool TRenumber, bool TReversed, typename Feature>
struct check_iterator_feature<indexed_selector<Iterator1, Iterator2, TUseIndex1, TRenumber, TReversed>, Feature> {
   typedef cons<end_sensitive, contractable> via_second;
   static const bool value= (list_contains<via_second, Feature>::value ||
			     check_iterator_feature<Iterator1, Feature>::value) &&
			    check_iterator_feature<Iterator2, Feature>::value;
};

template <typename Iterator1, typename Iterator2, bool TUseIndex1, bool TRenumber, bool TReversed>
struct check_iterator_feature<indexed_selector<Iterator1, Iterator2, TUseIndex1, TRenumber, TReversed>, indexed> {
   static const bool value= !TRenumber || check_iterator_feature<Iterator2, indexed>::value;
};

template <bool TRenumber, bool TReverse>
struct sparse_indexed_selector_coupler {
   typedef typename std::conditional<TReverse, reverse_zipper<set_intersection_zipper>, set_intersection_zipper>::type
      Controller;
   template <typename Iterator1, typename Iterator2, typename ExpectedFeatures>
   struct defs {
      typedef iterator_zipper<Iterator1, Iterator2, operations::cmp, Controller, true, false> iterator;
      typedef ExpectedFeatures needed_features1;  // is already sparse
      typedef typename mix_features<needed_features1,
				    typename concat_if<TRenumber, provide_construction<indexed>, end_sensitive>::type
                                   >::type
         needed_features2;
   };
};

template <bool TRenumber, bool TReverse>
struct reverse_coupler< sparse_indexed_selector_coupler<TRenumber, TReverse> > {
   typedef sparse_indexed_selector_coupler<TRenumber, !TReverse> type;
};

template <typename Top, typename TParams> class indexed_subset_typebase;

template <typename Top, typename Helper> class indexed_subset_complement_top;

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
      : list_contains< list( operations::apply2< BuildUnaryIt<operations::index2element> >,
			     pair< nothing, operations::apply2< BuildUnaryIt<operations::index2element> > >,
			     pair< operations::apply2< BuildUnaryIt<operations::index2element> >,
			           operations::apply2< BuildUnaryIt<operations::index2element> > >),
		       typename deref<ContainerRef>::type::operation > {};

   template <typename ContainerRef,
	     bool TRenumber=true,
	     typename TTag=typename object_traits<typename deref<ContainerRef>::type>::generic_tag>
   struct index2element : std::false_type {};

   template <typename ContainerRef>
   struct index2element<ContainerRef, true, is_set> {
      static const bool value = detect_set_of_indices<ContainerRef>::value || detect_indexed_slice<ContainerRef>::value;
   };

   template <typename ContainerRef1, typename ContainerRef2,
	     bool TRenumber,
             bool TIsComplement=is_derived_from_instance_of<typename deref<ContainerRef2>::type, Complement>::value>
   struct index_helper {
      typedef std::true_type is_complement;
      static const kind
         value = check_container_feature<typename deref<ContainerRef1>::type, pm::sparse>::value ||
                 index2element<ContainerRef1, TRenumber>::value
                 ? sparse : generic;
      static const bool random=false;
      static const bool use_index1=TRenumber && check_container_feature<typename deref<ContainerRef1>::type, sparse_compatible>::value;
      typedef operations::mul<sequence,ContainerRef2> operation;
      typedef typename deref<typename operation::result_type>::type container;
      typedef const container container_ref;
   };

   template <typename ContainerRef1, typename ContainerRef2, bool TRenumber>
   struct index_helper<ContainerRef1, ContainerRef2, TRenumber, false> {
      typedef std::false_type is_complement;
      static const bool random=iterator_traits<typename container_traits<ContainerRef1>::iterator>::is_random;
      static const bool use_index1=TRenumber && check_container_feature<typename deref<ContainerRef1>::type, sparse_compatible>::value;
      static const kind
         value = check_container_feature<typename deref<ContainerRef1>::type, pm::sparse>::value ||
                 index2element<ContainerRef1, TRenumber>::value
                 ? sparse :
                 std::is_same<typename container_traits<ContainerRef2>::iterator, sequence::iterator>::value && !use_index1
                 ? plain
		 : generic;
      typedef typename deref<ContainerRef2>::type container;
      typedef const container& container_ref;
   };

   template <typename IteratorPair, typename Operation>
   struct iterator_helper {
      typedef binary_transform_iterator<IteratorPair, Operation> iterator;
   };
   template <typename IteratorPair>
   struct iterator_helper<IteratorPair, void> {
      typedef IteratorPair iterator;
   };

   template <typename Top, typename Helper, typename _is_complement=typename Helper::is_complement>
   struct index_container_helper {
      typedef Top top_type;
   };

   template <typename Top, typename Helper>
   struct index_container_helper<Top, Helper, std::true_type> {
      typedef indexed_subset_complement_top<Top, Helper> top_type;
   };
}

template <typename Top, typename Helper>
class indexed_subset_complement_top : public Top {
public:
   const typename Helper::container get_container2() const
   {
      typename Helper::operation op;
      return op(sequence(0, get_dim(this->get_container1())), static_cast<const Top*>(this)->get_container2());
   }
protected:
   indexed_subset_complement_top();
   ~indexed_subset_complement_top();
};

template <typename> class RenumberTag {};
template <typename> class HintTag {};

template <typename Top, typename TParams>
class indexed_subset_typebase : public manip_container_top<Top, TParams> {
public:
   typedef typename mtagged_list_extract<TParams, Container1Tag>::type container1_ref;
   typedef typename mtagged_list_extract<TParams, Container2Tag>::type container2_ref;
   static constexpr bool renumber=tagged_list_extract_integral<TParams, RenumberTag>(false);

   typedef typename deref<container1_ref>::minus_ref container1;
   typedef typename deref<container2_ref>::minus_ref container2;
   typedef subset_classifier::index_helper<container1_ref, container2_ref, renumber> index_helper;
   typedef subset_classifier::index_container_helper<typename manip_container_top<Top, TParams>::manip_top_type, index_helper>
      index_container_helper;
   typedef typename index_helper::container index_container;
   typedef typename index_helper::container_ref index_container_ref;
   typedef container1 data_container;
   typedef typename temp_ref<container1_ref>::type data_container_ref;
   typedef typename container_traits<container1>::reference reference;
   typedef typename container_traits<container1>::const_reference const_reference;
   typedef typename container_traits<container1>::value_type value_type;
   typedef typename least_derived_class<typename container_traits<data_container>::category,
                                        typename container_traits<index_container>::category>::type
      container_category;
   typedef typename mselect<std::enable_if< mtagged_list_extract<TParams, OperationTag>::is_specified,
                                            typename mtagged_list_extract<TParams, OperationTag>::type >,
			    std::enable_if< subset_classifier::index2element<container1_ref, renumber>::value,
                                            pair< operations::apply2<BuildUnaryIt<operations::index2element>>,
                                                  operations::apply2<BuildUnaryIt<operations::index2element>> > >,
			    std::enable_if< renumber && index_helper::value == subset_classifier::sparse,
                                            pair< nothing,
                                                  operations::apply2<BuildUnaryIt<operations::index2element>> > >,
			    void>::type
      operation;

   typedef typename concat_if<renumber, indexed, cons<end_sensitive, rewindable> >::type must_enforce_features;
   typedef typename manip_container_top<Top, TParams>::expected_features expected_features;

protected:
   static const bool at_end_required = list_search<expected_features, end_sensitive, absorbing_feature>::value,
		     rewind_required = list_search<expected_features, rewindable, absorbing_feature>::value,
                      index_required = renumber && (list_search<expected_features, indexed, absorbing_feature>::value ||
                                                    check_container_feature<container1, sparse_compatible>::value);

   static const subset_classifier::kind suggested_kind=
      subset_classifier::kind(std::is_same<typename mtagged_list_extract<TParams, HintTag>::type, sparse>::value
                              ? subset_classifier::sparse : index_helper::value);
public:
   static const subset_classifier::kind kind=subset_classifier::kind(
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
   typedef typename list_search_all<expected_features,
				    typename concat_if<renumber, indexed, end_sensitive>::type,
				    std::is_same>::negative
      expected_features1;

   typedef typename std::conditional<rewind_required,
                                     typename list_replace<expected_features1, rewindable, provide_construction<rewindable,false> >::type,
                                     expected_features1>::type
      needed_features1;

   typedef typename std::conditional<index_required,
                                     typename mix_features<expected_features,
                                                           cons<provide_construction<indexed>, end_sensitive> >::type,
                                     typename mix_features<typename list_search_all<expected_features, indexed, absorbing_feature>::negative,
                                                           end_sensitive >::type >::type
      needed_features2;

   static bool const use_index1=index_helper::use_index1;

   int size_impl(std::false_type) const
   {
      return this->manip_top().get_container2().size();
   }
   int size_impl(std::true_type) const
   {
      return this->manip_top().get_container1().empty() ? 0 : this->manip_top().get_container1().size() - this->manip_top().get_container2().base().size();
   }
   bool empty_impl(std::false_type) const
   {
      return this->manip_top().get_container2().empty();
   }
   bool empty_impl(std::true_type) const
   {
      return size_impl(std::true_type())==0;
   }
public:
   int size() const { return size_impl(typename index_helper::is_complement()); }
   int max_size() const { return size(); }
   bool empty() const { return empty_impl(typename index_helper::is_complement()); }

   const typename index_container_helper::top_type& index_top() const
   {
      return static_cast<const typename index_container_helper::top_type&>(this->manip_top());
   }
};

template <typename Top, typename TParams,
          subset_classifier::kind TKind=indexed_subset_typebase<Top, TParams>::kind,
          typename Category=typename indexed_subset_typebase<Top, TParams>::container_category>
class indexed_subset_elem_access
   : public indexed_subset_typebase<Top, TParams> {
   typedef indexed_subset_typebase<Top, TParams> base_t;
public:
   typedef indexed_selector<typename ensure_features<typename base_t::container1, typename base_t::needed_features1>::iterator,
			    typename ensure_features<typename base_t::index_container, typename base_t::needed_features2>::const_iterator,
			    base_t::use_index1, base_t::renumber>
      iterator;
   typedef indexed_selector<typename ensure_features<typename base_t::container1, typename base_t::needed_features1>::const_iterator,
			    typename ensure_features<typename base_t::index_container, typename base_t::needed_features2>::const_iterator,
			    base_t::use_index1, base_t::renumber>
      const_iterator;

   iterator begin()
   {
      typename base_t::data_container_ref c1=this->manip_top().get_container1();
      return iterator(ensure(c1, (typename base_t::needed_features1*)0).begin(),
		      ensure(this->index_top().get_container2(), (typename base_t::needed_features2*)0).begin(),
		      true, 0);
   }
   iterator end()
   {
      typename base_t::data_container_ref c1=this->manip_top().get_container1();
      typename base_t::index_container_ref indices=this->index_top().get_container2();
      return iterator(ensure(c1, (typename base_t::needed_features1*)0).end(),
		      ensure(indices, (typename base_t::needed_features2*)0).end(),
		      !iterator_traits<typename iterator::first_type>::is_bidirectional || indices.empty() ? 0 : indices.back()-int(c1.size()));
   }
   const_iterator begin() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), (typename base_t::needed_features1*)0).begin(),
			    ensure(this->index_top().get_container2(), (typename base_t::needed_features2*)0).begin(),
			    true, 0);
   }
   const_iterator end() const
   {
      const typename base_t::container1& c1=this->manip_top().get_container1();
      typename base_t::index_container_ref indices=this->index_top().get_container2();
      return const_iterator(ensure(c1, (typename base_t::needed_features1*)0).end(),
			    ensure(indices, (typename base_t::needed_features2*)0).end(),
			    !iterator_traits<typename iterator::first_type>::is_bidirectional || indices.empty() ? 0 : indices.back()-int(c1.size()));
   }
};

template <typename Top, typename TParams, subset_classifier::kind TKind>
class indexed_subset_elem_access<Top, TParams, TKind, forward_iterator_tag>
   : public indexed_subset_elem_access<Top, TParams, TKind, input_iterator_tag> {
   typedef indexed_subset_typebase<Top, TParams> base_t;
public:
   typename base_t::reference front() { return *(this->begin()); }
   typename base_t::const_reference front() const { return *(this->begin()); }
};

template <typename Top, typename TParams, subset_classifier::kind TKind>
class indexed_subset_rev_elem_access
   : public indexed_subset_elem_access<Top, TParams, TKind, forward_iterator_tag> {
   typedef indexed_subset_elem_access<Top, TParams, TKind, forward_iterator_tag> base_t;
public:
   typedef indexed_selector<typename ensure_features<typename base_t::container1, typename base_t::needed_features1>::reverse_iterator,
			    typename ensure_features<typename base_t::index_container, typename base_t::needed_features2>::const_reverse_iterator,
			    base_t::use_index1, base_t::renumber, true>
      reverse_iterator;
   typedef indexed_selector<typename ensure_features<typename base_t::container1, typename base_t::needed_features1>::const_reverse_iterator,
			    typename ensure_features<typename base_t::index_container, typename base_t::needed_features2>::const_reverse_iterator,
			    base_t::use_index1, base_t::renumber, true>
      const_reverse_iterator;

   reverse_iterator rbegin()
   {
      typename base_t::data_container_ref c1=this->manip_top().get_container1();
      return reverse_iterator(ensure(c1, (typename base_t::needed_features1*)0).rbegin(),
			      ensure(this->index_top().get_container2(), (typename base_t::needed_features2*)0).rbegin(),
			      true, int(c1.size())-1);
   }
   reverse_iterator rend()
   {
      typename base_t::data_container_ref c1=this->manip_top().get_container1();
      typename base_t::index_container_ref indices=this->index_top().get_container2();
      return reverse_iterator(ensure(c1, (typename base_t::needed_features1*)0).rend(),
			      ensure(indices, (typename base_t::needed_features2*)0).rend(),
			      !iterator_traits<typename reverse_iterator::first_type>::is_bidirectional || indices.empty() ? 0 : indices.front()+1);
   }
   const_reverse_iterator rbegin() const
   {
      const typename base_t::container1& c1=this->manip_top().get_container1();
      return const_reverse_iterator(ensure(c1, (typename base_t::needed_features1*)0).rbegin(),
				    ensure(this->index_top().get_container2(), (typename base_t::needed_features2*)0).rbegin(),
				    true, int(c1.size())-1);
   }
   const_reverse_iterator rend() const
   {
      typename base_t::index_container_ref indices=this->index_top().get_container2();
      return const_reverse_iterator(ensure(this->manip_top().get_container1(), (typename base_t::needed_features1*)0).rend(),
				    ensure(indices, (typename base_t::needed_features2*)0).rend(),
				    !iterator_traits<typename reverse_iterator::first_type>::is_bidirectional || indices.empty() ? 0 : indices.front()+1);
   }
};

template <typename Top, typename TParams, subset_classifier::kind TKind>
class indexed_subset_elem_access<Top, TParams, TKind, bidirectional_iterator_tag>
   : public indexed_subset_rev_elem_access<Top, TParams, TKind> {
   typedef indexed_subset_rev_elem_access<Top, TParams, TKind> base_t;
public:
   typename base_t::reference back() { return *(this->rbegin()); }
   typename base_t::reference back() const { return *(this->rbegin()); }
};

template <typename Top, typename TParams>
class indexed_subset_elem_access<Top, TParams, subset_classifier::plain, input_iterator_tag>
   : public indexed_subset_typebase<Top, TParams> {
   typedef indexed_subset_typebase<Top, TParams> base_t;
protected:
   typedef typename base_t::expected_features needed_features1;
public:
   typedef typename ensure_features<typename base_t::container1, needed_features1>::iterator iterator;
   typedef typename ensure_features<typename base_t::container1, needed_features1>::const_iterator const_iterator;

   iterator begin()
   {
      typename base_t::data_container_ref c1=this->manip_top().get_container1();
      iterator b=ensure(c1, (needed_features1*)0).begin();
      std::advance(b, this->manip_top().get_container2().front());
      return b;
   }
   iterator end()
   {
      typename base_t::data_container_ref c1=this->manip_top().get_container1();
      if (iterator_traits<iterator>::is_bidirectional) {
	 iterator e=ensure(c1, (needed_features1*)0).end();
	 std::advance(e, this->manip_top().get_container2().back()+1-int(c1.size()));
	 return e;
      } else {
	 iterator b=ensure(c1, (needed_features1*)0).begin();
	 std::advance(b, this->manip_top().get_container2().back()+1);
	 return b;
      }
   }
   const_iterator begin() const
   {
      const_iterator b=ensure(this->manip_top().get_container1(), (needed_features1*)0).begin();
      std::advance(b, this->manip_top().get_container2().front());
      return b;
   }
   const_iterator end() const
   {
      const typename base_t::container1& c1=this->manip_top().get_container1();
      if (iterator_traits<const_iterator>::is_bidirectional) {
	 const_iterator e=ensure(c1, (needed_features1*)0).end();
	 std::advance(e, this->manip_top().get_container2().back()+1-int(c1.size()));
	 return e;
      } else {
	 const_iterator b=ensure(c1, (needed_features1*)0).begin();
	 std::advance(b, this->manip_top().get_container2().back()+1);
	 return b;
      }
   }
};

template <typename Top, typename TParams>
class indexed_subset_rev_elem_access<Top, TParams, subset_classifier::plain>
   : public indexed_subset_elem_access<Top, TParams, subset_classifier::plain, forward_iterator_tag> {
   typedef indexed_subset_elem_access<Top, TParams, subset_classifier::plain, forward_iterator_tag> base_t;
public:
   typedef typename ensure_features<typename base_t::container1, typename base_t::needed_features1>::reverse_iterator
      reverse_iterator;
   typedef typename ensure_features<typename base_t::container1, typename base_t::needed_features1>::const_reverse_iterator
      const_reverse_iterator;

   reverse_iterator rbegin()
   {
      typename base_t::data_container_ref c1=this->manip_top().get_container1();
      reverse_iterator rb=ensure(c1, (typename base_t::needed_features1*)0).rbegin();
      std::advance(rb, int(c1.size())-1 - this->manip_top().get_container2().back());
      return rb;
   }
   reverse_iterator rend()
   {
      typename base_t::data_container_ref c1=this->manip_top().get_container1();
      if (iterator_traits<reverse_iterator>::is_bidirectional) {
	 reverse_iterator re=ensure(c1, (typename base_t::needed_features1*)0).rend();
	 std::advance(re, -this->manip_top().get_container2().front());
	 return re;
      } else {
	 reverse_iterator rb=ensure(c1, (typename base_t::needed_features1*)0).rbegin();
	 std::advance(rb, int(c1.size()) - this->manip_top().get_container2().front());
	 return rb;
      }
   }
   const_reverse_iterator rbegin() const
   {
      const typename base_t::container1& c1=this->manip_top().get_container1();
      const_reverse_iterator rb=ensure(c1, (typename base_t::needed_features1*)0).rbegin();
      std::advance(rb, int(c1.size())-1 - this->manip_top().get_container2().back());
      return rb;
   }
   const_reverse_iterator rend() const
   {
      if (iterator_traits<reverse_iterator>::is_bidirectional) {
	 const_reverse_iterator re=ensure(this->manip_top().get_container1(), (typename base_t::needed_features1*)0).rend();
	 std::advance(re, -this->manip_top().get_container2().front());
	 return re;
      } else {
	 const_reverse_iterator rb=ensure(this->manip_top().get_container1(), (typename base_t::needed_features1*)0).rbegin();
	 std::advance(rb, int(this->manip_top().get_container1().size()) - this->manip_top().get_container2().front());
	 return rb;
      }
   }
};

template <typename Top, typename TParams>
class indexed_subset_elem_access<Top, TParams, subset_classifier::contiguous, input_iterator_tag>
   : public indexed_subset_typebase<Top, TParams> {
   typedef indexed_subset_typebase<Top, TParams> base_t;
protected:
   typedef typename concat_if<list_search<typename base_t::expected_features, indexed, absorbing_feature>::value,
			      provide_construction<indexed,false>,
	   typename concat_if<list_search<typename base_t::expected_features, rewindable, absorbing_feature>::value,
			      provide_construction<rewindable,false>,
			      void>::type >::type
      enforce_features1;
   typedef typename mix_features<typename base_t::expected_features, enforce_features1>::type needed_features1;
public:
   typedef typename ensure_features<typename base_t::container1, needed_features1>::iterator iterator;
   typedef typename ensure_features<typename base_t::container1, needed_features1>::const_iterator const_iterator;

   iterator begin()
   {
      typename base_t::data_container_ref c1=this->manip_top().get_container1();
      iterator b=ensure(c1, (needed_features1*)0).begin();
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
      const_iterator b=ensure(this->manip_top().get_container1(), (needed_features1*)0).begin();
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

template <typename Top, typename TParams>
class indexed_subset_rev_elem_access<Top, TParams, subset_classifier::contiguous>
   : public indexed_subset_elem_access<Top, TParams, subset_classifier::contiguous, forward_iterator_tag> {
   typedef indexed_subset_elem_access<Top, TParams, subset_classifier::contiguous, forward_iterator_tag> base_t;
public:
   typedef typename ensure_features<typename base_t::container1, typename base_t::needed_features1>::reverse_iterator reverse_iterator;
   typedef typename ensure_features<typename base_t::container1, typename base_t::needed_features1>::const_reverse_iterator const_reverse_iterator;

   reverse_iterator rbegin()
   {
      typename base_t::data_container_ref c1=this->manip_top().get_container1();
      const typename base_t::container2& indices=this->manip_top().get_container2();
      reverse_iterator rb=ensure(c1, (typename base_t::needed_features1*)0).rbegin();
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
      const typename base_t::container1& c1=this->manip_top().get_container1();
      const typename base_t::container2& indices=this->manip_top().get_container2();
      const_reverse_iterator rb=ensure(c1, (typename base_t::needed_features1*)0).rbegin();
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

template <typename Top, typename TParams>
class indexed_subset_elem_access<Top, TParams, subset_classifier::range, input_iterator_tag>
   : public indexed_subset_typebase<Top, TParams> {
   typedef indexed_subset_typebase<Top, TParams> base_t;
protected:
   typedef typename concat_if<list_search<typename base_t::expected_features, indexed, absorbing_feature>::value,
			      provide_construction<indexed, false>,
	   typename concat_if<list_search<typename base_t::expected_features, rewindable, absorbing_feature>::value,
			      provide_construction<rewindable, false>,
			      provide_construction<end_sensitive, false> >::type >::type
      enforce_features1;
   typedef typename mix_features<typename base_t::expected_features, enforce_features1>::type needed_features1;
public:
   typedef typename ensure_features<typename base_t::container1, needed_features1>::iterator iterator;
   typedef typename ensure_features<typename base_t::container1, needed_features1>::const_iterator const_iterator;

   iterator begin()
   {
      typename base_t::data_container_ref c1=this->manip_top().get_container1();
      const typename base_t::container2& indices=this->manip_top().get_container2();
      iterator b=ensure(c1, (needed_features1*)0).begin();
      b.contract(base_t::renumber, indices.front(), int(c1.size())-1-indices.back());
      return b;
   }
   iterator end()
   {
      return begin()+this->size();
   }
   const_iterator begin() const
   {
      const typename base_t::container1& c1=this->manip_top().get_container1();
      const typename base_t::container2& indices=this->manip_top().get_container2();
      const_iterator b=ensure(c1, (needed_features1*)0).begin();
      b.contract(base_t::renumber, indices.front(), int(c1.size())-1-indices.back());
      return b;
   }
   const_iterator end() const
   {
      return begin()+this->size();
   }
};

template <typename Top, typename TParams>
class indexed_subset_rev_elem_access<Top, TParams, subset_classifier::range>
   : public indexed_subset_elem_access<Top, TParams, subset_classifier::range, forward_iterator_tag> {
   typedef indexed_subset_elem_access<Top, TParams, subset_classifier::range, forward_iterator_tag> base_t;
public:
   typedef typename ensure_features<typename base_t::container1, typename base_t::needed_features1>::reverse_iterator reverse_iterator;
   typedef typename ensure_features<typename base_t::container1, typename base_t::needed_features1>::const_reverse_iterator const_reverse_iterator;

   reverse_iterator rbegin()
   {
      typename base_t::data_container_ref c1=this->manip_top().get_container1();
      const typename base_t::container2& indices=this->manip_top().get_container2();
      reverse_iterator rb=ensure(c1, (typename base_t::needed_features1*)0).rbegin();
      rb.contract(base_t::renumber, int(c1.size())-1 - indices.back(), indices.front());
      return rb;
   }
   reverse_iterator rend()
   {
      return rbegin()+this->size();
   }
   const_reverse_iterator rbegin() const
   {
      const typename base_t::container1& c1=this->manip_top().get_container1();
      const typename base_t::container2& indices=this->manip_top().get_container2();
      const_reverse_iterator rb=ensure(c1, (typename base_t::needed_features1*)0).rbegin();
      rb.contract(base_t::renumber, int(c1.size())-1 - indices.back(), indices.front());
      return rb;
   }
   const_reverse_iterator rend() const
   {
      return rbegin()+this->size();
   }
};

template <typename Top, typename TParams, subset_classifier::kind TKind>
class indexed_subset_elem_access<Top, TParams, TKind, random_access_iterator_tag>
   : public indexed_subset_elem_access<Top, TParams, TKind, bidirectional_iterator_tag> {
   typedef indexed_subset_elem_access<Top, TParams, TKind, bidirectional_iterator_tag> base_t;
public:
   typename base_t::reference front()
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2().front() ];
   }
   typename base_t::const_reference front() const
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2().front() ];
   }
   typename base_t::reference back()
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2().back() ];
   }
   typename base_t::const_reference back() const
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2().back() ];
   }
   typename base_t::reference operator[] (int i)
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2()[i] ];
   }
   typename base_t::const_reference operator[] (int i) const
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2()[i] ];
   }
};

template <typename Top, typename TParams>
class indexed_subset_elem_access<Top, TParams, subset_classifier::sparse, forward_iterator_tag>
   : public indexed_subset_typebase<Top, TParams> {
   typedef indexed_subset_typebase<Top, TParams> base_t;
protected:
   typedef typename mtagged_list_extract<TParams, IteratorCouplerTag, sparse_indexed_selector_coupler<base_t::renumber, false> >::type
      it_coupler;
   typedef typename it_coupler::template defs<typename container_traits<typename base_t::data_container>::iterator,
					      typename container_traits<typename base_t::index_container>::const_iterator,
					      typename base_t::expected_features>::needed_features1
      needed_features1;
   typedef typename it_coupler::template defs<typename container_traits<typename base_t::data_container>::const_iterator,
					      typename container_traits<typename base_t::index_container>::const_iterator,
					      typename base_t::expected_features>::needed_features2
      needed_features2;

   typedef typename it_coupler::template defs<typename ensure_features<typename base_t::data_container, needed_features1>::iterator,
					      typename ensure_features<typename base_t::index_container, needed_features2>::const_iterator,
					      typename base_t::expected_features>::iterator
      iterator_pair;
   typedef typename it_coupler::template defs<typename ensure_features<typename base_t::data_container, needed_features1>::const_iterator,
					      typename ensure_features<typename base_t::index_container, needed_features2>::const_iterator,
					      typename base_t::expected_features>::iterator
      const_iterator_pair;

public:
   typedef typename subset_classifier::iterator_helper<iterator_pair, typename base_t::operation>::iterator
      iterator;
   typedef typename subset_classifier::iterator_helper<const_iterator_pair, typename base_t::operation>::iterator
      const_iterator;

   iterator begin()
   {
      typename base_t::data_container_ref c1=this->manip_top().get_container1();
      return iterator(ensure(c1, (needed_features1*)0).begin(),
		      ensure(this->index_top().get_container2(), (needed_features2*)0).begin());
   }
   iterator end()
   {
      typename base_t::data_container_ref c1=this->manip_top().get_container1();
      return iterator(ensure(c1, (needed_features1*)0).end(),
		      ensure(this->index_top().get_container2(), (needed_features2*)0).end());
   }
   const_iterator begin() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), (needed_features1*)0).begin(),
			    ensure(this->index_top().get_container2(), (needed_features2*)0).begin());
   }
   const_iterator end() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), (needed_features1*)0).end(),
			    ensure(this->index_top().get_container2(), (needed_features2*)0).end());
   }

   int size() const { return count_it(begin()); }
   bool empty() const { return begin().at_end(); }

   typename base_t::reference front() { return *begin(); }
   typename base_t::const_reference front() const { return *begin(); }
};

template <typename Top, typename TParams>
class indexed_subset_elem_access<Top, TParams, subset_classifier::sparse, bidirectional_iterator_tag>
   : public indexed_subset_elem_access<Top, TParams, subset_classifier::sparse, forward_iterator_tag> {
   typedef indexed_subset_elem_access<Top, TParams, subset_classifier::sparse, forward_iterator_tag> base_t;
protected:
   typedef typename reverse_coupler<typename base_t::it_coupler>::type rev_coupler;
public:
   typedef typename rev_coupler::template defs<typename ensure_features<typename base_t::data_container, typename base_t::needed_features1>::reverse_iterator,
					       typename ensure_features<typename base_t::index_container, typename base_t::needed_features2>::const_reverse_iterator,
					       typename base_t::expected_features>::iterator
      reverse_iterator_pair;
   typedef typename rev_coupler::template defs<typename ensure_features<typename base_t::data_container, typename base_t::needed_features1>::const_reverse_iterator,
					       typename ensure_features<typename base_t::index_container, typename base_t::needed_features2>::const_reverse_iterator,
					       typename base_t::expected_features>::iterator
      const_reverse_iterator_pair;
   typedef typename subset_classifier::iterator_helper<reverse_iterator_pair, typename base_t::operation>::iterator
      reverse_iterator;
   typedef typename subset_classifier::iterator_helper<const_reverse_iterator_pair, typename base_t::operation>::iterator
      const_reverse_iterator;

   reverse_iterator rbegin()
   {
      typename base_t::data_container_ref c1=this->manip_top().get_container1();
      return reverse_iterator(ensure(c1, (typename base_t::needed_features1*)0).rbegin(),
			      ensure(this->index_top().get_container2(), (typename base_t::needed_features2*)0).rbegin());
   }
   reverse_iterator rend()
   {
      typename base_t::data_container_ref c1=this->manip_top().get_container1();
      return reverse_iterator(ensure(c1, (typename base_t::needed_features1*)0).rend(),
			      ensure(this->index_top().get_container2(), (typename base_t::needed_features2*)0).rend());
   }
   const_reverse_iterator rbegin() const
   {
      return const_reverse_iterator(ensure(this->manip_top().get_container1(), (typename base_t::needed_features1*)0).rbegin(),
				    ensure(this->index_top().get_container2(), (typename base_t::needed_features2*)0).rbegin());
   }
   const_reverse_iterator rend() const
   {
      return const_reverse_iterator(ensure(this->manip_top().get_container1(), (typename base_t::needed_features1*)0).rend(),
				    ensure(this->index_top().get_container2(), (typename base_t::needed_features2*)0).rend());
   }

   typename base_t::reference back() { return *rbegin(); }
   typename base_t::const_reference back() const { return *rbegin(); }
};

template <typename Top, typename TParams=typename Top::manipulator_params>
class indexed_subset_impl
   : public indexed_subset_elem_access<Top, TParams> {
public:
   typedef indexed_subset_impl<Top, TParams> manipulator_impl;
   typedef TParams manipulator_params;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef indexed_subset_impl<FeatureCollector, TParams> type;
   };
};

template <typename ContainerRef1, typename ContainerRef2, typename TParams=mlist<>> class IndexedSubset;

template <typename ContainerRef1, typename ContainerRef2, typename TParams,
          typename Generic1=typename object_traits<typename deref<ContainerRef1>::type>::generic_type,
          typename Generic2=typename object_traits<typename deref<ContainerRef2>::type>::generic_type>
class generic_of_indexed_subset {};

template <typename ContainerRef1, typename ContainerRef2, typename TParams>
class IndexedSubset
   : public container_pair_base<ContainerRef1, ContainerRef2>,
     public indexed_subset_impl< IndexedSubset<ContainerRef1, ContainerRef2, TParams>,
				 typename mlist_concat< Container1Tag<ContainerRef1>, Container2Tag<ContainerRef2>, TParams >::type >,
     public generic_of_indexed_subset<ContainerRef1, ContainerRef2, TParams> {
   typedef container_pair_base<ContainerRef1, ContainerRef2> base_t;
public:
   IndexedSubset(typename base_t::first_arg_type src1_arg, typename base_t::second_arg_type src2_arg)
      : base_t(src1_arg, src2_arg) {}

   int dim() const { return get_dim(this->get_container1()); }
};

template <typename ContainerRef1, typename ContainerRef2, typename TParams,
	  typename Set1, typename E, typename Comparator, typename Set2>
class generic_of_indexed_subset<ContainerRef1, ContainerRef2, TParams,
                                GenericSet<Set1, E, Comparator>, GenericSet<Set2, int, operations::cmp> >
   : public GenericSet<IndexedSubset<ContainerRef1, ContainerRef2, TParams>, E, Comparator> {
public:
   const Comparator& get_comparator() const
   {
      return this->manip_top().get_container1().get_comparator();
   }
};

template <typename ContainerRef1, typename ContainerRef2, typename TParams>
struct spec_object_traits< IndexedSubset<ContainerRef1, ContainerRef2, TParams> >
   : spec_object_traits<is_container> {
   static const bool is_temporary    = true,
                     is_lazy         = object_traits<typename deref<ContainerRef1>::type>::is_lazy,
                     is_always_const = effectively_const<ContainerRef1>::value;
};

template <typename ContainerRef1, typename ContainerRef2, typename TParams>
struct check_container_feature<IndexedSubset<ContainerRef1, ContainerRef2, TParams>, sparse_compatible>
   : check_container_feature<typename deref<ContainerRef1>::type, sparse_compatible> {};

template <typename ContainerRef1, typename ContainerRef2, typename TParams>
struct check_container_feature<IndexedSubset<ContainerRef1, ContainerRef2, TParams>, sparse>
   : check_container_feature<typename deref<ContainerRef1>::type, sparse> {};

template <typename ContainerRef1, typename ContainerRef2, typename TParams>
struct check_container_feature<IndexedSubset<ContainerRef1, ContainerRef2, TParams>, pure_sparse>
   : check_container_feature<typename deref<ContainerRef1>::type, pure_sparse> {};

template <typename IndexSet> inline
typename std::enable_if<!check_container_feature<typename Concrete<IndexSet>::type, sparse_compatible>::value, bool>::type
set_within_range(const IndexSet& s, int d)
{
   const unwary_t<typename Concrete<IndexSet>::type>& ss=unwary(concrete(s));
   return ss.empty() || (ss.front()>=0 && ss.back()<d);
}

template <typename IndexSet> inline
typename std::enable_if<check_container_feature<typename Concrete<IndexSet>::type, sparse_compatible>::value, bool>::type
set_within_range(const IndexSet& s, int d)
{
   return unwary(concrete(s)).dim()<=d;
}

template <typename IndexSet> inline
bool set_within_range(const Complement<IndexSet>& s, int d)
{
   // as a special case we allow a complement-based slice or minor of an empty vector resp. matrix
   return d==0 || set_within_range(s.base(), d);
}

template <typename Container, typename IndexSet> inline
IndexedSubset<unwary_t<Container>&, const unwary_t<typename Concrete<IndexSet>::type>&>
select(Container& c, const IndexSet& indices)
{
   if (POLYMAKE_DEBUG || !Unwary<Container>::value || !Unwary<typename Concrete<IndexSet>::type>::value) {
      if (!set_within_range(indices, get_dim(unwary(c))))
	 throw std::runtime_error("select - indices out of range");
   }
   return IndexedSubset<unwary_t<Container>&, const unwary_t<typename Concrete<IndexSet>::type>&>
                       (unwary(c), unwary(concrete(indices)));
}

template <typename Container, typename IndexSet> inline
IndexedSubset<const unwary_t<Container>&, const unwary_t<typename Concrete<IndexSet>::type>&>
select(const Container& c, const IndexSet& indices)
{
   if (POLYMAKE_DEBUG || !Unwary<Container>::value || !Unwary<typename Concrete<IndexSet>::type>::value) {
      if (!set_within_range(indices, get_dim(unwary(c))))
	 throw std::runtime_error("select - indices out of range");
   }
   return IndexedSubset<const unwary_t<Container>&, const unwary_t<typename Concrete<IndexSet>::type>&>
                       (unwary(c), unwary(concrete(indices)));
}

template <typename ContainerRef1, typename ContainerRef2, typename TParams=mlist<>>
class IndexedSlice;

template <typename ContainerRef1, typename ContainerRef2, typename TParams,
          typename Generic1=typename object_traits<typename deref<ContainerRef1>::type>::generic_type,
          typename Generic2=typename object_traits<typename deref<ContainerRef2>::type>::generic_type>
class generic_of_indexed_slice
   : public inherit_generic< IndexedSlice<ContainerRef1, ContainerRef2, TParams>, typename deref<ContainerRef1>::type>::type {};

template <typename ContainerRef1, typename ContainerRef2, typename TParams>
struct IndexedSlice_impl {
   typedef indexed_subset_impl< IndexedSlice<ContainerRef1, ContainerRef2, TParams>,
				typename mlist_concat< Container1Tag<ContainerRef1>, Container2Tag<ContainerRef2>,
                                                       RenumberTag<std::true_type>, TParams >::type >
      type;
};

template <typename ContainerRef1, typename ContainerRef2, typename TParams,
	  bool TImmutable=effectively_const<ContainerRef1>::value,
	  bool TSparse=check_container_ref_feature<ContainerRef1, sparse>::value,
	  typename tag=typename object_traits<typename deref<ContainerRef1>::type>::generic_tag,
	  bool TBidirectional=is_derived_from<typename IndexedSlice_impl<ContainerRef1, ContainerRef2, TParams>::type::container_category,
                                              bidirectional_iterator_tag>::value>
class IndexedSlice_mod {};

template <typename ContainerRef1, typename ContainerRef2, typename TParams>
class IndexedSlice
   : public container_pair_base<ContainerRef1, ContainerRef2>,
     public IndexedSlice_impl<ContainerRef1, ContainerRef2, TParams>::type,
     public IndexedSlice_mod<ContainerRef1, ContainerRef2, TParams>,
     public generic_of_indexed_slice<ContainerRef1, ContainerRef2, TParams> {
   typedef container_pair_base<ContainerRef1, ContainerRef2> base_t;
public:
   typedef typename inherit_generic<IndexedSlice, typename deref<ContainerRef1>::type>::type
      generic_mutable_type;

   IndexedSlice(typename base_t::first_arg_type src1_arg, typename base_t::second_arg_type src2_arg)
      : base_t(src1_arg,src2_arg) {}

   IndexedSlice& operator= (const IndexedSlice& other) { return generic_mutable_type::operator=(other); }
   using generic_mutable_type::operator=;

protected:
   int dim_impl(std::false_type) const { return this->get_container2().size(); }
   int dim_impl(std::true_type) const { return get_dim(this->get_container1()) - this->get_container2().base().size(); }
public:
   int dim() const
   {
      return dim_impl(is_derived_from_instance_of<typename deref<ContainerRef2>::type, Complement>());
   }

   template <typename,typename,typename,bool,bool,typename,bool> friend class IndexedSlice_mod;
};

template <typename ContainerRef1, typename ContainerRef2, typename TParams,
	  typename Set1, typename E, typename Comparator, typename Set2>
class generic_of_indexed_slice<ContainerRef1, ContainerRef2, TParams,
			       GenericSet<Set1, E, Comparator>, GenericSet<Set2, int, operations::cmp> >
   : public inherit_generic< IndexedSlice<ContainerRef1, ContainerRef2, TParams>, typename deref<ContainerRef1>::type>::type {
public:
   const Comparator& get_comparator() const
   {
      return this->top().get_container1().get_comparator();
   }
};

// set, forward category
template <typename ContainerRef1, typename ContainerRef2, typename TParams>
class IndexedSlice_mod<ContainerRef1, ContainerRef2, TParams, false, false, is_set, false> {
   typedef IndexedSlice<ContainerRef1, ContainerRef2, TParams> master;
protected:
   typedef typename IndexedSlice_impl<ContainerRef1, ContainerRef2, TParams>::type impl_t;
private:
   typename impl_t::iterator::second_type rewind_index_impl(const typename impl_t::iterator::second_type&, int i, forward_iterator_tag)
   {
      master& me=static_cast<master&>(*this);
      typename impl_t::iterator::second_type iit=ensure(me.index_top().get_container2(), (typename master::needed_features2*)0).begin();
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
      typename impl_t::iterator::second_type iit=rewind_index(ensure(me.index_top().get_container2(), (typename master::needed_features2*)0).end(), i);
      return typename impl_t::iterator(me.get_container1().insert(*iit), iit);
   }

   void erase(const typename impl_t::iterator& pos)
   {
      master& me=static_cast<master&>(*this);
      me.get_container1().erase(pos);
   }
};

// set, bidirectional category
template <typename ContainerRef1, typename ContainerRef2, typename TParams>
class IndexedSlice_mod<ContainerRef1, ContainerRef2, TParams, false, false, is_set, true>
   : public IndexedSlice_mod<ContainerRef1, ContainerRef2, TParams, false, false, is_set, false> {
   typedef IndexedSlice_mod<ContainerRef1, ContainerRef2, TParams, false, false, is_set, false> base_t;
   typedef IndexedSlice<ContainerRef1, ContainerRef2, TParams> master;
protected:
   typedef typename base_t::impl_t impl_t;
private:
   typename impl_t::reverse_iterator::second_type rewind_index_impl(const typename impl_t::reverse_iterator::second_type&, int i, forward_iterator_tag)
   {
      master& me=static_cast<master&>(*this);
      typename impl_t::reverse_iterator::second_type iit=ensure(me.index_top().get_container2(), (typename master::needed_features2*)0).rbegin();
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
template <typename ContainerRef1, typename ContainerRef2, typename TParams, typename TTag>
class IndexedSlice_mod<ContainerRef1, ContainerRef2, TParams, false, true, TTag, false>
   : public IndexedSlice_mod<ContainerRef1, ContainerRef2, TParams, false, false, is_set> {
   typedef IndexedSlice_mod<ContainerRef1, ContainerRef2, TParams, false, false, is_set> base_t;
   typedef IndexedSlice<ContainerRef1, ContainerRef2, TParams> master;
protected:
   typedef typename base_t::impl_t impl_t;
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
      typename impl_t::iterator::second_type iit=this->rewind_index(ensure(me.index_top().get_container2(), (typename master::needed_features2*)0).end(), i);
      return typename impl_t::iterator(me.get_container1().insert(*iit, d), iit);
   }

   using base_t::insert;
};

// sparse vector, bidirectional category
template <typename ContainerRef1, typename ContainerRef2, typename TParams, typename TTag>
class IndexedSlice_mod<ContainerRef1, ContainerRef2, TParams, false, true, TTag, true>
   : public IndexedSlice_mod<ContainerRef1, ContainerRef2, TParams, false, true, TTag, false> {
   typedef IndexedSlice_mod<ContainerRef1, ContainerRef2, TParams, false, true, TTag, false> base_t;
   typedef IndexedSlice<ContainerRef1, ContainerRef2, TParams> master;
protected:
   typedef typename base_t::impl_t impl_t;
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

template <typename ContainerRef1, typename ContainerRef2, typename TParams>
struct spec_object_traits< IndexedSlice<ContainerRef1, ContainerRef2, TParams> >
   : spec_object_traits<is_container> {
   static const bool is_temporary    = true,
                     is_lazy         = object_traits<typename deref<ContainerRef1>::type>::is_lazy,
                     is_always_const = effectively_const<ContainerRef1>::value;
};

template <typename ContainerRef1, typename ContainerRef2, typename TParams>
struct check_container_feature<IndexedSlice<ContainerRef1, ContainerRef2, TParams>, sparse_compatible>
   : check_container_ref_feature<ContainerRef1, sparse_compatible> {};

template <typename ContainerRef1, typename ContainerRef2, typename TParams>
struct check_container_feature<IndexedSlice<ContainerRef1, ContainerRef2, TParams>, sparse>
   : check_container_ref_feature<ContainerRef1, sparse> {};

template <typename ContainerRef1, typename ContainerRef2, typename TParams>
struct check_container_feature<IndexedSlice<ContainerRef1, ContainerRef2, TParams>, pure_sparse>
   : check_container_ref_feature<ContainerRef1, pure_sparse> {};

} // end namespace pm

namespace polymake {
using pm::select;
using pm::IndexedSubset;
using pm::IndexedSlice;
}

namespace std {
// due to silly overloading rules
template <typename ContainerRef1, typename ContainerRef2, typename TParams> inline
void swap(pm::IndexedSlice<ContainerRef1, ContainerRef2, TParams>& s1,
          pm::IndexedSlice<ContainerRef1, ContainerRef2, TParams>& s2) { s1.swap(s2); }
}

#endif // POLYMAKE_INDEXED_SUBSET_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
