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

template <typename Iterator, bool is_bidir=iterator_category_booleans<Iterator>::is_bidirectional>
struct default_iterator_reversed {
   using type = void;
};

template <typename Iterator>
struct default_iterator_reversed<Iterator, true> {
   using type = std::reverse_iterator<Iterator>;
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
using const_compatible_with = typename mlist_remove_duplicates< mlist<Iterator, typename iterator_traits<Iterator>::iterator> >::type;

template <typename Source, typename Iterator>
using is_const_compatible_with = is_among<pure_type_t<Source>, const_compatible_with<Iterator>>;

template <typename Iterator1, typename Iterator2>
using are_comparable_iterators
   = is_among<Iterator2, typename iterator_traits<Iterator1>::iterator, typename iterator_traits<Iterator1>::const_iterator>;


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

template <typename Container>
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
struct default_check_iterator_feature<Iterator, unlimited>
   : bool_constant<!iterator_traits<Iterator>::is_forward> {};

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
   : bool_constant<!on_top && feature_allow_order<Feature_before, Feature_after>::value> {};

template <typename Feature_before, typename Feature_after, bool on_top>
struct feature_allow_order< Feature_before, provide_construction<Feature_after, on_top> >
   : bool_constant<on_top || feature_allow_order<Feature_before, Feature_after>::value> {};

template <typename Feature_before, bool on_top_before, typename Feature_after, bool on_top_after>
struct feature_allow_order< provide_construction<Feature_before, on_top_before>, provide_construction<Feature_after, on_top_after> >
   : bool_constant<(on_top_before < on_top_after || (on_top_before==on_top_after && feature_allow_order<Feature_before, Feature_after>::value))> {};

template <typename Feature1, typename Feature2>
struct absorbing_feature
   : is_derived_from<Feature1, Feature2> {};

template <typename Feature1, bool on_top1, typename Feature2>
struct absorbing_feature< provide_construction<Feature1, on_top1>, Feature2>
   : absorbing_feature<Feature1, Feature2> {};

template <typename Feature1, bool on_top1, typename Feature2, bool on_top2>
struct absorbing_feature< provide_construction<Feature1, on_top1>, provide_construction<Feature2, on_top2> > {
   static constexpr bool value= on_top1>=on_top2 && is_derived_from<Feature1, Feature2>::value;
};

template <typename Feature1, typename Feature2>
struct equivalent_features
   : std::is_same<Feature1, Feature2> {};

template <typename Feature, bool on_top>
struct equivalent_features< provide_construction<Feature, on_top>, Feature >
   : std::true_type {};

template <typename Feature, bool on_top>
struct equivalent_features< Feature, provide_construction<Feature, on_top> >
   : std::true_type {};

template <typename Iterator>
using can_subtract_iterators
   = bool_constant<!check_iterator_feature<Iterator, unlimited>::value && iterator_traits<Iterator>::is_random>;

template <typename Iterator>
struct accompanying_iterator {
   typedef Iterator type;

   static void assign(type& it, const type& other) { it=other;}

   static void advance(type& it, const type&, Int n) { std::advance(it, n); }
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
   void contract1(bool, Int distance_front, Int, std::false_type)
   {
      std::advance(static_cast<base_t&>(*this), distance_front);
   }
   void contract1(bool renumber, Int distance_front, Int distance_back, std::true_type)
   {
      base_t::contract(renumber, distance_front, distance_back);
   }
public:
   void contract(bool renumber, Int distance_front, Int distance_back=0)
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
             typename=std::enable_if_t<is_const_compatible_with<SourceIterator1, Iterator>::value &&
                                       is_derived_from_any<SourceIterator2, const_compatible_with<end_type>>::value>>
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

