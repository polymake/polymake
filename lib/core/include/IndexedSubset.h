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

#ifndef POLYMAKE_INDEXED_SUBSET_H
#define POLYMAKE_INDEXED_SUBSET_H

#include "polymake/GenericSet.h"
#include "polymake/internal/Wary.h"

namespace pm {

template <typename Iterator1, typename Iterator2, bool renumber, bool _reversed=false>
class indexed_selector
   : public iterator_traits<Iterator1>::derivable_type {
public:
   typedef Iterator1 first_type;
   typedef Iterator2 second_type;

   typedef typename iterator_traits<Iterator1>::derivable_type super;
   Iterator2 second;
protected:
   // 0 - general case
   // 1 - sequence::iterator
   // 2 - series::iterator
   static const int step_kind= (derived_from<Iterator2, sequence::iterator>::value +
				derived_from<Iterator2, series::iterator>::value) *
                                identical<typename accompanying_iterator<Iterator2>::type, sequence::iterator>::value;
   typedef int2type<step_kind> step_discr;
private:
   void _forw(int2type<0>)
   {
      int pos=*second;
      ++second;
      if (!at_end())
	 std::advance(static_cast<super&>(*this), _reversed ? pos-*second : *second-pos);
   }
   void _forw(int2type<1>)
   {
      ++second;
      super::operator++();
   }
   void _forw(int2type<2>)
   {
      ++second;
      if (!at_end())
	 std::advance(static_cast<super&>(*this), second.step());
   }
   void _back(int2type<0>)
   {
      if (second.at_end()) {
	 --second;
      } else {
	 int pos=*second;
	 --second;
	 std::advance(static_cast<super&>(*this), _reversed ? pos-*second : *second-pos);
      }
   }
   void _back(int2type<1>)
   {
      --second;
      super::operator--();
   }
   void _back(int2type<2>)
   {
      if (second.at_end()) {
	 --second;
      } else {
	 --second;
	 std::advance(static_cast<super&>(*this), -second.step());
      }
   }
   int _step(int i, int2type<0>)
   {
      int pos=second.at_end() ? second[-1] : *second;
      second+=i;
      return _reversed ? pos-(second.at_end() ? second[-1] : *second) : (second.at_end() ? second[-1] : *second)-pos;
   }
   int _step(int i, int2type<1>)
   {
      second+=i;
      return i;
   }
   int _step(int i, int2type<2>)
   {
      second+=i;
      return second.step()*i;
   }

   void _contract1(int i, bool, False)
   {
      std::advance(static_cast<super&>(*this), i);
   }
   void _contract1(int i, bool _renumber, True)
   {
      super::contract(_renumber, i);
   }
   void _contract1(int i, bool _renumber)
   {
      _contract1(_reversed ? -i : i, _renumber, bool2type<check_iterator_feature<Iterator1,contractable>::value>());
   }
public:
   typedef typename least_derived< cons<typename iterator_traits<Iterator1>::iterator_category,
					typename iterator_traits<Iterator2>::iterator_category> >::type
      iterator_category;
   typedef typename iterator_traits<Iterator2>::difference_type difference_type;
   typedef indexed_selector<typename iterator_traits<Iterator1>::iterator, Iterator2, renumber, _reversed> iterator;
   typedef indexed_selector<typename iterator_traits<Iterator1>::const_iterator, Iterator2, renumber, _reversed> const_iterator;

   indexed_selector() {}
   indexed_selector(const iterator& it)
      : super(static_cast<const typename iterator::super&>(it)), second(it.second) {}

   indexed_selector(const Iterator1& first_arg, const Iterator2& second_arg, bool adjust=false, int offset=0)
      : super(first_arg), second(second_arg)
   {
      if (adjust && !at_end()) _contract1(offset+*second, !renumber);
   }
   indexed_selector(const Iterator1& first_arg, const Iterator2& second_arg, int offset)
      : super(first_arg), second(second_arg)
   {
      if (offset) _contract1(offset, !renumber);
   }
   indexed_selector(const Iterator1& first_arg,
		    typename alt_constructor<Iterator2>::arg_type& second_arg, bool adjust=false, int offset=0)
      : super(first_arg), second(second_arg)
   {
      if (adjust && !at_end()) _contract1(offset+*second, !renumber);
   }
   indexed_selector(const Iterator1& first_arg,
		    typename alt_constructor<Iterator2>::arg_type& second_arg, int offset)
      : super(first_arg), second(second_arg)
   {
      if (offset) _contract1(offset, !renumber);
   }
   indexed_selector(typename alt_constructor<Iterator1>::arg_type& first_arg,
		    const Iterator2& second_arg, bool adjust=false, int offset=0)
      : super(first_arg), second(second_arg)
   {
      if (adjust && !at_end()) _contract1(offset+*second, !renumber);
   }
   indexed_selector(typename alt_constructor<Iterator1>::arg_type& first_arg,
		    const Iterator2& second_arg, int offset)
      : super(first_arg), second(second_arg)
   {
      if (offset) _contract1(offset, !renumber);
   }
   indexed_selector(typename alt_constructor<Iterator1>::arg_type& first_arg,
		    typename alt_constructor<Iterator2>::arg_type& second_arg, bool adjust=false, int offset=0)
      : super(first_arg), second(second_arg)
   {
      if (adjust && !at_end()) _contract1(offset+*second, !renumber);
   }
   indexed_selector(typename alt_constructor<Iterator1>::arg_type& first_arg,
		    typename alt_constructor<Iterator2>::arg_type& second_arg, int offset)
      : super(first_arg), second(second_arg)
   {
      if (offset) _contract1(offset, !renumber);
   }

   indexed_selector& operator++ ()
   {
      _forw(step_discr());
      return *this;
   }
   const indexed_selector operator++ (int) { indexed_selector copy=*this; operator++(); return copy; }

   indexed_selector& operator--()
   {
      typedef typename enable_if<void, iterator_pair_traits<Iterator1,Iterator2>::is_bidirectional>::type error_if_unimplemented __attribute__((unused));
      _back(step_discr());
      return *this;
   }
   const indexed_selector operator-- (int) { indexed_selector copy=*this;  operator--();  return copy; }

   indexed_selector& operator+= (int i)
   {
      typedef typename enable_if<void, iterator_pair_traits<Iterator1,Iterator2>::is_random>::type error_if_unimplemented __attribute__((unused));
      static_cast<super&>(*this) += _step(i, step_discr());
      return *this;
   }
   indexed_selector& operator-= (int i)
   {
      typedef typename enable_if<void, iterator_pair_traits<Iterator1,Iterator2>::is_random>::type error_if_unimplemented __attribute__((unused));
      static_cast<super&>(*this) += _step(-i, step_discr());
      return *this;
   }

   const indexed_selector operator+ (int i) const { indexed_selector copy=*this; return copy+=i; }
   const indexed_selector operator- (int i) const { indexed_selector copy=*this; return copy-=i; }
   friend const indexed_selector operator+ (int i, const indexed_selector& it) { return it+i; }

   template <typename Iterator>
   typename enable_if<difference_type, comparable_iterator<indexed_selector, Iterator>::value>::type
   operator- (const Iterator& it) const
   {
      typedef typename enable_if<void, iterator_pair_traits<Iterator1,Iterator2>::is_random>::type error_if_unimplemented __attribute__((unused));
      return second - it.second;
   }

   typename super::reference operator[] (int i) const
   {
      typedef typename enable_if<void, iterator_pair_traits<Iterator1,Iterator2>::is_random>::type error_if_unimplemented __attribute__((unused));
      return static_cast<const super&>(*this)[ second[i] - (second.at_end() ? second[-1] : *second) ];
   }

   template <typename Iterator>
   typename enable_if<bool, comparable_iterator<indexed_selector, Iterator>::value>::type
   operator== (const Iterator& it) const
   {
      return second==it.second;
   }
   template <typename Iterator>
   typename enable_if<bool, comparable_iterator<indexed_selector, Iterator>::value>::type
   operator!= (const Iterator& it) const
   {
      return !operator==(it);
   }

   bool at_end() const
   {
      return second.at_end();
   }
private:
   int _index(False) const { return *second; }
   int _index(True) const { return second.index(); }
public:
   int index() const
   {
      return _index(bool2type<renumber>());
   }

   void rewind()
   {
      typedef typename enable_if<super, check_iterator_feature<super, rewindable>::value &&
	                                check_iterator_feature<Iterator2, rewindable>::value>::type
	 error_if_unimplemented __attribute__((unused));
      super::rewind();
      second.rewind();
   }

   void contract(bool _renumber, int distance_front, int distance_back=0)
   {
      typedef typename enable_if<void, check_iterator_feature<Iterator2, contractable>::value>::type
	 error_if_unimplemented __attribute__((unused));
      int pos=*second;
      second.contract(renumber && _renumber, distance_front, distance_back);
      _contract1(*second-pos, !renumber && _renumber);
   }
};

template <typename Iterator1, typename Iterator2, bool renumber, bool _reversed, typename Feature>
struct check_iterator_feature<indexed_selector<Iterator1,Iterator2,renumber,_reversed>, Feature> {
   typedef cons<end_sensitive, contractable> via_second;
   static const bool value= (list_contains<via_second, Feature>::value ||
			     check_iterator_feature<Iterator1,Feature>::value) &&
			    check_iterator_feature<Iterator2,Feature>::value;
};

template <typename Iterator1, typename Iterator2, bool renumber, bool _reversed>
struct check_iterator_feature<indexed_selector<Iterator1,Iterator2,renumber,_reversed>, indexed> {
   static const bool value= !renumber || check_iterator_feature<Iterator2,indexed>::value;
};

template <bool _reverse, bool renumber>
struct sparse_indexed_selector_coupler {
   typedef typename if_else<_reverse, reverse_zipper<set_intersection_zipper>, set_intersection_zipper>::type
      Controller;
   template <typename Iterator1, typename Iterator2, typename ExpectedFeatures>
   struct defs {
      typedef iterator_zipper<Iterator1, Iterator2, operations::cmp, Controller, true, false> iterator;
      typedef ExpectedFeatures needed_features1;  // is already sparse
      typedef typename mix_features<needed_features1,
				    typename concat_if<renumber, provide_construction<indexed>, end_sensitive>::type
                                   >::type
         needed_features2;
   };
};

template <bool _reverse, bool renumber>
struct reverse_coupler< sparse_indexed_selector_coupler<_reverse,renumber> > {
   typedef sparse_indexed_selector_coupler<!_reverse,renumber> type;
};

template <typename Top, typename Params> class indexed_subset_typebase;

template <typename Top, typename Helper> class indexed_subset_complement_top;

namespace subset_classifier {
   enum kind { generic, sparse, plain, contiguous, range };

