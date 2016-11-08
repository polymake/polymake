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

#ifndef POLYMAKE_INTERNAL_ITERATORS_H
#define POLYMAKE_INTERNAL_ITERATORS_H

#include "polymake/internal/type_manip.h"
#include "polymake/internal/converters_basic_defs.h"
#include "polymake/pair.h"
#include "polymake/meta_list.h"

#include <functional>
#include <iterator>
#include <stdexcept>
#include <initializer_list>

namespace std {

/** Specialization for inserters.
    Although an XXX_insert_iterator can't be dereferenced,
    it's useful to know in some algorithms what for data it accepts.
    The standard iterator_traits would not provide this information.
*/
template <typename Container>
struct iterator_traits< back_insert_iterator<Container> > {
  typedef output_iterator_tag iterator_category;
  typedef typename Container::value_type value_type;
  typedef void difference_type;
  typedef void pointer;
  typedef void reference;
};

template <typename Container>
struct iterator_traits< front_insert_iterator<Container> > {
  typedef output_iterator_tag iterator_category;
  typedef typename Container::value_type value_type;
  typedef void difference_type;
  typedef void pointer;
  typedef void reference;
};

template <typename Container>
struct iterator_traits< insert_iterator<Container> > {
  typedef output_iterator_tag iterator_category;
  typedef typename Container::value_type value_type;
  typedef void difference_type;
  typedef void pointer;
  typedef void reference;
};
} // end namespace std

namespace pm {

using std::input_iterator_tag;
using std::output_iterator_tag;
using std::forward_iterator_tag;
using std::bidirectional_iterator_tag;
using std::random_access_iterator_tag;

template <typename Iterator,
          bool _seems_persistent=
             is_derived_from<typename std::iterator_traits<Iterator>::iterator_category, forward_iterator_tag>::value>
struct iterator_cross_const_helper {
   typedef typename Iterator::iterator iterator;
   typedef typename Iterator::const_iterator const_iterator;
};

template <typename Iterator>
struct iterator_cross_const_helper<Iterator, false> {
   typedef Iterator iterator;
   typedef Iterator const_iterator;
};

template <typename T>
struct iterator_cross_const_helper<T*, true> {
   typedef typename attrib<T>::minus_const* iterator;
   typedef typename attrib<T>::plus_const* const_iterator;
};

template <typename Iterator>
struct iterator_category_booleans {
   static const bool
      is_forward=
         is_derived_from<typename std::iterator_traits<Iterator>::iterator_category, forward_iterator_tag>::value,
      is_bidirectional=
         is_derived_from<typename std::iterator_traits<Iterator>::iterator_category, bidirectional_iterator_tag>::value,
      is_random=
         is_derived_from<typename std::iterator_traits<Iterator>::iterator_category, random_access_iterator_tag>::value;
};

template <typename Iterator>
struct iterator_traits
   : public std::iterator_traits<pure_type_t<Iterator>>
   , public iterator_cross_const_helper<pure_type_t<Iterator>>
   , public iterator_category_booleans<pure_type_t<Iterator>> {};

template <typename Iterator, bool _is_rev=iterator_category_booleans<Iterator>::is_bidirectional>
struct default_iterator_reversed {
   typedef void type;
};

template <typename Iterator>
struct default_iterator_reversed<Iterator, true> {
   typedef std::reverse_iterator<Iterator> type;
   static Iterator reverse(const type& it) { return it.base(); }
};

template <typename Iterator>
struct iterator_reversed : default_iterator_reversed<Iterator> {};

template <typename Iterator>
struct iterator_reversed< std::reverse_iterator<Iterator> > {
   typedef Iterator type;
   static std::reverse_iterator<Iterator> reverse(const type& it) { return std::reverse_iterator<Iterator>(it); }
};

template <typename Iterator1, typename Iterator2>
struct iterator_pair_traits {
   static const bool
      is_forward=
         iterator_traits<Iterator1>::is_forward && iterator_traits<Iterator2>::is_forward,
      is_bidirectional=
         iterator_traits<Iterator1>::is_bidirectional && iterator_traits<Iterator2>::is_bidirectional,
      is_random=
         iterator_traits<Iterator1>::is_random && iterator_traits<Iterator2>::is_random;
};

template <typename Iterator>
struct iterator_cross_const_helper<std::reverse_iterator<Iterator>, true> {
   typedef std::reverse_iterator<typename iterator_cross_const_helper<Iterator>::iterator> iterator;
   typedef std::reverse_iterator<typename iterator_cross_const_helper<Iterator>::const_iterator> const_iterator;
};

template <typename Iterator>
struct const_compatible_with
   : mlist_remove_duplicates< mlist<Iterator, typename iterator_traits<Iterator>::iterator> > {};

template <typename Source, typename Iterator>
struct is_const_compatible_with
   : is_among<pure_type_t<Source>, typename const_compatible_with<Iterator>::type> {};

#if defined(__GLIBCXX__)
template <typename Iterator, typename Container>
struct iterator_cross_const_helper<__gnu_cxx::__normal_iterator<Iterator, Container>, true> {
   typedef __gnu_cxx::__normal_iterator<typename iterator_cross_const_helper<Iterator>::iterator, Container> iterator;
   typedef __gnu_cxx::__normal_iterator<typename iterator_cross_const_helper<Iterator>::const_iterator, Container> const_iterator;
};
#elif defined(_LIBCPP_VERSION)

template <typename Iterator>
struct iterator_cross_const_helper<std::__wrap_iter<Iterator>, true> {
   typedef std::__wrap_iter<typename iterator_cross_const_helper<Iterator>::iterator> iterator;
   typedef std::__wrap_iter<typename iterator_cross_const_helper<Iterator>::const_iterator> const_iterator;
};

#endif

} // end namespace pm

#if defined(__GLIBCXX__)
namespace std {
   struct _Bit_iterator;
   struct _Bit_const_iterator;
}

namespace pm {
template <>
struct iterator_cross_const_helper<std::_Bit_iterator, true> {
   typedef std::_Bit_iterator iterator;
   typedef std::_Bit_const_iterator const_iterator;
};

template <>
struct iterator_cross_const_helper<std::_Bit_const_iterator, true> {
   typedef std::_Bit_iterator iterator;
   typedef std::_Bit_const_iterator const_iterator;
};
} // end namespace pm
#endif

namespace pm {

template <typename Operation>
struct operation_cross_const_helper {
   typedef Operation operation;
   typedef Operation const_operation;
};

template <typename T>
class black_hole {
public:
   typedef output_iterator_tag iterator_category;
   typedef T value_type;
   typedef void reference;
   typedef void pointer;
   typedef void difference_type;

   black_hole& operator++ () { return *this; }
   black_hole& operator++ (int) { return *this; }
   black_hole& operator* () { return *this; }
   black_hole& operator= (typename function_argument<T>::type) { return *this; }
};

template <typename T, typename Counter>
class counting_black_hole : public black_hole<T> {
public:
   counting_black_hole() {}
   counting_black_hole(Counter *counter_arg) : counter(counter_arg) {}

   black_hole<T>& operator++ () { ++(*counter); return *this; }
   black_hole<T>& operator++ (int) { ++(*counter); return *this; }
protected:
   Counter* counter;
};

template <typename Container>
class insert_iterator {
protected:
   Container* container;
public:
   typedef output_iterator_tag iterator_category;
   typedef typename Container::value_type value_type;
   typedef void pointer;
   typedef void reference;
   typedef void difference_type;

   insert_iterator(Container& container_arg) : container(&container_arg) {}

   insert_iterator& operator= (typename function_argument<typename Container::value_type>::type x)
   {
      container->insert(x);
      return *this;
   }

   insert_iterator& operator* () { return *this; }
   insert_iterator& operator++ () { return *this; }
   insert_iterator& operator++ (int) { return *this; }
};

template <typename Container> inline
insert_iterator<Container> inserter(Container& c) { return c; }

struct end_sensitive {};
struct contractable {};
struct rewindable {};
struct indexed {};
struct dense {};
struct sparse_compatible : end_sensitive, indexed {};
struct sparse : sparse_compatible {};
struct pure_sparse : sparse {};

/** Feature of an iterator
    Is true if the data sequence doesn't have any natural limit whatever kind
*/
struct unlimited {};

template <typename Iterator, typename Feature>
struct default_check_iterator_feature : std::is_same<Feature, void> {};

template <typename Iterator>
struct default_check_iterator_feature<Iterator, unlimited> {
   static const bool value=!iterator_traits<Iterator>::is_forward;
};

template <typename Iterator, typename Feature>
struct check_iterator_feature
   : default_check_iterator_feature<Iterator, Feature> {};

template <typename Iterator, typename Feature>
struct check_iterator_feature<Iterator&, Feature>
   : check_iterator_feature<pure_type_t<Iterator>, Feature> {};

template <typename Feature_before, typename Feature_after>
struct feature_allow_order : std::true_type {};

template <typename Feature, bool on_top=true> struct provide_construction {};

template <typename Feature_before, bool on_top, typename Feature_after>
struct feature_allow_order< provide_construction<Feature_before, on_top>, Feature_after >
   : feature_allow_order<Feature_before, Feature_after> {};

template <typename Feature_before, typename Feature_after, bool on_top>
struct feature_allow_order< Feature_before, provide_construction<Feature_after, on_top> >
   : feature_allow_order<Feature_before, Feature_after> {};

template <typename Feature_before, bool on_top_before, typename Feature_after, bool on_top_after>
struct feature_allow_order< provide_construction<Feature_before, on_top_before>, provide_construction<Feature_after, on_top_after> >
   : feature_allow_order<Feature_before, Feature_after> {};

template <typename Feature1, typename Feature2>
struct absorbing_feature : is_derived_from<Feature1, Feature2> {};

template <typename Feature1, bool on_top1, typename Feature2>
struct absorbing_feature< provide_construction<Feature1, on_top1>, Feature2>
   : absorbing_feature<Feature1, Feature2> {};

template <typename Feature1, bool on_top1, typename Feature2, bool on_top2>
struct absorbing_feature< provide_construction<Feature1, on_top1>, provide_construction<Feature2, on_top2> > {
   static const bool value= on_top1>=on_top2 && is_derived_from<Feature1, Feature2>::value;
};

template <typename Feature1, typename Feature2>
struct equivalent_features : std::false_type {
   typedef void type;
};

template <typename Feature>
struct equivalent_features<Feature, Feature> : std::true_type {
   typedef Feature type;
};

template <typename Feature, bool on_top>
struct equivalent_features< provide_construction<Feature,on_top>, Feature > : std::true_type {
   typedef provide_construction<Feature,on_top> type;
};

template <typename Feature, bool on_top>
struct equivalent_features< Feature, provide_construction<Feature,on_top> > : std::true_type {
   typedef provide_construction<Feature> type;
};

template <typename Iterator>
struct accompanying_iterator {
   typedef Iterator type;

   static void assign(type& it, const type& other) { it=other;}

   static void advance(type& it, const type&, int n) { std::advance(it, n); }
};

template <typename Iterator>
class rewindable_iterator : public Iterator {
protected:
   typedef Iterator base_t;
   typename accompanying_iterator<Iterator>::type begin;

   template <typename> friend class rewindable_iterator;
public:
   typedef rewindable_iterator<typename iterator_traits<Iterator>::iterator> iterator;
   typedef rewindable_iterator<typename iterator_traits<Iterator>::const_iterator> const_iterator;

   rewindable_iterator() {}

   template <typename SourceIterator, typename enabled=typename std::enable_if<is_const_compatible_with<SourceIterator, Iterator>::value>::type>
   rewindable_iterator(const SourceIterator& cur_arg)
      : base_t(cur_arg)
      , begin(cur_arg) {}

   rewindable_iterator(const iterator& it)
      : base_t(static_cast<const typename iterator::base_t&>(it))
      , begin(it.begin) {}

   rewindable_iterator& operator= (const iterator& it)
   {
      static_cast<base_t&>(*this)=it;
      begin=it.begin;
      return *this;
   }

   template <typename SourceIterator, typename enabled=typename std::enable_if<is_const_compatible_with<SourceIterator, Iterator>::value>::type>
   rewindable_iterator& operator= (const SourceIterator& cur)
   {
      static_cast<base_t&>(*this)=cur;
      return *this;
   }

