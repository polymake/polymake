/* Copyright (c) 1997-2017
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

/** @file Series.h
    @brief Declaration of pm::GenericSet class
 */

#ifndef POLYMAKE_SERIES_H
#define POLYMAKE_SERIES_H

#include "polymake/internal/comparators_ops.h"
#include "polymake/internal/shared_object.h"
#include "polymake/internal/Wary.h"
#include <cassert>

namespace pm {

template <typename E, typename Comparator=operations::cmp> class Set;
template <typename E, typename Comparator=operations::cmp> class PowerSet;

struct is_set;

template <typename E, typename Comparator, typename Etag=typename object_traits<E>::generic_tag>
struct persistent_set {
   typedef Set<E, Comparator> type;
};

template <typename Set, typename Comparator>
struct persistent_set<Set, Comparator, is_set> {
   typedef PowerSet<typename Set::element_type, Comparator> type;
};


/** @class GenericSet
    @brief @ref generic "Generic type" for \ref set_sec "ordered sets"

    This should belong to GenericSet.h, but Series must be derived from GenericSet.
    On the other hand, naked Series don't need any set-theoretical stuff defined there.
 */

template <typename TSet, typename E=typename TSet::element_type, typename Comparator=typename TSet::element_comparator>
class GenericSet
   : public Generic<TSet>
   , public operators::base {
protected:
   GenericSet() {}
   GenericSet(const GenericSet&) {}

public:
   /// element types
   typedef E element_type;
   /// functor type for comparing elements
   typedef Comparator element_comparator;

   static_assert(!std::is_same<Comparator, operations::cmp_unordered>::value, "comparator must define a total ordering");
   static_assert(!std::is_same<Comparator, operations::cmp>::value || is_ordered<E>::value, "elements must have a total ordering");

   typedef typename persistent_set<E,Comparator>::type persistent_type;
   /// @ref generic "generic type"
   typedef GenericSet generic_type;
   /// @ref generic "top type"
   typedef typename Generic<TSet>::top_type top_type;

   template <typename Result>
   struct rebind_generic {
      typedef GenericSet<Result, E, Comparator> type;
   };

   template <typename E1=E, typename E2=E1>
   struct rebind_comparator {
      typedef binary_op_builder<Comparator, const E1*, const E2*> builder;
      typedef typename builder::operation type;
   };

#if POLYMAKE_DEBUG
   void dump() const __attribute__((used));
#endif
};

template <typename Set, typename E, typename Comparator>
struct spec_object_traits< GenericSet<Set,E,Comparator> >
   : spec_or_model_traits<Set,is_container> {
   typedef is_set generic_tag;
   static const IO_separator_kind IO_separator=IO_sep_inherit;
};

/* --------
 *  Series
 * -------- */

template <typename E, bool _step_equal_1> class Series;

template <typename E, bool is_forward>
class sequence_iterator {
   template <typename,bool> friend class sequence_iterator;
public:
   typedef random_access_iterator_tag iterator_category;
   typedef E value_type;
   // TODO: check whether these classes are used with pointers at all; if not, remove the precautions here
   typedef typename std::conditional<std::is_pointer<E>::value, typename deref_ptr<E>::minus_const, E>::type mutable_value_type;
   typedef typename std::conditional<std::is_pointer<E>::value, typename deref_ptr<E>::plus_const, E>::type const_value_type;
   typedef const E* pointer;
   typedef E reference;
   typedef ptrdiff_t difference_type;
   typedef sequence_iterator<mutable_value_type, is_forward> iterator;
   typedef sequence_iterator<const_value_type, is_forward> const_iterator;
   typedef typename std::conditional<std::is_pointer<E>::value,
                                     const typename std::conditional<deref_ptr<E>::is_const, iterator, const_iterator>::type,
                                     type2type<sequence_iterator> >::type
      cmp_iterator;
protected:
   E cur;
public:
   sequence_iterator() {}
   sequence_iterator(typename function_argument<E>::type cur_arg) : cur(cur_arg) {}
   sequence_iterator(const iterator& it) : cur(it.cur) {}

   sequence_iterator& operator= (const iterator& it) { cur=it.cur; return *this; }