   template <typename ContainerRef,
	     bool _try=derived_from_instance2<typename deref<ContainerRef>::type, modified_container_typebase>::value>
   struct detect_set_of_indices : False {};

   template <typename ContainerRef>
   struct detect_set_of_indices<ContainerRef, true>
      : identical<typename deref<ContainerRef>::type::operation, BuildUnaryIt<operations::index2element> > {};

   template <typename ContainerRef,
	     bool _try=derived_from_instance2<typename deref<ContainerRef>::type, indexed_subset_typebase>::value>
   struct detect_indexed_slice : False {};

   template <typename ContainerRef>
   struct detect_indexed_slice<ContainerRef, true>
      : list_contains< list( operations::apply2< BuildUnaryIt<operations::index2element> >,
			     pair< nothing, operations::apply2< BuildUnaryIt<operations::index2element> > >,
			     pair< operations::apply2< BuildUnaryIt<operations::index2element> >,
			           operations::apply2< BuildUnaryIt<operations::index2element> > >),
		       typename deref<ContainerRef>::type::operation > {};

   template <typename ContainerRef,
	     bool _renumber=true,
	     typename _tag=typename object_traits<typename deref<ContainerRef>::type>::generic_tag>
   struct index2element : False {};

   template <typename ContainerRef>
   struct index2element<ContainerRef, true, is_set> {
      static const bool value = detect_set_of_indices<ContainerRef>::value || detect_indexed_slice<ContainerRef>::value;
   };

   template <typename ContainerRef1, typename ContainerRef2,
	     bool _renumber,
             bool _complement=derived_from_instance3<typename deref<ContainerRef2>::type,Complement>::value>
   struct index_helper {
      typedef True is_complement;
      static const kind
         value = check_container_feature<typename deref<ContainerRef1>::type, pm::sparse>::value ||
                 index2element<ContainerRef1,_renumber>::value
                 ? sparse : generic;
      static const bool random=false;

      typedef operations::mul<sequence,ContainerRef2> operation;
      typedef typename deref<typename operation::result_type>::type container;
      typedef const container container_ref;
   };