   void rewind()
   {
      accompanying_iterator<Iterator>::assign(static_cast<base_t&>(*this), begin);
   }

private:
   void contract1(bool, int distance_front, int, std::false_type)
   {
      std::advance(static_cast<base_t&>(*this), distance_front);
   }
   void contract1(bool renumber, int distance_front, int distance_back, std::true_type)
   {
      base_t::contract(renumber, distance_front, distance_back);
   }
public:
   void contract(bool renumber, int distance_front, int distance_back=0)
   {
      contract1(renumber, distance_front, distance_back, bool_constant<check_iterator_feature<base_t, contractable>::value>());
      begin=static_cast<const base_t&>(*this);
   }
};

template <typename Iterator, typename Feature>
struct check_iterator_feature<rewindable_iterator<Iterator>, Feature>
   : check_iterator_feature<Iterator,Feature> {};

template <typename Iterator>
struct check_iterator_feature<rewindable_iterator<Iterator>, rewindable> : std::true_type {};

template <typename Iterator>
struct check_iterator_feature<rewindable_iterator<Iterator>, contractable> : std::true_type {};

template <typename Iterator>
struct accompanying_iterator< rewindable_iterator<Iterator> > : accompanying_iterator<Iterator> {};

template <typename Iterator>
class iterator_range
   : public Iterator {
protected:
   typedef Iterator base_t;
   typedef typename accompanying_iterator<Iterator>::type end_type;

   end_type end;
   template <typename> friend class iterator_range;
public:
   typedef iterator_range<typename iterator_traits<Iterator>::iterator> iterator;
   typedef iterator_range<typename iterator_traits<Iterator>::const_iterator> const_iterator;

   iterator_range() {}

   template <typename SourceIterator, typename enabled=typename std::enable_if<is_const_compatible_with<SourceIterator, Iterator>::value>::type>
   iterator_range(const SourceIterator& cur_arg)
      : base_t(cur_arg)
      , end(cur_arg) {}

   template <typename SourceIterator1, typename SourceIterator2,
             typename enabled=typename std::enable_if<is_const_compatible_with<SourceIterator1, Iterator>::value &&
                                                      is_derived_from_any<SourceIterator2, typename const_compatible_with<end_type>::type>::value>::type>
   iterator_range(const SourceIterator1& cur_arg, const SourceIterator2& end_arg)
      : base_t(cur_arg)
      , end(end_arg) {}

   iterator_range(const iterator& it)
      : base_t(static_cast<const typename iterator::base_t&>(it))
      , end(it.end) {}

   iterator_range& operator= (const iterator& it)
   {
      static_cast<base_t&>(*this)=it;
      end=it.end;
      return *this;
   }

   template <typename SourceIterator, typename enabled=typename std::enable_if<is_const_compatible_with<SourceIterator, Iterator>::value>::type>
   iterator_range& operator= (const SourceIterator& cur)
   {
      static_cast<base_t&>(*this)=cur;
      return *this;
   }

   bool at_end() const { return static_cast<const base_t&>(*this)==end; }

   iterator_range& operator++()
   {
      base_t::operator++(); return *this;
   }
   const iterator_range operator++ (int)
   {
      iterator_range copy=*this; operator++(); return copy;
   }

   iterator_range& operator--()
   {
      static_assert(iterator_traits<base_t>::is_bidirectional, "iterator is not bidirectional");
      base_t::operator--(); return *this;
   }
   const iterator_range operator-- (int)
   {
      iterator_range copy=*this; operator--(); return copy;
   }

   iterator_range& operator+= (int i)
   {
      static_assert(iterator_traits<base_t>::is_random, "iterator is not random-access");
      base_t::operator+=(i);
      return *this;
   }
   iterator_range& operator-= (int i)
   {
      static_assert(iterator_traits<base_t>::is_random, "iterator is not random-access");
      base_t::operator-=(i);
      return *this;
   }

   iterator_range operator+ (int i) const
   {
      static_assert(iterator_traits<base_t>::is_random, "iterator is not random-access");
      return iterator_range(static_cast<const base_t&>(*this)+i, end);
   }
   iterator_range operator- (int i) const
   {
      static_assert(iterator_traits<base_t>::is_random, "iterator is not random-access");
      return iterator_range(static_cast<const base_t&>(*this)-i, end);
   }
   friend iterator_range operator+ (int i, const iterator_range& me)
   {
      static_assert(iterator_traits<base_t>::is_random, "iterator is not random-access");
      return iterator_range(static_cast<const base_t&>(me)+i, me.end);
   }

   template <typename Other>
   typename std::enable_if<is_derived_from_any<Other, typename iterator::base_t, typename const_iterator::base_t>::value, typename base_t::difference_type>::type
   operator- (const Other& other) const
   {
      static_assert(iterator_traits<base_t>::is_random, "iterator is not random-access");
      typedef typename is_derived_from_any<Other, typename iterator::base_t, typename const_iterator::base_t>::type other_base_t;
      return static_cast<const base_t&>(*this) - static_cast<const other_base_t&>(other);
   }
private:
   void contract1_impl(bool, int distance_front, int, std::false_type)
   {
      std::advance(static_cast<base_t&>(*this), distance_front);
   }
   void contract1_impl(bool renumber, int distance_front, int distance_back, std::true_type)
   {
      base_t::contract(renumber, distance_front, distance_back);
   }
public:
   void contract(bool renumber, int distance_front, int distance_back=0)
   {
      contract1_impl(renumber, distance_front, distance_back, bool_constant<check_iterator_feature<base_t, contractable>::value>());
      accompanying_iterator<Iterator>::advance(end, static_cast<const base_t&>(*this), -distance_back);
   }
};

template <typename Iterator, typename Feature>
struct check_iterator_feature<iterator_range<Iterator>, Feature>
   : check_iterator_feature<Iterator,Feature> {};

template <typename Iterator>
struct check_iterator_feature<iterator_range<Iterator>, end_sensitive> : std::true_type {};

template <typename Iterator>
struct check_iterator_feature<iterator_range<Iterator>, contractable> : std::true_type {};

template <> struct feature_allow_order<end_sensitive, rewindable> : std::false_type {};

template <typename Iterator>
struct accompanying_iterator< iterator_range<Iterator> > : accompanying_iterator<Iterator> {};

template <typename Iterator>
class mimic_iterator_range {
   const Iterator& it;

   struct iterator : public Iterator {
      bool operator== (const iterator&) const { return this->at_end(); }
      bool operator!= (const iterator&) const { return !this->at_end(); }
   };
public:
   mimic_iterator_range(const Iterator& it_arg) : it(it_arg) {}

   template <typename Container>
   operator Container () const
   {
      return Container(static_cast<const iterator&>(it), static_cast<const iterator&>(it));
   }
};

template <typename Iterator> inline
mimic_iterator_range<Iterator>
as_iterator_range(const Iterator& it, typename std::enable_if<check_iterator_feature<Iterator, end_sensitive>::value, void**>::type=nullptr)
{
   return it;
}

struct manip_container_base {};

namespace object_classifier {
   enum { is_manip=is_scalar+1 };

   namespace _impl {
      size_discriminant<is_manip>::type analyzer_f(const manip_container_base*, bait*);
   }

   template <typename Container,
             bool iterator_preserved=std::is_same<typename Container::const_iterator,
                                                  typename Container::manipulator_impl::const_iterator>::value>
   struct check_begin_end {
      static const int value= iterator_preserved ? int(is_manip) : int(is_opaque);
   };

   template <typename Container>
   struct what_is<Container, is_manip> : check_begin_end<Container> {};

} // end namespace object_classifier

template <typename T>
struct spec_object_traits< cons<T, int_constant<object_classifier::is_manip> > >
   : spec_object_traits<is_container> {
   typedef typename deref<typename T::hidden_type>::type masquerade_for;
};

template <typename Container, typename ProvidedFeatures> class manip_feature_collector;

template <typename Container, bool is_const>
struct default_container_elem_traits {
   typedef typename Container::const_reference const_reference;
   typedef typename std::conditional<is_const, const_reference, typename Container::reference>::type reference;
   typedef typename Container::value_type value_type;
};

DeclTypedefCHECK(container_category);
DeclTypedefCHECK(iterator);
DeclTypedefCHECK(value_type);
DeclTypedefCHECK(key_type);
DeclTypedefCHECK(mapped_type);
DeclTypedefCHECK(iterator_category);
DeclTypedefCHECK(difference_type);

template <typename Iterator>
struct looks_like_iterator {
   typedef pure_type_t<Iterator> candidate;
   static const bool value=(has_value_type<candidate>::value &&
                            has_iterator_category<candidate>::value &&
                            has_difference_type<candidate>::value) || std::is_pointer<candidate>::value;
};

// SFINAE helpers

template <typename Iterator, bool enabled, template <typename...> class TestFunction, typename... TestParams>
struct assess_iterator_impl
   : std::false_type {};

template <typename Iterator, template <typename...> class TestFunction, typename... TestParams>
struct assess_iterator_impl<Iterator, true, TestFunction, TestParams...>
   : TestFunction<pure_type_t<Iterator>, TestParams...> {};

template <typename Iterator, template <typename...> class TestFunction, typename... TestParams>
struct assess_iterator
   : assess_iterator_impl<Iterator, looks_like_iterator<Iterator>::value, TestFunction, TestParams...> {};


template <typename Iterator, bool enabled, template <typename...> class TestFunction, typename... TestParams>
struct assess_iterator_value_impl
   : std::false_type {};

template <typename Iterator, template <typename...> class TestFunction, typename... TestParams>
struct assess_iterator_value_impl<Iterator, true, TestFunction, TestParams...>
   : TestFunction<typename iterator_traits<Iterator>::value_type, TestParams...> {};

template <typename Iterator, template <typename...> class TestFunction, typename... TestParams>
struct assess_iterator_value
   : assess_iterator_value_impl<Iterator, looks_like_iterator<Iterator>::value, TestFunction, TestParams...> {};


template <typename Container, bool _has_category=has_container_category<Container>::value>
struct container_category_traits {
   typedef typename iterator_traits<typename Container::iterator>::iterator_category category;
};

template <typename Container>
struct container_category_traits<Container, true> {
   typedef typename Container::container_category category;
};

template <typename Container,
          typename exclude_generic_tag=void,
          bool feasible= has_iterator<Container>::value && has_value_type<Container>::value>
struct is_suitable_container {
   static const bool value=std::is_same<typename object_traits<Container>::model, is_container>::value &&
                          !std::is_same<typename object_traits<Container>::generic_tag, exclude_generic_tag>::value;
};

template <typename Container, typename exclude_generic_tag>
struct is_suitable_container<Container, exclude_generic_tag, false> : std::false_type {};

template <typename T, size_t size, typename exclude_generic_tag>
struct is_suitable_container<T[size], exclude_generic_tag, false> : std::false_type {};

// special tag for isomorphic_to_container_of
struct allow_conversion {};

// @todo recursive check of generic_tags in case of Element being in turn a container
template <typename Container, typename Element,
          typename exclude_generic_tag=void,
          bool enable=is_suitable_container<Container, exclude_generic_tag>::value>
struct isomorphic_to_container_of
   : bool_constant< isomorphic_types<typename Container::value_type, Element>::value &&
                    (std::is_same<typename object_traits<Element>::generic_tag, typename object_traits<Element>::model>::value ||
                     std::is_same<typename object_traits<Element>::generic_tag, typename object_traits<typename Container::value_type>::generic_tag>::value ||
                     (std::is_same<exclude_generic_tag, allow_conversion>::value &&
                      (std::is_convertible<typename Container::value_type, Element>::value || explicitly_convertible_to<typename Container::value_type, Element>::value))) > {};

template <typename Container, typename Element, typename exclude_generic_tag>
struct isomorphic_to_container_of<Container, Element, exclude_generic_tag, false> : std::false_type {};

template <typename... Containers, typename Element, typename exclude_generic_tag>
struct isomorphic_to_container_of<mlist<Containers...>, Element, exclude_generic_tag, false>
   : mlist_and_nonempty< isomorphic_to_container_of<Containers, Element, exclude_generic_tag>... > {};

template <typename Container, bool is_const,
          bool _enabled=has_iterator<Container>::value,
          bool _reversible=is_derived_from<typename container_category_traits<Container>::category, bidirectional_iterator_tag>::value>
struct default_container_it_traits : default_container_elem_traits<Container, is_const> {
   typedef typename Container::const_iterator const_iterator;
   typedef typename std::conditional<is_const, const_iterator, typename Container::iterator>::type iterator;
};

template <typename Container, bool is_const>
struct default_container_it_traits<Container, is_const, true, true>
   : default_container_it_traits<Container, is_const, true, false> {
   typedef typename Container::const_reverse_iterator const_reverse_iterator;
   typedef typename std::conditional<is_const, const_reverse_iterator, typename Container::reverse_iterator>::type reverse_iterator;
};

template <typename Container, bool is_const, bool _reversible>
struct default_container_it_traits<Container, is_const, false, _reversible> : default_container_elem_traits<Container, is_const> {};

template <typename Container, bool is_const>
struct default_container_traits : container_category_traits<Container>, default_container_it_traits<Container, is_const> {};

template <typename ContainerRef>
struct container_traits
   : default_container_traits<typename deref<ContainerRef>::type, attrib<ContainerRef>::is_const>
{
   typedef default_container_traits<typename deref<ContainerRef>::type, attrib<ContainerRef>::is_const> base_t;
   static const bool
      is_forward       = is_derived_from<typename base_t::category, forward_iterator_tag>::value,
      is_bidirectional = is_derived_from<typename base_t::category, bidirectional_iterator_tag>::value,
      is_random        = is_derived_from<typename base_t::category, random_access_iterator_tag>::value;
};

template <typename Container>
struct is_assoc_container : bool_constant<has_key_type<Container>::value && has_mapped_type<Container>::value> {};

template <typename Iterator> inline
int count_it(Iterator src)
{
   typename iterator_traits<Iterator>::difference_type cnt=0;
   while (!src.at_end()) {
      ++cnt, ++src;
   }
   return cnt;
}

/*  Plain arrays
 *  Most of the stuff defined in this section becomes obsolete with the advent of proper range support in C++ 17
 */

/// Wrapper for a pointer used as an iterator.
template <typename T, bool is_reversed>
class ptr_wrapper {
public:
   typedef random_access_iterator_tag iterator_category;
   typedef T& reference;
   typedef T* pointer;
   typedef typename deref<T>::type value_type;  // T may have 'const' attribute
   typedef ptrdiff_t difference_type;
   typedef ptr_wrapper<value_type, is_reversed> iterator;
   typedef ptr_wrapper<const value_type, is_reversed> const_iterator;

