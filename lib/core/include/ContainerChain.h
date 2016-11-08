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

#ifndef POLYMAKE_CONTAINER_CHAIN_H
#define POLYMAKE_CONTAINER_CHAIN_H

#include "polymake/internal/operations.h"
#include "polymake/ContainerUnion.h"

namespace pm {

template <typename Top, typename TParams> class container_chain_typebase;
template <typename IteratorList, bool TReversed=false> class iterator_chain;

template <typename Iterator>
struct iterator_chain_helper {
   static const bool match=true;
   typedef Iterator type;
   typedef typename iterator_traits<Iterator>::iterator iterator_list;
   typedef typename iterator_traits<Iterator>::const_iterator const_iterator_list;
};

template <typename Iterator1, typename Helper>
struct iterator_chain_helper2 {
   static const bool
      exact_match_here=std::is_same<Iterator1, typename Helper::type>::value,
      match_here=exact_match_here || std::is_same<typename iterator_traits<Iterator1>::const_iterator,
                                                  typename iterator_traits<typename Helper::type>::const_iterator>::value,
      match=Helper::match && match_here;

   typedef typename std::conditional<exact_match_here, Iterator1, typename iterator_traits<Iterator1>::const_iterator>::type
      type;
};

template <typename Head, typename Tail>
struct iterator_chain_helper< cons<Head, Tail> >
   : iterator_chain_helper2<Head, iterator_chain_helper<Tail> > {
   typedef typename concat_list< typename iterator_chain_helper<Head>::iterator_list, typename iterator_chain_helper<Tail>::iterator_list >::type
      iterator_list;
   typedef typename concat_list< typename iterator_chain_helper<Head>::const_iterator_list, typename iterator_chain_helper<Tail>::const_iterator_list >::type
      const_iterator_list;
};


// FIXME: duplicates is_const_compatible_with
template <typename Iterator1, typename Iterator2>
struct matching_iterator {
   static const bool value= std::is_same<Iterator1, Iterator2>::value ||
                            std::is_same<typename iterator_traits<Iterator1>::iterator, Iterator2>::value;
};

template <typename Iterator1, typename Iterator2>
struct comparable_iterator {
   static const bool value= std::is_same<typename iterator_traits<Iterator1>::iterator, Iterator2>::value ||
                            std::is_same<typename iterator_traits<Iterator1>::const_iterator, Iterator2>::value;
};

template <typename IteratorList1, typename IteratorList2>
struct suitable_iterator_list
   : list_accumulate_binary<list_and, matching_iterator, IteratorList1, IteratorList2> {};

template <typename IteratorList1, typename IteratorList2>
struct comparable_iterator_list
   : list_accumulate_binary<list_and, comparable_iterator, IteratorList1, IteratorList2> {};


template <int size>
struct iterator_chain_offset_store {
   int off[size];

   static const int last=size-1;

   template <typename Container>
   void init(const Container&, int_constant<last>) {}

   template <typename Container, int discr>
   void init(const Container& c, int_constant<discr>)
   {
      off[discr+1]=off[discr]+get_dim(c);
   }

   template <typename Container>
   void init(const Container& c, int_constant<0>)
   {
      off[0]=0;
      off[1]=get_dim(c);
   }

   int operator[] (int discr) const { return off[discr]; }
};

template <>
struct iterator_chain_offset_store<0> {
   template <typename Container, int discr>
   void init(const Container&, int_constant<discr>) {}
};

template <typename IteratorList,
          bool _homogeneous=iterator_chain_helper<IteratorList>::match,
          int pos=0, int n=list_length<IteratorList>::value>
class iterator_chain_store {
protected:
   typedef typename iterator_chain_helper<IteratorList>::type it_type;
   static const bool _indexed=list_accumulate_binary<list_and, check_iterator_feature, IteratorList, same<indexed> >::value;
   static const int n_offsets=_indexed ? n : 0;
   it_type its[n];
   iterator_chain_offset_store<n_offsets> offsets;