   template <typename SourceIterator, typename=std::enable_if_t<is_const_compatible_with<SourceIterator, Iterator>::value>>
   iterator_range& operator= (const SourceIterator& cur_arg)
   {
      static_cast<base_t&>(*this) = cur_arg;
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

   iterator_range& operator+= (Int i)
   {
      static_assert(iterator_traits<base_t>::is_random, "iterator is not random-access");
      base_t::operator+=(i);
      return *this;
   }
   iterator_range& operator-= (Int i)
   {
      static_assert(iterator_traits<base_t>::is_random, "iterator is not random-access");
      base_t::operator-=(i);
      return *this;
   }

   iterator_range operator+ (Int i) const
   {
      static_assert(iterator_traits<base_t>::is_random, "iterator is not random-access");
      return iterator_range(static_cast<const base_t&>(*this)+i, end);
   }
   iterator_range operator- (Int i) const
   {
      static_assert(iterator_traits<base_t>::is_random, "iterator is not random-access");
      return iterator_range(static_cast<const base_t&>(*this)-i, end);
   }
   friend iterator_range operator+ (Int i, const iterator_range& me)
   {
      static_assert(iterator_traits<base_t>::is_random, "iterator is not random-access");
      return iterator_range(static_cast<const base_t&>(me)+i, me.end);
   }

   template <typename Other>
   std::enable_if_t<is_derived_from_any<Other, typename iterator::base_t, typename const_iterator::base_t>::value, typename base_t::difference_type>
   operator- (const Other& other) const
   {
      static_assert(iterator_traits<base_t>::is_random, "iterator is not random-access");
      using other_base_t = typename is_derived_from_any<Other, typename iterator::base_t, typename const_iterator::base_t>::match;
      return static_cast<const base_t&>(*this) - static_cast<const other_base_t&>(other);
   }
private:
   void contract1_impl(bool, Int distance_front, Int, std::false_type)
   {
      std::advance(static_cast<base_t&>(*this), distance_front);
   }
   void contract1_impl(bool renumber, Int distance_front, Int distance_back, std::true_type)
   {
      base_t::contract(renumber, distance_front, distance_back);
   }
public:
   void contract(bool renumber, Int distance_front, Int distance_back = 0)
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

template <typename Iterator>
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
      static constexpr int value = iterator_preserved ? int(is_manip) : int(is_opaque);
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
   using category = typename iterator_traits<typename Container::iterator>::iterator_category;
};

template <typename Container>
struct container_category_traits<Container, true> {
   using category = typename Container::container_category;
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
                      (std::is_convertible<typename Container::value_type, Element>::value || is_explicitly_convertible_to<typename Container::value_type, Element>::value))) > {};

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

template <typename Iterator>
Int count_it(Iterator src)
{
   Int cnt = 0;
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

   ptr_wrapper(pointer cur_arg = nullptr) : cur(cur_arg) {}
   ptr_wrapper(const iterator& it) : cur(it.cur) {}

   ptr_wrapper& operator= (pointer cur_arg) { cur=cur_arg; return *this; }
   ptr_wrapper& operator= (const iterator& it) { cur=it.cur; return *this; }

   reference operator* () const { return *cur; }
   pointer operator-> () const { return cur; }
   reference operator[] (Int i) const { return cur[is_reversed ? -i : i]; }

   ptr_wrapper& operator++ () { is_reversed ? --cur : ++cur; return *this; }
   ptr_wrapper& operator-- () { is_reversed ? ++cur : --cur; return *this; }
   const ptr_wrapper operator++ (int) { ptr_wrapper copy=*this; operator++(); return copy; }
   const ptr_wrapper operator-- (int) { ptr_wrapper copy=*this; operator--(); return copy; }
   ptr_wrapper& operator+= (Int i) { is_reversed ? cur-=i : cur+=i; return *this; }
   ptr_wrapper& operator-= (Int i) { is_reversed ? cur+=i : cur-=i; return *this; }
   ptr_wrapper operator+ (Int i) const { return ptr_wrapper(is_reversed ? cur-i : cur+i); }
   ptr_wrapper operator- (Int i) const { return ptr_wrapper(is_reversed ? cur+i : cur-i); }
   friend ptr_wrapper operator+ (Int i, const ptr_wrapper& p) { return p+i; }

   template <typename Other>
   std::enable_if_t<is_derived_from_any<Other, iterator, const_iterator>::value, ptrdiff_t>
   operator- (const Other& other) const
   {
      const typename is_derived_from_any<Other, iterator, const_iterator>::match& other_it = other;
      return is_reversed ? other_it.cur - cur : cur - other_it.cur;
   }

   template <typename Other>
   std::enable_if_t<is_derived_from_any<Other, iterator, const_iterator>::value, bool>
   operator== (const Other& other) const
   {
      const typename is_derived_from_any<Other, iterator, const_iterator>::match& other_it = other;
      return cur == other_it.cur;
   }

   template <typename Other>
   std::enable_if_t<is_derived_from_any<Other, iterator, const_iterator>::value, bool>
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

   bool operator== (const T* other) const { return cur == other; }
   bool operator!= (const T* other) const { return cur != other; }
   bool operator< (const T* other) const { return cur < other; }
   bool operator> (const T* other) const { return cur > other; }
   bool operator<= (const T* other) const { return cur <= other; }
   bool operator>= (const T* other) const { return cur >= other; }

   friend bool operator== (const T* other, const ptr_wrapper& me) { return me == other; }
   friend bool operator!= (const T* other, const ptr_wrapper& me) { return me != other; }
   friend bool operator< (const T* other, const ptr_wrapper& me) { return me > other; }
   friend bool operator> (const T* other, const ptr_wrapper& me) { return me < other; }
   friend bool operator<= (const T* other, const ptr_wrapper& me) { return me >= other; }
   friend bool operator>= (const T* other, const ptr_wrapper& me) { return me <= other; }

   ptrdiff_t operator- (const T* other) const { return cur - other; }
   friend ptrdiff_t operator- (const T* other, const ptr_wrapper& me) { return other - me.cur; }
protected:
   pointer cur;
};

template <typename Iterator>
struct pointer_as_iterator {
   using type = Iterator;
};

template <typename T>
struct pointer_as_iterator<T*> {
   using type = ptr_wrapper<T, false>;
};

template <typename Iterator>
using pointer2iterator_t = typename pointer_as_iterator<pure_type_t<Iterator>>::type;

template <typename Iterator>
Iterator&& pointer2iterator(Iterator&& it) { return std::forward<Iterator>(it); }

template <typename T>
ptr_wrapper<T, false> pointer2iterator(T* ptr) { return ptr; }

template <typename Iterator>
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
      return static_cast<Top*>(this)->get_data()+static_cast<const Top*>(this)->size();
   }
   typename base_t::const_iterator begin() const
   {
      return static_cast<const Top*>(this)->get_data();
   }
   typename base_t::const_iterator end() const
   {
      return static_cast<const Top*>(this)->get_data()+static_cast<const Top*>(this)->size();
   }

   typename base_t::reverse_iterator rbegin()
   {
      return static_cast<Top*>(this)->get_data()+static_cast<const Top*>(this)->size()-1;
   }
   typename base_t::reverse_iterator rend()
   {
      return static_cast<Top*>(this)->get_data()-1;
   }
   typename base_t::const_reverse_iterator rbegin() const
   {
      return static_cast<const Top*>(this)->get_data()+static_cast<const Top*>(this)->size()-1;
   }
   typename base_t::const_reverse_iterator rend() const
   {
      return static_cast<const Top*>(this)->get_data()-1;
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
   typename base_t::reference operator[] (Int i)
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
   typename base_t::const_reference operator[] (Int i) const
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
   Int size() const { return data.size(); }
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

template <typename E>
struct spec_object_traits< array_traits<E> >
   : spec_object_traits<is_container> {};

template <typename E, size_t Tsize>
struct spec_object_traits< E[Tsize] >
   : spec_object_traits<is_opaque> {};

template <typename FeatureList1, typename FeatureList2>
using mix_features
   = mlist_concat<typename mlist_match_all<FeatureList1, FeatureList2, absorbing_feature>::complement2,
                  typename mlist_match_all<FeatureList2, FeatureList1, absorbing_feature>::complement2,
                  typename mlist_intersection<FeatureList1, FeatureList2>::type>;

template <typename FeatureList1, typename FeatureList2>
using toggle_features
   = mlist_concat<typename mlist_match_all<FeatureList1, FeatureList2, equivalent_features>::complement,
                  typename mlist_match_all<FeatureList1, FeatureList2, equivalent_features>::complement2>;

template <typename Container, int kind=object_classifier::what_is<Container>::value>
struct enforce_feature_helper {
   using must_enforce_features = mlist<>;
   using can_enforce_features = mlist<>;
   using cannot_enforce_features = mlist<>;
};

template <typename Container>
struct enforce_feature_helper<Container, object_classifier::is_manip> {
   using must_enforce_features
      = typename mlist_match_all<typename Container::expected_features,
                                 typename Container::must_enforce_features, absorbing_feature>::complement2;
   using can_enforce_features = typename Container::can_enforce_features;
   using cannot_enforce_features = typename Container::cannot_enforce_features;
};

struct checked_via_iterator {};

// to be specialized on the second parameter only
template <typename Container, typename Feature>
struct default_check_container_feature
   : bool_constant<(check_iterator_feature<typename container_traits<Container>::iterator, Feature>::value &&
                    mlist_is_empty<typename mlist_match_all<Feature, typename enforce_feature_helper<Container>::must_enforce_features, absorbing_feature>::type>::value)>
   , checked_via_iterator {};

// can be specialized either on the first parameter or on both
template <typename Container, typename Feature>
struct check_container_feature
   : default_check_container_feature<Container, Feature> {};

template <typename ContainerRef, typename Feature>
using check_container_ref_feature
   = check_container_feature<typename deref<ContainerRef>::type, Feature>;

template <typename Feature, typename Container>
struct is_iterator_feature
   : is_derived_from<default_check_container_feature<Container, Feature>, checked_via_iterator> {};

template <typename Feature, typename Container>
struct is_iterator_feature<provide_construction<Feature, false>, Container> 
   : is_iterator_feature<Feature, Container> {};

template <typename Features>
using filter_iterator_features 
   = mlist_match_all<Features, array_traits<char>, is_iterator_feature>;

template <typename Features>
struct reorder_features {
   // 'int' here serves just as some inexisting feature
   using normal_features = typename mlist_match_all<Features, int, feature_allow_order>::type;
   using always_last_features = typename mlist_match_all<Features, int, feature_allow_order>::complement;
   using non_iterator_features_first = typename mlist_concat< typename filter_iterator_features<normal_features>::complement,
                                                              typename filter_iterator_features<normal_features>::type >::type;
   using type = typename mlist_concat< typename mlist_sort<non_iterator_features_first, feature_allow_order>::type,
                                       always_last_features >::type;
};

// Provides a construction (masquerading Container) that will have a desired feature.
// Must be specialized for each enforcible feature.
template <typename Container, typename Feature>
struct default_enforce_feature;

// Can be specialized for some container classes. Handles exactly one missing feature.
template <typename Container, typename Feature>
struct enforce_feature {
   using container = typename default_enforce_feature<Container, Feature>::container;
};

// Can be specialized for various container families (according to object_classifier::what_is).
template <typename Container, typename Features, int kind>
struct default_enforce_features
   : enforce_feature<Container, Features> {};

// Can be specialized for some container classes. Handles a list of missing features
template <typename Container, typename Features>
struct enforce_features
   : default_enforce_features<Container, Features, object_classifier::what_is<Container>::value> {};

template <typename Container>
struct default_enforce_feature<Container, mlist<>> {
   using container = Container;
};

template <typename Container, typename Feature, bool on_top>
struct default_check_container_feature<Container, provide_construction<Feature, on_top> >
   : std::false_type {};

template <typename Container, typename Feature, bool on_top>
struct default_enforce_feature<Container, provide_construction<Feature, on_top>>
   : enforce_feature<Container, Feature> {};

template <typename Container, typename Lacking>
struct enforce_lacking_features_helper
   : enforce_features<Container, Lacking> {};

template <typename Container>
struct enforce_lacking_features_helper<Container, mlist<>> {
   using container = Container;
};

template <typename Container, typename Features>
struct enforce_lacking_features {
   using lacking = typename mlist_match_all<Container, Features, check_container_feature>::complement2;
   using container = typename enforce_lacking_features_helper<Container, lacking>::container;
};

template <typename Container, typename Feature, typename... MoreFeatures>
struct default_enforce_features<Container, mlist<Feature, MoreFeatures...>, object_classifier::is_opaque> {
   using needed_features = typename reorder_features<mlist<Feature, MoreFeatures...>>::type;
   using container = typename enforce_lacking_features<typename enforce_feature<Container, typename mlist_head<needed_features>::type>::container,
                                                       typename mlist_tail<needed_features>::type>::container;
};

template <typename Container, typename Features>
class feature_collector
   : public enforce_lacking_features<Container, Features>::container {
protected:
   feature_collector();
   ~feature_collector();
};

template <typename Container, typename Features>
struct redirect_object_traits< feature_collector<Container, Features> >
   : object_traits<Container> {
   using masquerade_for = Container;
   static constexpr bool is_temporary = false;
};

template <typename Container, typename ProvidedFeatures, typename Feature>
struct check_container_feature<feature_collector<Container, ProvidedFeatures>, Feature>
   : mlist_or< check_container_feature<Container, Feature>,
               mlist_contains<ProvidedFeatures, Feature, absorbing_feature> > {};

template <typename Container, typename Features>
struct ensure_features_helper {
   using container = typename inherit_const<feature_collector<typename deref<Container>::type, Features>, Container>::type;
};

template <typename Container, typename ProvidedFeatures, typename Features>
struct ensure_features_helper<feature_collector<Container, ProvidedFeatures>, Features>
   : ensure_features_helper<Container, typename mix_features<ProvidedFeatures, Features>::type> {};

template <typename Container, typename ProvidedFeatures, typename Features>
struct ensure_features_helper<const feature_collector<Container, ProvidedFeatures>, Features>
   : ensure_features_helper<const Container, typename mix_features<ProvidedFeatures, Features>::type> {};

template <typename Container, typename Features>
struct ensure_features
   : ensure_features_helper<Container, Features>
   , container_traits<typename ensure_features_helper<Container, Features>::container> {};

template <typename Container, typename... Features>
decltype(auto) ensure(Container&& c, Features...)
{
   using result = typename ensure_features<std::remove_reference_t<Container>, typename mlist_wrap<Features...>::type>::container;
   return reinterpret_cast<inherit_reference_t<result, Container&&>>(c);
}

template <typename Container>
Container&& ensure(Container&& c)
{
   return std::forward<Container>(c);
}


// not to be used in for-loops and other contexts prolonging the life of the iterator beyond the next sequence point
template <typename... MoreFeatures, typename Container>
auto entire_range(Container& c)
{
   return ensure(c, typename mix_features<end_sensitive, typename mlist_wrap<MoreFeatures...>::type>::type()).begin();
}

template <typename... MoreFeatures, typename Container>
auto entire_range(const Container& c)
{
   return ensure(c, typename mix_features<end_sensitive, typename mlist_wrap<MoreFeatures...>::type>::type()).begin();
}


template <typename E, typename Features>
struct ensure_features<std::initializer_list<E>, Features>
   : ensure_features<const initializer_list_adapter<E>, Features> {};

template <typename E, typename Features>
struct ensure_features<const std::initializer_list<E>, Features>
   : ensure_features<const initializer_list_adapter<E>, Features> {};

template <typename E, typename... Features>
typename ensure_features<std::initializer_list<E>, typename mlist_wrap<Features...>::type>::type
ensure(std::initializer_list<E>& l, Features...)
{
   return typename ensure_features<std::initializer_list<E>, typename mlist_wrap<Features...>::type>::type(l);
}

template <typename E, typename... Features>
typename ensure_features<std::initializer_list<E>, typename mlist_wrap<Features...>::type>::type
ensure(const std::initializer_list<E>& l, Features...)
{
   return typename ensure_features<std::initializer_list<E>, typename mlist_wrap<Features...>::type>::type(l);
}


template <typename ContainerRef, typename Features>
struct masquerade_add_features
   : inherit_ref<typename ensure_features<typename deref<ContainerRef>::minus_ref, Features>::container, ContainerRef> {};

template <typename ContainerRef, typename Features>
struct deref< masquerade_add_features<ContainerRef,Features> >
   : deref< typename masquerade_add_features<ContainerRef,Features>::type > {
   using  plus_const = masquerade_add_features<typename attrib<ContainerRef>::plus_const, Features>;
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
template <typename> class ContainerRefTag;
template <typename> class Container1RefTag;
template <typename> class Container2RefTag;
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

template <typename ContainerRef, typename Features>
struct extract_iterator_with_features {
   using type = typename ensure_features<std::remove_reference_t<ContainerRef>, muntag_t<Features>>::iterator;
};

template <typename ContainerRef, typename Features>
struct extract_const_iterator_with_features {
   using type = typename ensure_features<std::remove_reference_t<ContainerRef>, muntag_t<Features>>::const_iterator;
};

template <typename ContainerRef, typename Features>
struct extract_reverse_iterator_with_features {
   using type = typename ensure_features<std::remove_reference_t<ContainerRef>, muntag_t<Features>>::reverse_iterator;
};

template <typename ContainerRef, typename Features>
struct extract_const_reverse_iterator_with_features {
   using type = typename ensure_features<std::remove_reference_t<ContainerRef>, muntag_t<Features>>::const_reverse_iterator;
};

template <typename ContainerRef>
using extract_iterator = extract_iterator_with_features<ContainerRef, mlist<>>;

template <typename ContainerRef>
using extract_const_iterator = extract_const_iterator_with_features<ContainerRef, mlist<>>;

template <typename ContainerRef>
using extract_reverse_iterator = extract_reverse_iterator_with_features<ContainerRef, mlist<>>;

template <typename ContainerRef>
using extract_const_reverse_iterator = extract_const_reverse_iterator_with_features<ContainerRef, mlist<>>;

template <typename ContainerRef>
struct extract_category {
   using type = typename container_traits<ContainerRef>::category;
};

template <typename Params, template <typename> class RefTag, template <typename> class NoRefTag, typename Default=void>
using extract_container_ref
   = mtagged_list_extract<Params, RefTag,
                          std::add_lvalue_reference_t<typename mtagged_list_extract<Params, NoRefTag, Default>::type>>;

template <typename Top, typename Params, bool has_hidden=mtagged_list_extract<Params, HiddenTag>::is_specified>
class manip_container_top
   : public manip_container_base {
public:
   using hidden_type = void;
   using expected_features = typename mtagged_list_extract<Params, ExpectedFeaturesTag, mlist<>>::type;
   using manip_top_type = Top;
   using must_enforce_features = mlist<>;
   using can_enforce_features = mlist<>;
   using cannot_enforce_features = mlist<>;

   Top& manip_top() { return *static_cast<Top*>(this); }
   const Top& manip_top() const { return *static_cast<const Top*>(this); }
};

template <typename Container, typename ProvidedFeatures, typename Params>
class manip_container_top<manip_feature_collector<Container, ProvidedFeatures>, Params, false>
   : public manip_container_base {
public:
   using hidden_type = void;
   using expected_features = typename mix_features<typename mtagged_list_extract<Params, ExpectedFeaturesTag, mlist<>>::type, ProvidedFeatures>::type;
   using manip_top_type = typename Container::manip_top_type;
   using must_enforce_features = mlist<>;
   using can_enforce_features = typename Container::can_enforce_features;
   using cannot_enforce_features = typename Container::cannot_enforce_features;

   manip_top_type& manip_top()
   {
      return *static_cast<manip_top_type*>(reinterpret_cast<Container*>(this));
   }
   const manip_top_type& manip_top() const
   {
      return *static_cast<const manip_top_type*>(reinterpret_cast<const Container*>(this));
   }
};

template <typename Top, typename Hidden>
struct manip_container_hidden_helper {
   using type = Hidden;
};

template <typename Top>
struct manip_container_hidden_helper<Top, std::true_type>
   : mget_template_parameter<Top, 0> {};

template <typename Top, typename Params,
          bool is_binary=(mtagged_list_extract<Params, Container1Tag>::is_specified ||
                          mtagged_list_extract<Params, Container2Tag>::is_specified ||
                          mtagged_list_extract<Params, Container1RefTag>::is_specified ||
                          mtagged_list_extract<Params, Container2RefTag>::is_specified)>
class manip_container_hidden_defaults {
public:
   using hidden_type = typename manip_container_hidden_helper<Top, typename mtagged_list_extract<Params, HiddenTag>::type>::type;
   using container_ref_raw = typename extract_container_ref<Params, ContainerRefTag, ContainerTag, hidden_type>::type;
   using container = typename deref<container_ref_raw>::minus_ref;

   container& get_container()
   {
      return reinterpret_cast<container&>(static_cast<manip_container_top<Top, Params, true>*>(this)->manip_top());
   }
   const container& get_container() const
   {
      return reinterpret_cast<const container&>(static_cast<const manip_container_top<Top, Params, true>*>(this)->manip_top());
   }
};

template <typename Top, typename Params>
class manip_container_hidden_defaults<Top, Params, true> {
public:
   using hidden_type = typename manip_container_hidden_helper<Top, typename mtagged_list_extract<Params, HiddenTag>::type>::type;
   using container1_ref_raw = typename extract_container_ref<Params, Container1RefTag, Container1Tag, hidden_type>::type;
   using container2_ref_raw = typename extract_container_ref<Params, Container2RefTag, Container2Tag, hidden_type>::type;
   using container1 = typename deref<container1_ref_raw>::minus_ref;
   using container2 = typename deref<container2_ref_raw>::minus_ref;

   container1& get_container1()
   {
      return reinterpret_cast<container1&>(static_cast<manip_container_top<Top, Params, true>*>(this)->manip_top());
   };
   const container1& get_container1() const
   {
      return reinterpret_cast<const container1&>(static_cast<const manip_container_top<Top, Params, true>*>(this)->manip_top());
   }
   container2& get_container2()
   {
      return reinterpret_cast<container2&>(static_cast<manip_container_top<Top, Params, true>*>(this)->manip_top());
   }
   const container2& get_container2() const
   {
      return reinterpret_cast<const container2&>(static_cast<const manip_container_top<Top, Params, true>*>(this)->manip_top());
   }
};

template <typename Top, typename Params>
class manip_container_top<Top, Params, true>
   : public manip_container_top<Top, Params, false>
   , public manip_container_hidden_defaults<Top, Params> {
protected:
   manip_container_top() = delete;
   ~manip_container_top() = delete;
public:
   using typename manip_container_hidden_defaults<Top, Params>::hidden_type;

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
   manip_feature_collector() = delete;
   ~manip_feature_collector() = delete;
};

template <typename Container, typename Features>
struct manip_feature_collector_helper {
   using container = manip_feature_collector<Container, Features>;
};
template <typename Container, typename PrevFeatures, typename Features>
struct manip_feature_collector_helper<manip_feature_collector<Container, PrevFeatures>, Features> {
   using container = manip_feature_collector<Container, typename mlist_concat<PrevFeatures, Features>::type>;
};
template <typename Container>
struct manip_feature_collector_helper<Container, mlist<>> {
   using container = Container;
};
template <typename Container, typename PrevFeatures>    // resolving ambiguity
struct manip_feature_collector_helper<manip_feature_collector<Container, PrevFeatures>, mlist<>> {
   using container = manip_feature_collector<Container, PrevFeatures>;
};

template <typename Container, typename Features>
struct default_enforce_features<Container, Features, object_classifier::is_manip> {
   using after1 = typename mlist_match_all<Features, int, feature_allow_order>::complement;
   using not_last = typename mlist_match_all<Features, int, feature_allow_order>::type;
   using after2 = typename mlist_match_all<not_last, typename Container::cannot_enforce_features, absorbing_feature>::type;
   using not_after = typename mlist_match_all<not_last, typename Container::cannot_enforce_features, absorbing_feature>::complement;
   using via_manip1 = typename mlist_match_all<not_after, typename Container::can_enforce_features, equivalent_features>::type;
   using via_manip2 = typename filter_iterator_features<typename mlist_match_all<not_after, typename Container::can_enforce_features, equivalent_features>::complement>::type;
   using via_manip = typename mlist_concat<via_manip1, via_manip2>::type;
   using before = typename mlist_match_all<not_after, via_manip, equivalent_features>::complement;
   using after = typename mlist_concat<after2, after1>::type;

   using enforced_before = typename default_enforce_features<Container, before, object_classifier::is_opaque>::container;
   using enforced_via_manip = typename manip_feature_collector_helper<enforced_before, via_manip>::container;
   using container = typename default_enforce_features<enforced_via_manip, after, object_classifier::is_opaque>::container;
};

template <typename Container,
          bool is_bidir=container_traits<Container>::is_bidirectional>
class construct_rewindable
   : public std::enable_if<container_traits<Container>::is_forward, Container>::type {
protected:
   construct_rewindable() = delete;
   ~construct_rewindable() = delete;
public:
   using iterator = rewindable_iterator<typename Container::iterator>;
   using const_iterator = rewindable_iterator<typename Container::const_iterator>;

   iterator begin() { return Container::begin(); }
   iterator end() { return Container::end(); }
   const_iterator begin() const { return Container::begin(); }
   const_iterator end() const { return Container::end(); }
};

template <typename Container>
class construct_rewindable<Container, true>
   : public construct_rewindable<Container, false> {
public:
   using reverse_iterator = rewindable_iterator<typename Container::reverse_iterator>;
   using const_reverse_iterator = rewindable_iterator<typename Container::const_reverse_iterator>;

   reverse_iterator rbegin() { return Container::rbegin(); }
   reverse_iterator rend() { return Container::rend(); }
   const_reverse_iterator rbegin() const { return Container::rbegin(); }
   const_reverse_iterator rend() const { return Container::rend(); }
};

template <typename Container, bool is_bidir>
struct redirect_object_traits< construct_rewindable<Container, is_bidir> >
   : object_traits<Container> {
   using masquerade_for = Container;
   static constexpr bool is_temporary=false;
};

template <typename Container>
struct end_sensitive_helper {
   using end_source = Container;
};

template <typename Container, bool is_bidir>
struct end_sensitive_helper< construct_rewindable<Container, is_bidir> > {
   using end_source = Container;
};

template <typename Container, bool is_bidir=container_traits<Container>::is_bidirectional>
class construct_end_sensitive : public Container {
protected:
   construct_end_sensitive() = delete;
   ~construct_end_sensitive() = delete;

   using end_source = typename end_sensitive_helper<Container>::end_source;
public:
   using iterator = iterator_range<typename Container::iterator>;
   using const_iterator = iterator_range<typename Container::const_iterator>;

   iterator begin() { return iterator(Container::begin(), end_source::end()); }
   iterator end() { return iterator(Container::end()); }
   const_iterator begin() const { return const_iterator(Container::begin(), end_source::end()); }
   const_iterator end() const { return const_iterator(Container::end()); }
};

template <typename Container>
class construct_end_sensitive<Container, true>
   : public construct_end_sensitive<Container, false> {
   using base_t = construct_end_sensitive<Container, false>;
public:
   using reverse_iterator = iterator_range<typename Container::reverse_iterator>;
   using const_reverse_iterator = iterator_range<typename Container::const_reverse_iterator>;

   reverse_iterator rbegin()
   {
      return reverse_iterator(Container::rbegin(), base_t::end_source::rend());
   }
   reverse_iterator rend()
   {
      return reverse_iterator(Container::rend());
   }
   const_reverse_iterator rbegin() const
   {
      return const_reverse_iterator(Container::rbegin(), base_t::end_source::rend());
   }
   const_reverse_iterator rend() const
   {
      return const_reverse_iterator(Container::rend());
   }
};

template <typename Container, bool is_bidir>
struct redirect_object_traits< construct_end_sensitive<Container, is_bidir> >
   : object_traits<Container> {
   using masquerade_for = Container;
   static constexpr bool is_temporary=false;
};

template <typename Container>
struct default_enforce_feature<Container, rewindable> {
   using container = construct_rewindable<Container>;
};

template <typename Container>
struct default_enforce_feature<Container, end_sensitive> {
   using container = construct_end_sensitive<Container>;
};

template <bool on_top>
struct absorbing_feature<provide_construction<end_sensitive, on_top>, contractable> : std::true_type {};

struct reversed {};

template <typename Container,
          bool is_random=container_traits<Container>::is_random>
class construct_reversed {
protected:
   Container& hidden() { return reinterpret_cast<Container&>(*this); }
   const Container& hidden() const { return reinterpret_cast<const Container&>(*this); }
public:
   using value_type = typename container_traits<Container>::value_type;
   using reference = typename container_traits<Container>::reference;
   using const_reference = typename container_traits<Container>::const_reference;
   using container_category = typename container_traits<Container>::category;

   using iterator = typename container_traits<Container>::reverse_iterator;
   using const_iterator = typename container_traits<Container>::const_reverse_iterator;
   using reverse_iterator = typename container_traits<Container>::iterator;
   using const_reverse_iterator = typename container_traits<Container>::const_iterator;

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

   Int size() const { return hidden().size(); }
   Int dim() const { return get_dim(hidden()); }
   bool empty() const { return hidden().empty(); }
};

template <typename Container>
class construct_reversed<Container, true>
   : public construct_reversed<Container,false> {
   using base_t = construct_reversed<Container,false>;
public:
   typename base_t::reference operator[] (Int i)
   {
      return (base_t::hidden())[this->size()-1-i];
   }

   typename base_t::const_reference operator[] (Int i) const
   {
      return (base_t::hidden())[this->size()-1-i];
   }
};

template <typename Container>
struct default_check_container_feature<Container, reversed>
   : std::false_type {};

template <typename Container>
struct default_enforce_feature<Container, reversed> {
   using container = construct_reversed<Container>;
};

template <typename Feature>
struct feature_allow_order<reversed, Feature>
   : std::false_type {};

template <typename Container, bool is_random>
struct redirect_object_traits< construct_reversed<Container, is_random> >
   : spec_object_traits<Container> {
   using masquerade_for = Container;
   static constexpr bool is_temporary=false;
};

template <typename Container, bool is_random, typename Feature>
struct check_container_feature<construct_reversed<Container, is_random>, Feature>
   : check_container_feature<Container, Feature> {};

template <typename Container, bool is_random>
struct check_container_feature<construct_reversed<Container, is_random>, reversed>
   : std::true_type {};

template <typename Container, bool is_random, typename Features>
struct enforce_features<construct_reversed<Container, is_random>, Features> {
   using container = construct_reversed<typename enforce_features<Container, Features>::container>;
};

template <typename Container>
typename ensure_features<Container, reversed>::container&
reversed_view(Container& c)
{
   return reinterpret_cast<typename ensure_features<Container, reversed>::container&>(c);
}

template <typename Container>
const typename ensure_features<Container, reversed>::container&
reversed_view(const Container& c)
{
   return reinterpret_cast<const typename ensure_features<Container, reversed>::container&>(c);
}

template <typename Value, bool is_simple=std::is_pod<Value>::value>
class op_value_cache {
   Value* value;
   allocator val_alloc;

   void operator= (const op_value_cache&) = delete;
public:
   op_value_cache() : value(nullptr) {}

   op_value_cache(const op_value_cache& op) : value(nullptr) {}

   template <typename... Args>
   op_value_cache(Args&&... args)
   {
      value = val_alloc.construct<Value>(std::forward<Args>(args)...);
   }

   ~op_value_cache()
   {
      if (value) val_alloc.destroy(value);
   }

   Value& operator= (Value&& arg)
   {
      if (value)
         destroy_at(value);
      else
         value = val_alloc.construct<Value>(std::move(value));
      return *value;
   }

   Value& get() { return *value; }
   const Value& get() const { return *value; }
};

template <typename Value>
class op_value_cache<Value, true> {
   Value value;

   void operator= (const op_value_cache&) = delete;
public:
   op_value_cache() {}
   op_value_cache(const op_value_cache&) {}

   template <typename... Args>
   op_value_cache(Args&&... args)
      : value(std::forward<Args>(args)...) {}

   Value& operator= (Value&& arg)
   {
      value = std::move(arg);
      return value;
   }

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

   unary_transform_eval() = default;

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

   unary_transform_eval() = default;

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
   Int index() const
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

   unary_transform_eval() = default;

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
   Int index() const
   {
      return iop(*ihelper::get(*this));
   }
};

template <typename Target, typename SourceIterator>
decltype(auto)
prepare_iterator_arg(const SourceIterator& it,
                     std::enable_if_t<is_derived_from_any<SourceIterator, const_compatible_with<Target>>::value, void**> =nullptr)
{
   return static_cast<const typename is_derived_from_any<SourceIterator, const_compatible_with<Target>>::match&>(it);
}

template <typename Target, typename SourceIterator>
typename mproject1st<const SourceIterator&, typename iterator_traits<SourceIterator>::iterator_category>::type
prepare_iterator_arg(const SourceIterator& it,
                     std::enable_if_t<(!is_derived_from_any<SourceIterator, const_compatible_with<Target>>::value &&
                                       can_construct_any<SourceIterator, const_compatible_with<Target>>::value),
                                      void**> =nullptr)
{
   return it;
}

// TODO: revise its use, derived classes might not be accepted everywhere
template <typename SourceIterator, typename Target>
struct suitable_arg_for_iterator
   : std::enable_if<is_derived_from_any<SourceIterator, const_compatible_with<Target>>::value ||
                    can_construct_any<SourceIterator, const_compatible_with<Target>>::value> {};


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

   unary_transform_iterator() = default;

   template <typename Operation2>
   unary_transform_iterator(const unary_transform_iterator<typename iterator_traits<Iterator>::iterator, Operation2>& it)
      : base_t(it) {}

   template <typename Operation2>
   explicit unary_transform_iterator(const unary_transform_iterator<typename iterator_reversed<Iterator>::type, Operation2>& it)
      : base_t(it) {}

   template <typename SourceIterator,
             typename=std::enable_if_t<std::is_default_constructible<op_arg_type>::value,
                                       typename suitable_arg_for_iterator<SourceIterator, Iterator>::type>>
   unary_transform_iterator(const SourceIterator& cur_arg)
      : base_t(prepare_iterator_arg<Iterator>(cur_arg), op_arg_type()) {}

   template <typename SourceIterator,
             typename=typename suitable_arg_for_iterator<SourceIterator, Iterator>::type>
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

   unary_transform_iterator& operator+= (ptrdiff_t i)
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      raw_it::operator+=(i);
      return *this;
   }
   unary_transform_iterator& operator-= (ptrdiff_t i)
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      raw_it::operator-=(i);
      return *this;
   }
   unary_transform_iterator operator+ (ptrdiff_t i) const
   {
      unary_transform_iterator copy=*this;  return copy+=i;
   }
   unary_transform_iterator operator- (ptrdiff_t i) const
   {
      unary_transform_iterator copy=*this; return copy-=i;
   }
   friend unary_transform_iterator operator+ (ptrdiff_t i, const unary_transform_iterator& me)
   {
      return me+i;
   }

   template <typename Other>
   std::enable_if_t<is_derived_from_any<Other, typename iterator::raw_it, typename const_iterator::raw_it>::value,
                    typename raw_it::difference_type>
   operator- (const Other& it) const 
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      using other_raw_it = typename is_derived_from_any<Other, typename iterator::raw_it, typename const_iterator::raw_it>::match;
      return static_cast<const raw_it&>(*this) - static_cast<const other_raw_it&>(it);
   }