   template <typename, bool> friend class ptr_wrapper;

   ptr_wrapper(pointer cur_arg=nullptr) : cur(cur_arg) {}
   ptr_wrapper(const iterator& it) : cur(it.cur) {}

   ptr_wrapper& operator= (pointer cur_arg) { cur=cur_arg; return *this; }
   ptr_wrapper& operator= (const iterator& it) { cur=it.cur; return *this; }

   reference operator* () const { return *cur; }
   pointer operator-> () const { return cur; }
   reference operator[] (int i) const { return cur[is_reversed ? -i : i]; }

   ptr_wrapper& operator++ () { is_reversed ? --cur : ++cur; return *this; }
   ptr_wrapper& operator-- () { is_reversed ? ++cur : --cur; return *this; }
   const ptr_wrapper operator++ (int) { ptr_wrapper copy=*this; operator++(); return copy; }
   const ptr_wrapper operator-- (int) { ptr_wrapper copy=*this; operator--(); return copy; }
   ptr_wrapper& operator+= (int i) { is_reversed ? cur-=i : cur+=i; return *this; }
   ptr_wrapper& operator-= (int i) { is_reversed ? cur+=i : cur-=i; return *this; }
   ptr_wrapper operator+ (int i) const { return ptr_wrapper(is_reversed ? cur-i : cur+i); }
   ptr_wrapper operator- (int i) const { return ptr_wrapper(is_reversed ? cur+i : cur-i); }
   friend ptr_wrapper operator+ (int i, const ptr_wrapper& p) { return p+i; }

   template <typename Other>
   typename std::enable_if<is_derived_from_any<Other, iterator, const_iterator>::value, ptrdiff_t>::type
   operator- (const Other& other) const
   {
      const typename is_derived_from_any<Other, iterator, const_iterator>::type& other_it=other;
      return is_reversed ? other_it.cur-cur : cur-other_it.cur;
   }

   template <typename Other>
   typename std::enable_if<is_derived_from_any<Other, iterator, const_iterator>::value, bool>::type
   operator== (const Other& other) const
   {
      const typename is_derived_from_any<Other, iterator, const_iterator>::type& other_it=other;
      return cur==other_it.cur;
   }

   template <typename Other>
   typename std::enable_if<is_derived_from_any<Other, iterator, const_iterator>::value, bool>::type
   operator!= (const Other& other) const
   {
     return !(*this==other);
   }

   template <typename Other>
   typename std::enable_if<is_among<Other, iterator, const_iterator>::value, bool>::type
   operator< (const Other& other) const
   {
      return is_reversed ? cur > other.cur : cur < other.cur;
   }

   template <typename Other>
   typename std::enable_if<is_among<Other, iterator, const_iterator>::value, bool>::type
   operator> (const Other& other) const
   {
      return other < *this;
   }

   template <typename Other>
   typename std::enable_if<is_among<Other, iterator, const_iterator>::value, bool>::type
   operator<= (const Other& other) const
   {
      return !(other < *this);
   }

   template <typename Other>
   typename std::enable_if<is_among<Other, iterator, const_iterator>::value, bool>::type
   operator>= (const Other& other) const
   {
      return !(*this < other);
   }

protected:
   pointer cur;
};

template <typename Iterator>
struct pointer_as_iterator {
   typedef pure_type_t<Iterator> type;
};

template <typename T>
struct pointer_as_iterator<T*> {
   typedef ptr_wrapper<T, false> type;
};

template <typename Iterator>
using pointer2iterator_t = typename pointer_as_iterator<Iterator>::type;

template <typename Iterator> inline
Iterator&& pointer2iterator(Iterator&& it) { return std::forward<Iterator>(it); }

template <typename T> inline
ptr_wrapper<T, false> pointer2iterator(T* ptr) { return ptr; }

template <typename Iterator> inline
auto make_iterator_range(Iterator&& first, Iterator&& last)
{
   return iterator_range<pointer2iterator_t<Iterator>>(pointer2iterator(std::forward<Iterator>(first)), pointer2iterator(std::forward<Iterator>(last)));
}


// TODO: places where this class is used separately from plain_array are highly questionnable
template <typename E>
struct array_traits {
   typedef E& reference;
   typedef const E& const_reference;
   typedef E value_type;
   typedef ptr_wrapper<E, false> iterator;
   typedef ptr_wrapper<const E, false> const_iterator;
   typedef ptr_wrapper<E, true> reverse_iterator;
   typedef ptr_wrapper<const E, true> const_reverse_iterator;
   typedef random_access_iterator_tag container_category;
};

template <typename Top, typename E=typename container_traits<Top>::value_type>
class plain_array : public array_traits<E> {
   typedef array_traits<E> base_t;
public:
   typename base_t::iterator begin()
   {
      return static_cast<Top*>(this)->get_data();
   }
   typename base_t::iterator end()
   {
      return static_cast<Top*>(this)->get_data() + static_cast<const Top*>(this)->size();
   }
   typename base_t::const_iterator begin() const
   {
      return static_cast<const Top*>(this)->get_data();
   }
   typename base_t::const_iterator end() const
   {
      return static_cast<const Top*>(this)->get_data() + static_cast<const Top*>(this)->size();
   }

   typename base_t::reverse_iterator rbegin()
   {
      return static_cast<Top*>(this)->get_data() + static_cast<const Top*>(this)->size() - 1;
   }
   typename base_t::reverse_iterator rend()
   {
      return static_cast<Top*>(this)->get_data()-1;
   }
   typename base_t::const_reverse_iterator rbegin() const
   {
      return static_cast<const Top*>(this)->get_data() + static_cast<const Top*>(this)->size() - 1;
   }
   typename base_t::const_reverse_iterator rend() const
   {
      return static_cast<const Top*>(this)->get_data() - 1;
   }

   typename base_t::reference front()
   {
      if (POLYMAKE_DEBUG) {
         if (empty())
            throw std::runtime_error("front() on an empty array");
      }
      return *static_cast<Top*>(this)->get_data();
   }
   typename base_t::reference back()
   {
      if (POLYMAKE_DEBUG) {
         if (empty())
            throw std::runtime_error("back() on an empty array");
      }
      return *rbegin();
   }
   typename base_t::reference operator[] (int i)
   {
      if (POLYMAKE_DEBUG) {
         if (i < 0 || i >= static_cast<const Top*>(this)->size())
            throw std::runtime_error("array::operator[] - index out of range");
      }
      return static_cast<Top*>(this)->get_data()[i];
   }
   typename base_t::const_reference front() const
   {
      if (POLYMAKE_DEBUG) {
         if (empty())
            throw std::runtime_error("front() on an empty array");
      }
      return *static_cast<const Top*>(this)->get_data();
   }
   typename base_t::const_reference back() const
   {
      if (POLYMAKE_DEBUG) {
         if (empty())
            throw std::runtime_error("back() on an empty array");
      }
      return *rbegin();
   }
   typename base_t::const_reference operator[] (int i) const
   {
      if (POLYMAKE_DEBUG) {
         if (i < 0 || i >= static_cast<const Top*>(this)->size())
            throw std::runtime_error("array::operator[] - index out of range");
      }
      return static_cast<const Top*>(this)->get_data()[i];
   }