   struct traits {
      typedef typename iterator_traits<it_type>::iterator iterator_list;
      typedef typename iterator_traits<it_type>::const_iterator const_iterator_list;
   };

public:
   typedef typename iterator_traits<it_type>::reference reference;
   typedef typename iterator_traits<it_type>::pointer pointer;
   typedef typename iterator_traits<it_type>::value_type value_type;

protected:
   iterator_chain_store() {}

   template <typename IteratorList2>
   iterator_chain_store(const iterator_chain_store<IteratorList2, _homogeneous, pos, n>& s2)
      : offsets(s2.offsets)
   {
      std::copy(s2.its, s2.its+n, its);
   }

   template <typename IteratorList2>
   void operator= (const iterator_chain_store<IteratorList2, _homogeneous, pos, n>& s2)
   {
      std::copy(s2.its, s2.its+n, its);
      offsets=s2.offsets;
   }

   reference star(int discr) const
   {
      return *its[discr];
   }

   pointer arrow(int discr) const
   {
      return its[discr].operator->();
   }

   int index(int discr) const
   {
      return its[discr].index();
   }

   bool incr(int discr)
   {
      return (++its[discr]).at_end();
   }

   template <typename IteratorList2>
   bool eq(int discr, const iterator_chain_store<IteratorList2, _homogeneous, pos, n>& s2) const
   {
      return its[discr] == s2.its[discr];
   }

   bool at_end(int discr) const
   {
      return its[discr].at_end();
   }

   void rewind()
   {
      for (it_type *itp=its, itp_end=itp+n; itp<itp_end; ++itp) itp->rewind();
   }

private:
   template <typename Container, typename needed_features, int discr, bool is_end>
   void init_step(Container& c, needed_features*, int_constant<discr>, bool_constant<is_end>)
   {
      if (!is_end)
         its[discr]=ensure(c, (needed_features*)0).begin();
      else
         its[discr]=ensure(c, (needed_features*)0).end();
      offsets.init(c, int_constant<discr>());
   }

   template <typename Container, typename needed_features, int discr, bool is_end>
   void init_step(const Container& c, needed_features*, int_constant<discr>, bool_constant<is_end>)
   {
      if (!is_end)
         its[discr]=ensure(c, (needed_features*)0).begin();
      else
         its[discr]=ensure(c, (needed_features*)0).end();
      offsets.init(c, int_constant<discr>());
   }

   template <typename Chain, typename needed_features, int discr, bool is_end>
   void init(Chain& c, needed_features*, int_constant<discr>, bool_constant<is_end>)
   {
      init_step(c.get_container(int_constant<discr>()), (needed_features*)0, int_constant<discr>(), bool_constant<is_end>());
      init(c, (needed_features*)0, int_constant<discr+1>(), bool_constant<is_end>());
   }

   template <typename Chain, typename needed_features, bool is_end>
   void init(Chain& c, needed_features*, int_constant<n>, bool_constant<is_end>) {}

protected:
   template <typename Chain, bool is_rev, bool is_end>
   bool init(Chain& c, bool_constant<is_rev>, bool_constant<is_end>)
   {
      typedef typename std::conditional<is_rev, typename toggle_features<typename Chain::needed_features, _reversed>::type,
                                                typename Chain::needed_features>::type
         needed_features;
      init(c, (needed_features*)0, int_constant<0>(), bool_constant<is_end>());
      return is_end || its[0].at_end();
   }