protected:
   typename base_t::reference random_impl(Int i, std::true_type) const
   {
      return this->op(raw_it::operator[](i));
   }
   typename base_t::reference random_impl(Int i, std::false_type) const
   {
      return this->op(static_cast<const raw_it&>(*this) + i);
   }
public:
   typename base_t::reference operator[] (Int i) const
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      return random_impl(i, bool_constant<base_t::helper::data_arg>());
   }
};

template <typename Iterator, typename Operation, typename Feature>
struct check_iterator_feature<unary_transform_iterator<Iterator, Operation>, Feature>
   : check_iterator_feature<Iterator, Feature> {};

template <typename Iterator, typename Operation>
struct check_iterator_feature<unary_transform_iterator<Iterator, Operation>, indexed>
   : mlist_or< is_instance_of<Operation, pair>,
               check_iterator_feature<Iterator,indexed> > {};

template <typename Iterator, typename Operation>
auto make_unary_transform_iterator(Iterator&& it, const Operation& op)
{
   return unary_transform_iterator<pointer2iterator_t<Iterator>, Operation>(pointer2iterator(std::forward<Iterator>(it)), op);
}

template <typename... Params>
struct unary_transform_constructor {
   using params = typename mlist_wrap<Params...>::type;

   template <typename Iterator, typename Operation, typename... ExpectedFeatures>
   struct defs {
      using expected_features = typename mlist_wrap<ExpectedFeatures...>::type;
      using needed_features = std::conditional_t<is_instance_of<Operation, pair>::value,
                                                 typename mlist_match_all<expected_features, indexed, equivalent_features>::complement,
                                                 expected_features>;
      using iterator = unary_transform_iterator<Iterator, Operation>;
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
struct default_check_container_feature<Container, dense>
   : bool_not<check_container_feature<Container, sparse>> {};

template <typename Container>
std::enable_if_t<check_container_feature<Container, sparse_compatible>::value, Int>
get_dim(const Container& c)
{
   return c.dim();
}

template <typename Container>
std::enable_if_t<!check_container_feature<Container, sparse_compatible>::value, Int>
get_dim(const Container& c)
{
   return c.size();
}

template <typename Container>
Int total_size(const Container& c)
{
   return c.size();
}

template <typename First, typename Second, typename... Other>
Int total_size(const First& c1, const Second& c2, const Other&... other)
{
   return c1.size() + total_size(c2, other...);
}

template <typename Container>
Int index_within_range(const Container& c, Int i)
{
   const Int d = get_dim(c);
   if (i < 0) i += d;
   if (i < 0 || i >= d) throw std::runtime_error("index out of range");
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

   output_transform_iterator() = default;

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

template <typename Iterator, typename Operation>
output_transform_iterator<Iterator,Operation>
make_output_transform_iterator(Iterator it, const Operation& op)
{
   return output_transform_iterator<Iterator,Operation>(it,op);
}

struct output_transform_constructor {
   template <typename Iterator, typename Operation, typename... ExpectedFeatures>
   struct defs {
      using expected_features = typename mlist_wrap<ExpectedFeatures...>::type;
      using needed_features = expected_features;
      using iterator = output_transform_iterator<Iterator, Operation>;
   };
};

template <typename Iterator1, typename Iterator2, typename Params=mlist<>>
class iterator_pair
   : public Iterator1 {
public:
   using first_type = Iterator1;
   using second_type = Iterator2;

   Iterator2 second;

   using features_via_second = typename mtagged_list_extract<Params, FeaturesViaSecondTag, mlist<>>::type;

   using iterator_category = typename least_derived_class<typename iterator_traits<Iterator1>::iterator_category,
                                                          typename iterator_traits<Iterator2>::iterator_category>::type;
   using difference_type = typename iterator_traits<std::conditional_t<check_iterator_feature<Iterator1, unlimited>::value,
                                                                       Iterator2, Iterator1>>::difference_type;
   using iterator = iterator_pair<typename iterator_traits<Iterator1>::iterator,
                                  typename iterator_traits<Iterator2>::iterator, Params>;
   using const_iterator = iterator_pair<typename iterator_traits<Iterator1>::const_iterator,
                                        typename iterator_traits<Iterator2>::const_iterator, Params>;

   iterator_pair() = default;

   template <typename SourceIterator1, typename SourceIterator2,
             typename=typename suitable_arg_for_iterator<SourceIterator1, Iterator1>::type,
             typename=typename suitable_arg_for_iterator<SourceIterator2, Iterator2>::type>
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

   iterator_pair& operator+= (Int i)
   {
      static_assert(iterator_pair_traits<Iterator1, Iterator2>::is_random, "iterator is not random-access");
      first_type::operator+=(i);  this->second+=i;
      return *this;
   }
   iterator_pair& operator-= (Int i)
   {
      static_assert(iterator_pair_traits<Iterator1, Iterator2>::is_random, "iterator is not random-access");
      first_type::operator-=(i);  this->second-=i;
      return *this;
   }
   iterator_pair operator+ (Int i) const
   {
      iterator_pair copy=*this; return copy+=i;
   }
   iterator_pair operator- (Int i) const
   {
      iterator_pair copy=*this; return copy-=i;
   }
   friend iterator_pair operator+ (Int i, const iterator_pair& it)
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

   using diff_via_second = bool_constant<(mlist_contains<features_via_second, end_sensitive, absorbing_feature>::value ||
                                          check_iterator_feature<Iterator1, unlimited>::value)>;

public:
   template <typename Other>
   std::enable_if_t<is_derived_from_any<Other, iterator, const_iterator>::value, difference_type>
   operator- (const Other& it) const
   {
      static_assert(iterator_pair_traits<Iterator1, Iterator2>::is_random, "iterator is not random-access");
      return diff_impl(static_cast<const typename is_derived_from_any<Other, iterator, const_iterator>::match&>(it), diff_via_second());
   }

   template <typename Other>
   std::enable_if_t<is_derived_from_any<Other, iterator, const_iterator>::value, bool>
   operator== (const Other& it) const
   {
      return eq_impl(static_cast<const typename is_derived_from_any<Other, iterator, const_iterator>::match&>(it), diff_via_second());
   }

   template <typename Other>
   std::enable_if_t<is_derived_from_any<Other, iterator, const_iterator>::value, bool>
   operator!= (const Other& it) const
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
   using at_end_via_second = bool_constant<(mlist_contains<features_via_second, end_sensitive, absorbing_feature>::value ||
                                            !check_iterator_feature<Iterator1, end_sensitive>::value)>;
   static constexpr bool at_end_defined=
      check_iterator_feature<std::conditional_t<at_end_via_second::value, Iterator2, Iterator1>, end_sensitive>::value;
public:
   bool at_end() const
   {
      static_assert(at_end_defined, "iterator not end-sensitive");
      return at_end_impl(at_end_via_second());
   }
private:
   Int index_impl(std::false_type) const
   {
      return first_type::index();
   }
   Int index_impl(std::true_type) const
   {
      return second.index();
   }
   using index_via_second = bool_constant<(mlist_contains<features_via_second, indexed, absorbing_feature>::value ||
                                           !check_iterator_feature<Iterator1, indexed>::value)>;
   static constexpr bool index_defined=
      check_iterator_feature<std::conditional_t<index_via_second::value, Iterator2, Iterator1>, indexed>::value;
public:
   Int index() const
   {
      static_assert(index_defined, "iterator not indexed");
      return index_impl(index_via_second());
   }
protected:
   using rewind_first = typename bool_not<mlist_contains<features_via_second, rewindable, absorbing_feature>>::type;
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
   void contract1(bool, Int distance_front, Int, std::false_type)
   {
      std::advance(static_cast<first_type&>(*this), distance_front);
   }
   void contract1(bool renumber, Int distance_front, Int distance_back, std::true_type)
   {
      first_type::contract(renumber, distance_front, distance_back);
   }
   void contract2(bool, Int distance_front, Int, std::false_type)
   {
      std::advance(second, distance_front);
   }
   void contract2(bool renumber, Int distance_front, Int distance_back, std::true_type)
   {
      second.contract(renumber, distance_front, distance_back);
   }
public:
   void contract(bool renumber, Int distance_front, Int distance_back = 0)
   {
      if (!mlist_contains<features_via_second, contractable, equivalent_features>::value)
         contract1(renumber, distance_front, distance_back, bool_constant<check_iterator_feature<Iterator1, contractable>::value>());
      contract2(renumber, distance_front, distance_back, bool_constant<check_iterator_feature<Iterator2, contractable>::value>());
   }
};

template <typename Iterator1, typename Iterator2, typename Params, typename Feature>
struct check_iterator_feature< iterator_pair<Iterator1, Iterator2, Params>, Feature> {
   using usual_or_features = mlist<end_sensitive, indexed>;

   static constexpr bool
      check1 = check_iterator_feature<Iterator1, Feature>::value,
      check2 = check_iterator_feature<Iterator2, Feature>::value,
      value = mlist_contains<typename mtagged_list_extract<Params, FeaturesViaSecondTag, mlist<>>::type, Feature, absorbing_feature>::value
              ? check2 :
              mlist_contains<usual_or_features, Feature>::value
              ? check1 || check2
              : check1 && check2;

   using type = bool_constant<value>;
   using value_type = bool;
};

template <typename Params=mlist<>>
struct pair_coupler {
   using usual_or_features = mlist<end_sensitive, indexed>;

   template <typename Iterator1, typename Iterator2, typename... ExpectedFeatures>
   struct defs {
      using expected_features = typename mlist_wrap<ExpectedFeatures...>::type;
      using or_features = typename mlist_match_all<expected_features, usual_or_features, equivalent_features>::type;
      using and_features = typename mlist_match_all<expected_features, usual_or_features, equivalent_features>::complement;
      using first_can = typename mlist_match_all<Iterator1, or_features, check_iterator_feature>::type2;
      using first_can_not = typename mlist_match_all<Iterator1, or_features, check_iterator_feature>::complement2;
      using explicitly_via_second = std::conditional_t<check_iterator_feature<Iterator2,unlimited>::value, mlist<>, first_can_not>;
      using it_params = typename mlist_prepend_if<!mlist_is_empty<explicitly_via_second>::value,
                                                  FeaturesViaSecondTag<explicitly_via_second>, Params>::type;
      using iterator = iterator_pair<Iterator1, Iterator2, it_params>;
      using needed_features1 = std::conditional_t<check_iterator_feature<Iterator2, unlimited>::value,
                                                  expected_features, and_features>;
      using needed_features2 = std::conditional_t<check_iterator_feature<Iterator2, unlimited>::value,
                                                  and_features,
                                                  typename mlist_match_all<expected_features, first_can, equivalent_features>::complement>;
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

   binary_transform_eval() = default;

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

   binary_transform_eval() = default;

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
   Int index() const
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

   binary_transform_eval() = default;

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
   Int index() const
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

   binary_transform_iterator() = default;

   template <typename Operation2>
   binary_transform_iterator(const binary_transform_iterator<typename iterator_traits<IteratorPair>::iterator, Operation2, is_partial>& it)
      : base_t(it) {}

   template <typename SourceIteratorPair,
             typename=std::enable_if_t<std::is_default_constructible<op_arg_type>::value,
                                       typename suitable_arg_for_iterator<SourceIteratorPair, IteratorPair>::type>>
   binary_transform_iterator(const SourceIteratorPair& cur_arg)
      : base_t(prepare_iterator_arg<IteratorPair>(cur_arg), op_arg_type()) {}

   template <typename SourceIteratorPair,
             typename=typename suitable_arg_for_iterator<SourceIteratorPair, IteratorPair>::type>
   binary_transform_iterator(const SourceIteratorPair& cur_arg, const op_arg_type& op_arg)
      : base_t(prepare_iterator_arg<IteratorPair>(cur_arg), op_arg) {}

   template <typename SourceIterator1, typename SourceIterator2,
             typename=std::enable_if_t<std::is_default_constructible<op_arg_type>::value,
                                       typename suitable_arg_for_iterator<SourceIterator1, typename IteratorPair::first_type>::type>,
             typename=typename suitable_arg_for_iterator<SourceIterator2, typename IteratorPair::second_type>::type>
   binary_transform_iterator(const SourceIterator1& first_arg, const SourceIterator2& second_arg)
      : base_t(prepare_iterator_arg<typename IteratorPair::first_type>(first_arg),
               prepare_iterator_arg<typename IteratorPair::second_type>(second_arg),
               op_arg_type()) {}

   template <typename SourceIterator1, typename SourceIterator2,
             typename=typename suitable_arg_for_iterator<SourceIterator1, typename IteratorPair::first_type>::type,
             typename=typename suitable_arg_for_iterator<SourceIterator2, typename IteratorPair::second_type>::type>
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

   binary_transform_iterator& operator+= (Int i)
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      raw_it::operator+=(i);
      return *this;
   }
   binary_transform_iterator& operator-= (Int i)
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      raw_it::operator-=(i);
      return *this;
   }
   binary_transform_iterator operator+ (Int i) const
   {
      binary_transform_iterator copy=*this;  return copy+=i;
   }
   binary_transform_iterator operator- (Int i) const
   {
      binary_transform_iterator copy=*this;  return copy-=i;
   }
   friend binary_transform_iterator operator+ (Int i, const binary_transform_iterator& it)
   {
      return it+i;
   }

   template <typename Other>
   std::enable_if_t<is_derived_from_any<Other, typename iterator::raw_it, typename const_iterator::raw_it>::value,
                    typename raw_it::difference_type>
   operator- (const Other& it) const
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      using other_raw_it = typename is_derived_from_any<Other, typename iterator::raw_it, typename const_iterator::raw_it>::match;
      return static_cast<const raw_it&>(*this) - static_cast<const other_raw_it&>(it);
   }

protected:
   typename base_t::reference random_impl(Int i, std::true_type, std::true_type) const
   {
      return this->op(raw_it::operator[](i), this->second[i]);
   }
   typename base_t::reference random_impl(Int i, std::true_type, std::false_type) const
   {
      return this->op(raw_it::operator[](i), this->second+i);
   }
   typename base_t::reference random_impl(Int i, std::false_type, std::true_type) const
   {
      return this->op(static_cast<const typename raw_it::first_type&>(*this)+i, this->second[i]);
   }
   typename base_t::reference random_impl(Int i, std::false_type, std::false_type) const
   {
      return this->op(static_cast<const typename raw_it::first_type&>(*this)+i, this->second+i);
   }
public:
   typename raw_it::reference operator[] (Int i) const
   {
      static_assert(iterator_traits<raw_it>::is_random, "iterator is not random-access");
      return random_impl(i, bool_constant<base_t::helper::first_data_arg>(), bool_constant<base_t::helper::second_data_arg>());
   }
};

template <typename IteratorPair, typename Operation, bool is_partial, typename Feature>
struct check_iterator_feature<binary_transform_iterator<IteratorPair, Operation, is_partial>, Feature>
   : check_iterator_feature<IteratorPair,Feature> {};

template <typename IteratorPair, typename Operation, bool is_partial>
struct check_iterator_feature<binary_transform_iterator<IteratorPair, Operation, is_partial>, indexed>
   : mlist_or< is_instance_of<Operation, pair>,
               check_iterator_feature<IteratorPair, indexed> > {};

template <typename Iterator>
struct has_partial_state : std::false_type {};

template <typename... Params>
struct binary_transform_constructor {
   using params = typename mlist_wrap<Params...>::type;