   template <typename ContainerRef1, typename ContainerRef2, bool _renumber>
   struct index_helper<ContainerRef1, ContainerRef2, _renumber, false> {
      typedef False is_complement;
      static const bool random=iterator_traits<typename container_traits<ContainerRef1>::iterator>::is_random;
      static const kind
         value = check_container_feature<typename deref<ContainerRef1>::type, pm::sparse>::value ||
                 index2element<ContainerRef1,_renumber>::value
                 ? sparse :
      		 identical<typename container_traits<ContainerRef2>::iterator, sequence::iterator>::value
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
   struct index_container_helper<Top, Helper, True> {
      typedef indexed_subset_complement_top<Top,Helper> top_type;
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

template <typename> class Renumber {};
template <typename> class Hint {};

template <typename Top, typename Params>
class indexed_subset_typebase : public manip_container_top<Top, Params> {
public:
   typedef typename extract_type_param<Params,Container1>::type container1_ref;
   typedef typename extract_type_param<Params,Container2>::type container2_ref;
   static const bool renumber=extract_bool_param<Params,Renumber>::value;

   typedef typename deref<container1_ref>::minus_ref container1;
   typedef typename deref<container2_ref>::minus_ref container2;
   typedef subset_classifier::index_helper<container1_ref, container2_ref, renumber> index_helper;
   typedef subset_classifier::index_container_helper<typename manip_container_top<Top, Params>::manip_top_type, index_helper>
      index_container_helper;
   typedef typename index_helper::container index_container;
   typedef typename index_helper::container_ref index_container_ref;
   typedef container1 data_container;
   typedef typename temp_ref<container1_ref>::type data_container_ref;
   typedef typename container_traits<container1>::reference reference;
   typedef typename container_traits<container1>::const_reference const_reference;
   typedef typename container_traits<container1>::value_type value_type;
   typedef typename least_derived< cons<typename container_traits<data_container>::category,
					typename container_traits<index_container>::category> >::type
      container_category;
   typedef typename choice< if_else< extract_type_param<Params,Operation>::specified,
				     typename extract_type_param<Params,Operation>::type,
			    if_else< subset_classifier::index2element<container1_ref, renumber>::value,
				     pair< operations::apply2< BuildUnaryIt<operations::index2element> >,
					   operations::apply2< BuildUnaryIt<operations::index2element> > >,
			    if_else< renumber && index_helper::value == subset_classifier::sparse,
				     pair<nothing, operations::apply2< BuildUnaryIt<operations::index2element> > >,
			    void > > > >::type
      operation;

   typedef typename concat_if<renumber, indexed, cons<end_sensitive, rewindable> >::type must_enforce_features;
   typedef typename manip_container_top<Top, Params>::expected_features expected_features;

protected:
   static const bool at_end_required = list_search<expected_features, end_sensitive, absorbing_feature>::value,
		     rewind_required = list_search<expected_features, rewindable, absorbing_feature>::value,
                      index_required = renumber && (list_search<expected_features, indexed, absorbing_feature>::value ||
                                                    check_container_feature<container1, sparse_compatible>::value);

   static const subset_classifier::kind suggested_kind=
      subset_classifier::kind(identical<typename extract_type_param<Params,Hint>::type,sparse>::value ? subset_classifier::sparse : index_helper::value);
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
				    identical>::negative
      expected_features1;

   typedef typename if_else<rewind_required,
			    typename list_replace<expected_features1, rewindable, provide_construction<rewindable,false> >::type,
			    expected_features1>::type
      needed_features1;

   typedef typename if_else<index_required,
			    typename mix_features<expected_features,
						  cons<provide_construction<indexed>, end_sensitive> >::type,
			    typename mix_features<typename list_search_all<expected_features, indexed, absorbing_feature>::negative,
						  end_sensitive >::type >::type
      needed_features2;

   int _size(False) const
   {
      return this->manip_top().get_container2().size();
   }
   int _size(True) const
   {
      return this->manip_top().get_container1().empty() ? 0 : this->manip_top().get_container1().size() - this->manip_top().get_container2().base().size();
   }
   bool _empty(False) const
   {
      return this->manip_top().get_container2().empty();
   }
   bool _empty(True) const
   {
      return _size(True())==0;
   }
public:
   int size() const { return _size(typename index_helper::is_complement()); }
   int max_size() const { return size(); }
   bool empty() const { return _empty(typename index_helper::is_complement()); }