   bool empty() const
   {
      return static_cast<const Top*>(this)->size()==0;
   }
};

template <typename E>
class initializer_list_adapter
   : public plain_array<initializer_list_adapter<E>, const E> {
public:
   explicit initializer_list_adapter(const std::initializer_list<E>& arg)
      : data(arg) {}

   const E* get_data() const { return data.begin(); }
   int size() const { return data.size(); }
protected:
   const std::initializer_list<E> data;
};

template <typename E>
struct container_traits<std::initializer_list<E>>
   : container_traits<initializer_list_adapter<E>> {};

template <typename E>
struct container_traits<const std::initializer_list<E>>
   : container_traits<initializer_list_adapter<E>> {};

template <typename E>
struct spec_object_traits<initializer_list_adapter<E>>
   : spec_object_traits<is_container> {
   static const bool is_always_const=true, is_persistent=false;
};

template <typename E>
struct spec_object_traits<std::initializer_list<E>>
   : spec_object_traits<initializer_list_adapter<E>> {};

/// the following should die when all C arrays disappear from client code

template <typename E, size_t Tsize>
class fixed_array
   : public plain_array< fixed_array<E, Tsize>, E> {
   E data[Tsize];
public:
   E* get_data() { return data; }
   const E* get_data() const { return data; }
   int size() const { return Tsize; }
   int max_size() const { return Tsize; }
protected:
   fixed_array();
   ~fixed_array();
};

template <typename E, size_t Tsize, size_t Tsubsize>
class fixed_array<E[Tsubsize], Tsize>
   : public plain_array< fixed_array<E[Tsubsize], Tsize>, fixed_array<E, Tsubsize> > {
   fixed_array<E, Tsubsize> data[Tsize];
public:
   fixed_array<E, Tsubsize>* get_data() { return data; }
   const fixed_array<E, Tsubsize>* get_data() const { return data; }
   int size() const { return Tsize; }
   int max_size() const { return Tsize; }
protected:
   fixed_array();
   ~fixed_array();
};

template <typename E, size_t Tsize> inline
typename std::enable_if<sizeof(E[Tsize])==sizeof(fixed_array<E, Tsize>), fixed_array<E, Tsize>>::type&
array2container(E (&a)[Tsize])
{
   return reinterpret_cast<fixed_array<E, Tsize>&>(a);
}

template <typename E, size_t Tsize> inline
const typename std::enable_if<sizeof(E[Tsize])==sizeof(fixed_array<E, Tsize>), fixed_array<E, Tsize>>::type&
array2container(const E (&a)[Tsize])
{
   return reinterpret_cast<const fixed_array<E, Tsize>&>(a);
}

template <typename E, size_t Tsize>
class Serialized<E[Tsize]> : public fixed_array<E, Tsize> {
protected:
   Serialized() throw();
   ~Serialized() throw();
};

template <typename E>
struct spec_object_traits< array_traits<E> >
   : spec_object_traits<is_container> {};

template <typename E, size_t Tsize>
struct spec_object_traits< fixed_array<E, Tsize> >
   : spec_object_traits<is_container> {};

template <typename E, size_t Tsize>
struct spec_object_traits< E[Tsize] >
   : spec_object_traits<is_opaque> {};


template <typename FeatureList1, typename FeatureList2>
struct mix_features {
   typedef typename concat_list< typename list_search_all<FeatureList1, FeatureList2, absorbing_feature>::negative2,
           typename concat_list< typename list_search_all<FeatureList2, FeatureList1, absorbing_feature>::negative2,
                                 typename list_search_all<FeatureList1, FeatureList2, std::is_same>::positive >::type >::type
      type;
};

template <typename List, typename Feature>
struct min_feature
   : list_logical_or< typename list_search_all<List, Feature, absorbing_feature>::positive, Feature > {};

template <typename FeatureList1, typename FeatureList2>
struct toggle_features {
   typedef typename concat_list< typename list_search_all<FeatureList1, FeatureList2, equivalent_features>::negative,
                                 typename list_search_all<FeatureList1, FeatureList2, equivalent_features>::negative2 >::type
      type;
};

template <typename Container, int kind=object_classifier::what_is<Container>::value>
struct enforce_feature_helper {
   typedef void must_enforce_features;
   typedef void can_enforce_features;
   typedef void cannot_enforce_features;
};

template <typename Container>
struct enforce_feature_helper<Container, object_classifier::is_manip> {
   typedef typename list_search_all<typename Container::expected_features,
                                    typename Container::must_enforce_features, absorbing_feature>::negative2
      must_enforce_features;
   typedef typename Container::can_enforce_features can_enforce_features;
   typedef typename Container::cannot_enforce_features cannot_enforce_features;
};

struct checked_via_iterator {};

// to be specialized on the second parameter only
template <typename Container, typename Feature>
struct default_check_container_feature : checked_via_iterator {
   static const bool value=
      check_iterator_feature<typename container_traits<Container>::iterator, Feature>::value &&
      !list_search_all<Feature, typename enforce_feature_helper<Container>::must_enforce_features, absorbing_feature>::value;
};

// can be specialized either on the first parameter or on both
template <typename Container, typename Feature>
struct check_container_feature : default_check_container_feature<Container,Feature> {};

template <typename ContainerRef, typename Feature>
struct check_container_ref_feature : check_container_feature<typename deref<ContainerRef>::type, Feature> {};

template <typename Container, typename Features>
struct check_container_features : check_container_feature<Container, Features> {};

template <typename Container>
struct check_container_features<Container, void> : std::true_type {};

template <typename Container, typename Head, typename Tail>
struct check_container_features<Container, cons<Head,Tail> > {
   static const bool value=check_container_features<Container, Head>::value &&
                           check_container_features<Container, Tail>::value;
};

template <typename Feature, typename Container>
struct filter_iterator_features_helper
   : is_derived_from<default_check_container_feature<Container,Feature>, checked_via_iterator> {};

template <typename Feature, typename Container>
struct filter_iterator_features_helper<provide_construction<Feature,false>, Container> 
   : filter_iterator_features_helper<Feature, Container> {};

template <typename Feature>
struct filter_iterator_features 
   : list_search_all<Feature, fixed_array<int,1>, filter_iterator_features_helper> {};

template <typename Feature>
struct reorder_features_helper {
   typedef Feature type;
};

template <typename Head, typename Tail,
          typename before=typename list_search_all<Head,Tail,feature_allow_order>::negative2,
          typename after=typename list_search_all<Head,Tail,feature_allow_order>::positive2>
struct reorder_features_helper2
   : reorder_features_helper< typename concat_list<before, typename concat_list<Head,after>::type >::type > {};

template <typename Head, typename Tail, typename after>
struct reorder_features_helper2<Head, Tail, void, after> {
   typedef cons<Head, typename reorder_features_helper<Tail>::type> type;
};

template <typename Head, typename Tail>
struct reorder_features_helper< cons<Head,Tail> >
   : reorder_features_helper2<Head, Tail> {};

template <typename Features>
struct reorder_features {
   // 'int' here serves just as some inexisting feature
   typedef typename list_search_all<Features,int,feature_allow_order>::positive normal;
   typedef typename list_search_all<Features,int,feature_allow_order>::negative last;
   typedef typename concat_list< typename filter_iterator_features<normal>::negative,
                                 typename filter_iterator_features<normal>::positive >::type
      normal_list;
   typedef typename concat_list< typename reorder_features_helper<normal_list>::type, last >::type type;
};

/* Provides a construction (masquerading Container) that will have a desired feature.
   Must be specialized for each enforcible feature. */
template <typename Container, typename Feature>
struct default_enforce_feature;

// Can be specialized for some container classes. Handles exactly one missing feature.
template <typename Container, typename Feature>
struct enforce_feature {
   typedef typename default_enforce_feature<Container,Feature>::container container;
};

// Can be specialized for various container families (according to object_classifier::what_is).
template <typename Container, typename Features, int kind>
struct default_enforce_features
   : enforce_feature<Container,Features> {};

// Can be specialized for some container classes. Handles a list of missing features
template <typename Container, typename Features>
struct enforce_features
   : default_enforce_features<Container, Features, object_classifier::what_is<Container>::value> {};

template <typename Container>
struct default_enforce_feature<Container, void> {
   typedef Container container;
};

template <typename Container, typename Feature, bool on_top>
struct default_enforce_feature<Container, provide_construction<Feature,on_top> >
   : enforce_feature<Container, Feature> {};

template <typename Container, typename Features, typename Lacking>
struct enforce_lacking_features_helper
   : enforce_features<Container,Lacking> {};

template <typename Container, typename Features>
struct enforce_lacking_features_helper<Container, Features, void> {
   typedef Container container;
};

template <typename Container, typename Features>
struct enforce_lacking_features {
   typedef typename list_search_all<Container,Features,check_container_feature>::negative2 lacking;
   typedef typename enforce_lacking_features_helper<Container, Features, lacking>::container container;
};

template <typename Container, typename Head, typename Tail>
struct default_enforce_features<Container, cons<Head,Tail>, object_classifier::is_opaque> {
   typedef typename reorder_features< cons<Head,Tail> >::type needed_features;
   typedef typename enforce_feature<Container, typename needed_features::head>::container enforced_head;
   typedef typename enforce_lacking_features<enforced_head, typename needed_features::tail>::container container;
};

template <typename Container, typename Features>
class feature_collector : public enforce_lacking_features<Container, Features>::container {
protected:
   feature_collector();
   ~feature_collector();
};

template <typename Container, typename Features>
struct redirect_object_traits< feature_collector<Container, Features> >
   : object_traits<Container> {
   typedef Container masquerade_for;
   static const bool is_temporary=false;
};

template <typename Container, typename ProvidedFeatures, typename Feature>
struct check_container_feature<feature_collector<Container, ProvidedFeatures>, Feature> {
   static const bool value=check_container_feature<Container,Feature>::value ||
                           list_search<ProvidedFeatures, Feature, absorbing_feature>::value;
};

template <typename Container, typename Features>
struct ensure_features_helper {
   typedef typename inherit_const<feature_collector<typename deref<Container>::type, Features>, Container>::type
      container;
};

template <typename Container, typename ProvidedFeatures, typename Features>
struct ensure_features_helper<feature_collector<Container, ProvidedFeatures>, Features>
   : ensure_features_helper<Container, typename mix_features<ProvidedFeatures,Features>::type> {};

template <typename Container, typename ProvidedFeatures, typename Features>
struct ensure_features_helper<const feature_collector<Container, ProvidedFeatures>, Features>
   : ensure_features_helper<const Container, typename mix_features<ProvidedFeatures,Features>::type> {};

template <typename Container, typename Features>
struct ensure_features
   : ensure_features_helper<Container, Features>
   , container_traits<typename ensure_features_helper<Container, Features>::container> {};

template <typename Container, typename Features> inline
typename ensure_features<Container, Features>::container&
ensure(Container& c, Features*)
{
   return reinterpret_cast<typename ensure_features<Container, Features>::container&>(c);
}

template <typename Container, typename Features> inline
typename ensure_features<const Container, Features>::container&
ensure(const Container& c, Features*)
{
   return reinterpret_cast<typename ensure_features<const Container, Features>::container&>(c);
}

template <typename Container> inline
Container& ensure(Container& c, void*) { return c; }

template <typename Container> inline
const Container& ensure(const Container& c, void*) { return c; }


template <typename E, typename Features>
struct ensure_features<std::initializer_list<E>, Features>
   : ensure_features<const initializer_list_adapter<E>, Features> {};

template <typename E, typename Features>
struct ensure_features<const std::initializer_list<E>, Features>
   : ensure_features<const initializer_list_adapter<E>, Features> {};

template <typename E, typename Features> inline
typename ensure_features<std::initializer_list<E>, Features>::type
ensure(std::initializer_list<E>& l, Features*)
{
   return typename ensure_features<std::initializer_list<E>, Features>::type(l);
}

template <typename E, typename Features> inline
typename ensure_features<std::initializer_list<E>, Features>::type
ensure(const std::initializer_list<E>& l, Features*)
{
   return typename ensure_features<std::initializer_list<E>, Features>::type(l);
}


template <typename ContainerRef, typename Features>
struct masquerade_add_features : inherit_ref<typename ensure_features<typename deref<ContainerRef>::minus_ref, Features>::container, ContainerRef> {};

template <typename ContainerRef, typename Features>
struct deref< masquerade_add_features<ContainerRef,Features> >
   : deref< typename masquerade_add_features<ContainerRef,Features>::type > {
   typedef masquerade_add_features<typename attrib<ContainerRef>::plus_const, Features> plus_const;
};

namespace operations {
struct incomplete {
   typedef void argument_type;
   typedef void first_argument_type;
   typedef void second_argument_type;
   typedef void result_type;
   void operator() () const;
};
}

template <template <typename> class Operation>
struct BuildUnary : operations::incomplete {};

template <template <typename> class Operation>
struct BuildUnaryIt : operations::incomplete {};

template <template <typename,typename> class Operation>
struct BuildBinary : operations::incomplete {};

template <template <typename,typename> class Operation>
struct BuildBinaryIt : operations::incomplete {};

template <typename Operation, typename Iterator, typename Reference=typename iterator_traits<Iterator>::reference, typename enabled=void>
struct unary_op_builder {
   typedef Operation operation;
   static const operation& create(const Operation& op) { return op; }

   template <typename IndexOperation>
   static const operation& create(const pair<Operation, IndexOperation>& p) { return p.first; }
};

template <typename Operation>
struct empty_op_builder {
   typedef Operation operation;
   template <typename X>
   static operation create(const X&) { return operation(); }
};

template <template <typename> class Operation, typename Iterator, typename Reference>
struct unary_op_builder<BuildUnary<Operation>, Iterator, Reference>
   : empty_op_builder< Operation<Reference> > {};

template <template <typename> class Operation, typename Iterator, typename Reference>
struct unary_op_builder<BuildUnaryIt<Operation>, Iterator, Reference>
   : empty_op_builder< Operation<const Iterator&> > {};

template <typename Operation, typename Iterator1, typename Iterator2,
          typename LeftRef=typename iterator_traits<Iterator1>::reference,
          typename RightRef=typename iterator_traits<Iterator2>::reference>
struct binary_op_builder {
   typedef Operation operation;
   static const operation& create(const Operation& op) { return op; }

   template <typename IndexOperation>
   static const operation& create(const pair<Operation, IndexOperation>& p) { return p.first; }
};

template <template <typename,typename> class Operation, typename Iterator1, typename Iterator2, typename LeftRef, typename RightRef>
struct binary_op_builder<BuildBinary<Operation>, Iterator1, Iterator2, LeftRef, RightRef>
   : empty_op_builder< Operation<LeftRef, RightRef> > {};

template <template <typename,typename> class Operation, typename Iterator1, typename Iterator2, typename LeftRef, typename RightRef>
struct binary_op_builder<BuildBinaryIt<Operation>, Iterator1, Iterator2, LeftRef, RightRef >
   : empty_op_builder< Operation<const Iterator1&, const Iterator2&> > {};

template <typename T, typename Iterator>
struct value_type_match {
   static const bool value=compatible<typename iterator_traits<Iterator>::reference, T>::value ||
                           std::is_same<typename object_traits<typename iterator_traits<Iterator>::value_type>::persistent_type,
                                        typename object_traits<typename deref<T>::type>::persistent_type>::value;
};

template <typename Iterator, typename arg_type,
          bool _not_deref=std::is_same<arg_type,void>::value || is_derived_from<Iterator, typename deref<arg_type>::type>::value>
struct star_helper {
   typedef const Iterator& const_result_type;
   typedef Iterator& mutable_result_type;
   static const bool data_arg=true;
   static const_result_type get(const Iterator& it) { return it; }
   static mutable_result_type get(Iterator& it) { return it; }
};

template <typename Iterator, typename arg_type>
struct star_helper<Iterator, arg_type, true> {
   typedef const Iterator* const_result_type;
   typedef Iterator* mutable_result_type;
   static const bool data_arg=false;
   static const_result_type get(const Iterator& it) { return &it; }
   static mutable_result_type get(Iterator& it) { return &it; }
};

template <typename Iterator, typename Operation>
struct unary_helper
   : unary_op_builder<Operation, Iterator> {
   typedef star_helper<Iterator, typename unary_helper::operation::argument_type> star;
   static const bool data_arg=star::data_arg;
   static typename star::const_result_type get(const Iterator& it) { return star::get(it); }
   static typename star::mutable_result_type get(Iterator& it) { return star::get(it); }
};

template <typename IteratorPair, typename Operation>
struct binary_helper
   : binary_op_builder<Operation, typename IteratorPair::first_type, typename IteratorPair::second_type> {
   typedef typename IteratorPair::first_type it_first;
   typedef typename IteratorPair::second_type it_second;
   typedef binary_op_builder<Operation, it_first, it_second> base_t;
   typedef star_helper<it_first, typename base_t::operation::first_argument_type> star1;
   typedef star_helper<it_second, typename base_t::operation::second_argument_type> star2;
   static const bool first_data_arg=star1::data_arg, second_data_arg=star2::data_arg;
   static typename star1::const_result_type get1(const it_first& it) { return star1::get(it); }
   static typename star2::const_result_type get2(const it_second& it) { return star2::get(it); }
   static typename star1::mutable_result_type get1(it_first& it) { return star1::get(it); }
   static typename star2::mutable_result_type get2(it_second& it) { return star2::get(it); }
};

template <typename Operation, typename IndexOperation, typename Iterator, typename Reference>
struct unary_op_builder<pair<Operation, IndexOperation>, Iterator, Reference>
   : unary_op_builder<Operation, Iterator, Reference> {};

template <typename Iterator, typename Operation, typename IndexOperation>
struct unary_helper<Iterator, pair<Operation, IndexOperation> >
   : unary_helper<Iterator, Operation> {};

template <typename Operation, typename IndexOperation, typename Iterator1, typename Iterator2, 
          typename Reference1, typename Reference2>
struct binary_op_builder<pair<Operation, IndexOperation>, Iterator1, Iterator2, Reference1, Reference2>
   : binary_op_builder<Operation, Iterator1, Iterator2, Reference1, Reference2> {};

template <typename IteratorPair, typename Operation, typename IndexOperation>
struct binary_helper<IteratorPair, pair<Operation, IndexOperation> >
   : binary_helper<IteratorPair, Operation> {};

template <typename> class ContainerTag;
template <typename> class Container1Tag;
template <typename> class Container2Tag;
template <typename> class OperationTag;
template <typename> class IteratorConstructorTag;
template <typename> class IteratorCouplerTag;
template <typename> class HiddenTag;
template <typename> class ReverseTag;
typedef HiddenTag<std::true_type> MasqueradedTop;
template <typename> class ExpectedFeaturesTag;
template <typename> class FeaturesViaSecondTag;
template <typename> class BijectiveTag;
template <typename> class PartiallyDefinedTag;

template <typename Top, typename TParams, bool THas_hidden=mtagged_list_extract<TParams, HiddenTag>::is_specified>
class manip_container_top : public manip_container_base {
public:
   typedef void hidden_type;
   typedef typename mtagged_list_extract<TParams, ExpectedFeaturesTag>::type expected_features;
   typedef Top manip_top_type;
   typedef void must_enforce_features;
   typedef void can_enforce_features;
   typedef void cannot_enforce_features;