   reference operator* () const { return cur; }
   pointer operator-> () const { return &cur; }

   reference operator[] (int i) const { return is_forward ? cur+i : cur-i; }

   sequence_iterator& operator++ () { is_forward ? ++cur : --cur; return *this; }
   sequence_iterator& operator-- () { is_forward ? --cur : ++cur; return *this; }
   const sequence_iterator operator++ (int) { sequence_iterator copy=*this; operator++(); return copy; }
   const sequence_iterator operator-- (int) { sequence_iterator copy=*this; operator--(); return copy; }
   sequence_iterator& operator+= (int i) { is_forward ? (cur+=i) : (cur-=i); return *this; }
   sequence_iterator& operator-= (int i) { is_forward ? (cur-=i) : (cur+=i); return *this; }
   sequence_iterator operator+ (int i) const { return is_forward ? cur+i : cur-i; }
   sequence_iterator operator- (int i) const { return is_forward ? cur-i : cur+i; }
   friend sequence_iterator operator+ (int i, const sequence_iterator& it) { return it+i; }

   template <bool is_forward2>
   bool operator== (const sequence_iterator<E, is_forward2>& it) const { return cur==it.cur; }
   template <bool is_forward2>
   bool operator!= (const sequence_iterator<E, is_forward2>& it) const { return cur!=it.cur; }
   bool operator== (cmp_iterator& it) const { return cur==it.cur; }
   bool operator!= (cmp_iterator& it) const { return cur!=it.cur; }
   difference_type operator- (const sequence_iterator& it) const { return is_forward ? cur-it.cur : it.cur-cur; }
   difference_type operator- (cmp_iterator& it) const { return is_forward ? cur-it.cur : it.cur-cur; }
   bool operator< (const sequence_iterator& it) const { return is_forward ? cur<it.cur : it.cur<cur; }
   bool operator> (const sequence_iterator& it) const { return it < *this; }
   bool operator<= (const sequence_iterator& it) const { return !(it < *this); }
   bool operator>= (const sequence_iterator& it) const { return !(*this < it); }
   bool operator< (cmp_iterator& it) const { return is_forward ? cur<it.cur : it.cur<cur; }
   bool operator> (cmp_iterator& it) const { return it < *this; }
   bool operator<= (cmp_iterator& it) const { return !(it < *this); }
   bool operator>= (cmp_iterator& it) const { return !(*this < it); }
};

template <typename E, bool is_forward>
class series_iterator : public sequence_iterator<E, is_forward> {
   typedef sequence_iterator<E, is_forward> super;
   template <typename, bool> friend class series_iterator;
protected:
   typedef typename std::conditional<std::is_pointer<E>::value, ptrdiff_t, E>::type step_type;
   step_type _step;
public:
   typedef series_iterator<typename super::mutable_value_type, is_forward> iterator;
   typedef series_iterator<typename super::const_value_type, is_forward> const_iterator;
   typedef typename std::conditional<std::is_pointer<E>::value,
                                     const typename std::conditional<deref_ptr<E>::is_const, iterator, const_iterator>::type,
                                     type2type<series_iterator> >::type
      cmp_iterator;

   series_iterator() {}
   series_iterator(typename function_argument<E>::type cur_arg, typename function_argument<step_type>::type step_arg)
      : super(cur_arg), _step(step_arg) {}
   series_iterator(const iterator& it) : super(it), _step(it._step) {}

   series_iterator& operator= (const iterator& it) { super::operator=(it); _step=it._step; return *this; }

   typename super::reference operator[] (int i) const { return is_forward ? this->cur+i*_step : this->cur-i*_step; }

   step_type step() const { return _step; }