   const typename index_container_helper::top_type& index_top() const
   {
      return static_cast<const typename index_container_helper::top_type&>(this->manip_top());
   }
};

template <typename Top, typename Params,
          subset_classifier::kind _kind=indexed_subset_typebase<Top,Params>::kind,
          typename Category=typename indexed_subset_typebase<Top,Params>::container_category>
class indexed_subset_elem_access
   : public indexed_subset_typebase<Top,Params> {
   typedef indexed_subset_typebase<Top,Params> _super;
public:
   typedef indexed_selector<typename ensure_features<typename _super::container1, typename _super::needed_features1>::iterator,
			    typename ensure_features<typename _super::index_container, typename _super::needed_features2>::const_iterator,
			    _super::renumber>
      iterator;
   typedef indexed_selector<typename ensure_features<typename _super::container1, typename _super::needed_features1>::const_iterator,
			    typename ensure_features<typename _super::index_container, typename _super::needed_features2>::const_iterator,
			    _super::renumber>
      const_iterator;

   iterator begin()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
      return iterator(ensure(c1, (typename _super::needed_features1*)0).begin(),
		      ensure(this->index_top().get_container2(), (typename _super::needed_features2*)0).begin(),
		      true);
   }
   iterator end()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
      typename _super::index_container_ref indices=this->index_top().get_container2();
      return iterator(ensure(c1, (typename _super::needed_features1*)0).end(),
		      ensure(indices, (typename _super::needed_features2*)0).end(),
		      !iterator_traits<typename iterator::super>::is_bidirectional || indices.empty() ? 0 : indices.back()-int(c1.size()));
   }
   const_iterator begin() const
   {
      return const_iterator(ensure(this->manip_top().get_container1(), (typename _super::needed_features1*)0).begin(),
			    ensure(this->index_top().get_container2(), (typename _super::needed_features2*)0).begin(),
			    true);
   }
   const_iterator end() const
   {
      const typename _super::container1& c1=this->manip_top().get_container1();
      typename _super::index_container_ref indices=this->index_top().get_container2();
      return const_iterator(ensure(c1, (typename _super::needed_features1*)0).end(),
			    ensure(indices, (typename _super::needed_features2*)0).end(),
			    !iterator_traits<typename iterator::super>::is_bidirectional || indices.empty() ? 0 : indices.back()-int(c1.size()));
   }
};

template <typename Top, typename Params, subset_classifier::kind _kind>
class indexed_subset_elem_access<Top, Params, _kind, forward_iterator_tag>
   : public indexed_subset_elem_access<Top, Params, _kind, input_iterator_tag> {
   typedef indexed_subset_typebase<Top,Params> _super;
public:
   typename _super::reference front()
   {
      typename _super::container1::iterator b1=this->manip_top().get_container1().begin();
      std::advance(b1, this->index_top().get_container2().front());
      return *b1;
   }
   typename _super::const_reference front() const
   {
      typename _super::container1::const_iterator b1=this->manip_top().get_container1().begin();
      std::advance(b1, this->index_top().get_container2().front());
      return *b1;
   }
};

template <typename Top, typename Params, subset_classifier::kind _kind>
class indexed_subset_rev_elem_access
   : public indexed_subset_elem_access<Top, Params, _kind, forward_iterator_tag> {
   typedef indexed_subset_elem_access<Top, Params, _kind, forward_iterator_tag> _super;
public:
   typedef indexed_selector<typename ensure_features<typename _super::container1, typename _super::needed_features1>::reverse_iterator,
			    typename ensure_features<typename _super::index_container, typename _super::needed_features2>::const_reverse_iterator,
			    _super::renumber, true>
      reverse_iterator;
   typedef indexed_selector<typename ensure_features<typename _super::container1, typename _super::needed_features1>::const_reverse_iterator,
			    typename ensure_features<typename _super::index_container, typename _super::needed_features2>::const_reverse_iterator,
			    _super::renumber, true>
      const_reverse_iterator;

   reverse_iterator rbegin()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
      return reverse_iterator(ensure(c1, (typename _super::needed_features1*)0).rbegin(),
			      ensure(this->index_top().get_container2(), (typename _super::needed_features2*)0).rbegin(),
			      true, 1-int(c1.size()));
   }
   reverse_iterator rend()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
      typename _super::index_container_ref indices=this->index_top().get_container2();
      return reverse_iterator(ensure(c1, (typename _super::needed_features1*)0).rend(),
			      ensure(indices, (typename _super::needed_features2*)0).rend(),
			      !iterator_traits<typename reverse_iterator::super>::is_bidirectional || indices.empty() ? 0 : indices.front());
   }
   const_reverse_iterator rbegin() const
   {
      const typename _super::container1& c1=this->manip_top().get_container1();
      return const_reverse_iterator(ensure(c1, (typename _super::needed_features1*)0).rbegin(),
				    ensure(this->index_top().get_container2(), (typename _super::needed_features2*)0).rbegin(),
				    true, 1-int(c1.size()));
   }
   const_reverse_iterator rend() const
   {
      typename _super::index_container_ref indices=this->index_top().get_container2();
      return const_reverse_iterator(ensure(this->manip_top().get_container1(), (typename _super::needed_features1*)0).rend(),
				    ensure(indices, (typename _super::needed_features2*)0).rend(),
				    !iterator_traits<typename reverse_iterator::super>::is_bidirectional || indices.empty() ? 0 : indices.front());
   }
};

template <typename Top, typename Params, subset_classifier::kind _kind>
class indexed_subset_elem_access<Top, Params, _kind, bidirectional_iterator_tag>
   : public indexed_subset_rev_elem_access<Top, Params, _kind> {
   typedef indexed_subset_rev_elem_access<Top, Params, _kind> _super;
public:
   typename _super::reference back()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
      typename _super::container1::reverse_iterator rb1=c1.rbegin();
      std::advance(rb1, int(c1.size())-1 - this->index_top().get_container2().back());
      return *rb1;
   }
   typename _super::reference back() const
   {
      const typename _super::container1& c1=this->manip_top().get_container1();
      typename _super::container1::const_reverse_iterator rb1=c1.rbegin();
      std::advance(rb1, int(c1.size())-1 - this->index_top().get_container2().back());
      return *rb1;
   }
};

template <typename Top, typename Params>
class indexed_subset_elem_access<Top, Params, subset_classifier::plain, input_iterator_tag>
   : public indexed_subset_typebase<Top,Params> {
   typedef indexed_subset_typebase<Top,Params> _super;
protected:
   typedef typename _super::expected_features needed_features1;
public:
   typedef typename ensure_features<typename _super::container1, needed_features1>::iterator iterator;
   typedef typename ensure_features<typename _super::container1, needed_features1>::const_iterator const_iterator;

   iterator begin()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
      iterator b=ensure(c1, (needed_features1*)0).begin();
      std::advance(b, this->manip_top().get_container2().front());
      return b;
   }
   iterator end()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
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
      const typename _super::container1& c1=this->manip_top().get_container1();
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

template <typename Top, typename Params>
class indexed_subset_rev_elem_access<Top, Params, subset_classifier::plain>
   : public indexed_subset_elem_access<Top, Params, subset_classifier::plain, forward_iterator_tag> {
   typedef indexed_subset_elem_access<Top, Params, subset_classifier::plain, forward_iterator_tag> _super;
public:
   typedef typename ensure_features<typename _super::container1, typename _super::needed_features1>::reverse_iterator
      reverse_iterator;
   typedef typename ensure_features<typename _super::container1, typename _super::needed_features1>::const_reverse_iterator
      const_reverse_iterator;

   reverse_iterator rbegin()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
      reverse_iterator rb=ensure(c1, (typename _super::needed_features1*)0).rbegin();
      std::advance(rb, int(c1.size())-1 - this->manip_top().get_container2().back());
      return rb;
   }
   reverse_iterator rend()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
      if (iterator_traits<reverse_iterator>::is_bidirectional) {
	 reverse_iterator re=ensure(c1, (typename _super::needed_features1*)0).rend();
	 std::advance(re, -this->manip_top().get_container2().front());
	 return re;
      } else {
	 reverse_iterator rb=ensure(c1, (typename _super::needed_features1*)0).rbegin();
	 std::advance(rb, int(c1.size()) - this->manip_top().get_container2().front());
	 return rb;
      }
   }
   const_reverse_iterator rbegin() const
   {
      const typename _super::container1& c1=this->manip_top().get_container1();
      const_reverse_iterator rb=ensure(c1, (typename _super::needed_features1*)0).rbegin();
      std::advance(rb, int(c1.size())-1 - this->manip_top().get_container2().back());
      return rb;
   }
   const_reverse_iterator rend() const
   {
      if (iterator_traits<reverse_iterator>::is_bidirectional) {
	 const_reverse_iterator re=ensure(this->manip_top().get_container1(), (typename _super::needed_features1*)0).rend();
	 std::advance(re, -this->manip_top().get_container2().front());
	 return re;
      } else {
	 const_reverse_iterator rb=ensure(this->manip_top().get_container1(), (typename _super::needed_features1*)0).rbegin();
	 std::advance(rb, int(this->manip_top().get_container1().size()) - this->manip_top().get_container2().front());
	 return rb;
      }
   }
};