   template <typename, bool, int, int> friend class iterator_chain_store;
};

template <typename IteratorList, int pos, int n>
class iterator_chain_store<IteratorList, false, pos, n>
   : public iterator_chain_store<IteratorList, false, pos+1, n> {

   template <typename, bool, int, int> friend class iterator_chain_store;

   typedef iterator_chain_store<IteratorList, false, pos+1, n> base_t;
   typedef typename std::conditional<pos+1!=n, base_t, iterator_chain_store>::type next;

   typedef typename n_th<IteratorList, pos>::type it_type;
   it_type it;

protected:
   iterator_chain_store() {}

   template <typename IteratorList2>
   iterator_chain_store(const iterator_chain_store<IteratorList2, false, pos, n>& s2)
      : base_t(static_cast<const iterator_chain_store<IteratorList2, false, pos+1, n>&>(s2)),
        it(s2.it) {}

   template <typename IteratorList2>
   void operator= (const iterator_chain_store<IteratorList2, false, pos, n>& s2)
   {
      it=s2.it;
      base_t::operator=(s2);
   }

   typename base_t::reference star(int discr) const
   {
      switch (discr) {
      case pos:
         return *it;
      default:
         return next::star(discr);
      }
   }

   typename base_t::pointer arrow(int discr) const   {
      switch (discr) {
      case pos:
         return it.operator->();
      default:
         return next::arrow(discr);
      }
   }

   int index(int discr) const
   {
      switch (discr) {
      case pos:
         return it.index();
      default:
         return next::index(discr);
      }
   }

   bool incr(int discr)
   {
      switch (discr) {
      case pos:
         return (++it).at_end();
      default:
         return next::incr(discr);
      }
   }

   template <typename IteratorList2>
   bool eq(int discr, const iterator_chain_store<IteratorList2, false, pos, n>& s2) const
   {
      switch (discr) {
      case pos:
         return it == s2.it;
      default:
         return next::eq(discr,s2);
      }
   }

   bool at_end(int discr) const
   {
      switch (discr) {
      case pos:
         return it.at_end();
      default:
         return next::at_end(discr);
      }
   }

   void rewind()
   {
      it.rewind();
      if (pos+1 < n) next::rewind();
   }

public:
   const it_type& get_part(int_constant<pos>) const { return it; }
   using base_t::get_part;

private:
   template <typename Container, typename needed_features, bool is_end>
   void init_step(Container& c, needed_features*, bool_constant<is_end>)
   {
      if (!is_end)
         it=ensure(c, (needed_features*)0).begin();
      else
         it=ensure(c, (needed_features*)0).end();
      base_t::offsets.init(c, int_constant<pos>());
   }

   template <typename Container, typename needed_features, bool is_end>
   void init_step(const Container& c, needed_features*, bool_constant<is_end>)
   {
      if (!is_end)
         it=ensure(c, (needed_features*)0).begin();
      else
         it=ensure(c, (needed_features*)0).end();
      base_t::offsets.init(c, int_constant<pos>());
   }

protected:
   template <typename Chain, bool is_rev, bool is_end>
   bool init(Chain& c, bool_constant<is_rev>, bool_constant<is_end>)
   {
      typedef typename std::conditional<is_rev, typename toggle_features<typename Chain::needed_features, _reversed>::type,
                                                typename Chain::needed_features>::type
         needed_features;
      init_step(c.get_container(int_constant<pos>()), (needed_features*)0, bool_constant<is_end>());
      if (pos+1 < n) next::init(c, bool_constant<is_rev>(), bool_constant<is_end>());
      return pos==0 && (is_end || it.at_end());
   }
};

template <typename IteratorList, int n>
class iterator_chain_store<IteratorList, false, n, n> {
   template <typename, bool, int, int> friend class iterator_chain_store;
protected:
   static const bool _indexed=list_accumulate_binary<list_and, check_iterator_feature, IteratorList, same<indexed> >::value;
   static const int n_offsets=_indexed ? n : 0;
   iterator_chain_offset_store<n_offsets> offsets;

   typedef union_iterator_traits<IteratorList> traits;

   iterator_chain_store() {}

   template <typename IteratorList2>
   iterator_chain_store(const iterator_chain_store<IteratorList2, false, n, n>& s2)
      : offsets(s2.offsets) {}

   template <typename IteratorList2>
   void operator= (const iterator_chain_store<IteratorList2, false, n, n>& s2)
   {
      offsets=s2.offsets;
   }

public:
   typedef typename traits::reference reference;
   typedef typename traits::pointer pointer;
   typedef typename traits::value_type value_type;

   void get_part(int_constant<n>) const = delete;
};

template <typename IteratorList, bool TReversed>
class iterator_chain : public iterator_chain_store<IteratorList> {
   typedef iterator_chain_store<IteratorList> base_t;
protected:
   int discr;
   static const int d_step= TReversed ? -1 : 1,
                    d_start= TReversed ? list_length<IteratorList>::value-1 : 0,
                    d_finish= TReversed ? -1 : list_length<IteratorList>::value;