   series_iterator& operator++ () { is_forward ? (this->cur+=_step) : (this->cur-=_step); return *this; }
   series_iterator& operator-- () { is_forward ? (this->cur-=_step) : (this->cur+=_step); return *this; }
   const series_iterator operator++ (int) { series_iterator copy=*this; operator++(); return copy; }
   const series_iterator operator-- (int) { series_iterator copy=*this; operator--(); return copy; }
   series_iterator& operator+= (int i) { is_forward ? (this->cur+=i*_step) : (this->cur-=i*_step); return *this; }
   series_iterator& operator-= (int i) { is_forward ? (this->cur-=i*_step) : (this->cur+=i*_step); return *this; }
   series_iterator operator+ (int i) const { return series_iterator(is_forward ? this->cur+i*_step : this->cur-i*_step, _step); }
   series_iterator operator- (int i) const { return series_iterator(is_forward ? this->cur-i*_step : this->cur+i*_step, _step); }
   friend series_iterator operator+ (int i, const series_iterator& it) { return it+i; }
   ptrdiff_t operator- (const super& it) const { return (is_forward ? this->cur-*it : *it-this->cur)/_step; }
   bool operator< (const series_iterator& it) const { return is_forward ^ (_step<0) ? this->cur<it.cur : it.cur<this->cur; }
   bool operator> (const series_iterator& it) const { return it < *this; }
   bool operator<= (const series_iterator& it) const { return !(it < *this); }
   bool operator>= (const series_iterator& it) const { return !(*this < it); }
   bool operator< (cmp_iterator& it) const { return is_forward ^ (_step<0) ? this->cur<it.cur : it.cur<this->cur; }
   bool operator> (cmp_iterator& it) const { return it < *this; }
   bool operator<= (cmp_iterator& it) const { return !(it < *this); }
   bool operator>= (cmp_iterator& it) const { return !(*this < it); }
};

template <typename E, bool is_forward>
struct accompanying_iterator< series_iterator<E, is_forward> > {
   typedef sequence_iterator<E, is_forward> type;

   static void assign(series_iterator<E, is_forward>& it, const type& other)
   {
      static_cast<type&>(it)=other;
   }

   static void advance(sequence_iterator<E, is_forward>& it, const series_iterator<E, is_forward>& other, int n)
   {
      it+=n*other.step();
   }
};

template <typename E>
class count_down_iterator : public sequence_iterator<E,false> {
public:
   typedef count_down_iterator iterator;
   typedef count_down_iterator const_iterator;

   count_down_iterator() {}
   count_down_iterator(E cur_arg) : sequence_iterator<E,false>(cur_arg) {}
   bool at_end() const { return !this->cur; }
};

template <typename E>
struct check_iterator_feature<count_down_iterator<E>, end_sensitive> : std::true_type {};

template <typename E>
class Series<E,true> : public GenericSet<Series<E,true>, E, operations::cmp> {
protected:
   E _start;
   int _size;
public:
   static const bool step_equal_1=true;
   typedef E value_type;
   typedef E reference;
   typedef E const_reference;
   typedef typename std::conditional<std::is_pointer<E>::value, ptrdiff_t, E>::type step_type;

   Series() : _start(0), _size(0) {}

   explicit Series(typename function_argument<E>::type start_arg, int size_arg=1)
      : _start(start_arg), _size(size_arg)
   {
      assert(size_arg>=0);
   }

   // for the sake of interchangeability with Series<E,false>
   Series(typename function_argument<E>::type start_arg, int size_arg,
          typename function_argument<step_type>::type /*dummy_step*/)
      : _start(start_arg), _size(size_arg)
   {
      assert(size_arg>=0);
   }

   int size() const { return _size; }
   bool empty() const { return !_size; }

   step_type step() const { return step_type(1); }

   typedef sequence_iterator<E,true> iterator;
   typedef iterator const_iterator;
   typedef sequence_iterator<E,false> reverse_iterator;
   typedef reverse_iterator const_reverse_iterator;

   iterator begin() const { return _start; }
   iterator end() const { return _start+_size; }
   reverse_iterator rbegin() const { return _start+_size-1; }
   reverse_iterator rend() const { return _start-1; }

   reference front() const { return _start; }
   reference back() const { return _start+_size-1; }

   reference operator[] (int i) const
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=_size)
            throw std::runtime_error("Series::operator[] - index out of range");
      }
      return _start+i;
   }

   bool contains(typename function_argument<const_reference>::type k) const
   {
      return k>=_start && k<_start+_size;
   }
};

template <typename E, bool _step_equal_1>
class Series : public GenericSet<Series<E,_step_equal_1>, E, operations::cmp> {
public:
   typedef E value_type;
   typedef E reference;
   typedef E const_reference;
   typedef typename std::conditional<std::is_pointer<E>::value, ptrdiff_t, E>::type step_type;
   static const bool step_equal_1=false;
protected:
   E _start;
   int _size;
   step_type _step;
public:
   Series() : _start(0), _size(0), _step(0) {}