template <typename Top, typename Params>
class indexed_subset_elem_access<Top, Params, subset_classifier::contiguous, input_iterator_tag>
   : public indexed_subset_typebase<Top,Params> {
   typedef indexed_subset_typebase<Top,Params> _super;
protected:
   typedef typename concat_if<list_search<typename _super::expected_features, indexed, absorbing_feature>::value,
			      provide_construction<indexed,false>,
	   typename concat_if<list_search<typename _super::expected_features, rewindable, absorbing_feature>::value,
			      provide_construction<rewindable,false>,
			      void>::type >::type
      enforce_features1;
   typedef typename mix_features<typename _super::expected_features, enforce_features1>::type needed_features1;
public:
   typedef typename ensure_features<typename _super::container1, needed_features1>::iterator iterator;
   typedef typename ensure_features<typename _super::container1, needed_features1>::const_iterator const_iterator;

   iterator begin()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
      iterator b=ensure(c1, (needed_features1*)0).begin();
      b.contract(_super::renumber, this->manip_top().get_container2().front());
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
      b.contract(_super::renumber, this->manip_top().get_container2().front());
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
   typedef indexed_subset_elem_access<Top, Params, subset_classifier::contiguous, forward_iterator_tag> _super;
public:
   typedef typename ensure_features<typename _super::container1, typename _super::needed_features1>::reverse_iterator reverse_iterator;
   typedef typename ensure_features<typename _super::container1, typename _super::needed_features1>::const_reverse_iterator const_reverse_iterator;

   reverse_iterator rbegin()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
      const typename _super::container2& indices=this->manip_top().get_container2();
      reverse_iterator rb=ensure(c1, (typename _super::needed_features1*)0).rbegin();
      rb.contract(_super::renumber,  int(c1.size())-1 - indices.back(),  _super::index_required ? indices.front() : 0);
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
      const typename _super::container1& c1=this->manip_top().get_container1();
      const typename _super::container2& indices=this->manip_top().get_container2();
      const_reverse_iterator rb=ensure(c1, (typename _super::needed_features1*)0).rbegin();
      rb.contract(_super::renumber,  int(c1.size())-1 - indices.back(),  _super::index_required ? indices.front() : 0);
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
   : public indexed_subset_typebase<Top,Params> {
   typedef indexed_subset_typebase<Top,Params> _super;
protected:
   typedef typename concat_if<list_search<typename _super::expected_features, indexed, absorbing_feature>::value,
			      provide_construction<indexed,false>,
	   typename concat_if<list_search<typename _super::expected_features, rewindable, absorbing_feature>::value,
			      provide_construction<rewindable,false>,
			      provide_construction<end_sensitive,false> >::type >::type
      enforce_features1;
   typedef typename mix_features<typename _super::expected_features, enforce_features1>::type needed_features1;
public:
   typedef typename ensure_features<typename _super::container1, needed_features1>::iterator iterator;
   typedef typename ensure_features<typename _super::container1, needed_features1>::const_iterator const_iterator;

   iterator begin()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
      const typename _super::container2& indices=this->manip_top().get_container2();
      iterator b=ensure(c1, (needed_features1*)0).begin();
      b.contract(_super::renumber, indices.front(), int(c1.size())-1-indices.back());
      return b;
   }
   iterator end()
   {
      return begin()+this->size();
   }
   const_iterator begin() const
   {
      const typename _super::container1& c1=this->manip_top().get_container1();
      const typename _super::container2& indices=this->manip_top().get_container2();
      const_iterator b=ensure(c1, (needed_features1*)0).begin();
      b.contract(_super::renumber, indices.front(), int(c1.size())-1-indices.back());
      return b;
   }
   const_iterator end() const
   {
      return begin()+this->size();
   }
};

template <typename Top, typename Params>
class indexed_subset_rev_elem_access<Top, Params, subset_classifier::range>
   : public indexed_subset_elem_access<Top, Params, subset_classifier::range, forward_iterator_tag> {
   typedef indexed_subset_elem_access<Top, Params, subset_classifier::range, forward_iterator_tag> _super;
public:
   typedef typename ensure_features<typename _super::container1, typename _super::needed_features1>::reverse_iterator reverse_iterator;
   typedef typename ensure_features<typename _super::container1, typename _super::needed_features1>::const_reverse_iterator const_reverse_iterator;

   reverse_iterator rbegin()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
      const typename _super::container2& indices=this->manip_top().get_container2();
      reverse_iterator rb=ensure(c1, (typename _super::needed_features1*)0).rbegin();
      rb.contract(_super::renumber, int(c1.size())-1 - indices.back(), indices.front());
      return rb;
   }
   reverse_iterator rend()
   {
      return rbegin()+this->size();
   }
   const_reverse_iterator rbegin() const
   {
      const typename _super::container1& c1=this->manip_top().get_container1();
      const typename _super::container2& indices=this->manip_top().get_container2();
      const_reverse_iterator rb=ensure(c1, (typename _super::needed_features1*)0).rbegin();
      rb.contract(_super::renumber, int(c1.size())-1 - indices.back(), indices.front());
      return rb;
   }
   const_reverse_iterator rend() const
   {
      return rbegin()+this->size();
   }
};

template <typename Top, typename Params, subset_classifier::kind _kind>
class indexed_subset_elem_access<Top, Params, _kind, random_access_iterator_tag>
   : public indexed_subset_elem_access<Top, Params, _kind, bidirectional_iterator_tag> {
   typedef indexed_subset_elem_access<Top, Params, _kind, bidirectional_iterator_tag> _super;
public:
   typename _super::reference front()
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2().front() ];
   }
   typename _super::const_reference front() const
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2().front() ];
   }
   typename _super::reference back()
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2().back() ];
   }
   typename _super::const_reference back() const
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2().back() ];
   }
   typename _super::reference operator[] (int i)
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2()[i] ];
   }
   typename _super::const_reference operator[] (int i) const
   {
      return this->manip_top().get_container1()[ this->manip_top().get_container2()[i] ];
   }
};