   void valid_position()
   {
      do {
         discr+=d_step;
      } while (!at_end() && base_t::at_end(discr));
   }

   template <typename, bool> friend class iterator_chain;
public:
   typedef forward_iterator_tag iterator_category;
   typedef ptrdiff_t difference_type;
   typedef iterator_chain<typename iterator_chain_helper<IteratorList>::iterator_list, TReversed> iterator;
   typedef iterator_chain<typename iterator_chain_helper<IteratorList>::const_iterator_list, TReversed> const_iterator;

   iterator_chain() {}

   template <typename IteratorList2, typename enabled=typename std::enable_if<suitable_iterator_list<IteratorList, IteratorList2>::value>::type>
   iterator_chain(const iterator_chain<IteratorList2, TReversed>& it)
      : base_t(it)
      , discr(it.discr) {}

   template <typename IteratorList2, typename enabled=typename std::enable_if<suitable_iterator_list<IteratorList, IteratorList2>::value>::type>
   iterator_chain& operator= (const iterator_chain<IteratorList2, TReversed>& it)
   {
      base_t::operator=(it);  discr=it.discr;
      return *this;
   }

   template <typename Top, typename TParams>
   iterator_chain(container_chain_typebase<Top, TParams>& c)
      : discr(d_start)
   {
      if (this->init(c, bool_constant<TReversed>(), std::false_type()))
         valid_position();
   }

   template <typename Top, typename TParams>
   iterator_chain(const container_chain_typebase<Top, TParams>& c)
      : discr(d_start)
   {
      if (this->init(c, bool_constant<TReversed>(), std::false_type()))
         valid_position();
   }

   template <typename Top, typename TParams>
   iterator_chain(container_chain_typebase<Top, TParams>& c, bool is_end)
      : discr(d_finish)
   {
      this->init(c, bool_constant<TReversed>(), std::true_type());
   }

   template <typename Top, typename TParams>
   iterator_chain(const container_chain_typebase<Top, TParams>& c, bool is_end)
      : discr(d_finish)
   {
      this->init(c, bool_constant<TReversed>(), std::true_type());
   }

   typename base_t::reference operator* () const
   {
      return this->star(discr);
   }

   typename base_t::pointer operator-> () const
   {
      return this->arrow(discr);
   }

   iterator_chain& operator++ ()
   {
      if (this->incr(discr)) valid_position();
      return *this;
   }
   const iterator_chain operator++ (int) { iterator_chain copy=*this; operator++(); return copy; }

   template <typename IteratorList2>
   typename std::enable_if<comparable_iterator_list<IteratorList, IteratorList2>::value, bool>::type
   operator== (const iterator_chain<IteratorList2, TReversed>& it) const
   {
      return discr==it.discr && this->eq(discr,it);
   }

   template <typename IteratorList2>
   typename std::enable_if<comparable_iterator_list<IteratorList, IteratorList2>::value, bool>::type
   operator!= (const iterator_chain<IteratorList2, TReversed>& it) const
   {
      return !operator==(it);
   }

   bool at_end() const
   {
      return discr == d_finish;
   }

   void rewind()
   {
      static_assert(check_iterator_feature<iterator_chain, rewindable>::value, "iterator is not rewindable");
      base_t::rewind();
      discr=d_start;
      if (base_t::at_end(discr)) valid_position();
   }

   int index() const
   {
      static_assert(check_iterator_feature<iterator_chain, indexed>::value, "iterator is not indexed");
      return base_t::index(discr) + this->offsets[discr];
   }