   Series(typename function_argument<E>::type start_arg, int size_arg,
          typename function_argument<step_type>::type step_arg)
      : _start(start_arg), _size(size_arg), _step(step_arg)
   {
      assert(size_arg>=0);
   }

   int size() const { return _size; }
   bool empty() const { return !_size; }

   step_type step() const { return _step; }

   typedef series_iterator<E, true> iterator;
   typedef iterator const_iterator;
   typedef series_iterator<E, false> reverse_iterator;
   typedef reverse_iterator const_reverse_iterator;

   iterator begin() const { return const_iterator(_start, _step); }
   iterator end() const { return const_iterator(_start+_size*_step, _step); }
   reverse_iterator rbegin() const { return const_reverse_iterator(_start+(_size-1)*_step, _step); }
   reverse_iterator rend() const { return const_reverse_iterator(_start-_step, _step); }

   reference front() const { return _start; }
   reference back() const { return _start+(_size-1)*_step; }

   reference operator[] (int i) const
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=_size)
            throw std::runtime_error("Series::operator[] - index out of range");
      }
      return _start+i*_step;
   }

   bool contains(typename function_argument<const_reference>::type k) const
   {
      return k>=_start && k<_start+_size*_step && !((k-_start)%_step);
   }
};

template <typename E, bool _step_equal_1>
struct spec_object_traits< Series<E,_step_equal_1> > : spec_object_traits<is_container> {
   static const bool is_always_const=true;
};

// alias for an integer series
typedef Series<int,false> series;

// alias for an integer sequence
typedef Series<int,true> sequence;

// Create a sequence of all integral numbers between and including $start$ and $end$
template <typename E> inline
Series<E,true>
range(E start, E end)
{
   return Series<E,true>(start, int(end-start)+1);
}

template <typename E>
class CountDown {
protected:
   int _size;
public:
   typedef const E const_reference;
   typedef const_reference reference;
   typedef E value_type;

   explicit CountDown(int size_arg) : _size(size_arg) {}

   int size() const { return _size; }
   bool empty() const { return !_size; }

   typedef count_down_iterator<E> iterator;
   typedef iterator const_iterator;
   typedef typename Series<E,true>::iterator reverse_iterator;
   typedef reverse_iterator const_reverse_iterator;

   iterator begin() const { return _size; }
   iterator end() const { return 0; }
   reverse_iterator rbegin() const { return 1; }
   reverse_iterator rend() const { return _size+1; }

   E operator[] (int i) const
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=_size)
            throw std::runtime_error("CountDown::operator[] - index out of range");
      }
      return _size-i;
   }

   E front() const { return _size; }
   E back() const { return 1; }
};

typedef CountDown<int> count_down;

template <typename E>
struct spec_object_traits< CountDown<E> >
   : spec_object_traits<is_container> {};