template <typename Top, typename Params>
class indexed_subset_elem_access<Top, Params, subset_classifier::sparse, forward_iterator_tag>
   : public indexed_subset_typebase<Top,Params> {
   typedef indexed_subset_typebase<Top,Params> _super;
protected:
   typedef typename extract_type_param<Params, IteratorCoupler, sparse_indexed_selector_coupler<false, _super::renumber> >::type
      it_coupler;
   typedef typename it_coupler::template defs<typename container_traits<typename _super::data_container>::iterator,
					      typename container_traits<typename _super::index_container>::const_iterator,
					      typename _super::expected_features>::needed_features1
      needed_features1;
   typedef typename it_coupler::template defs<typename container_traits<typename _super::data_container>::const_iterator,
					      typename container_traits<typename _super::index_container>::const_iterator,
					      typename _super::expected_features>::needed_features2
      needed_features2;

   typedef typename it_coupler::template defs<typename ensure_features<typename _super::data_container, needed_features1>::iterator,
					      typename ensure_features<typename _super::index_container, needed_features2>::const_iterator,
					      typename _super::expected_features>::iterator
      iterator_pair;
   typedef typename it_coupler::template defs<typename ensure_features<typename _super::data_container, needed_features1>::const_iterator,
					      typename ensure_features<typename _super::index_container, needed_features2>::const_iterator,
					      typename _super::expected_features>::iterator
      const_iterator_pair;

public:
   typedef typename subset_classifier::iterator_helper<iterator_pair, typename _super::operation>::iterator
      iterator;
   typedef typename subset_classifier::iterator_helper<const_iterator_pair, typename _super::operation>::iterator
      const_iterator;

   iterator begin()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
      return iterator(ensure(c1, (needed_features1*)0).begin(),
		      ensure(this->index_top().get_container2(), (needed_features2*)0).begin());
   }
   iterator end()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
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

   typename _super::reference front() { return *begin(); }
   typename _super::const_reference front() const { return *begin(); }
};

template <typename Top, typename Params>
class indexed_subset_elem_access<Top, Params, subset_classifier::sparse, bidirectional_iterator_tag>
   : public indexed_subset_elem_access<Top, Params, subset_classifier::sparse, forward_iterator_tag> {
   typedef indexed_subset_elem_access<Top, Params, subset_classifier::sparse, forward_iterator_tag> _super;
protected:
   typedef typename reverse_coupler<typename _super::it_coupler>::type rev_coupler;
public:
   typedef typename rev_coupler::template defs<typename ensure_features<typename _super::data_container, typename _super::needed_features1>::reverse_iterator,
					       typename ensure_features<typename _super::index_container, typename _super::needed_features2>::const_reverse_iterator,
					       typename _super::expected_features>::iterator
      reverse_iterator_pair;
   typedef typename rev_coupler::template defs<typename ensure_features<typename _super::data_container, typename _super::needed_features1>::const_reverse_iterator,
					       typename ensure_features<typename _super::index_container, typename _super::needed_features2>::const_reverse_iterator,
					       typename _super::expected_features>::iterator
      const_reverse_iterator_pair;
   typedef typename subset_classifier::iterator_helper<reverse_iterator_pair,typename _super::operation>::iterator
      reverse_iterator;
   typedef typename subset_classifier::iterator_helper<const_reverse_iterator_pair,typename _super::operation>::iterator
      const_reverse_iterator;

   reverse_iterator rbegin()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
      return reverse_iterator(ensure(c1, (typename _super::needed_features1*)0).rbegin(),
			      ensure(this->index_top().get_container2(), (typename _super::needed_features2*)0).rbegin());
   }
   reverse_iterator rend()
   {
      typename _super::data_container_ref c1=this->manip_top().get_container1();
      return reverse_iterator(ensure(c1, (typename _super::needed_features1*)0).rend(),
			      ensure(this->index_top().get_container2(), (typename _super::needed_features2*)0).rend());
   }
   const_reverse_iterator rbegin() const
   {
      return const_reverse_iterator(ensure(this->manip_top().get_container1(), (typename _super::needed_features1*)0).rbegin(),
				    ensure(this->index_top().get_container2(), (typename _super::needed_features2*)0).rbegin());
   }
   const_reverse_iterator rend() const
   {
      return const_reverse_iterator(ensure(this->manip_top().get_container1(), (typename _super::needed_features1*)0).rend(),
				    ensure(this->index_top().get_container2(), (typename _super::needed_features2*)0).rend());
   }

   typename _super::reference back() { return *rbegin(); }
   typename _super::const_reference back() const { return *rbegin(); }
};

template <typename Top, typename Params=typename Top::manipulator_params>
class indexed_subset_impl
   : public indexed_subset_elem_access<Top,Params> {
public:
   typedef indexed_subset_impl<Top,Params> manipulator_impl;
   typedef Params manipulator_params;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef indexed_subset_impl<FeatureCollector,Params> type;
   };
};

template <typename ContainerRef1, typename ContainerRef2, typename Params=void> class IndexedSubset;

template <typename ContainerRef1, typename ContainerRef2, typename Params,
          typename Generic1=typename object_traits<typename deref<ContainerRef1>::type>::generic_type,
          typename Generic2=typename object_traits<typename deref<ContainerRef2>::type>::generic_type>
class generic_of_indexed_subset {};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
class IndexedSubset
   : public container_pair_base<ContainerRef1, ContainerRef2>,
     public indexed_subset_impl< IndexedSubset<ContainerRef1,ContainerRef2,Params>,
				 typename concat_list< cons< Container1< ContainerRef1 >,
							     Container2< ContainerRef2 > >,
						       Params >::type >,
     public generic_of_indexed_subset<ContainerRef1, ContainerRef2, Params> {
   typedef container_pair_base<ContainerRef1, ContainerRef2> _base;
public:
   IndexedSubset(typename _base::first_arg_type src1_arg, typename _base::second_arg_type src2_arg)
      : _base(src1_arg,src2_arg) {}

   int dim() const { return get_dim(this->get_container1()); }
};

template <typename ContainerRef1, typename ContainerRef2, typename Params,
	  typename Set1, typename E, typename Comparator, typename Set2>
class generic_of_indexed_subset<ContainerRef1, ContainerRef2, Params,
                                GenericSet<Set1,E,Comparator>, GenericSet<Set2,int,operations::cmp> >
   : public GenericSet<IndexedSubset<ContainerRef1,ContainerRef2,Params>, E, Comparator> {
public:
   const Comparator& get_comparator() const
   {
      return this->manip_top().get_container1().get_comparator();
   }
};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct spec_object_traits< IndexedSubset<ContainerRef1, ContainerRef2, Params> >
   : spec_object_traits<is_container> {
   static const bool is_temporary    = true,
                     is_lazy         = object_traits<typename deref<ContainerRef1>::type>::is_lazy,
                     is_always_const = effectively_const<ContainerRef1>::value;
};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct check_container_feature<IndexedSubset<ContainerRef1,ContainerRef2,Params>, sparse_compatible>
   : check_container_feature<typename deref<ContainerRef1>::type, sparse_compatible> {};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct check_container_feature<IndexedSubset<ContainerRef1,ContainerRef2,Params>, sparse>
   : check_container_feature<typename deref<ContainerRef1>::type, sparse> {};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct check_container_feature<IndexedSubset<ContainerRef1,ContainerRef2,Params>, pure_sparse>
   : check_container_feature<typename deref<ContainerRef1>::type, pure_sparse> {};