   int get_part_index() const { return discr; }
};

template <typename IteratorList, bool TReversed, typename Feature>
struct check_iterator_feature<iterator_chain<IteratorList, TReversed>, Feature>
   : list_accumulate_binary<list_and, check_iterator_feature, IteratorList, same<Feature> > {};

template <typename IteratorList, bool TReversed>
struct check_iterator_feature<iterator_chain<IteratorList, TReversed>, contractable> : std::false_type {};

template <typename Container,
          bool TDescend = is_instance2b<typename container_traits<Container>::iterator, iterator_chain>::value,
          bool TRedirected = is_derived_from_instance_of<typename deref<Container>::type, redirected_container_typebase>::value>
struct container_chain_helper {
   static const bool descend = TDescend,  // false
                     redirected = false,
                     is_const = effectively_const<Container>::value;
   typedef Container container_list;
   typedef const Container const_container_list;
};

template <typename Container>
struct container_chain_helper<Container, true, false> {
   typedef typename Container::manipulator_impl manip;
   static const bool descend=true, redirected=false, is_const=manip::is_const;
   typedef typename Container::container_list container_list;
   typedef typename Container::const_container_list const_container_list;
};

template <typename Container>
struct container_chain_helper<Container, true, true> {
   typedef typename Container::container::manipulator_impl manip;
   static const bool descend=true, redirected=true, is_const=manip::is_const;
   typedef typename Container::container::container_list container_list;
   typedef typename Container::container::const_container_list const_container_list;
};

template <typename Container, typename Features, bool _rev=container_traits<Container>::is_bidirectional>
struct container_chain_traits_helper {
   static const bool reversible=_rev;
   typedef typename ensure_features<Container, Features>::iterator iterator_list;
   typedef typename ensure_features<Container, Features>::const_iterator const_iterator_list;
   typedef typename container_traits<Container>::category category;
};

template <typename Container, typename Features>
struct container_chain_traits_helper<Container, Features, true> : container_chain_traits_helper<Container, Features, false> {
   static const bool reversible=true;
   typedef typename ensure_features<Container, Features>::reverse_iterator reverse_iterator_list;
   typedef typename ensure_features<Container, Features>::const_reverse_iterator const_reverse_iterator_list;
};

template <typename Container, typename Features>
struct container_chain_traits : container_chain_traits_helper<Container, Features> {};

template <typename Traits1, typename Traits2, bool _rev=Traits1::reversible && Traits2::reversible>
struct container_chain_traits_helper2 {
   static const bool reversible=_rev;
   typedef typename concat_list<typename Traits1::iterator_list, typename Traits2::iterator_list>::type iterator_list;
   typedef typename concat_list<typename Traits1::const_iterator_list, typename Traits2::const_iterator_list>::type const_iterator_list;
   typedef typename least_derived_class<typename Traits1::category, typename Traits2::category>::type category;
};

template <typename Traits1, typename Traits2>
struct container_chain_traits_helper2<Traits1, Traits2, true> : container_chain_traits_helper2<Traits1, Traits2, false> {
   static const bool reversible=true;
   typedef typename concat_list<typename Traits1::reverse_iterator_list,
                                typename Traits2::reverse_iterator_list>::type
      reverse_iterator_list;
   typedef typename concat_list<typename Traits1::const_reverse_iterator_list,
                                typename Traits2::const_reverse_iterator_list>::type
      const_reverse_iterator_list;
};

template <typename Head, typename Tail, typename Features>
struct container_chain_traits<cons<Head, Tail>, Features>
   : container_chain_traits_helper2< container_chain_traits<Head, Features>, container_chain_traits<Tail, Features> > {};

template <typename Top, typename TParams>
class container_chain_typebase : public manip_container_top<Top, TParams> {
   typedef manip_container_top<Top, TParams> base_t;
public:
   typedef typename mtagged_list_extract<TParams, Container1Tag>::type container1_ref;
   typedef typename mtagged_list_extract<TParams, Container2Tag>::type container2_ref;
   typedef typename deref<container1_ref>::minus_ref container1;
   typedef typename deref<container2_ref>::minus_ref container2;
   typedef container_chain_helper<container1> helper1;
   typedef container_chain_helper<container2> helper2;
   typedef typename concat_list<typename helper1::container_list, typename helper2::container_list>::type
      _container_list;
   typedef typename concat_list<typename helper1::const_container_list, typename helper2::const_container_list>::type
      const_container_list;