template <typename Iterator, bool is_reverse=false>
class indexed_random_iterator
   : public Iterator {
protected:
   typedef Iterator base_t;
   typename accompanying_iterator<Iterator>::type begin;

   template <typename, bool> friend class indexed_random_iterator;
public:
   typedef indexed_random_iterator<typename iterator_traits<Iterator>::iterator, is_reverse>
      iterator;
   typedef indexed_random_iterator<typename iterator_traits<Iterator>::const_iterator, is_reverse>
      const_iterator;

   indexed_random_iterator() {}

   template <typename SourceIterator, typename enable=typename std::enable_if<is_const_compatible_with<SourceIterator, Iterator>::value>::type>
   indexed_random_iterator(const SourceIterator& cur_arg)
      : base_t(cur_arg)
      , begin(cur_arg) {}

   template <typename SourceIterator1, typename SourceIterator2,
             typename enable=typename std::enable_if<is_const_compatible_with<SourceIterator1, Iterator>::value &&
                                                     is_const_compatible_with<SourceIterator2, Iterator>::value>::type>
   indexed_random_iterator(const SourceIterator1& cur_arg, const SourceIterator2& begin_arg)
      : base_t(cur_arg)
      , begin(begin_arg) {}

   indexed_random_iterator(const iterator& it)
      : base_t(static_cast<const typename iterator::base_t&>(it))
      , begin(it.begin) {}

   indexed_random_iterator& operator= (const iterator& it)
   {
      static_cast<base_t&>(*this)=it;
      begin=it.begin;
      return *this;
   }

   template <typename SourceIterator, typename enable=typename std::enable_if<is_const_compatible_with<SourceIterator, Iterator>::value>::type>
   indexed_random_iterator& operator= (const SourceIterator& cur)
   {
      static_cast<base_t&>(*this)=cur;
      return *this;
   }

   indexed_random_iterator operator+ (int i) const
   {
      return static_cast<const base_t&>(*this)+i;
   }
   indexed_random_iterator operator- (int i) const
   {
      return static_cast<const base_t&>(*this)-i;
   }
   friend indexed_random_iterator operator+ (int i, const indexed_random_iterator& me)
   {
      return me+i;
   }

   template <typename Other>
   typename std::enable_if<is_among<Other, iterator, const_iterator>::value, typename base_t::difference_type>::type
   operator- (const Other& it) const
   {
      return static_cast<const base_t&>(*this)-it;
   }

   int index() const
   {
      return is_reverse ? begin-static_cast<const base_t&>(*this)-1 : static_cast<const base_t&>(*this)-begin;
   }

private:
   void contract1_impl(int distance_front, int, std::false_type)
   {
      static_cast<base_t&>(*this)+=distance_front;
   }
   void contract1_impl(int distance_front, int distance_back, std::true_type)
   {
      base_t::contract(false, distance_front, distance_back);
   }
public:
   void contract(bool renumber, int distance_front, int distance_back=0)
   {
      contract1_impl(distance_front, distance_back, bool_constant<check_iterator_feature<base_t, contractable>::value>());
      if (renumber)
         accompanying_iterator<Iterator>::advance(begin, static_cast<const base_t&>(*this), is_reverse ? distance_back : distance_front);
   }
};

template <typename Iterator, bool is_reverse, typename Feature>
struct check_iterator_feature<indexed_random_iterator<Iterator, is_reverse>, Feature>
   : check_iterator_feature<Iterator,Feature> {};

template <typename Iterator, bool is_reverse>
struct check_iterator_feature<indexed_random_iterator<Iterator, is_reverse>, indexed> : std::true_type {};

template <typename Iterator, bool is_reverse>
struct check_iterator_feature<indexed_random_iterator<Iterator, is_reverse>, contractable> : std::true_type {};

template <typename Iterator, bool is_reverse>
struct accompanying_iterator< indexed_random_iterator<Iterator, is_reverse> >
   : accompanying_iterator<Iterator> {};

template <> struct feature_allow_order<rewindable, indexed> : std::false_type {};
template <> struct feature_allow_order<end_sensitive, indexed> : std::false_type {};

template <typename Subset, typename Source,
          typename source_generic=typename object_traits<Source>::generic_type>
class generic_of_subset {};

template <typename Subset, typename Source, typename Set, typename E, typename Comparator>
class generic_of_subset<Subset, Source, GenericSet<Set,E,Comparator> >
   : public GenericSet<Subset,E,Comparator> {};

template <typename Subsets, typename Source,
          typename source_generic=typename object_traits<Source>::generic_type>
class generic_of_subsets {
public:
   typedef operations::cmp subset_element_comparator;
};

template <typename Subsets, typename Source, typename Set>
class generic_of_subsets<Subsets, Source, GenericSet<Set> >
   : public GenericSet<Subsets, typename object_traits<Source>::persistent_type, operations::cmp> {
public:
   typedef typename Source::element_comparator subset_element_comparator;
};

} // end namespace pm

namespace polymake {
   using pm::Series;
   using pm::series;
   using pm::sequence;
   using pm::range;
}

namespace std {

// TODO: remove this when `alias' starts using perfect forwarding for construction of contained objects
template <typename E, bool step_equal_1>
struct is_pod< pm::Series<E, step_equal_1> > : is_pod<E> {};

}

#endif // POLYMAKE_SERIES_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