template <typename IndexSet> inline
typename disable_if<bool, check_container_feature<typename Concrete<IndexSet>::type, sparse_compatible>::value>::type
set_within_range(const IndexSet& s, int d)
{
   const typename Unwary<typename Concrete<IndexSet>::type>::type& ss=unwary(concrete(s));
   return ss.empty() || (ss.front()>=0 && ss.back()<d);
}

template <typename IndexSet> inline
typename enable_if<bool, check_container_feature<typename Concrete<IndexSet>::type, sparse_compatible>::value>::type
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
IndexedSubset<typename Unwary<Container>::type&, const typename Unwary<typename Concrete<IndexSet>::type>::type&>
select(Container& c, const IndexSet& indices)
{
   if (POLYMAKE_DEBUG || !Unwary<Container>::value || !Unwary<typename Concrete<IndexSet>::type>::value) {
      if (!set_within_range(indices, get_dim(unwary(c))))
	 throw std::runtime_error("select - indices out of range");
   }
   return IndexedSubset<typename Unwary<Container>::type&, const typename Unwary<typename Concrete<IndexSet>::type>::type&>
                       (unwary(c), unwary(concrete(indices)));
}

template <typename Container, typename IndexSet> inline
IndexedSubset<const typename Unwary<Container>::type&, const typename Unwary<typename Concrete<IndexSet>::type>::type&>
select(const Container& c, const IndexSet& indices)
{
   if (POLYMAKE_DEBUG || !Unwary<Container>::value || !Unwary<typename Concrete<IndexSet>::type>::value) {
      if (!set_within_range(indices, get_dim(unwary(c))))
	 throw std::runtime_error("select - indices out of range");
   }
   return IndexedSubset<const typename Unwary<Container>::type&, const typename Unwary<typename Concrete<IndexSet>::type>::type&>
                       (unwary(c), unwary(concrete(indices)));
}

template <typename ContainerRef1, typename ContainerRef2, typename Params=void>
class IndexedSlice;

template <typename ContainerRef1, typename ContainerRef2, typename Params,
          typename Generic1=typename object_traits<typename deref<ContainerRef1>::type>::generic_type,
          typename Generic2=typename object_traits<typename deref<ContainerRef2>::type>::generic_type>
class generic_of_indexed_slice
   : public inherit_generic< IndexedSlice<ContainerRef1,ContainerRef2,Params>, typename deref<ContainerRef1>::type>::type {};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct IndexedSlice_impl {
   typedef indexed_subset_impl< IndexedSlice<ContainerRef1,ContainerRef2,Params>,
				typename concat_list< cons< Container1< ContainerRef1 >,
						      cons< Container2< ContainerRef2 >,
							    Renumber< True > > >,
						      Params >::type >
      type;
};

template <typename ContainerRef1, typename ContainerRef2, typename Params,
	  bool _immutable=effectively_const<ContainerRef1>::value,
	  bool _sparse=check_container_ref_feature<ContainerRef1, sparse>::value,
	  typename tag=typename object_traits<typename deref<ContainerRef1>::type>::generic_tag,
	  bool _bidir=derived_from<typename IndexedSlice_impl<ContainerRef1, ContainerRef2, Params>::type::container_category,
				   bidirectional_iterator_tag>::value>
class IndexedSlice_mod {};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
class IndexedSlice
   : public container_pair_base<ContainerRef1, ContainerRef2>,
     public IndexedSlice_impl<ContainerRef1, ContainerRef2, Params>::type,
     public IndexedSlice_mod<ContainerRef1, ContainerRef2, Params>,
     public generic_of_indexed_slice<ContainerRef1,ContainerRef2,Params> {
   typedef container_pair_base<ContainerRef1, ContainerRef2> _base;
public:
   typedef typename inherit_generic<IndexedSlice, typename deref<ContainerRef1>::type>::type
      generic_mutable_type;

   IndexedSlice(typename _base::first_arg_type src1_arg, typename _base::second_arg_type src2_arg)
      : _base(src1_arg,src2_arg) {}

   IndexedSlice& operator= (const IndexedSlice& other) { return generic_mutable_type::operator=(other); }
   using generic_mutable_type::operator=;

protected:
   int dim(False) const { return this->get_container2().size(); }
   int dim(True) const { return get_dim(this->get_container1()) - this->get_container2().base().size(); }
public:
   int dim() const
   {
      return dim(derived_from_instance3<typename deref<ContainerRef2>::type, Complement>());
   }

   template <typename,typename,typename,bool,bool,typename,bool> friend class IndexedSlice_mod;
};

template <typename ContainerRef1, typename ContainerRef2, typename Params,
	  typename Set1, typename E, typename Comparator, typename Set2>
class generic_of_indexed_slice<ContainerRef1, ContainerRef2, Params,
			       GenericSet<Set1,E,Comparator>, GenericSet<Set2,int,operations::cmp> >
   : public inherit_generic< IndexedSlice<ContainerRef1,ContainerRef2,Params>, typename deref<ContainerRef1>::type>::type {
public:
   const Comparator& get_comparator() const
   {
      return this->top().get_container1().get_comparator();
   }
};

// set, forward category
template <typename ContainerRef1, typename ContainerRef2, typename Params>
class IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, false, is_set, false> {
   typedef IndexedSlice<ContainerRef1,ContainerRef2,Params> master;
protected:
   typedef typename IndexedSlice_impl<ContainerRef1,ContainerRef2,Params>::type _impl;
private:
   typename _impl::iterator::second_type _rewind_index(const typename _impl::iterator::second_type&, int i, forward_iterator_tag)
   {
      master& me=static_cast<master&>(*this);
      typename _impl::iterator::second_type iit=ensure(me.index_top().get_container2(), (typename master::needed_features2*)0).begin();
      while (iit.index() < i) ++iit;
      return iit;
   }