   template <typename IteratorPair, typename Operation, typename... ExpectedFeatures>
   struct defs {
      using expected_features = typename mlist_wrap<ExpectedFeatures...>::type;
      static const bool is_partially_defined = tagged_list_extract_integral<params, PartiallyDefinedTag>(has_partial_state<IteratorPair>::value);

      using needed_pair_features = std::conditional_t<is_instance_of<Operation, pair>::value,
                                                      typename mlist_match_all<expected_features, indexed, equivalent_features>::complement,
                                                      expected_features>;
      using needed_features1 = mlist<>;
      using needed_features2 = mlist<>;
      using iterator = binary_transform_iterator<IteratorPair, Operation, is_partially_defined>;
   };
};

template <typename Iterator1, typename Iterator2, typename Operation>
auto make_binary_transform_iterator(Iterator1&& first, Iterator2&& second, const Operation& op)
{
   return binary_transform_iterator<iterator_pair<pointer2iterator_t<Iterator1>, pointer2iterator_t<Iterator2>>, Operation>
      (pointer2iterator(std::forward<Iterator1>(first)), pointer2iterator(std::forward<Iterator2>(second)), op);
}

} // end namespace pm

namespace polymake {
   using pm::BuildUnary;
   using pm::BuildBinary;
   using pm::BuildUnaryIt;
   using pm::BuildBinaryIt;
   using pm::make_unary_transform_iterator;
   using pm::make_binary_transform_iterator;
   using pm::make_output_transform_iterator;
   using pm::as_iterator_range;
   using pm::indexed;
   using pm::rewindable;
   using pm::reversed;
   using pm::reversed_view;
   using pm::black_hole;
   using pm::inserter;
   using pm::allow_conversion;
} // end namespace polymake


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