   typedef dense can_enforce_features;
   /* either some containers are sparse (then the resulting iterator_chain is automatically indexed),
      or the whole chain should be made indexed (it is cheaper) */
   typedef cons<indexed, cons<provide_construction<rewindable,false>, provide_construction<end_sensitive,false> > > cannot_enforce_features;

   static const bool
      is_const       = helper1::is_const || helper2::is_const,
      is_sparse      = list_accumulate_binary<list_or, check_container_ref_feature, _container_list, same<sparse> >::value,
      is_pure_sparse = list_accumulate_binary<list_and, check_container_ref_feature, _container_list, same<pure_sparse> >::value;

   typedef typename std::conditional<is_const, const_container_list, _container_list>::type container_list;
   typedef typename mix_features<typename base_t::expected_features,
                                 typename std::conditional<is_sparse,
                                                           typename std::conditional<list_search_all<typename base_t::expected_features, dense, std::is_same>::value,
                                                                                     cons<indexed, end_sensitive>, sparse_compatible>::type,
                                                           end_sensitive>::type
                                >::type
      needed_features;

   typedef iterator_chain<typename container_chain_traits<container_list, needed_features>::iterator_list> iterator;
   typedef iterator_chain<typename container_chain_traits<container_list, needed_features>::const_iterator_list> const_iterator;
   typedef typename iterator::reference reference;
   typedef typename const_iterator::reference const_reference;
   typedef typename iterator::value_type value_type;
   typedef typename container_chain_traits<container_list, needed_features>::category container_category;

   template <int pos>
   struct pos_helper {
      static const int side= pos >= list_length<typename helper1::container_list>::value,
                      discr= (side ? helper2::redirected*2+helper2::descend
                                   : helper1::redirected*2+helper1::descend)*2+side;
      typedef cons< int_constant<pos>, int_constant<discr> > type;
   };

   template <int pos>
   typename n_th<container_list, pos>::type&
   get_container(cons< int_constant<pos>, int_constant<0> >)
   {
      return this->manip_top().get_container1();
   }
   template <int pos>
   typename n_th<const_container_list, pos>::type&
   get_container(cons< int_constant<pos>, int_constant<0> >) const
   {
      return this->manip_top().get_container1();
   }

   template <int pos>
   typename n_th<container_list, pos>::type&
   get_container(cons< int_constant<pos>, int_constant<1> >)
   {
      return this->manip_top().get_container2();
   }
   template <int pos>
   typename n_th<const_container_list, pos>::type&
   get_container(cons< int_constant<pos>, int_constant<1> >) const
   {
      return this->manip_top().get_container2();
   }

   template <int pos>
   typename n_th<container_list, pos>::type&
   get_container(cons< int_constant<pos>, int_constant<2> >)
   {
      return this->manip_top().get_container1().get_container(int_constant<pos>());
   }
   template <int pos>
   typename n_th<const_container_list, pos>::type&
   get_container(cons< int_constant<pos>, int_constant<2> >) const
   {
      return this->manip_top().get_container1().get_container(int_constant<pos>());
   }

   template <int pos>
   typename n_th<container_list, pos>::type&
   get_container(cons< int_constant<pos>, int_constant<6> >)
   {
      return this->manip_top().get_container1().get_container().get_container(int_constant<pos>());
   }
   template <int pos>
   typename n_th<const_container_list, pos>::type&
   get_container(cons< int_constant<pos>, int_constant<6> >) const
   {
      return this->manip_top().get_container1().get_container().get_container(int_constant<pos>());
   }

   template <int pos>
   typename n_th<container_list, pos>::type&
   get_container(cons< int_constant<pos>, int_constant<3> >)
   {
      return this->manip_top().get_container2().get_container(int_constant<(pos-list_length<typename helper1::container_list>::value)>());
   }
   template <int pos>
   typename n_th<const_container_list, pos>::type&
   get_container(cons< int_constant<pos>, int_constant<3> >) const
   {
      return this->manip_top().get_container2().get_container(int_constant<(pos-list_length<typename helper1::container_list>::value)>());
   }