   typename _impl::iterator::second_type _rewind_index(typename _impl::iterator::second_type iit, int i, bidirectional_iterator_tag)
   {
      if (iit.at_end()) --iit;
      std::advance(iit, i-iit.index());
      return iit;
   }
protected:
   typename _impl::iterator::second_type rewind_index(const typename _impl::iterator::second_type& iit, int i)
   {
      return _rewind_index(iit, i, typename iterator_traits<typename _impl::iterator::second_type>::iterator_category());
   }
public:
   void clear()
   {
      master& me=static_cast<master&>(*this);
      for (typename _impl::iterator it=me.begin(); !it.at_end(); )
	 me.get_container1().erase(it++);
   }

   typename _impl::iterator insert(const typename _impl::iterator& pos, int i)
   {
      master& me=static_cast<master&>(*this);
      typename _impl::iterator::second_type iit=rewind_index(pos.second, i);
      return typename _impl::iterator(me.get_container1().insert(pos, *iit), iit);
   }

   typename _impl::iterator insert(int i)
   {
      master& me=static_cast<master&>(*this);
      typename _impl::iterator::second_type iit=rewind_index(ensure(me.index_top().get_container2(), (typename master::needed_features2*)0).end(), i);
      return typename _impl::iterator(me.get_container1().insert(*iit), iit);
   }

   void erase(const typename _impl::iterator& pos)
   {
      master& me=static_cast<master&>(*this);
      me.get_container1().erase(pos);
   }
};

// set, bidirectional category
template <typename ContainerRef1, typename ContainerRef2, typename Params>
class IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, false, is_set, true>
   : public IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, false, is_set, false> {
   typedef IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, false, is_set, false> _super;
   typedef IndexedSlice<ContainerRef1,ContainerRef2,Params> master;
protected:
   typedef typename _super::_impl _impl;
private:
   typename _impl::reverse_iterator::second_type _rewind_index(const typename _impl::reverse_iterator::second_type&, int i, forward_iterator_tag)
   {
      master& me=static_cast<master&>(*this);
      typename _impl::reverse_iterator::second_type iit=ensure(me.index_top().get_container2(), (typename master::needed_features2*)0).rbegin();
      while (iit.index() > i) ++iit;
      return iit;
   }

   typename _impl::reverse_iterator::second_type _rewind_index(typename _impl::reverse_iterator::second_type iit, int i, bidirectional_iterator_tag)
   {
      if (iit.at_end()) --iit;
      std::advance(iit, iit.index()-i);
      return iit;
   }
protected:
   typename _impl::reverse_iterator::second_type rewind_index(const typename _impl::reverse_iterator::second_type& iit, int i)
   {
      return _rewind_index(iit, i, typename iterator_traits<typename _impl::reverse_iterator::second_type>::iterator_category());
   }
   using _super::rewind_index;
public:
   typename _impl::reverse_iterator insert(const typename _impl::reverse_iterator& pos, int i)
   {
      master& me=static_cast<master&>(*this);
      typename _impl::reverse_iterator::second_type iit=rewind_index(pos.second, i);
      return typename _impl::reverse_iterator(me.get_container1().insert(pos, *iit), iit);
   }
   using _super::insert;

   void erase(const typename _impl::reverse_iterator& pos)
   {
      master& me=static_cast<master&>(*this);
      me.get_container1().erase(pos);
   }
   using _super::erase;
};

// sparse vector, forward category
template <typename ContainerRef1, typename ContainerRef2, typename Params, typename tag>
class IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, true, tag, false>
   : public IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, false, is_set> {
   typedef IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, false, is_set> _super;
   typedef IndexedSlice<ContainerRef1,ContainerRef2,Params> master;
protected:
   typedef typename _super::_impl _impl;
public:
   template <typename Data>
   typename _impl::iterator insert(const typename _impl::iterator& pos, int i, const Data& d)
   {
      master& me=static_cast<master&>(*this);
      typename _impl::iterator::second_type iit=this->rewind_index(pos.second, i);
      return typename _impl::iterator(me.get_container1().insert(pos, *iit, d), iit);
   }

   template <typename Data>
   typename _impl::iterator insert(int i, const Data& d)
   {
      master& me=static_cast<master&>(*this);
      typename _impl::iterator::second_type iit=this->rewind_index(ensure(me.index_top().get_container2(), (typename master::needed_features2*)0).end(), i);
      return typename _impl::iterator(me.get_container1().insert(*iit, d), iit);
   }

   using _super::insert;
};

// sparse vector, bidirectional category
template <typename ContainerRef1, typename ContainerRef2, typename Params, typename tag>
class IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, true, tag, true>
   : public IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, true, tag, false> {
   typedef IndexedSlice_mod<ContainerRef1, ContainerRef2, Params, false, true, tag, false> _super;
   typedef IndexedSlice<ContainerRef1,ContainerRef2,Params> master;
protected:
   typedef typename _super::_impl _impl;
public:
   template <typename Data>
   typename _impl::reverse_iterator insert(const typename _impl::reverse_iterator& pos, int i, const Data& d)
   {
      master& me=static_cast<master&>(*this);
      typename _impl::reverse_iterator::second_type iit=this->rewind_index(pos.second, i);
      return typename _impl::reverse_iterator(me.get_container1().insert(pos, *iit, d), iit);
   }
   using _super::insert;
};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct spec_object_traits< IndexedSlice<ContainerRef1,ContainerRef2,Params> >
   : spec_object_traits<is_container> {
   static const bool is_temporary    = true,
                     is_lazy         = object_traits<typename deref<ContainerRef1>::type>::is_lazy,
                     is_always_const = effectively_const<ContainerRef1>::value;
};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct check_container_feature<IndexedSlice<ContainerRef1,ContainerRef2,Params>, sparse_compatible>
   : check_container_ref_feature<ContainerRef1, sparse_compatible> {};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct check_container_feature<IndexedSlice<ContainerRef1,ContainerRef2,Params>, sparse>
   : check_container_ref_feature<ContainerRef1, sparse> {};

template <typename ContainerRef1, typename ContainerRef2, typename Params>
struct check_container_feature<IndexedSlice<ContainerRef1,ContainerRef2,Params>, pure_sparse>
   : check_container_ref_feature<ContainerRef1, pure_sparse> {};

} // end namespace pm

namespace polymake {
using pm::select;
using pm::IndexedSubset;
using pm::IndexedSlice;
}

namespace std {
// due to silly overloading rules
template <typename ContainerRef1, typename ContainerRef2, typename Params> inline
void swap(pm::IndexedSlice<ContainerRef1,ContainerRef2,Params>& s1,
          pm::IndexedSlice<ContainerRef1,ContainerRef2,Params>& s2) { s1.swap(s2); }
}

#endif // POLYMAKE_INDEXED_SUBSET_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
