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

#ifndef POLYMAKE_INTERNAL_SERIESRAW_H
#define POLYMAKE_INTERNAL_SERIESRAW_H

#include "polymake/internal/comparators_ops.h"
#include <stdexcept>

namespace pm {

/* --------
 *  Series
 * -------- */

template <typename E, bool is_forward>
class sequence_iterator {
   template <typename,bool> friend class sequence_iterator;
public:
   static_assert(!std::is_pointer<E>::value, "must be a numeric type");
   using iterator_category = random_access_iterator_tag;
   using value_type = E;
   using pointer = const E*;
   using reference = E;
   using difference_type = ptrdiff_t;
   using iterator = sequence_iterator;
   using const_iterator = sequence_iterator;
protected:
   E cur;
public:
   sequence_iterator() {}
   sequence_iterator(const E& cur_arg) : cur(cur_arg) {}
   sequence_iterator(E&& cur_arg) : cur(std::move(cur_arg)) {}
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
   difference_type operator- (const sequence_iterator& it) const { return is_forward ? cur-it.cur : it.cur-cur; }
   bool operator< (const sequence_iterator& it) const { return is_forward ? cur<it.cur : it.cur<cur; }
   bool operator> (const sequence_iterator& it) const { return it < *this; }
   bool operator<= (const sequence_iterator& it) const { return !(it < *this); }
   bool operator>= (const sequence_iterator& it) const { return !(*this < it); }
};

template <typename E, bool is_forward>
class series_iterator : public sequence_iterator<E, is_forward> {
   using base_t = sequence_iterator<E, is_forward>;
   template <typename, bool> friend class series_iterator;
protected:
   E step_;
public:
   using iterator = series_iterator;
   using const_iterator = series_iterator;

   series_iterator() {}
   series_iterator(const E& cur_arg, const E& step_arg)
      : base_t(cur_arg), step_(step_arg) {}
   series_iterator(const iterator& it) : base_t(it), step_(it.step_) {}

   series_iterator& operator= (const iterator& it) { base_t::operator=(it); step_=it.step_; return *this; }

   typename base_t::reference operator[] (int i) const { return is_forward ? this->cur+i*step_ : this->cur-i*step_; }

   const E& step() const { return step_; }

   series_iterator& operator++ () { is_forward ? (this->cur+=step_) : (this->cur-=step_); return *this; }
   series_iterator& operator-- () { is_forward ? (this->cur-=step_) : (this->cur+=step_); return *this; }
   const series_iterator operator++ (int) { series_iterator copy=*this; operator++(); return copy; }
   const series_iterator operator-- (int) { series_iterator copy=*this; operator--(); return copy; }
   series_iterator& operator+= (int i) { is_forward ? (this->cur+=i*step_) : (this->cur-=i*step_); return *this; }
   series_iterator& operator-= (int i) { is_forward ? (this->cur-=i*step_) : (this->cur+=i*step_); return *this; }
   series_iterator operator+ (int i) const { return series_iterator(is_forward ? this->cur+i*step_ : this->cur-i*step_, step_); }
   series_iterator operator- (int i) const { return series_iterator(is_forward ? this->cur-i*step_ : this->cur+i*step_, step_); }
   friend series_iterator operator+ (int i, const series_iterator& it) { return it+i; }
   ptrdiff_t operator- (const base_t& it) const { return (is_forward ? this->cur-*it : *it-this->cur)/step_; }
   bool operator< (const series_iterator& it) const { return is_forward ^ (step_<0) ? this->cur<it.cur : it.cur<this->cur; }
   bool operator> (const series_iterator& it) const { return it < *this; }
   bool operator<= (const series_iterator& it) const { return !(it < *this); }
   bool operator>= (const series_iterator& it) const { return !(*this < it); }
};

template <typename E>
class count_down_iterator : public sequence_iterator<E,false> {
public:
   using iterator = count_down_iterator;
   using const_iterator = count_down_iterator;

   count_down_iterator() {}
   count_down_iterator(const E& cur_arg) : sequence_iterator<E,false>(cur_arg) {}
   bool at_end() const { return is_zero(this->cur); }
};

template <typename E>
struct check_iterator_feature<count_down_iterator<E>, end_sensitive> : std::true_type {};

template <typename E, bool step_equal_1> class SeriesRaw;

template <typename E>
class SeriesRaw<E, true> {
protected:
   E start_;
   int size_;
public:
   using value_type = E;
   using reference = E;
   using const_reference = E;

   SeriesRaw() : start_(), size_(0) {}

   explicit SeriesRaw(const E& start_arg, int size_arg=1, E=E())
      : start_(start_arg), size_(size_arg)
   {
      if (POLYMAKE_DEBUG && size_arg<0)
         throw std::runtime_error("Series - wrong size");
   }

   int size() const { return size_; }
   bool empty() const { return size_==0; }

   const E& step() const { return one_value<E>(); }

   using iterator = sequence_iterator<E, true>;
   using const_iterator = iterator;
   using reverse_iterator = sequence_iterator<E, false>;
   using const_reverse_iterator = reverse_iterator;

   iterator begin() const { return start_; }
   iterator end() const { return start_+size_; }
   reverse_iterator rbegin() const { return start_+size_-1; }
   reverse_iterator rend() const { return start_-1; }