   template <int pos>
   typename n_th<container_list, pos>::type&
   get_container(cons< int_constant<pos>, int_constant<7> >)
   {
      return this->manip_top().get_container2().get_container().get_container(int_constant<(pos-list_length<typename helper1::container_list>::value)>());
   }
   template <int pos>
   typename n_th<const_container_list, pos>::type&
   get_container(cons< int_constant<pos>, int_constant<7> >) const
   {
      return this->manip_top().get_container2().get_container().get_container(int_constant<(pos-list_length<typename helper1::container_list>::value)>());
   }

   template <int pos>
   typename n_th<container_list, pos>::type&
   get_container(int_constant<pos>)
   {
      return get_container(typename pos_helper<pos>::type());
   }

   template <int pos>
   typename n_th<const_container_list, pos>::type&
   get_container(int_constant<pos>) const
   {
      return get_container(typename pos_helper<pos>::type());
   }
};

template <typename Top, typename TParams=typename Top::manipulator_params,
          typename Category=typename container_chain_typebase<Top, TParams>::container_category>
class container_chain_impl
   : public container_chain_typebase<Top, TParams> {
   typedef container_chain_typebase<Top, TParams> base_t;
public:
   typedef container_chain_impl<Top, TParams> manipulator_impl;
   typedef TParams manipulator_params;
   typedef typename base_t::iterator iterator;
   typedef typename base_t::const_iterator const_iterator;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef container_chain_impl<FeatureCollector, TParams> type;
   };

   iterator begin()
   {
      return iterator(*this);
   }
   iterator end()
   {
      return iterator(*this, true);
   }
   const_iterator begin() const
   {
      return const_iterator(*this);
   }
   const_iterator end() const
   {
      return const_iterator(*this, true);
   }

   int size() const
   {
      return this->manip_top().get_container1().size() + this->manip_top().get_container2().size();
   }
   bool empty() const
   {
      return this->manip_top().get_container1().empty() && this->manip_top().get_container2().empty();
   }
   int dim() const
   {
      return get_dim(this->manip_top().get_container1()) + get_dim(this->manip_top().get_container2());
   }
};

template <typename Top, typename TParams>
class container_chain_impl<Top, TParams, forward_iterator_tag>
   : public container_chain_impl<Top, TParams, input_iterator_tag> {
   typedef container_chain_impl<Top, TParams, input_iterator_tag> base_t;
public:
   typename base_t::reference front()
   {
      if (this->manip_top().get_container1().empty())
         return this->manip_top().get_container2().front();
      return this->manip_top().get_container1().front();
   }
   typename base_t::const_reference front() const
   {
      if (this->manip_top().get_container1().empty())
         return this->manip_top().get_container2().front();
      return this->manip_top().get_container1().front();
   }
};

template <typename Top, typename TParams>
class container_chain_impl<Top, TParams, bidirectional_iterator_tag>
   : public container_chain_impl<Top, TParams, forward_iterator_tag> {
   typedef container_chain_impl<Top, TParams, forward_iterator_tag> base_t;
public:
   typedef iterator_chain<typename container_chain_traits<typename base_t::container_list, typename base_t::needed_features>::reverse_iterator_list, true>
      reverse_iterator;
   typedef iterator_chain<typename container_chain_traits<typename base_t::container_list, typename base_t::needed_features>::const_reverse_iterator_list, true>
      const_reverse_iterator;

   reverse_iterator rbegin()
   {
      return reverse_iterator(*this);
   }
   reverse_iterator rend()
   {
      return reverse_iterator(*this, true);
   }
   const_reverse_iterator rbegin() const
   {
      return const_reverse_iterator(*this);
   }
   const_reverse_iterator rend() const
   {
      return const_reverse_iterator(*this, true);
   }

   typename base_t::reference back()
   {
      if (this->manip_top().get_container2().empty())
         return this->manip_top().get_container1().back();
      return this->manip_top().get_container2().back();
   }
   typename base_t::const_reference back() const
   {
      if (this->manip_top().get_container2().empty())
         return this->manip_top().get_container1().back();
      return this->manip_top().get_container2().back();
   }
};