   Top& manip_top() { return *static_cast<Top*>(this); }
   const Top& manip_top() const { return *static_cast<const Top*>(this); }
};

template <typename Container, typename ProvidedFeatures, typename TParams>
class manip_container_top<manip_feature_collector<Container, ProvidedFeatures>, TParams, false>
   : public manip_container_base {
public:
   typedef void hidden_type;
   typedef typename mix_features<typename mtagged_list_extract<TParams, ExpectedFeaturesTag>::type, ProvidedFeatures>::type
      expected_features;
   typedef typename Container::manip_top_type manip_top_type;
   typedef void must_enforce_features;
   typedef typename Container::can_enforce_features can_enforce_features;
   typedef typename Container::cannot_enforce_features cannot_enforce_features;

   manip_top_type& manip_top()
   {
      return *static_cast<manip_top_type*>(reinterpret_cast<Container*>(this));
   }
   const manip_top_type& manip_top() const
   {
      return *static_cast<const manip_top_type*>(reinterpret_cast<const Container*>(this));
   }
};

template <typename Top, typename THidden>
struct manip_container_hidden_helper {
   typedef THidden type;
};

template <typename Top>
struct manip_container_hidden_helper<Top, std::true_type>
   : mget_template_parameter<Top, 0> {};

template <typename Top, typename TParams,
          bool TBinary=(mtagged_list_extract<TParams, Container1Tag>::is_specified ||
                        mtagged_list_extract<TParams, Container2Tag>::is_specified)>
class manip_container_hidden_defaults {
public:
   typedef typename manip_container_hidden_helper<Top, typename mtagged_list_extract<TParams, HiddenTag>::type>::type hidden_type;
   typedef typename deref<typename mtagged_list_extract<TParams, ContainerTag, hidden_type>::type>::minus_ref container;

   container& get_container()
   {
      return reinterpret_cast<container&>(static_cast<manip_container_top<Top, TParams, true>*>(this)->manip_top());
   }
   const container& get_container() const
   {
      return reinterpret_cast<const container&>(static_cast<const manip_container_top<Top, TParams, true>*>(this)->manip_top());
   }
};

template <typename Top, typename TParams>
class manip_container_hidden_defaults<Top, TParams, true> {
public:
   typedef typename manip_container_hidden_helper<Top, typename mtagged_list_extract<TParams, HiddenTag>::type>::type hidden_type;
   typedef typename deref<typename mtagged_list_extract<TParams, Container1Tag, hidden_type>::type>::minus_ref container1;
   typedef typename deref<typename mtagged_list_extract<TParams, Container2Tag, hidden_type>::type>::minus_ref container2;

   container1& get_container1()
   {
      return reinterpret_cast<container1&>(static_cast<manip_container_top<Top, TParams, true>*>(this)->manip_top());
   };
   const container1& get_container1() const
   {
      return reinterpret_cast<const container1&>(static_cast<const manip_container_top<Top, TParams, true>*>(this)->manip_top());
   }
   container2& get_container2()
   {
      return reinterpret_cast<container2&>(static_cast<manip_container_top<Top, TParams, true>*>(this)->manip_top());
   }
   const container2& get_container2() const
   {
      return reinterpret_cast<const container2&>(static_cast<const manip_container_top<Top, TParams, true>*>(this)->manip_top());
   }
};

template <typename Top, typename TParams>
class manip_container_top<Top, TParams, true>
   : public manip_container_top<Top, TParams, false>
   , public manip_container_hidden_defaults<Top, TParams> {
protected:
   manip_container_top();
   ~manip_container_top();
public:
   typedef typename manip_container_hidden_defaults<Top, TParams>::hidden_type hidden_type;

   hidden_type& hidden()
   {
      return reinterpret_cast<hidden_type&>(this->manip_top());
   }
   const hidden_type& hidden() const
   {
      return reinterpret_cast<const hidden_type&>(this->manip_top());
   }
};

template <typename Container, typename ProvidedFeatures>
class manip_feature_collector
   : public Container::template rebind_feature_collector< manip_feature_collector<Container, ProvidedFeatures> >::type {
protected:
   manip_feature_collector();
   ~manip_feature_collector();
};

template <typename Container, typename Features>
struct manip_feature_collector_helper {
   typedef manip_feature_collector<Container, Features> container;
};
template <typename Container, typename PrevFeatures, typename Features>
struct manip_feature_collector_helper<manip_feature_collector<Container,PrevFeatures>, Features> {
   typedef manip_feature_collector<Container, typename concat_list<PrevFeatures,Features>::type> container;
};
template <typename Container>
struct manip_feature_collector_helper<Container,void> {
   typedef Container container;
};
template <typename Container, typename PrevFeatures>    // resolving ambiguity
struct manip_feature_collector_helper<manip_feature_collector<Container,PrevFeatures>, void> {
   typedef manip_feature_collector<Container,PrevFeatures> container;
};

template <typename Container, typename Features>
struct default_enforce_features<Container, Features, object_classifier::is_manip> {
   typedef typename list_search_all<Features, int, feature_allow_order>::negative after1;
   typedef typename list_search_all<Features, int, feature_allow_order>::positive not_last;
   typedef typename list_search_all<not_last, typename Container::cannot_enforce_features, absorbing_feature>::positive
      after2;
   typedef typename list_search_all<not_last, typename Container::cannot_enforce_features, absorbing_feature>::negative
      not_after;
   typedef typename list_search_all<not_after, typename Container::can_enforce_features, equivalent_features>::positive
      via_manip1;
   typedef typename filter_iterator_features<typename list_search_all<not_after, typename Container::can_enforce_features, equivalent_features>::negative>::positive
      via_manip2;
   typedef typename concat_list<via_manip1,via_manip2>::type via_manip;
   typedef typename list_search_all<not_after, via_manip, equivalent_features>::negative before;
   typedef typename concat_list<after2,after1>::type after;

   typedef typename default_enforce_features<Container, before, object_classifier::is_opaque>::container
      enforced_before;
   typedef typename manip_feature_collector_helper<enforced_before, via_manip>::container enforced_via_manip;
   typedef typename default_enforce_features<enforced_via_manip, after, object_classifier::is_opaque>::container
      container;
};

template <typename Container,
          bool is_reversible=container_traits<Container>::is_bidirectional>
class construct_rewindable
   : public std::enable_if<container_traits<Container>::is_forward, Container>::type {
protected:
   construct_rewindable();
   ~construct_rewindable();
public:
   typedef rewindable_iterator<typename Container::iterator> iterator;
   typedef rewindable_iterator<typename Container::const_iterator> const_iterator;

   iterator begin() { return Container::begin(); }
   iterator end() { return Container::end(); }
   const_iterator begin() const { return Container::begin(); }
   const_iterator end() const { return Container::end(); }
};

template <typename Container>
class construct_rewindable<Container, true>
   : public construct_rewindable<Container, false> {
public:
   typedef rewindable_iterator<typename Container::reverse_iterator> reverse_iterator;
   typedef rewindable_iterator<typename Container::const_reverse_iterator> const_reverse_iterator;

   reverse_iterator rbegin() { return Container::rbegin(); }
   reverse_iterator rend() { return Container::rend(); }
   const_reverse_iterator rbegin() const { return Container::rbegin(); }
   const_reverse_iterator rend() const { return Container::rend(); }
};

template <typename Container, bool _reversible>
struct redirect_object_traits< construct_rewindable<Container,_reversible> >
   : object_traits<Container> {
   typedef Container masquerade_for;
   static const bool is_temporary=false;
};

template <typename Container>
struct end_sensitive_helper {
   typedef Container end_source;
};

template <typename Container, bool _reversible>
struct end_sensitive_helper<construct_rewindable<Container,_reversible> > {
   typedef Container end_source;
};

template <typename Container,
          bool _reversible=container_traits<Container>::is_bidirectional>
class construct_end_sensitive : public Container {
protected:
   construct_end_sensitive();
   ~construct_end_sensitive();

   typedef typename end_sensitive_helper<Container>::end_source end_source;
public:
   typedef iterator_range<typename Container::iterator> iterator;
   typedef iterator_range<typename Container::const_iterator> const_iterator;

   iterator begin() { return iterator(Container::begin(), end_source::end()); }
   iterator end() { return iterator(Container::end()); }
   const_iterator begin() const { return const_iterator(Container::begin(), end_source::end()); }
   const_iterator end() const { return const_iterator(Container::end()); }
};

template <typename Container>
class construct_end_sensitive<Container, true>
   : public construct_end_sensitive<Container, false> {
   typedef construct_end_sensitive<Container, false> base_t;
public:
   typedef iterator_range<typename Container::reverse_iterator> reverse_iterator;
   typedef iterator_range<typename Container::const_reverse_iterator> const_reverse_iterator;

   reverse_iterator rbegin()
   {
      typedef typename base_t::end_source end_source;
      return reverse_iterator(Container::rbegin(), end_source::rend());
   }
   reverse_iterator rend()
   {
      return reverse_iterator(Container::rend());
   }
   const_reverse_iterator rbegin() const
   {
      typedef typename base_t::end_source end_source;
      return const_reverse_iterator(Container::rbegin(), end_source::rend());
   }
   const_reverse_iterator rend() const
   {
      return const_reverse_iterator(Container::rend());
   }
};

template <typename Container, bool _reversible>
struct redirect_object_traits< construct_end_sensitive<Container,_reversible> >
   : object_traits<Container> {
   typedef Container masquerade_for;
   static const bool is_temporary=false;
};

template <typename Container>
struct default_enforce_feature<Container, rewindable> {
   typedef construct_rewindable<Container> container;
};

template <typename Container>
struct default_enforce_feature<Container, end_sensitive> {
   typedef construct_end_sensitive<Container> container;
};

template <bool on_top>
struct absorbing_feature<provide_construction<end_sensitive, on_top>, contractable> : std::true_type {};

template <typename Container, typename Feature, bool on_top>
struct default_check_container_feature<Container, provide_construction<Feature,on_top> > : std::false_type {};

template <typename Container>
struct Entire : ensure_features<Container, end_sensitive> {};

template <typename Container> inline
typename Entire<typename Concrete<Container>::type>::iterator
entire(Container& c)
{
   return ensure(c, (end_sensitive*)0).begin();
}

template <typename Container> inline
typename Entire<typename Concrete<Container>::type>::const_iterator
entire(const Container& c)
{
   return ensure(c, (end_sensitive*)0).begin();
}

template <typename Container> inline
typename Entire<typename Concrete<Container>::type>::reverse_iterator
rentire(Container& c)
{
   return ensure(c, (end_sensitive*)0).rbegin();
}

template <typename Container> inline
typename Entire<typename Concrete<Container>::type>::const_reverse_iterator
rentire(const Container& c)
{
   return ensure(c, (end_sensitive*)0).rbegin();
}

struct _reversed {};

template <typename Container,
          bool _random=container_traits<Container>::is_random>
class construct_reversed {
protected:
   Container& hidden() { return reinterpret_cast<Container&>(*this); }
   const Container& hidden() const { return reinterpret_cast<const Container&>(*this); }
public:
   typedef typename container_traits<Container>::value_type value_type;
   typedef typename container_traits<Container>::reference reference;
   typedef typename container_traits<Container>::const_reference const_reference;
   typedef typename container_traits<Container>::category container_category;

   typedef typename container_traits<Container>::reverse_iterator iterator;
   typedef typename container_traits<Container>::const_reverse_iterator const_iterator;
   typedef typename container_traits<Container>::iterator reverse_iterator;
   typedef typename container_traits<Container>::const_iterator const_reverse_iterator;

   iterator begin() { return hidden().rbegin(); }
   iterator end() { return hidden().rend(); }
   const_iterator begin() const { return hidden().rbegin(); }
   const_iterator end() const { return hidden().rend(); }
   reverse_iterator rbegin() { return hidden().begin(); }
   reverse_iterator rend() { return hidden().end(); }
   const_reverse_iterator rbegin() const { return hidden().begin(); }
   const_reverse_iterator rend() const { return hidden().end(); }

   reference front() { return hidden().back(); }
   reference back() { return hidden().front(); }
   const_reference front() const { return hidden().back(); }
   const_reference back() const { return hidden().front(); }