   reference front() const { return start_; }
   reference back() const { return start_+size_-1; }

   reference operator[] (int i) const
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=size_)
            throw std::runtime_error("Series::operator[] - index out of range");
      }
      return start_+i;
   }

   bool contains(const E& k) const
   {
      return k>=front() && k<back();
   }
};

using sequence_raw = SeriesRaw<int, true>;

template <typename E, bool step_equal_1>
class SeriesRaw {
public:
   using value_type = E;
   using reference = E;
   using const_reference = E;
protected:
   E start_;
   E step_;
   int size_;
public:
   SeriesRaw() : start_(), step_(), size_(0) {}

   SeriesRaw(const E& start_arg, int size_arg, const E& step_arg)
      : start_(start_arg), step_(step_arg), size_(size_arg)
   {
      if (POLYMAKE_DEBUG && size_arg<0)
         throw std::runtime_error("Series - wrong size");
   }

   int size() const { return size_; }
   bool empty() const { return size_==0; }

   const E& step() const { return step_; }

   using iterator = series_iterator<E, true>;
   using const_iterator = iterator;
   using reverse_iterator = series_iterator<E, false>;
   using const_reverse_iterator = reverse_iterator;

   iterator begin() const { return iterator(start_, step_); }
   iterator end() const { return iterator(start_+size_*step_, step_); }
   reverse_iterator rbegin() const { return reverse_iterator(start_+(size_-1)*step_, step_); }
   reverse_iterator rend() const { return reverse_iterator(start_-step_, step_); }

   reference front() const { return start_; }
   reference back() const { return start_+(size_-1)*step_; }

   reference operator[] (int i) const
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=size_)
            throw std::runtime_error("Series::operator[] - index out of range");
      }
      return start_+i*step_;
   }

   bool contains(const E& k) const
   {
      return k>=front() && k<back() && (k-front()) % step_ == 0;
   }
};

template <typename E, bool step_equal_1>
struct spec_object_traits< SeriesRaw<E, step_equal_1> > : spec_object_traits<is_container> {
   static const bool is_always_const=true;
};

template <typename E>
class CountDown {
protected:
   int size_;
public:
   using const_reference = const E;
   using reference = const_reference;
   using value_type = E;

   explicit CountDown(int size_arg) : size_(size_arg) {}

   int size() const { return size_; }
   bool empty() const { return size_==0; }

   using iterator = count_down_iterator<E>;
   using const_iterator = iterator;
   using reverse_iterator = typename SeriesRaw<E, true>::iterator;
   using const_reverse_iterator = reverse_iterator;

   iterator begin() const { return size_; }
   iterator end() const { return 0; }
   reverse_iterator rbegin() const { return 1; }
   reverse_iterator rend() const { return size_+1; }

   E operator[] (int i) const
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=size_)
            throw std::runtime_error("CountDown::operator[] - index out of range");
      }
      return size_-i;
   }

   E front() const { return size_; }
   E back() const { return 1; }
};

using  count_down = CountDown<int>;

template <typename E>
struct spec_object_traits< CountDown<E> >
   : spec_object_traits<is_container> {};

template <typename Iterator, bool is_reverse=false>
class indexed_random_iterator
   : public Iterator {
protected:
   using base_t = Iterator;
   typename accompanying_iterator<Iterator>::type begin;

   template <typename, bool> friend class indexed_random_iterator;
public:
   using iterator = indexed_random_iterator<typename iterator_traits<Iterator>::iterator, is_reverse>;
   using const_iterator = indexed_random_iterator<typename iterator_traits<Iterator>::const_iterator, is_reverse>;

   indexed_random_iterator() = default;

   template <typename SourceIterator, typename=std::enable_if_t<is_const_compatible_with<SourceIterator, Iterator>::value>>
   indexed_random_iterator(const SourceIterator& cur_arg)
      : base_t(cur_arg)
      , begin(cur_arg) {}

   template <typename SourceIterator1, typename SourceIterator2,
             typename=std::enable_if_t<is_const_compatible_with<SourceIterator1, Iterator>::value &&
                                       is_const_compatible_with<SourceIterator2, Iterator>::value>>
   indexed_random_iterator(SourceIterator1&& cur_arg, SourceIterator2&& begin_arg)
      : base_t(std::forward<SourceIterator1>(cur_arg))
      , begin(std::forward<SourceIterator2>(begin_arg)) {}

   indexed_random_iterator(const iterator& it)
      : base_t(static_cast<const typename iterator::base_t&>(it))
      , begin(it.begin) {}

   indexed_random_iterator& operator= (const iterator& it)
   {
      static_cast<base_t&>(*this)=it;
      begin=it.begin;
      return *this;
   }

   template <typename SourceIterator, typename=std::enable_if_t<is_const_compatible_with<SourceIterator, Iterator>::value>>
   indexed_random_iterator& operator= (SourceIterator&& cur)
   {
      static_cast<base_t&>(*this)=std::forward<SourceIterator>(cur);
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

/// generic tag for sets
struct is_set;

} // end namespace pm

#endif // POLYMAKE_INTERNAL_SERIESRAW_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