template <typename Top, typename TParams>
class container_chain_impl<Top, TParams, random_access_iterator_tag>
   : public container_chain_impl<Top, TParams, bidirectional_iterator_tag> {
   typedef container_chain_impl<Top, TParams, bidirectional_iterator_tag> base_t;
public:
   typename base_t::reference operator[] (int i)
   {
      const int d1=get_dim(this->manip_top().get_container1());
      if (i<d1) return this->manip_top().get_container1()[i];
      return this->manip_top().get_container2()[i-d1];
   }
   typename base_t::const_reference operator[] (int i) const
   {
      const int d1=get_dim(this->manip_top().get_container1());
      if (i<d1) return this->manip_top().get_container1()[i];
      return this->manip_top().get_container2()[i-d1];
   }
};

template <typename ContainerRef1, typename ContainerRef2>
class ContainerChain
   : public container_pair_base<ContainerRef1, ContainerRef2>,
     public container_chain_impl< ContainerChain<ContainerRef1,ContainerRef2>,
                                  mlist< Container1Tag<ContainerRef1>, Container2Tag<ContainerRef2> > > {
   typedef container_pair_base<ContainerRef1, ContainerRef2> base_t;
public:
   ContainerChain(typename base_t::first_arg_type src1_arg, typename base_t::second_arg_type src2_arg)
      : base_t(src1_arg,src2_arg) {}
};

template <typename ContainerRef1, typename ContainerRef2>
struct spec_object_traits< ContainerChain<ContainerRef1, ContainerRef2> >
   : spec_object_traits<is_container> {
   static const bool
      is_temporary=true,
      is_lazy = object_traits<typename deref<ContainerRef1>::type>::is_lazy || object_traits<typename deref<ContainerRef2>::type>::is_lazy,
      is_always_const = effectively_const<ContainerRef1>::value || effectively_const<ContainerRef2>::value;
};

template <typename ContainerRef1, typename ContainerRef2>
struct check_container_feature<ContainerChain<ContainerRef1, ContainerRef2>, sparse> {
   static const bool value=check_container_ref_feature<ContainerRef1, sparse>::value ||
                           check_container_ref_feature<ContainerRef2, sparse>::value;
};

template <typename ContainerRef1, typename ContainerRef2>
struct check_container_feature<ContainerChain<ContainerRef1, ContainerRef2>, pure_sparse> {
   static const bool value=check_container_ref_feature<ContainerRef1, pure_sparse>::value &&
                           check_container_ref_feature<ContainerRef2, pure_sparse>::value;
};

template <typename Container1, typename Container2> inline
ContainerChain<Container1&, Container2&>
concatenate(Container1& c1, Container2& c2)
{
   return ContainerChain<Container1&, Container2&> (c1,c2);
}

template <typename Container1, typename Container2> inline
ContainerChain<Container1&, const Container2&>
concatenate(Container1& c1, const Container2& c2)
{
   return ContainerChain<Container1&, const Container2&> (c1,c2);
}

template <typename Container1, typename Container2> inline
ContainerChain<const Container1&, Container2&>
concatenate(const Container1& c1, Container2& c2)
{
   return ContainerChain<const Container1&, Container2&> (c1,c2);
}

template <typename Container1, typename Container2> inline
ContainerChain<const Container1&, const Container2&>
concatenate(const Container1& c1, const Container2& c2)
{
   return ContainerChain<const Container1&, const Container2&> (c1,c2);
}

namespace operations {

template <typename LeftRef, typename RightRef,
          typename Discr=typename isomorphic_types<typename deref<LeftRef>::type, typename deref<RightRef>::type>::discriminant>
struct concat_impl;

template <typename LeftRef, typename RightRef>
struct concat : concat_impl<LeftRef,RightRef> {};

} // end namespace operations
} // end namespace pm

namespace polymake {
   using pm::concatenate;

   namespace operations {
      typedef BuildBinary<pm::operations::concat> concat;
   }
}

#endif // POLYMAKE_CONTAINER_CHAIN_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