   int size() const { return hidden().size(); }
   int dim() const { return get_dim(hidden()); }
   bool empty() const { return hidden().empty(); }
};

template <typename Container>
class construct_reversed<Container, true>
   : public construct_reversed<Container,false> {
   typedef construct_reversed<Container,false> base_t;
public:
   typename base_t::reference operator[] (int i)
   {
      return (base_t::hidden())[this->size()-1-i];
   }

   typename base_t::const_reference operator[] (int i) const
   {
      return (base_t::hidden())[this->size()-1-i];
   }
};

template <typename Container>
struct default_check_container_feature<Container, _reversed> : std::false_type {};

template <typename Container>
struct default_enforce_feature<Container, _reversed> {
   typedef construct_reversed<Container> container;
};

template <typename Feature>
struct feature_allow_order<_reversed,Feature> : std::false_type {};

template <typename Container, bool _random>
struct redirect_object_traits< construct_reversed<Container,_random> >
   : spec_object_traits<Container> {
   typedef Container masquerade_for;
   static const bool is_temporary=false;
};

template <typename Container, bool _random, typename Feature>
struct check_container_feature<construct_reversed<Container,_random>, Feature>
   : check_container_feature<Container,Feature> {};

template <typename Container, bool _random>
struct check_container_feature<construct_reversed<Container,_random>, _reversed> : std::true_type {};

template <typename Container, bool _random, typename Features>
struct enforce_features<construct_reversed<Container,_random>, Features> {
   typedef construct_reversed<typename enforce_features<Container,Features>::container> container;
};

template <typename Container> inline
typename ensure_features<Container,_reversed>::container&
reversed(Container& c)
{
   return reinterpret_cast<typename ensure_features<Container,_reversed>::container&>(c);
}

template <typename Container> inline
const typename ensure_features<Container,_reversed>::container&
reversed(const Container& c)
{
   return reinterpret_cast<const typename ensure_features<Container,_reversed>::container&>(c);
}

template <typename Value, bool is_simple=std::is_pod<Value>::value>
class op_value_cache {
   Value* value;
   std::allocator<Value> alloc;

public:
   op_value_cache() : value(0) {}

   op_value_cache(const op_value_cache& op) : value(0) {}

   op_value_cache(typename function_argument<Value>::type arg)
   {
      value=alloc.allocate(1);
      new(value) Value(arg);
   }

   ~op_value_cache()
   {
      if (value) {
         alloc.destroy(value);
         alloc.deallocate(value,1);
      }
   }

   op_value_cache& operator= (const op_value_cache&) { return *this; }

   Value& operator= (Value arg)
   {
      if (value)
         alloc.destroy(value);
      else
         value=alloc.allocate(1);
      new(value) Value(arg);
      return *value;
   }

   Value& get() { return *value; }
   const Value& get() const { return *value; }
};

template <typename Value>
class op_value_cache<Value, true> {
   Value value;
public:
   op_value_cache() {}
   op_value_cache(const op_value_cache&) {}

   op_value_cache(typename function_argument<Value>::type arg)
      : value(arg) {}

   Value& operator= (Value arg)
   {
      value=arg;
      return value;
   }

   op_value_cache& operator= (const op_value_cache&) { return *this; }

   Value& get() { return value; }
   const Value& get() const { return value; }
};

template <typename ResultRef, bool need_proxy=!std::is_reference<ResultRef>::value>
struct arrow_helper {
   typedef std::remove_reference_t<ResultRef>* pointer;

   template <typename Iterator>
   static pointer get(const Iterator& it) { return &(*it); }
};

template <typename Result>
struct arrow_helper<Result, true> {
   class pointer {
      template <typename, bool> friend struct arrow_helper;
      typedef typename deref<Result>::type value_type;
      value_type value;

      template <typename Iterator>
      pointer(const Iterator& it) : value(*it) {}
   public:
      value_type* operator->() { return &value; }
   };

   template <typename Iterator>
   static pointer get(const Iterator& it) { return it; }
};

DeclNestedTemplateCHECK(mix_in);

template <typename Iterator, typename Operation, bool has_mixin=has_nested_mix_in<Operation>::value>
struct transform_iterator_base {
   typedef Iterator type;
};

template <typename Iterator, typename Operation>
struct transform_iterator_base<Iterator, Operation, true> {
   typedef typename Operation::template mix_in<Iterator> type;
};

template <typename Iterator, typename Operation>
class unary_transform_eval
   : public transform_iterator_base<Iterator, Operation>::type {
   typedef typename transform_iterator_base<Iterator, Operation>::type base_t;
public:
   typedef unary_helper<Iterator,Operation> helper;
   typedef typename helper::operation operation;
protected:
   operation op;
   typedef Operation op_arg_type;

   unary_transform_eval() {}

   template <typename Operation2>
   unary_transform_eval(const unary_transform_eval<typename iterator_traits<Iterator>::iterator, Operation2>& it)
      : base_t(static_cast<const typename std::remove_reference_t<decltype(it)>::base_t&>(it))
      , op(helper::create(it.op)) {}

   template <typename Operation2>
   unary_transform_eval(const unary_transform_eval<typename iterator_reversed<Iterator>::type, Operation2>& it)
      : base_t(iterator_reversed<Iterator>::reverse(it))
      , op(helper::create(it.op)) {}

   template <typename SourceIterator>
   unary_transform_eval(const SourceIterator& cur_arg, const op_arg_type& op_arg)
      : base_t(cur_arg)
      , op(helper::create(op_arg)) {}

   template <typename, typename> friend class unary_transform_eval;
public:
   typedef typename operation::result_type reference;

   reference operator* () const
   {
      return op(*helper::get(*this));
   }

   typedef typename arrow_helper<reference>::pointer pointer;
   pointer operator-> () const
   {
      return arrow_helper<reference>::get(*this);
   }
};

template <typename Iterator, typename Operation, typename IndexOperation>
class unary_transform_eval<Iterator, pair<Operation, IndexOperation> >
   : public unary_transform_eval<Iterator, Operation> {
   typedef unary_transform_eval<Iterator, Operation> base_t;
protected:
   typedef unary_helper<Iterator,IndexOperation> ihelper;
   typename ihelper::operation iop;
   typedef pair<Operation, IndexOperation> op_arg_type;

   unary_transform_eval() {}

   template <typename Operation2, typename IndexOperation2>
   unary_transform_eval(const unary_transform_eval<typename iterator_traits<Iterator>::iterator, pair<Operation2, IndexOperation2> >& it)
      : base_t(it)
      , iop(ihelper::create(it.iop)) {}

   template <typename Operation2, typename IndexOperation2>
   unary_transform_eval(const unary_transform_eval<typename iterator_reversed<Iterator>::type, pair<Operation2, IndexOperation2> >& it)
      : base_t(it)
      , iop(ihelper::create(it.op)) {}

   template <typename SourceIterator>
   unary_transform_eval(const SourceIterator& cur_arg, const op_arg_type& op_arg)
      : base_t(cur_arg, op_arg.first)
      , iop(ihelper::create(op_arg.second)) {}

   template <typename, typename> friend class unary_transform_eval;
public:
   int index() const
   {
      return iop(*ihelper::get(*this));
   }
};

template <typename Iterator, typename IndexOperation>
class unary_transform_eval<Iterator, pair<nothing, IndexOperation> >
   : public transform_iterator_base<Iterator,IndexOperation>::type {
   typedef typename transform_iterator_base<Iterator,IndexOperation>::type base_t;
protected:
   typedef unary_helper<Iterator,IndexOperation> ihelper;
   typename ihelper::operation iop;
   typedef IndexOperation op_arg_type;

   unary_transform_eval() {}

   template <typename IndexOperation2>
   unary_transform_eval(const unary_transform_eval<typename iterator_traits<Iterator>::iterator, pair<nothing, IndexOperation2> >& it)
      : base_t(static_cast<const typename std::remove_reference_t<decltype(it)>::base_t&>(it))
      , iop(ihelper::create(it.iop)) {}

   template <typename IndexOperation2>
   unary_transform_eval(const unary_transform_eval<typename iterator_reversed<Iterator>::type, pair<nothing, IndexOperation2> >& it)
      : base_t(iterator_reversed<Iterator>::reverse(it))
      , iop(ihelper::create(it.op)) {}

   template <typename SourceIterator>
   unary_transform_eval(const SourceIterator& cur_arg, const op_arg_type& op_arg)
      : base_t(cur_arg)
      , iop(ihelper::create(op_arg)) {}

   template <typename, typename> friend class unary_transform_eval;
public:
   int index() const
   {
      return iop(*ihelper::get(*this));
   }
};

template <typename Target, typename SourceIterator> inline
const typename is_derived_from_any<SourceIterator, typename const_compatible_with<Target>::type>::type&
prepare_iterator_arg(const SourceIterator& it)
{
   return it;
}

template <typename Target, typename SourceIterator> inline
typename mproject1st<const SourceIterator&, typename iterator_traits<SourceIterator>::iterator_category>::type
prepare_iterator_arg(const SourceIterator& it,
                     typename std::enable_if<(!is_derived_from_any<SourceIterator, typename const_compatible_with<Target>::type>::value &&
                                              can_construct_any<SourceIterator, typename const_compatible_with<Target>::type>::value),
                                             void**>::type=nullptr)
{
   return it;
}

template <typename SourceIterator, typename Target>
struct suitable_arg_for_iterator
   : std::enable_if<is_derived_from_any<SourceIterator, typename const_compatible_with<Target>::type>::value ||
                    can_construct_any<SourceIterator, typename const_compatible_with<Target>::type>::value> {};


template <typename Iterator, typename Operation>
class unary_transform_iterator
   : public unary_transform_eval<Iterator, Operation> {
   typedef unary_transform_eval<Iterator, Operation> base_t;
   typedef Iterator raw_it;

   template <typename, typename> friend class unary_transform_iterator;
protected:
   using typename base_t::op_arg_type;
public:
   // deref must stay here until all masquerading classes are exterminated
   typedef typename deref<std::remove_reference_t<typename base_t::reference>>::type value_type;
   typedef unary_transform_iterator<typename iterator_traits<Iterator>::iterator,
                                    typename operation_cross_const_helper<Operation>::operation>
      iterator;
   typedef unary_transform_iterator<typename iterator_traits<Iterator>::const_iterator,
                                    typename operation_cross_const_helper<Operation>::const_operation>
      const_iterator;

   unary_transform_iterator() {}

   template <typename Operation2>
   unary_transform_iterator(const unary_transform_iterator<typename iterator_traits<Iterator>::iterator, Operation2>& it)
      : base_t(it) {}

   template <typename Operation2>
   explicit unary_transform_iterator(const unary_transform_iterator<typename iterator_reversed<Iterator>::type, Operation2>& it)
      : base_t(it) {}

   template <typename SourceIterator,
             typename suitable=typename std::enable_if<std::is_default_constructible<op_arg_type>::value,
                                                       typename suitable_arg_for_iterator<SourceIterator, Iterator>::type>::type>
   unary_transform_iterator(const SourceIterator& cur_arg)
      : base_t(prepare_iterator_arg<Iterator>(cur_arg), op_arg_type()) {}

   template <typename SourceIterator,
             typename suitable=typename suitable_arg_for_iterator<SourceIterator, Iterator>::type>
   unary_transform_iterator(const SourceIterator& cur_arg, const op_arg_type& op_arg)
      : base_t(prepare_iterator_arg<Iterator>(cur_arg), op_arg) {}

   unary_transform_iterator& operator++ ()
   {
      raw_it::operator++(); return *this;
   }
   const unary_transform_iterator operator++ (int)
   {
      unary_transform_iterator copy=*this;  operator++();  return copy;
   }

   unary_transform_iterator& operator-- ()
   {
      static_assert(iterator_traits<raw_it>::is_bidirectional, "iterator is not bidirectional");
      raw_it::operator--();  return *this;
   }
   const unary_transform_iterator operator-- (int)
   {
      unary_transform_iterator copy=*this;  operator--();  return copy;
   }

   unary_transform_iterator& operator+= (int i)
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      raw_it::operator+=(i);
      return *this;
   }
   unary_transform_iterator& operator-= (int i)
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      raw_it::operator-=(i);
      return *this;
   }
   unary_transform_iterator operator+ (int i) const
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      unary_transform_iterator copy=*this;  return copy+=i;
   }
   unary_transform_iterator operator- (int i) const
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      unary_transform_iterator copy=*this; return copy-=i;
   }
   friend unary_transform_iterator operator+ (int i, const unary_transform_iterator& me)
   {
      return me+i;
   }

   template <typename Other>
   typename std::enable_if<is_derived_from_any<Other, typename iterator::raw_it, typename const_iterator::raw_it>::value,
                           typename raw_it::difference_type>::type
   operator- (const Other& it) const 
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      typedef typename is_derived_from_any<Other, typename iterator::raw_it, typename const_iterator::raw_it>::type other_raw_it;
      return static_cast<const raw_it&>(*this) - static_cast<const other_raw_it&>(it);
   }

protected:
   typename base_t::reference random_impl(int i, std::true_type) const
   {
      return this->op(raw_it::operator[](i));
   }
   typename base_t::reference random_impl(int i, std::false_type) const
   {
      return this->op(static_cast<const raw_it&>(*this) + i);
   }
public:
   typename base_t::reference operator[] (int i) const
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      return random_impl(i, bool_constant<base_t::helper::data_arg>());
   }
};

template <typename Iterator, typename Operation, typename Feature>
struct check_iterator_feature<unary_transform_iterator<Iterator, Operation>, Feature>
   : check_iterator_feature<Iterator, Feature> {};

template <typename Iterator, typename Operation>
struct check_iterator_feature<unary_transform_iterator<Iterator, Operation>, indexed> {
   static const bool value=is_instance_of<Operation, pair>::value ||
                           check_iterator_feature<Iterator,indexed>::value;
};

template <typename Iterator, typename Operation> inline
auto make_unary_transform_iterator(Iterator&& it, const Operation& op)
{
   return unary_transform_iterator<pointer2iterator_t<Iterator>, Operation>(pointer2iterator(std::forward<Iterator>(it)), op);
}

template <typename... TParams>
struct unary_transform_constructor {
   typedef typename mlist_wrap<TParams...>::type params;

   template <typename Iterator, typename Operation, typename ExpectedFeatures>
   struct defs {
      typedef typename std::conditional<is_instance_of<Operation, pair>::value,
                                        typename list_search_all<ExpectedFeatures, indexed, equivalent_features>::negative,
                                        ExpectedFeatures>::type
         needed_features;
      typedef unary_transform_iterator<Iterator, Operation> iterator;
   };
};

template <typename Container>
struct default_check_container_feature<Container, sparse>
   : check_container_feature<Container, pure_sparse> {};

template <typename Container>
struct default_check_container_feature<Container, sparse_compatible>
   : check_container_feature<Container, sparse> {};

template <typename Container>
struct default_check_container_feature<Container, pure_sparse> : std::false_type {};

template <typename Container>
struct default_check_container_feature<Container, dense> {
   static const bool value=!check_container_feature<Container, sparse>::value;
};

template <typename Container> inline
typename std::enable_if<check_container_feature<Container, sparse_compatible>::value, int>::type
get_dim(const Container& c)
{
   return c.dim();
}
template <typename Container> inline
typename std::enable_if<!check_container_feature<Container, sparse_compatible>::value, int>::type
get_dim(const Container& c)
{
   return c.size();
}

template <typename Container> inline
int total_size(const Container& c)
{
   return c.size();
}

template <typename First, typename Second, typename... Other> inline
int total_size(const First& c1, const Second& c2, const Other&... other)
{
   return c1.size() + total_size(c2, other...);
}

template <typename Container> inline
int index_within_range(const Container& c, int i)
{
   const int d=get_dim(c);
   if (i<0) i+=d;
   if (i<0 || i>=d) throw std::runtime_error("index out of range");
   return i;
}


template <typename Iterator, typename Operation>
class output_transform_iterator : public Iterator {
protected:
   Operation op;

   typedef Iterator base_t;
public:
   typedef output_iterator_tag iterator_category;
   typedef typename deref<std::remove_reference_t<typename Operation::argument_type>>::type value_type;

   output_transform_iterator() {}

   output_transform_iterator(const Iterator& cur_arg, const Operation& op_arg=Operation())
      : base_t(cur_arg)
      , op(op_arg) {}

   output_transform_iterator& operator= (typename Operation::argument_type arg)
   {
      static_cast<base_t&>(*this)=op(arg);
      return *this;
   }

   template <typename Arg>
   output_transform_iterator& operator= (const Arg& arg)
   {
      static_cast<base_t&>(*this)=op(arg);
      return *this;
   }

   output_transform_iterator& operator* () { return *this; }
   output_transform_iterator& operator++ () { return *this; }
   output_transform_iterator& operator++ (int) { return *this; }
};

template <typename Iterator, typename Operation> inline
output_transform_iterator<Iterator,Operation>
make_output_transform_iterator(Iterator it, const Operation& op)
{
   return output_transform_iterator<Iterator,Operation>(it,op);
}

struct output_transform_constructor {
   template <typename Iterator, typename Operation, typename ExpectedFeatures>
   struct defs {
      typedef ExpectedFeatures needed_features;
      typedef output_transform_iterator<Iterator,Operation> iterator;
   };
};

template <typename Iterator1, typename Iterator2, typename TParams=mlist<>>
class iterator_pair
   : public Iterator1 {
public:
   typedef Iterator1 first_type;
   typedef Iterator2 second_type;

   Iterator2 second;

   typedef typename mtagged_list_extract<TParams, FeaturesViaSecondTag>::type features_via_second;

   typedef typename least_derived_class<typename iterator_traits<Iterator1>::iterator_category,
                                        typename iterator_traits<Iterator2>::iterator_category>::type
      iterator_category;
   typedef typename iterator_traits< typename std::conditional<check_iterator_feature<Iterator1, unlimited>::value,
                                                               Iterator2, Iterator1>::type >::difference_type
      difference_type;
   typedef iterator_pair<typename iterator_traits<Iterator1>::iterator,
                         typename iterator_traits<Iterator2>::iterator, TParams>
      iterator;
   typedef iterator_pair<typename iterator_traits<Iterator1>::const_iterator,
                         typename iterator_traits<Iterator2>::const_iterator, TParams>
      const_iterator;

   iterator_pair() {}

   template <typename SourceIterator1, typename SourceIterator2,
             typename suitable1=typename suitable_arg_for_iterator<SourceIterator1, Iterator1>::type,
             typename suitable2=typename suitable_arg_for_iterator<SourceIterator2, Iterator2>::type>
   iterator_pair(const SourceIterator1& first_arg, const SourceIterator2& second_arg)
      : first_type(prepare_iterator_arg<Iterator1>(first_arg))
      , second(prepare_iterator_arg<Iterator2>(second_arg)) {}

   iterator_pair(const iterator& it)
      : first_type(static_cast<const typename iterator::first_type&>(it))
      , second(it.second) {}

   iterator_pair& operator= (const iterator& it)
   {
      first_type::operator=(static_cast<const typename iterator::first_type&>(it));
      second=it.second;
      return *this;
   }

   iterator_pair& operator++ ()
   {
      first_type::operator++(); ++second;
      return *this;
   }
   const iterator_pair operator++ (int)
   {
      iterator_pair copy=*this; operator++(); return copy;
   }

   iterator_pair& operator-- ()
   {
      static_assert(iterator_pair_traits<Iterator1, Iterator2>::is_bidirectional, "iterator is not bidirectional");
      first_type::operator--();  --this->second;
      return *this;
   }
   const iterator_pair operator-- (int)
   {
      iterator_pair copy=*this;  operator--();  return copy;
   }

   iterator_pair& operator+= (int i)
   {
      static_assert(iterator_pair_traits<Iterator1, Iterator2>::is_random, "iterator is not random-access");
      first_type::operator+=(i);  this->second+=i;
      return *this;
   }
   iterator_pair& operator-= (int i)
   {
      static_assert(iterator_pair_traits<Iterator1, Iterator2>::is_random, "iterator is not random-access");
      first_type::operator-=(i);  this->second-=i;
      return *this;
   }
   iterator_pair operator+ (int i) const
   {
      iterator_pair copy=*this; return copy+=i;
   }
   iterator_pair operator- (int i) const
   {
      iterator_pair copy=*this; return copy-=i;
   }
   friend iterator_pair operator+ (int i, const iterator_pair& it)
   {
      return it+i;
   }

private:
   template <typename IteratorPair>
   difference_type diff_impl(const IteratorPair& it, std::false_type) const
   {
      return static_cast<const first_type&>(*this)-static_cast<const typename IteratorPair::first_type&>(it);
   }
   template <typename IteratorPair>
   difference_type diff_impl(const IteratorPair& it, std::true_type) const
   {
      return second-it.second;
   }
   template <typename IteratorPair>
   bool eq_impl(const IteratorPair& it, std::false_type) const
   {
      return static_cast<const first_type&>(*this) == static_cast<const typename IteratorPair::first_type&>(it);
   }
   template <typename IteratorPair>
   bool eq_impl(const IteratorPair& it, std::true_type) const
   {
      return second==it.second;
   }
   typedef bool_constant<list_search<features_via_second, end_sensitive, absorbing_feature>::value> diff_via_second;

public:
   template <typename Other>
   typename std::enable_if<is_derived_from_any<Other, iterator, const_iterator>::value, difference_type>::type
   operator- (const Other& it) const
   {
      static_assert(iterator_pair_traits<Iterator1, Iterator2>::is_random, "iterator is not random-access");
      return diff_impl(static_cast<const typename is_derived_from_any<Other, iterator, const_iterator>::type&>(it), diff_via_second());
   }

   template <typename Other>
   typename std::enable_if<is_derived_from_any<Other, iterator, const_iterator>::value, bool>::type
   operator== (const Other& it) const
   {
      return eq_impl(static_cast<const typename is_derived_from_any<Other, iterator, const_iterator>::type&>(it), diff_via_second());
   }

   template <typename Other>
   typename std::enable_if<is_derived_from_any<Other, iterator, const_iterator>::value, bool>::type
   operator!= (const iterator& it) const
   {
      return !operator==(it);
   }

private:
   bool at_end_impl(std::false_type) const
   {
      return first_type::at_end();
   }
   bool at_end_impl(std::true_type) const
   {
      return second.at_end();
   }
   typedef bool_constant<(list_search<features_via_second, end_sensitive, absorbing_feature>::value ||
                          !check_iterator_feature<Iterator1, end_sensitive>::value)>
      at_end_via_second;
   static const bool at_end_defined= at_end_via_second::value ? check_iterator_feature<Iterator2, end_sensitive>::value
                                                              : check_iterator_feature<Iterator1, end_sensitive>::value;
public:
   bool at_end() const
   {
      static_assert(at_end_defined, "iterator not end-sensitive");
      return at_end_impl(at_end_via_second());
   }
private:
   int index_impl(std::false_type) const
   {
      return first_type::index();
   }
   int index_impl(std::true_type) const
   {
      return second.index();
   }
   typedef bool_constant<(list_search<features_via_second, indexed, absorbing_feature>::value ||
                          !check_iterator_feature<Iterator1, indexed>::value)>
      index_via_second;
   static const bool index_defined= index_via_second::value ? check_iterator_feature<Iterator2, indexed>::value
                                                            : check_iterator_feature<Iterator1, indexed>::value;
public:
   int index() const
   {
      static_assert(index_defined, "iterator not indexed");
      return index_impl(index_via_second());
   }
protected:
   typedef bool_constant<(!list_search<features_via_second, rewindable, absorbing_feature>::value)>
      rewind_first;
   static const bool rewind_defined= (check_iterator_feature<Iterator1, rewindable>::value || !rewind_first::value) &&
                                     check_iterator_feature<Iterator2, rewindable>::value;

   void rewind1(std::true_type) { first_type::rewind(); }
   void rewind1(std::false_type) {}
public:
   void rewind()
   {
      static_assert(rewind_defined, "iterator not rewindable");
      rewind1(rewind_first());
      second.rewind();
   }
protected:
   void contract1(bool, int distance_front, int, std::false_type)
   {
      std::advance(static_cast<first_type&>(*this), distance_front);
   }
   void contract1(bool renumber, int distance_front, int distance_back, std::true_type)
   {
      first_type::contract(renumber, distance_front, distance_back);
   }
   void contract2(bool, int distance_front, int, std::false_type)
   {
      std::advance(second, distance_front);
   }
   void contract2(bool renumber, int distance_front, int distance_back, std::true_type)
   {
      second.contract(renumber, distance_front, distance_back);
   }
public:
   void contract(bool renumber, int distance_front, int distance_back=0)
   {
      if (!list_search<features_via_second, contractable, equivalent_features>::value)
         contract1(renumber, distance_front, distance_back, bool_constant<check_iterator_feature<Iterator1, contractable>::value>());
      contract2(renumber, distance_front, distance_back, bool_constant<check_iterator_feature<Iterator2, contractable>::value>());
   }
};

template <typename Iterator1, typename Iterator2, typename TParams, typename Feature>
struct check_iterator_feature< iterator_pair<Iterator1, Iterator2, TParams>, Feature> {
   typedef cons<end_sensitive, indexed> usual_or_features;

   static const bool
      check1 = check_iterator_feature<Iterator1, Feature>::value,
      check2 = check_iterator_feature<Iterator2, Feature>::value,
      value = list_search<typename mtagged_list_extract<TParams, FeaturesViaSecondTag>::type, Feature, absorbing_feature>::value
              ? check2 :
              list_contains<usual_or_features, Feature>::value
              ? check1 || check2
              : check1 && check2;
};

template <typename TParams=mlist<>>
struct pair_coupler {
   typedef cons<end_sensitive, indexed> usual_or_features;

   template <typename Iterator1, typename Iterator2, typename ExpectedFeatures>
   struct defs {
      typedef typename list_search_all<ExpectedFeatures, usual_or_features, equivalent_features>::positive or_features;
      typedef typename list_search_all<ExpectedFeatures, usual_or_features, equivalent_features>::negative and_features;
      typedef typename list_search_all<Iterator1, or_features, check_iterator_feature>::positive2 first_can;
      typedef typename list_search_all<Iterator1, or_features, check_iterator_feature>::negative2 first_can_not;
      typedef typename std::conditional<check_iterator_feature<Iterator2,unlimited>::value, void, first_can_not>::type
         explicitly_via_second;
      typedef typename mlist_prepend_if<mlist_length<explicitly_via_second>::value != 0,
                                        FeaturesViaSecondTag<explicitly_via_second>, TParams>::type
         it_params;
      typedef iterator_pair<Iterator1, Iterator2, it_params> iterator;
      typedef typename std::conditional<check_iterator_feature<Iterator2, unlimited>::value,
                                        ExpectedFeatures,
                                        and_features >::type
         needed_features1;
      typedef typename std::conditional<check_iterator_feature<Iterator2, unlimited>::value,
                                        and_features,
                                        typename list_search_all<ExpectedFeatures, first_can, equivalent_features>::negative >::type
         needed_features2;
   };
};

template <typename IteratorPair, typename Operation, bool is_partial>
class binary_transform_eval
   : public transform_iterator_base<IteratorPair, Operation>::type {
   typedef typename transform_iterator_base<IteratorPair, Operation>::type base_t;
public:
   typedef binary_helper<IteratorPair, Operation> helper;
   typedef typename helper::operation operation;
protected:
   operation op;

   typedef Operation op_arg_type;

   binary_transform_eval() {}

   template <typename Operation2>
   binary_transform_eval(const binary_transform_eval<typename iterator_traits<IteratorPair>::iterator, Operation2, is_partial>& it)
      : base_t(static_cast<const typename std::remove_reference_t<decltype(it)>::base_t&>(it))
      , op(helper::create(it.op)) {}

   template <typename SourceIteratorPair>
   binary_transform_eval(const SourceIteratorPair& cur_arg, const op_arg_type& op_arg)
      : base_t(cur_arg)
      , op(helper::create(op_arg)) {}

   template <typename SourceIterator1, typename SourceIterator2>
   binary_transform_eval(const SourceIterator1& first_arg, const SourceIterator2& second_arg, const op_arg_type& op_arg)
      : base_t(first_arg, second_arg)
      , op(helper::create(op_arg)) {}

   template <typename, typename, bool> friend class binary_transform_eval;
public:
   typedef typename operation::result_type reference;

   reference operator* () const
   {
      return op(*helper::get1(*this), *helper::get2(this->second));
   }

   typedef typename arrow_helper<reference>::pointer pointer;
   pointer operator-> () const { return arrow_helper<reference>::get(*this); }
};

template <typename IteratorPair, typename Operation, typename IndexOperation, bool is_partial>
class binary_transform_eval<IteratorPair, pair<Operation, IndexOperation>, is_partial>
   : public binary_transform_eval<IteratorPair, Operation, is_partial> {
   typedef binary_transform_eval<IteratorPair, Operation, is_partial> base_t;
protected:
   typedef binary_helper<IteratorPair,IndexOperation> ihelper;
   typename ihelper::operation iop;
   typedef pair<Operation, IndexOperation> op_arg_type;

   binary_transform_eval() {}

   template <typename Operation2, typename IndexOperation2>
   binary_transform_eval(const binary_transform_eval<typename iterator_traits<IteratorPair>::iterator, pair<Operation2, IndexOperation2>, is_partial>& it)
      : base_t(it)
      , iop(ihelper::create(it.iop)) {}

   template <typename SourceIteratorPair>
   binary_transform_eval(const SourceIteratorPair& cur_arg, const op_arg_type& op_arg)
      : base_t(cur_arg, op_arg.first)
      , iop(ihelper::create(op_arg.second)) {}

   template <typename SourceIterator1, typename SourceIterator2>
   binary_transform_eval(const SourceIterator1& first_arg, const SourceIterator2& second_arg, const op_arg_type& op_arg)
      : base_t(first_arg, second_arg, op_arg.first)
      , iop(ihelper::create(op_arg.second)) {}

   template <typename, typename, bool> friend class binary_transform_eval;
public:
   int index() const
   {
      return iop(*ihelper::get1(*this), *ihelper::get2(this->second));
   }
};

template <typename IteratorPair, typename IndexOperation, bool is_partial>
class binary_transform_eval<IteratorPair, pair<nothing, IndexOperation>, is_partial>
   : public transform_iterator_base<IteratorPair,IndexOperation>::type {
   typedef typename transform_iterator_base<IteratorPair, IndexOperation>::type base_t;
protected:
   typedef binary_helper<IteratorPair,IndexOperation> ihelper;
   typename ihelper::operation iop;
   typedef IndexOperation op_arg_type;

   binary_transform_eval() {}

   template <typename IndexOperation2>
   binary_transform_eval(const binary_transform_eval<typename iterator_traits<IteratorPair>::iterator, pair<nothing, IndexOperation2>, is_partial>& it)
      : base_t(static_cast<const typename std::remove_reference_t<decltype(it)>::base_t&>(it))
      , iop(ihelper::create(it.iop)) {}

   template <typename SourceIteratorPair>
   binary_transform_eval(const SourceIteratorPair& cur_arg, const op_arg_type& op_arg)
      : base_t(cur_arg)
      , iop(ihelper::create(op_arg)) {}

   template <typename SourceIterator1, typename SourceIterator2>
   binary_transform_eval(const SourceIterator1& first_arg, const SourceIterator2& second_arg, const op_arg_type& op_arg)
      : base_t(first_arg, second_arg)
      , iop(ihelper::create(op_arg)) {}

   template <typename, typename, bool> friend class binary_transform_eval;
public:
   int index() const
   {
      return iop(*ihelper::get1(*this), *ihelper::get2(this->second));
   }
};

template <typename IteratorPair, typename Operation, bool is_partial=false>
class binary_transform_iterator
   : public binary_transform_eval<IteratorPair, Operation, is_partial> {
   typedef binary_transform_eval<IteratorPair, Operation, is_partial> base_t;
   typedef IteratorPair raw_it;

   template <typename, typename, bool> friend class binary_transform_iterator;
protected:
   using typename base_t::op_arg_type;
public:
   typedef typename deref<std::remove_reference_t<typename base_t::reference>>::type value_type;
   typedef binary_transform_iterator<typename iterator_traits<IteratorPair>::iterator,
                                     typename operation_cross_const_helper<Operation>::operation, is_partial>
      iterator;
   typedef binary_transform_iterator<typename iterator_traits<IteratorPair>::const_iterator,
                                     typename operation_cross_const_helper<Operation>::const_operation, is_partial>
      const_iterator;

   binary_transform_iterator() {}

   template <typename Operation2>
   binary_transform_iterator(const binary_transform_iterator<typename iterator_traits<IteratorPair>::iterator, Operation2, is_partial>& it)
      : base_t(it) {}

   template <typename SourceIteratorPair,
             typename suitable=typename std::enable_if<std::is_default_constructible<op_arg_type>::value,
                                                       typename suitable_arg_for_iterator<SourceIteratorPair, IteratorPair>::type>::type>
   binary_transform_iterator(const SourceIteratorPair& cur_arg)
      : base_t(prepare_iterator_arg<IteratorPair>(cur_arg), op_arg_type()) {}

   template <typename SourceIteratorPair,
             typename suitable=typename suitable_arg_for_iterator<SourceIteratorPair, IteratorPair>::type>
   binary_transform_iterator(const SourceIteratorPair& cur_arg, const op_arg_type& op_arg)
      : base_t(prepare_iterator_arg<IteratorPair>(cur_arg), op_arg) {}

   template <typename SourceIterator1, typename SourceIterator2,
             typename suitable1=typename std::enable_if<std::is_default_constructible<op_arg_type>::value,
                                                        typename suitable_arg_for_iterator<SourceIterator1, typename IteratorPair::first_type>::type>::type,
             typename suitable2=typename suitable_arg_for_iterator<SourceIterator2, typename IteratorPair::second_type>::type>
   binary_transform_iterator(const SourceIterator1& first_arg, const SourceIterator2& second_arg)
      : base_t(prepare_iterator_arg<typename IteratorPair::first_type>(first_arg),
               prepare_iterator_arg<typename IteratorPair::second_type>(second_arg),
               op_arg_type()) {}

   template <typename SourceIterator1, typename SourceIterator2,
             typename suitable1=typename suitable_arg_for_iterator<SourceIterator1, typename IteratorPair::first_type>::type,
             typename suitable2=typename suitable_arg_for_iterator<SourceIterator2, typename IteratorPair::second_type>::type>
   binary_transform_iterator(const SourceIterator1& first_arg, const SourceIterator2& second_arg, const op_arg_type& op_arg)
      : base_t(prepare_iterator_arg<typename IteratorPair::first_type>(first_arg),
               prepare_iterator_arg<typename IteratorPair::second_type>(second_arg),
               op_arg) {}

   binary_transform_iterator& operator++ ()
   {
      raw_it::operator++(); return *this;
   }
   const binary_transform_iterator operator++ (int)
   {
      binary_transform_iterator copy=*this;  operator++();  return copy;
   }

   binary_transform_iterator& operator-- ()
   {
      static_assert(iterator_traits<raw_it>::is_bidirectional, "iterator is not bidirectional");
      raw_it::operator--();
      return *this;
   }
   const binary_transform_iterator operator-- (int)
   {
      binary_transform_iterator copy=*this;  operator--();  return copy;
   }

   binary_transform_iterator& operator+= (int i)
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      raw_it::operator+=(i);
      return *this;
   }
   binary_transform_iterator& operator-= (int i)
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      raw_it::operator-=(i);
      return *this;
   }
   binary_transform_iterator operator+ (int i) const
   {
      binary_transform_iterator copy=*this;  return copy+=i;
   }
   binary_transform_iterator operator- (int i) const
   {
      binary_transform_iterator copy=*this;  return copy-=i;
   }
   friend binary_transform_iterator operator+ (int i, const binary_transform_iterator& it)
   {
      return it+i;
   }

   template <typename Other>
   typename std::enable_if<is_derived_from_any<Other, typename iterator::raw_it, typename const_iterator::raw_it>::value,
                           typename raw_it::difference_type>::type
   operator- (const Other& it) const
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      typedef typename is_derived_from_any<Other, typename iterator::raw_it, typename const_iterator::raw_it>::type other_raw_it;
      return static_cast<const raw_it&>(*this) - static_cast<const other_raw_it&>(it);
   }

protected:
   typename base_t::reference random_impl(int i, std::true_type, std::true_type) const
   {
      return this->op(raw_it::operator[](i), this->second[i]);
   }
   typename base_t::reference random_impl(int i, std::true_type, std::false_type) const
   {
      return this->op(raw_it::operator[](i), this->second+i);
   }
   typename base_t::reference random_impl(int i, std::false_type, std::true_type) const
   {
      return this->op(static_cast<const typename raw_it::first_type&>(*this)+i, this->second[i]);
   }
   typename base_t::reference random_impl(int i, std::false_type, std::false_type) const
   {
      return this->op(static_cast<const typename raw_it::first_type&>(*this)+i, this->second+i);
   }
public:
   typename raw_it::reference operator[] (int i) const
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      return random_impl(i, bool_constant<base_t::helper::first_data_arg>(), bool_constant<base_t::helper::second_data_arg>());
   }
};

template <typename IteratorPair, typename Operation, bool is_partial, typename Feature>
struct check_iterator_feature<binary_transform_iterator<IteratorPair, Operation, is_partial>, Feature>
   : check_iterator_feature<IteratorPair,Feature> {};

template <typename IteratorPair, typename Operation, bool is_partial>
struct check_iterator_feature<binary_transform_iterator<IteratorPair, Operation, is_partial>, indexed> {
   static const bool value=is_instance_of<Operation, pair>::value ||
                           check_iterator_feature<IteratorPair, indexed>::value;
};

template <typename Iterator>
struct has_partial_state : std::false_type {};

template <typename... TParams>
struct binary_transform_constructor {
   typedef typename mlist_wrap<TParams...>::type params;

   template <typename IteratorPair, typename Operation, typename ExpectedFeatures>
   struct defs {
      static const bool is_partially_defined = tagged_list_extract_integral<params, PartiallyDefinedTag>(has_partial_state<IteratorPair>::value);

      typedef typename std::conditional<is_instance_of<Operation, pair>::value,
                                        typename list_search_all<ExpectedFeatures, indexed, equivalent_features>::negative,
                                        ExpectedFeatures >::type
         needed_pair_features;
      typedef void needed_features1;
      typedef void needed_features2;
      typedef binary_transform_iterator<IteratorPair, Operation, is_partially_defined> iterator;
   };
};

template <typename Iterator1, typename Iterator2, typename Operation> inline
auto make_binary_transform_iterator(Iterator1&& first, Iterator2&& second, const Operation& op)
{
   return binary_transform_iterator<iterator_pair<pointer2iterator_t<Iterator1>, pointer2iterator_t<Iterator2>>, Operation>
      (pointer2iterator(std::forward<Iterator1>(first)), pointer2iterator(std::forward<Iterator2>(second)), op);
}

} // end namespace pm

namespace polymake {
   using pm::array2container;
   using pm::Entire;
   using pm::entire;
   using pm::BuildUnary;
   using pm::BuildBinary;
   using pm::BuildUnaryIt;
   using pm::BuildBinaryIt;
   using pm::make_unary_transform_iterator;
   using pm::make_binary_transform_iterator;
   using pm::make_output_transform_iterator;
   using pm::as_iterator_range;
   using pm::rewindable;
   using pm::reversed;
   using pm::black_hole;
   using pm::inserter;
   using pm::allow_conversion;
} // end namespace polymake

#endif // POLYMAKE_INTERNAL_ITERATORS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
