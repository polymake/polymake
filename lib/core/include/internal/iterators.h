/* Copyright (c) 1997-2015
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
#include "polymake/pair.h"

#include <functional>
#include <iterator>
#include <stdexcept>

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
             derived_from<typename std::iterator_traits<Iterator>::iterator_category, forward_iterator_tag>::value>
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
   typedef typename attrib<T>::plus_const const_iterator;
};

template <typename Iterator>
struct iterator_category_booleans {
   static const bool
      is_forward=
         derived_from<typename std::iterator_traits<Iterator>::iterator_category, forward_iterator_tag>::value,
      is_bidirectional=
         derived_from<typename std::iterator_traits<Iterator>::iterator_category, bidirectional_iterator_tag>::value,
      is_random=
         derived_from<typename std::iterator_traits<Iterator>::iterator_category, random_access_iterator_tag>::value;
};

template <typename Iterator>
struct iterator_traits
   : public std::iterator_traits<Iterator>,
     public iterator_cross_const_helper<Iterator>,
     public iterator_category_booleans<Iterator> {
   typedef Iterator derivable_type;
};

template <typename Iterator, bool _is_rev=iterator_category_booleans<Iterator>::is_bidirectional>
struct default_iterator_reversed {
   typedef void type;
};

template <typename Iterator>
struct default_iterator_reversed<Iterator, true> {
   typedef std::reverse_iterator<Iterator> type;
   static Iterator revert(const type& it) { return it.base(); }
};

template <typename Iterator>
struct iterator_reversed : default_iterator_reversed<Iterator> {};

template <typename Iterator>
struct iterator_reversed< std::reverse_iterator<Iterator> > {
   typedef Iterator type;
   static std::reverse_iterator<Iterator> revert(const type& it) { return std::reverse_iterator<Iterator>(it); }
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
struct iterator_traits<Iterator&> : iterator_traits<typename deref<Iterator>::type> {};

template <typename Iterator>
struct iterator_cross_const_helper<std::reverse_iterator<Iterator>, true> {
   typedef std::reverse_iterator<typename iterator_traits<Iterator>::iterator> iterator;
   typedef std::reverse_iterator<typename iterator_traits<Iterator>::const_iterator> const_iterator;
};

template <typename Iterator>
struct alt_constructor {
   typedef typename if_else<identical<Iterator, typename iterator_traits<Iterator>::iterator>::value,
                            Iterator, const typename iterator_traits<Iterator>::iterator>::type
      arg_type;
};

#if defined(__GNUC__)

template <typename Iterator, typename Container>
struct iterator_cross_const_helper<__gnu_cxx::__normal_iterator<Iterator, Container>, true> {
   typedef __gnu_cxx::__normal_iterator<typename iterator_traits<Iterator>::iterator, Container> iterator;
   typedef __gnu_cxx::__normal_iterator<typename iterator_traits<Iterator>::const_iterator, Container> const_iterator;
};

#endif // __GNUC__

} // end namespace pm

#if defined(__GNUC__)
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

template <typename Iterator1, typename Iterator2>
struct matching_iterator {
   static const bool value= identical<Iterator1, Iterator2>::value ||
                            identical<typename iterator_traits<Iterator1>::iterator, Iterator2>::value;
};

template <typename Iterator1, typename Iterator2>
struct comparable_iterator {
   static const bool value= identical<typename iterator_traits<Iterator1>::iterator, Iterator2>::value ||
                            identical<typename iterator_traits<Iterator1>::const_iterator, Iterator2>::value;
};

template <typename IteratorList1, typename IteratorList2>
struct matching_iterator_list
   : list_accumulate_binary<list_and, matching_iterator, IteratorList1, IteratorList2> {};

template <typename IteratorList1, typename IteratorList2>
struct comparable_iterator_list
   : list_accumulate_binary<list_and, comparable_iterator, IteratorList1, IteratorList2> {};

template <typename Operation>
struct operation_cross_const_helper {
   typedef Operation operation;
   typedef Operation const_operation;
};

/** Wrapper for a pointer used as an iterator.

    Several iterator modifiers defined later derive itself from the corresponding basis iterator.
    A built-in pointer is a valid iterator; however, the C++ syntax rules don't allow inheritance
    from a pointer, since it's not a class.

    This wrapper solves this problem: it behaves exactly the same as a pointer, and is a valid
    base class to be derived from.
*/
template <typename T>
class ptr_wrapper {
public:
   typedef random_access_iterator_tag iterator_category;
   typedef T& reference;
   typedef T* pointer;
   typedef typename deref<T>::type value_type;  // T may have 'const' attribute
   typedef ptrdiff_t difference_type;
   typedef ptr_wrapper<value_type> iterator;
   typedef ptr_wrapper<const value_type> const_iterator;

   ptr_wrapper(pointer cur_arg=0) : cur(cur_arg) {}
   ptr_wrapper(const iterator& it) : cur(it.operator->()) {}

   ptr_wrapper& operator= (pointer cur_arg) { cur=cur_arg; return *this; }
   ptr_wrapper& operator= (const iterator& it) { cur=it.operator->(); return *this; }

   reference operator* () const { return *cur; }
   pointer operator-> () const { return cur; }
   reference operator[] (int i) const { return cur[i]; }

   ptr_wrapper& operator++ () { ++cur; return *this; }
   ptr_wrapper& operator-- () { --cur; return *this; }
   const ptr_wrapper operator++ (int) { ptr_wrapper copy=*this; operator++(); return copy; }
   const ptr_wrapper operator-- (int) { ptr_wrapper copy=*this; operator--(); return copy; }
   ptr_wrapper& operator+= (int i) { cur+=i; return *this; }
   ptr_wrapper& operator-= (int i) { cur-=i; return *this; }
   const ptr_wrapper operator+ (int i) const { return ptr_wrapper(cur+i); }
   const ptr_wrapper operator- (int i) const { return ptr_wrapper(cur-i); }
   friend const ptr_wrapper operator+ (int i, const ptr_wrapper& p) { return ptr_wrapper(p.cur+i); }
   ptrdiff_t operator- (const ptr_wrapper& p) const { return cur-p.cur; }
   ptrdiff_t operator- (const T* x) const { return cur-x; }

   bool operator== (const iterator& p)       const { return cur==p.cur; }
   bool operator!= (const iterator& p)       const { return cur!=p.cur; }
   bool operator<  (const iterator& p)       const { return cur<p.cur; }
   bool operator>  (const iterator& p)       const { return cur>p.cur; }
   bool operator<= (const iterator& p)       const { return cur<=p.cur; }
   bool operator>= (const iterator& p)       const { return cur>=p.cur; }
   bool operator== (const const_iterator& p) const { return cur==p.cur; }
   bool operator!= (const const_iterator& p) const { return cur!=p.cur; }
   bool operator<  (const const_iterator& p) const { return cur<p.cur; }
   bool operator>  (const const_iterator& p) const { return cur>p.cur; }
   bool operator<= (const const_iterator& p) const { return cur<=p.cur; }
   bool operator>= (const const_iterator& p) const { return cur>=p.cur; }

protected:
   pointer cur;
};

template <typename T>
struct iterator_traits<T*> : public std::iterator_traits<T*>, public iterator_category_booleans<T*> {
   typedef ptr_wrapper<T> derivable_type;
   typedef typename deref<T>::type* iterator;
   typedef const typename deref<T>::type* const_iterator;
};

template <typename T>
struct alt_constructor<T*> {
   typedef const ptr_wrapper<T> arg_type;
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
struct default_check_iterator_feature : identical<Feature,void> {};

template <typename Iterator>
struct default_check_iterator_feature<Iterator, unlimited> {
   static const bool value=!iterator_traits<Iterator>::is_forward;
};

template <typename Iterator, typename Feature>
struct check_iterator_feature : default_check_iterator_feature<Iterator, Feature> {};

template <typename Iterator, typename Feature>
struct enable_if_iterator : enable_if<void**, check_iterator_feature<Iterator, Feature>::value> {};

template <typename Feature_before, typename Feature_after>
struct feature_allow_order : True {};

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
struct absorbing_feature : derived_from<Feature1,Feature2> {};

template <typename Feature1, bool on_top1, typename Feature2>
struct absorbing_feature< provide_construction<Feature1, on_top1>, Feature2>
   : absorbing_feature<Feature1,Feature2> {};

template <typename Feature1, bool on_top1, typename Feature2, bool on_top2>
struct absorbing_feature< provide_construction<Feature1,on_top1>, provide_construction<Feature2,on_top2> > {
   static const bool value= on_top1>=on_top2 && derived_from<Feature1,Feature2>::value;
};

template <typename Feature1, typename Feature2>
struct equivalent_features : identical<Feature1, Feature2> {};

template <typename Feature, bool on_top>
struct equivalent_features< provide_construction<Feature,on_top>, Feature > : True {
   typedef provide_construction<Feature,on_top> type;
};

template <typename Feature, bool on_top>
struct equivalent_features< Feature, provide_construction<Feature,on_top> > : True {
   typedef provide_construction<Feature> type;
};

template <typename Iterator>
struct accompanying_iterator {
   typedef typename iterator_traits<Iterator>::derivable_type type;

   static void assign(type& it, const type& other) { it=other;}

   static void advance(type& it, const type&, int n) { std::advance(it, n); }
};

template <typename Iterator>
class rewindable_iterator : public iterator_traits<Iterator>::derivable_type {
protected:
   typedef typename iterator_traits<Iterator>::derivable_type super;
   typename accompanying_iterator<Iterator>::type _begin;

   template <typename> friend class rewindable_iterator;
public:
   typedef rewindable_iterator<typename iterator_traits<Iterator>::iterator> iterator;
   typedef rewindable_iterator<typename iterator_traits<Iterator>::const_iterator> const_iterator;

   rewindable_iterator() {}
   rewindable_iterator(const Iterator& cur_arg)
      : super(cur_arg), _begin(cur_arg) {}
   rewindable_iterator(typename alt_constructor<Iterator>::arg_type& cur_arg)
      : super(cur_arg), _begin(cur_arg) {}

   rewindable_iterator(const iterator& it)
      : super(static_cast<const typename iterator::super&>(it)), _begin(it._begin) {}

   rewindable_iterator& operator= (const iterator& it)
   {
      static_cast<super&>(*this)=it;
      _begin=it._begin;
      return *this;
   }

   rewindable_iterator& operator= (const Iterator& cur)
   {
      static_cast<super&>(*this)=cur;
      return *this;
   }
   rewindable_iterator& operator= (typename alt_constructor<Iterator>::arg_type& cur)
   {
      static_cast<super&>(*this)=cur;
      return *this;
   }

   void rewind()
   {
      accompanying_iterator<Iterator>::assign(static_cast<super&>(*this), _begin);
   }
private:
   void contract1(bool, int distance_front, int, False)
   {
      std::advance(static_cast<super&>(*this), distance_front);
   }
   void contract1(bool renumber, int distance_front, int distance_back, True)
   {
      super::contract(renumber, distance_front, distance_back);
   }
public:
   void contract(bool renumber, int distance_front, int distance_back=0)
   {
      contract1(renumber, distance_front, distance_back, bool2type<check_iterator_feature<super, contractable>::value>());
      _begin=static_cast<const super&>(*this);
   }
};

template <typename Iterator, typename Feature>
struct check_iterator_feature<rewindable_iterator<Iterator>, Feature>
   : check_iterator_feature<Iterator,Feature> {};

template <typename Iterator>
struct check_iterator_feature<rewindable_iterator<Iterator>, rewindable> : True {};

template <typename Iterator>
struct check_iterator_feature<rewindable_iterator<Iterator>, contractable> : True {};

template <typename Iterator>
struct accompanying_iterator< rewindable_iterator<Iterator> > : accompanying_iterator<Iterator> {};

template <typename Iterator>
class iterator_range : public iterator_traits<Iterator>::derivable_type {
protected:
   typedef typename iterator_traits<Iterator>::derivable_type super;
   typedef typename accompanying_iterator<Iterator>::type end_type;

   end_type _end;
   template <typename> friend class iterator_range;
public:
   typedef iterator_range<typename iterator_traits<Iterator>::iterator> iterator;
   typedef iterator_range<typename iterator_traits<Iterator>::const_iterator> const_iterator;

   iterator_range() {}
   iterator_range(const Iterator& cur_arg)
      : super(cur_arg), _end(cur_arg) {}
   iterator_range(const Iterator& cur_arg, const end_type& end_arg)
      : super(cur_arg), _end(end_arg) {}

   iterator_range(typename alt_constructor<Iterator>::arg_type& cur_arg)
      : super(cur_arg), _end(cur_arg) {}
   iterator_range(typename alt_constructor<Iterator>::arg_type& cur_arg, const end_type& end_arg)
      : super(cur_arg), _end(cur_arg) {}
   iterator_range(typename alt_constructor<Iterator>::arg_type& cur_arg, typename alt_constructor<end_type>::arg_type& end_arg)
      : super(cur_arg), _end(end_arg) {}

   iterator_range(const iterator& it)
      : super(static_cast<const typename iterator::super&>(it)), _end(it._end) {}

   iterator_range& operator= (const iterator& it)
   {
      static_cast<super&>(*this)=it;
      _end=it._end;
      return *this;
   }

   iterator_range& operator= (const Iterator& cur)
   {
      static_cast<super&>(*this)=cur;
      return *this;
   }
   iterator_range& operator= (typename alt_constructor<Iterator>::arg_type& cur)
   {
      static_cast<super&>(*this)=cur;
      return *this;
   }

   bool at_end() const { return static_cast<const super&>(*this)==_end; }

   iterator_range& operator++()
   {
      super::operator++(); return *this;
   }
   const iterator_range operator++ (int)
   {
      iterator_range copy=*this; operator++(); return copy;
   }

   iterator_range& operator--()
   {
      typedef typename enable_if<super, iterator_traits<Iterator>::is_bidirectional>::type error_if_unimplemented __attribute__((unused));
      error_if_unimplemented::operator--(); return *this;
   }
   const iterator_range operator-- (int)
   {
      iterator_range copy=*this; operator--(); return copy;
   }

   iterator_range& operator+= (int i)
   {
      typedef typename enable_if<super, iterator_traits<Iterator>::is_random>::type error_if_unimplemented __attribute__((unused));
      error_if_unimplemented::operator+=(i);
      return *this;
   }
   iterator_range& operator-= (int i)
   {
      typedef typename enable_if<super, iterator_traits<Iterator>::is_random>::type error_if_unimplemented __attribute__((unused));
      error_if_unimplemented::operator-=(i);
      return *this;
   }

   const iterator_range operator+ (int i) const
   {
      typedef typename enable_if<super, iterator_traits<Iterator>::is_random>::type error_if_unimplemented __attribute__((unused));
      return iterator_range(static_cast<const error_if_unimplemented&>(*this)+i, _end);
   }
   const iterator_range operator- (int i) const
   {
      typedef typename enable_if<super, iterator_traits<Iterator>::is_random>::type error_if_unimplemented __attribute__((unused));
      return iterator_range(static_cast<const error_if_unimplemented&>(*this)-i, _end);
   }
   friend const iterator_range operator+ (int i, const iterator_range& me)
   {
      typedef typename enable_if<super, iterator_traits<Iterator>::is_random>::type error_if_unimplemented __attribute__((unused));
      return iterator_range(static_cast<const error_if_unimplemented&>(me)+i, me._end);
   }

   typename super::difference_type operator- (const super& it) const
   {
      typedef typename enable_if<super, iterator_traits<Iterator>::is_random>::type error_if_unimplemented __attribute__((unused));
      return static_cast<const error_if_unimplemented&>(*this)-it;
   }
   typename super::difference_type
   operator- (typename if_else<identical<super,Iterator>::value, super, const Iterator>::type& it) const
   {
      typedef typename enable_if<super, iterator_traits<Iterator>::is_random>::type error_if_unimplemented __attribute__((unused));
      return static_cast<const error_if_unimplemented&>(*this)-it;
   }
private:
   void contract1(bool, int distance_front, int, False)
   {
      std::advance(static_cast<super&>(*this), distance_front);
   }
   void contract1(bool renumber, int distance_front, int distance_back, True)
   {
      super::contract(renumber, distance_front, distance_back);
   }
public:
   void contract(bool renumber, int distance_front, int distance_back=0)
   {
      contract1(renumber, distance_front, distance_back, bool2type<check_iterator_feature<super, contractable>::value>());
      accompanying_iterator<Iterator>::advance(_end, static_cast<const super&>(*this), -distance_back);
   }
};

template <typename Iterator, typename Feature>
struct check_iterator_feature<iterator_range<Iterator>, Feature>
   : check_iterator_feature<Iterator,Feature> {};

template <typename Iterator>
struct check_iterator_feature<iterator_range<Iterator>, end_sensitive> : True {};

template <typename Iterator>
struct check_iterator_feature<iterator_range<Iterator>, contractable> : True {};

template <> struct feature_allow_order<end_sensitive,rewindable> : False {};

template <typename Iterator>
struct accompanying_iterator< iterator_range<Iterator> > : accompanying_iterator<Iterator> {};

template <typename Iterator> inline
iterator_range<Iterator>
make_iterator_range(Iterator first, Iterator last)
{
   return iterator_range<Iterator>(first,last);
}

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
as_iterator_range(const Iterator& it, typename enable_if<void*, check_iterator_feature<Iterator,end_sensitive>::value>::type=0)
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
             bool _iterator_preserved=identical<typename Container::const_iterator,
                                                typename Container::manipulator_impl::const_iterator>::value>
   struct check_begin_end {
      static const int value= _iterator_preserved ? int(is_manip) : int(is_opaque);
   };

   template <typename Container>
   struct what_is<Container, is_manip> : check_begin_end<Container> {};

} // end namespace object_classifier

template <typename T>
struct spec_object_traits< cons<T, int2type<object_classifier::is_manip> > >
   : spec_object_traits<is_container> {
   typedef typename deref<typename T::hidden_type>::type masquerade_for;
};

template <typename Container, typename ProvidedFeatures> class manip_feature_collector;

template <typename Container, bool is_const>
struct default_container_elem_traits {
   typedef typename Container::const_reference const_reference;
   typedef typename if_else<is_const, const_reference, typename Container::reference>::type reference;
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
   static const bool value=has_value_type<Iterator>::value &&
                           has_iterator_category<Iterator>::value &&
                           has_difference_type<Iterator>::value;
};

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
          bool _enable= has_iterator<Container>::value && has_value_type<Container>::value>
struct is_suitable_container {
   static const bool value=identical<typename object_traits<Container>::model, is_container>::value &&
                          !identical<typename object_traits<Container>::generic_tag, exclude_generic_tag>::value;
};

template <typename Container, typename exclude_generic_tag>
struct is_suitable_container<Container, exclude_generic_tag, false> : False {};

template <typename T, size_t size, typename exclude_generic_tag>
struct is_suitable_container<T[size], exclude_generic_tag, false> : False {};

// special tag for isomorphic_to_container_of
struct allow_conversion;

template <typename Container, typename Element,
          typename exclude_generic_tag=void,
          bool _enable=is_suitable_container<Container, exclude_generic_tag>::value>
struct isomorphic_to_container_of
{
   // @todo recursive check of generic_tags in case of Element being in turn a container
   static const bool value=isomorphic_types<typename Container::value_type, Element>::value &&
                           (identical<typename object_traits<Element>::generic_tag, typename object_traits<Element>::model>::value ||
                            identical<typename object_traits<Element>::generic_tag, typename object_traits<typename Container::value_type>::generic_tag>::value ||
                            (identical<exclude_generic_tag, allow_conversion>::value &&
                             convertible_to<typename Container::value_type, Element>::value));
};

template <typename Container, typename Element, typename exclude_generic_tag>
struct isomorphic_to_container_of<Container, Element, exclude_generic_tag, false> : False {};

template <typename Container, bool is_const,
          bool _enabled=has_iterator<Container>::value,
          bool _reversible=derived_from<typename container_category_traits<Container>::category, bidirectional_iterator_tag>::value>
struct default_container_it_traits : default_container_elem_traits<Container, is_const> {
   typedef typename Container::const_iterator const_iterator;
   typedef typename if_else<is_const, const_iterator, typename Container::iterator>::type iterator;
};

template <typename Container, bool is_const>
struct default_container_it_traits<Container, is_const, true, true>
   : default_container_it_traits<Container, is_const, true, false> {
   typedef typename Container::const_reverse_iterator const_reverse_iterator;
   typedef typename if_else<is_const, const_reverse_iterator, typename Container::reverse_iterator>::type reverse_iterator;
};

template <typename Container, bool is_const, bool _reversible>
struct default_container_it_traits<Container, is_const, false, _reversible> : default_container_elem_traits<Container, is_const> {};

template <typename Container, bool is_const>
struct default_container_traits : container_category_traits<Container>, default_container_it_traits<Container, is_const> {};

template <typename ContainerRef>
struct container_traits
   : default_container_traits<typename deref<ContainerRef>::type, attrib<ContainerRef>::is_const>
{
   typedef default_container_traits<typename deref<ContainerRef>::type, attrib<ContainerRef>::is_const> _super;
   static const bool
      is_forward       = derived_from<typename _super::category, forward_iterator_tag>::value,
      is_bidirectional = derived_from<typename _super::category, bidirectional_iterator_tag>::value,
      is_random        = derived_from<typename _super::category, random_access_iterator_tag>::value;
};

template <typename Container>
struct is_assoc_container : bool2type<has_key_type<Container>::value && has_mapped_type<Container>::value> {};

template <typename Iterator> inline
int count_it(Iterator src)
{
   typename iterator_traits<Iterator>::difference_type cnt=0;
   while (!src.at_end()) {
      ++cnt, ++src;
   }
   return cnt;
}


/* --------------
 *  plain arrays
 * -------------- */

template <typename T>
struct array_traits {
   typedef T& reference;
   typedef const T& const_reference;
   typedef T value_type;
   typedef T* iterator;
   typedef const T* const_iterator;
   typedef std::reverse_iterator<iterator> reverse_iterator;
   typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
   typedef random_access_iterator_tag container_category;
};

template <typename Top, typename E=typename container_traits<Top>::value_type>
class plain_array : public array_traits<E> {
   typedef array_traits<E> _super;
public:
   typename _super::iterator begin()
   {
      return &(static_cast<Top*>(this)->front());
   }
   typename _super::iterator end()
   {
      return begin()+static_cast<const Top*>(this)->size();
   }
   typename _super::const_iterator begin() const
   {
      return &(static_cast<const Top*>(this)->front());
   }
   typename _super::const_iterator end() const
   {
      return begin()+static_cast<const Top*>(this)->size();
   }

   typename _super::reverse_iterator rbegin()
   {
      return typename _super::reverse_iterator(end());
   }
   typename _super::reverse_iterator rend()
   {
      return typename _super::reverse_iterator(begin());
   }
   typename _super::const_reverse_iterator rbegin() const
   {
      return typename _super::const_reverse_iterator(end());
   }
   typename _super::const_reverse_iterator rend() const
   {
      return typename _super::const_reverse_iterator(begin());
   }

   typename _super::reference back()
   {
      return end()[-1];
   }
   typename _super::reference operator[] (int i)
   {
      return begin()[i];
   }
   typename _super::const_reference back() const
   {
      return end()[-1];
   }
   typename _super::const_reference operator[] (int i) const
   {
      return begin()[i];
   }

   bool empty() const
   {
      return static_cast<const Top*>(this)->size()==0;
   }
};

template <typename T, size_t _size>
class fixed_array
   : public plain_array< fixed_array<T,_size>, T> {
   T data[_size];
public:
   T& front() { return data[0]; }
   const T& front() const { return data[0]; }
   int size() const { return _size; }
   int max_size() const { return _size; }
protected:
   fixed_array();
   ~fixed_array();
};

template <typename T, size_t _size, size_t subsize>
class fixed_array<T[subsize], _size>
   : public plain_array< fixed_array<T[subsize],_size>, fixed_array<T,subsize> > {
   fixed_array<T,subsize> data[_size];
public:
   fixed_array<T,subsize>& front() { return data[0]; }
   const fixed_array<T,subsize>& front() const { return data[0]; }
   int size() const { return _size; }
   int max_size() const { return _size; }
protected:
   fixed_array();
   ~fixed_array();
};

template <typename T, size_t n> inline
typename enable_if<fixed_array<T,n>, sizeof(T[n])==sizeof(fixed_array<T,n>)>::type&
array2container(T (&a)[n])
{
   return reinterpret_cast<fixed_array<T,n>&>(a);
}

template <typename T, size_t n> inline
const typename enable_if<fixed_array<T,n>, sizeof(T[n])==sizeof(fixed_array<T,n>)>::type&
array2container(const T (&a)[n])
{
   return reinterpret_cast<const fixed_array<T,n>&>(a);
}

template <typename T, size_t n>
class Serialized<T[n]> : public fixed_array<T,n> {
protected:
   Serialized() throw();
   ~Serialized() throw();
};

template <typename T>
struct spec_object_traits< array_traits<T> >
   : spec_object_traits<is_container> {};

template <typename T, size_t n>
struct spec_object_traits< fixed_array<T,n> >
   : spec_object_traits<is_container> {};

template <typename T, size_t n>
struct spec_object_traits< T[n] >
   : spec_object_traits<is_opaque> {};

template <typename FeatureList1, typename FeatureList2>
struct mix_features {
   typedef typename concat_list< typename list_search_all<FeatureList1, FeatureList2, absorbing_feature>::negative2,
           typename concat_list< typename list_search_all<FeatureList2, FeatureList1, absorbing_feature>::negative2,
                                 typename list_search_all<FeatureList1, FeatureList2, identical>::positive >::type >::type
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
struct check_container_features<Container, void> : True {};

template <typename Container, typename Head, typename Tail>
struct check_container_features<Container, cons<Head,Tail> > {
   static const bool value=check_container_features<Container, Head>::value &&
                           check_container_features<Container, Tail>::value;
};

template <typename Feature, typename Container>
struct filter_iterator_features_helper
   : derived_from<default_check_container_feature<Container,Feature>, checked_via_iterator> {};

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
   : ensure_features_helper<Container, Features>,
     container_traits<typename ensure_features_helper<Container, Features>::container> {};

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
                           identical<typename object_traits<typename iterator_traits<Iterator>::value_type>::persistent_type,
                                     typename object_traits<typename deref<T>::type>::persistent_type
                                    >::value;
};

template <typename Iterator, typename arg_type,
          bool _not_deref=identical<arg_type,void>::value || derived_from<Iterator, typename deref<arg_type>::type>::value>
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
   : unary_op_builder<Operation, typename iterator_traits<Iterator>::derivable_type> {
   typedef typename iterator_traits<Iterator>::derivable_type it_super;
   typedef star_helper<it_super, typename unary_helper::operation::argument_type> star;
   static const bool data_arg=star::data_arg;
   static typename star::const_result_type get(const it_super& it) { return star::get(it); }
   static typename star::mutable_result_type get(it_super& it) { return star::get(it); }
};

template <typename IteratorPair, typename Operation>
struct binary_helper
   : binary_op_builder<Operation, typename IteratorPair::super, typename IteratorPair::second_type> {
   typedef typename IteratorPair::super it_super;
   typedef typename IteratorPair::second_type it_second;
   typedef binary_op_builder<Operation, it_super, it_second> _super;
   typedef star_helper<it_super, typename _super::operation::first_argument_type> star1;
   typedef star_helper<it_second, typename _super::operation::second_argument_type> star2;
   static const bool first_data_arg=star1::data_arg, second_data_arg=star2::data_arg;
   static typename star1::const_result_type get1(const it_super& it) { return star1::get(it); }
   static typename star2::const_result_type get2(const it_second& it) { return star2::get(it); }
   static typename star1::mutable_result_type get1(it_super& it) { return star1::get(it); }
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

template <typename> class Container;
template <typename> class Container1;
template <typename> class Container2;
template <typename> class Operation;
template <typename> class IteratorConstructor;
template <typename> class IteratorCoupler;
template <typename> class Hidden;
typedef Hidden<True> MasqueradedTop;
template <typename> class ExpectedFeatures;
template <typename> class FeaturesViaSecond;
template <typename> class Bijective;
template <typename> class PartiallyDefined;

template <typename Top, typename Params, bool _has_hidden=extract_type_param<Params,Hidden>::specified>
class manip_container_top : public manip_container_base {
public:
   typedef void hidden_type;
   typedef typename extract_type_param<Params, ExpectedFeatures>::type expected_features;
   typedef Top manip_top_type;
   typedef void must_enforce_features;
   typedef void can_enforce_features;
   typedef void cannot_enforce_features;

   Top& manip_top() { return *static_cast<Top*>(this); }
   const Top& manip_top() const { return *static_cast<const Top*>(this); }
};

template <typename Container, typename ProvidedFeatures, typename Params>
class manip_container_top<manip_feature_collector<Container, ProvidedFeatures>, Params, false>
   : public manip_container_base {
public:
   typedef void hidden_type;
   typedef typename mix_features<typename extract_type_param<Params, ExpectedFeatures>::type, ProvidedFeatures>::type
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

template <typename Top, typename hidden>
struct manip_container_hidden_helper {
   typedef hidden type;
};

template <typename Top>
struct manip_container_hidden_helper<Top, True>
   : extract_template_type_param<Top,0> {};

template <typename Top, typename Params,
          bool _binary=extract_type_param<Params, Container1>::specified || extract_type_param<Params, Container2>::specified>
class manip_container_hidden_defaults {
public:
   typedef typename manip_container_hidden_helper<Top, typename extract_type_param<Params,Hidden>::type>::type hidden_type;
   typedef typename deref<typename extract_type_param<Params,Container,hidden_type>::type>::minus_ref container;

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
   typedef typename manip_container_hidden_helper<Top, typename extract_type_param<Params,Hidden>::type>::type hidden_type;
   typedef typename deref<typename extract_type_param<Params,Container1,hidden_type>::type>::minus_ref container1;
   typedef typename deref<typename extract_type_param<Params,Container2,hidden_type>::type>::minus_ref container2;

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
   : public manip_container_top<Top, Params, false>,
     public manip_container_hidden_defaults<Top, Params> {
protected:
   manip_container_top();
   ~manip_container_top();
public:
   typedef typename manip_container_hidden_defaults<Top,Params>::hidden_type hidden_type;

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
          bool _reversible=container_traits<Container>::is_bidirectional>
class construct_rewindable
   : public enable_if<Container, container_traits<Container>::is_forward>::type {
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
   typedef construct_end_sensitive<Container, false> _super;
public:
   typedef iterator_range<typename Container::reverse_iterator> reverse_iterator;
   typedef iterator_range<typename Container::const_reverse_iterator> const_reverse_iterator;

   reverse_iterator rbegin()
   {
      typedef typename _super::end_source end_source;
      return reverse_iterator(Container::rbegin(), end_source::rend());
   }
   reverse_iterator rend()
   {
      return reverse_iterator(Container::rend());
   }
   const_reverse_iterator rbegin() const
   {
      typedef typename _super::end_source end_source;
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
struct absorbing_feature<provide_construction<end_sensitive, on_top>, contractable> : True {};

template <typename Container, typename Feature, bool on_top>
struct default_check_container_feature<Container, provide_construction<Feature,on_top> > : False {};

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
   typedef construct_reversed<Container,false> _super;
public:
   typename _super::reference operator[] (int i)
   {
      return (_super::hidden())[this->size()-1-i];
   }

   typename _super::const_reference operator[] (int i) const
   {
      return (_super::hidden())[this->size()-1-i];
   }
};

template <typename Container>
struct default_check_container_feature<Container, _reversed> : False {};

template <typename Container>
struct default_enforce_feature<Container, _reversed> {
   typedef construct_reversed<Container> container;
};

template <typename Feature>
struct feature_allow_order<_reversed,Feature> : False {};

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
struct check_container_feature<construct_reversed<Container,_random>, _reversed> : True {};

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

template <typename Value, bool _simple=is_pod<Value>::value>
class op_value_cache {
   Value *value;
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

template <typename ResultRef, bool _need_proxy=!attrib<ResultRef>::is_reference>
struct arrow_helper {
   typedef typename attrib<ResultRef>::minus_ref *pointer;

   template <typename Iterator>
   static pointer get(const Iterator& it) { return &(*it); }
};

template <typename Result>
struct arrow_helper<Result, true> {
   class pointer {
      template <typename,bool> friend struct arrow_helper;
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

DeclNestedTemplate1CHECK(mix_in);

template <typename Iterator, typename Operation, bool _has_mixin=has_nested_mix_in<Operation>::value>
struct transform_iterator_base {
   typedef typename iterator_traits<Iterator>::derivable_type type;
};

template <typename Iterator, typename Operation>
struct transform_iterator_base<Iterator, Operation, true> {
   typedef typename Operation::template mix_in<Iterator> type;
};

template <typename Iterator, typename Operation>
class unary_transform_eval : public transform_iterator_base<Iterator,Operation>::type {
protected:
   typedef typename transform_iterator_base<Iterator,Operation>::type super;
public:
   typedef unary_helper<Iterator,Operation> helper;
   typedef typename helper::operation operation;
protected:
   operation op;

   typedef Operation op_arg_type;

   unary_transform_eval() {}

   template <typename Operation2>
   unary_transform_eval(const unary_transform_eval<typename iterator_traits<Iterator>::iterator, Operation2>& it)
      : super(it),
        op(helper::create(it.op)) {}

   template <typename Operation2>
   unary_transform_eval(const unary_transform_eval<typename iterator_reversed<Iterator>::type, Operation2>& it)
      : super(iterator_reversed<Iterator>::revert(it)),
        op(helper::create(it.op)) {}

   unary_transform_eval(const Iterator& cur_arg, const op_arg_type& op_arg)
      : super(cur_arg), op(helper::create(op_arg)) {}
   unary_transform_eval(typename alt_constructor<Iterator>::arg_type& cur_arg, const op_arg_type& op_arg)
      : super(cur_arg), op(helper::create(op_arg)) {}

   template <typename,typename> friend class unary_transform_eval;
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
   typedef unary_transform_eval<Iterator, Operation> _super;
protected:
   typedef unary_helper<Iterator,IndexOperation> ihelper;
   typename ihelper::operation iop;

   typedef pair<Operation, IndexOperation> op_arg_type;

   unary_transform_eval() {}

   template <typename Operation2, typename IndexOperation2>
   unary_transform_eval(const unary_transform_eval<typename iterator_traits<Iterator>::iterator,
                                                   pair<Operation2, IndexOperation2> >& it)
      : _super(it),
        iop(ihelper::create(it.iop)) {}

   template <typename Operation2, typename IndexOperation2>
   unary_transform_eval(const unary_transform_eval<typename iterator_reversed<Iterator>::type, pair<Operation2, IndexOperation2> >& it)
      : _super(it),
        iop(ihelper::create(it.op)) {}

   unary_transform_eval(const Iterator& cur_arg, const op_arg_type& op_arg)
      : _super(cur_arg, op_arg.first),
        iop(ihelper::create(op_arg.second)) {}
   unary_transform_eval(typename alt_constructor<Iterator>::arg_type& cur_arg, const op_arg_type& op_arg)
      : _super(cur_arg, op_arg.first),
        iop(ihelper::create(op_arg.second)) {}

   template <typename,typename> friend class unary_transform_eval;
public:
   int index() const
   {
      return iop(*ihelper::get(*this));
   }
};

template <typename Iterator, typename IndexOperation>
class unary_transform_eval<Iterator, pair<nothing, IndexOperation> >
   : public transform_iterator_base<Iterator,IndexOperation>::type {
protected:
   typedef typename transform_iterator_base<Iterator,IndexOperation>::type super;

   typedef unary_helper<Iterator,IndexOperation> ihelper;
   typename ihelper::operation iop;

   typedef IndexOperation op_arg_type;

   unary_transform_eval() {}

   template <typename IndexOperation2>
   unary_transform_eval(const unary_transform_eval<typename iterator_traits<Iterator>::iterator,
                                                   pair<nothing, IndexOperation2> >& it)
      : super(it),
        iop(ihelper::create(it.iop)) {}

   template <typename IndexOperation2>
   unary_transform_eval(const unary_transform_eval<typename iterator_reversed<Iterator>::type, pair<nothing, IndexOperation2> >& it)
      : super(it),
        iop(ihelper::create(it.op)) {}

   unary_transform_eval(const Iterator& cur_arg, const op_arg_type& op_arg)
      : super(cur_arg),
        iop(ihelper::create(op_arg)) {}
   unary_transform_eval(typename alt_constructor<Iterator>::arg_type& cur_arg, const op_arg_type& op_arg)
      : super(cur_arg),
        iop(ihelper::create(op_arg)) {}

   template <typename,typename> friend class unary_transform_eval;
public:
   int index() const
   {
      return iop(*ihelper::get(*this));
   }
};

template <typename Iterator, typename Operation>
class unary_transform_iterator
   : public unary_transform_eval<Iterator, Operation> {
   typedef unary_transform_eval<Iterator, Operation> _super;
protected:
   typedef typename _super::super super;
public:
   typedef typename deref<typename _super::reference>::type value_type;
   typedef unary_transform_iterator<typename iterator_traits<Iterator>::iterator,
                                    typename operation_cross_const_helper<Operation>::operation>
      iterator;
   typedef unary_transform_iterator<typename iterator_traits<Iterator>::const_iterator,
                                    typename operation_cross_const_helper<Operation>::const_operation>
      const_iterator;

   unary_transform_iterator() {}

   template <typename Operation2>
   unary_transform_iterator(const unary_transform_iterator<typename iterator_traits<Iterator>::iterator, Operation2>& it)
      : _super(it) {}

   template <typename Operation2>
   explicit unary_transform_iterator(const unary_transform_iterator<typename iterator_reversed<Iterator>::type, Operation2>& it)
      : _super(it) {}

   unary_transform_iterator(const Iterator& cur_arg,
                            const typename _super::op_arg_type& op_arg=typename _super::op_arg_type())
      : _super(cur_arg,op_arg) {}

   unary_transform_iterator& operator++ ()
   {
      super::operator++(); return *this;
   }
   const unary_transform_iterator operator++ (int)
   {
      unary_transform_iterator copy=*this;  operator++();  return copy;
   }

   unary_transform_iterator& operator-- ()
   {
      typedef typename enable_if<super, iterator_traits<Iterator>::is_bidirectional>::type error_if_unimplemented __attribute__((unused));
      error_if_unimplemented::operator--(); return *this;
   }
   const unary_transform_iterator operator-- (int)
   {
      unary_transform_iterator copy=*this;  operator--();  return copy;
   }

   unary_transform_iterator& operator+= (int i)
   {
      typedef typename enable_if<super, iterator_traits<Iterator>::is_random>::type error_if_unimplemented __attribute__((unused));
      error_if_unimplemented::operator+=(i);
      return *this;
   }
   unary_transform_iterator& operator-= (int i)
   {
      typedef typename enable_if<super, iterator_traits<Iterator>::is_random>::type error_if_unimplemented __attribute__((unused));
      error_if_unimplemented::operator-=(i);
      return *this;
   }
   const unary_transform_iterator operator+ (int i) const
   {
      typedef typename enable_if<unary_transform_iterator, iterator_traits<Iterator>::is_random>::type error_if_unimplemented __attribute__((unused));
      error_if_unimplemented copy=*this;  return copy+=i;
   }
   const unary_transform_iterator operator- (int i) const
   {
      typedef typename enable_if<unary_transform_iterator, iterator_traits<Iterator>::is_random>::type error_if_unimplemented __attribute__((unused));
      error_if_unimplemented copy=*this; return copy-=i;
   }
   friend const unary_transform_iterator operator+ (int i, const unary_transform_iterator& me)
   {
      return me+i;
   }

   typename super::difference_type
   operator- (const iterator& it) const 
   {
      typedef typename enable_if<super, iterator_traits<Iterator>::is_random>::type error_if_unimplemented __attribute__((unused));
      return static_cast<const error_if_unimplemented&>(*this) - it;
   }
   typename super::difference_type
   operator- (typename assign_const<const_iterator, !identical<iterator,const_iterator>::value>::type& it) const
   {
      typedef typename enable_if<super, iterator_traits<Iterator>::is_random>::type error_if_unimplemented __attribute__((unused));
      return static_cast<const error_if_unimplemented&>(*this) - it;
   }

protected:
   typename _super::reference _random(int i, True) const
   {
      return this->op(super::operator[](i));
   }
   typename _super::reference _random(int i, False) const
   {
      return this->op(static_cast<const super&>(*this) + i);
   }
public:
   typename _super::reference operator[] (int i) const
   {
      typedef typename enable_if<typename _super::helper, iterator_traits<Iterator>::is_random>::type error_if_unimplemented __attribute__((unused));
      return _random(i, bool2type<error_if_unimplemented::data_arg>());
   }
};

template <typename Iterator, typename Operation, typename Feature>
struct check_iterator_feature<unary_transform_iterator<Iterator, Operation>, Feature>
   : check_iterator_feature<Iterator, Feature> {};

template <typename Iterator, typename Operation>
struct check_iterator_feature<unary_transform_iterator<Iterator, Operation>, indexed> {
   static const bool value=is_instance2_of<Operation,pair>::value ||
                           check_iterator_feature<Iterator,indexed>::value;
};

template <typename Iterator, typename Operation> inline
unary_transform_iterator<Iterator, Operation>
make_unary_transform_iterator(Iterator it, const Operation& op)
{
   return unary_transform_iterator<Iterator, Operation>(it, op);
}

template <typename Params=void>
struct unary_transform_constructor {
   typedef Params params;
   template <typename Iterator, typename Operation, typename ExpectedFeatures>
   struct defs {
      typedef typename if_else<is_instance2_of<Operation,pair>::value,
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
struct default_check_container_feature<Container, pure_sparse> : False {};

template <typename Container>
struct default_check_container_feature<Container, dense> {
   static const bool value=!check_container_feature<Container, sparse>::value;
};

template <typename Container> inline
typename enable_if<int, check_container_feature<Container,sparse_compatible>::value>::type
get_dim(const Container& c)
{
   return c.dim();
}
template <typename Container> inline
typename disable_if<int, check_container_feature<Container,sparse_compatible>::value>::type
get_dim(const Container& c)
{
   return c.size();
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
class output_transform_iterator : public iterator_traits<Iterator>::derivable_type {
protected:
   Operation op;

   typedef typename iterator_traits<Iterator>::derivable_type super;
public:
   typedef output_iterator_tag iterator_category;
   typedef typename deref<typename Operation::argument_type>::type value_type;

   output_transform_iterator() {}

   output_transform_iterator(const Iterator& cur_arg, const Operation& op_arg=Operation())
      : super(cur_arg), op(op_arg) {}

   output_transform_iterator& operator= (typename Operation::argument_type arg)
   {
      *static_cast<super*>(this)=op(arg);
      return *this;
   }

   template <typename Arg>
   output_transform_iterator& operator= (const Arg& arg)
   {
      *static_cast<super*>(this)=op(arg);
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

template <typename Iterator1, typename Iterator2, typename Params=void>
class iterator_pair
   : public iterator_traits<Iterator1>::derivable_type {
public:
   typedef Iterator1 first_type;
   typedef Iterator2 second_type;

   typedef typename iterator_traits<Iterator1>::derivable_type super;
   Iterator2 second;
protected:
   typedef typename extract_type_param<Params,FeaturesViaSecond>::type features_via_second;
public:
   typedef typename least_derived< cons<typename iterator_traits<Iterator1>::iterator_category,
                                        typename iterator_traits<Iterator2>::iterator_category> >::type
      iterator_category;
   typedef typename iterator_traits< typename if_else<check_iterator_feature<Iterator1,unlimited>::value,
                                                      Iterator2, Iterator1>::type >::difference_type
      difference_type;
   typedef iterator_pair<typename iterator_traits<Iterator1>::iterator,
                         typename iterator_traits<Iterator2>::iterator, Params>
      iterator;
   typedef iterator_pair<typename iterator_traits<Iterator1>::const_iterator,
                         typename iterator_traits<Iterator2>::const_iterator, Params>
      const_iterator;

   iterator_pair() {}

   iterator_pair(const Iterator1& first_arg, const Iterator2& second_arg)
      : super(first_arg), second(second_arg) {}
   iterator_pair(const Iterator1& first_arg,
                 typename alt_constructor<Iterator2>::arg_type& second_arg)
      : super(first_arg), second(second_arg) {}
   iterator_pair(typename alt_constructor<Iterator1>::arg_type& first_arg,
                 const Iterator2& second_arg)
      : super(first_arg), second(second_arg) {}
   iterator_pair(typename alt_constructor<Iterator1>::arg_type& first_arg,
                 typename alt_constructor<Iterator2>::arg_type& second_arg)
      : super(first_arg), second(second_arg) {}

   iterator_pair(const iterator& it)
      : super(static_cast<const typename iterator::super&>(it)), second(it.second) {}

   iterator_pair& operator= (const iterator& it)
   {
      super::operator=(static_cast<const typename iterator::super&>(it));
      second=it.second;
      return *this;
   }

   iterator_pair& operator++ ()
   {
      super::operator++(); ++second;
      return *this;
   }
   const iterator_pair operator++ (int)
   {
      iterator_pair copy=*this; operator++(); return copy;
   }

   iterator_pair& operator-- ()
   {
      typedef typename enable_if<super, iterator_pair_traits<Iterator1,Iterator2>::is_bidirectional>::type
         error_if_unimplemented __attribute__((unused));
      error_if_unimplemented::operator--(); --this->second;
      return *this;
   }
   const iterator_pair operator-- (int)
   {
      iterator_pair copy=*this;  operator--();  return copy;
   }

   iterator_pair& operator+= (int i)
   {
      typedef typename enable_if<super, iterator_pair_traits<Iterator1,Iterator2>::is_bidirectional>::type
         error_if_unimplemented __attribute__((unused));
      error_if_unimplemented::operator+=(i); this->second+=i;
      return *this;
   }
   iterator_pair& operator-= (int i)
   {
      typedef typename enable_if<super, iterator_pair_traits<Iterator1,Iterator2>::is_random>::type
         error_if_unimplemented __attribute__((unused));
      error_if_unimplemented::operator-=(i); this->second-=i;
      return *this;
   }
   const iterator_pair operator+ (int i) const
   {
      iterator_pair copy=*this; return copy+=i;
   }
   const iterator_pair operator- (int i) const
   {
      iterator_pair copy=*this; return copy-=i;
   }
   friend const iterator_pair operator+ (int i, const iterator_pair& it)
   {
      return it+i;
   }

private:
   template <typename IteratorPair>
   difference_type _diff(const IteratorPair& it, False) const
   {
      return static_cast<const super&>(*this)-it;
   }
   template <typename IteratorPair>
   difference_type _diff(const IteratorPair& it, True) const
   {
      return second-it.second;
   }
   template <typename IteratorPair>
   bool _eq(const IteratorPair& it, False) const
   {
      return static_cast<const super&>(*this) == it;
   }
   template <typename IteratorPair>
   bool _eq(const IteratorPair& it, True) const
   {
      return second==it.second;
   }
   static const bool _cmp2 = list_search<features_via_second, end_sensitive, absorbing_feature>::value;

public:
   difference_type operator- (const iterator& it) const
   {
      typedef typename enable_if<super, iterator_pair_traits<Iterator1,Iterator2>::is_random>::type
         error_if_unimplemented __attribute__((unused));
      return _diff(it, bool2type<_cmp2>());
   }
   difference_type operator- (typename assign_const<const_iterator, !identical<iterator,const_iterator>::value>::type& it) const
   {
      typedef typename enable_if<difference_type, iterator_pair_traits<Iterator1,Iterator2>::is_random>::type
         error_if_unimplemented __attribute__((unused));
      return _diff(it, bool2type<_cmp2>());
   }

   bool operator== (const iterator& it) const
   {
      return _eq(it, bool2type<_cmp2>());
   }
   bool operator== (typename assign_const<const_iterator, !identical<iterator,const_iterator>::value>::type& it) const
   {
      return _eq(it, bool2type<_cmp2>());
   }

   bool operator!= (const iterator& it) const
   {
      return !operator==(it);
   }
   bool operator!= (typename assign_const<const_iterator, !identical<iterator,const_iterator>::value>::type& it) const
   {
      return !operator==(it);
   }
private:
   bool _at_end(False) const
   {
      return super::at_end();
   }
   bool _at_end(True) const
   {
      return second.at_end();
   }
   static const bool _at_end2=list_search<features_via_second, end_sensitive, absorbing_feature>::value ||
                              !check_iterator_feature<Iterator1, end_sensitive>::value,
                     _enable_at_end= _at_end2 ? check_iterator_feature<Iterator2, end_sensitive>::value
                                              : check_iterator_feature<Iterator1, end_sensitive>::value;
public:
   bool at_end() const
   {
      typedef typename enable_if<bool, _enable_at_end>::type error_if_unimplemented __attribute__((unused));
      return _at_end(bool2type<_at_end2>());
   }
private:
   int _index(False) const
   {
      return super::index();
   }
   int _index(True) const
   {
      return second.index();
   }
   static const bool _index2=list_search<features_via_second, indexed, absorbing_feature>::value ||
                             !check_iterator_feature<Iterator1, indexed>::value,
                     _enable_index= _index2 ? check_iterator_feature<Iterator2, indexed>::value
                                            : check_iterator_feature<Iterator1, indexed>::value;
public:
   int index() const
   {
      typedef typename enable_if<int, _enable_index>::type error_if_unimplemented __attribute__((unused));
      return _index(bool2type<_index2>());
   }
protected:
   static const bool _rewind1 = !(list_search<features_via_second, rewindable, absorbing_feature>::value),
                     _enable_rewind= (check_iterator_feature<Iterator1, rewindable>::value || !_rewind1) &&
                                     check_iterator_feature<Iterator2, rewindable>::value;
   void rewind1(True) { super::rewind(); }
   void rewind1(False) {}
public:
   void rewind()
   {
      typedef typename enable_if<void,_enable_rewind>::type error_if_unimplemented __attribute__((unused));
      rewind1(bool2type<_rewind1>());
      second.rewind();
   }
protected:
   void contract1(bool, int distance_front, int, False)
   {
      std::advance(static_cast<super&>(*this), distance_front);
   }
   void contract1(bool renumber, int distance_front, int distance_back, True)
   {
      super::contract(renumber, distance_front, distance_back);
   }
   void contract2(bool, int distance_front, int, False)
   {
      std::advance(second, distance_front);
   }
   void contract2(bool renumber, int distance_front, int distance_back, True)
   {
      second.contract(renumber, distance_front, distance_back);
   }
public:
   void contract(bool renumber, int distance_front, int distance_back=0)
   {
      if (!list_search<features_via_second, contractable, equivalent_features>::value)
         contract1(renumber, distance_front, distance_back, bool2type<check_iterator_feature<Iterator1, contractable>::value>());
      contract2(renumber, distance_front, distance_back, bool2type<check_iterator_feature<Iterator2, contractable>::value>());
   }
};

template <typename Iterator1, typename Iterator2, typename Params, typename Feature>
struct check_iterator_feature< iterator_pair<Iterator1, Iterator2, Params>, Feature> {
   typedef cons<end_sensitive, indexed> usual_or_features;

   static const bool
      check1 = check_iterator_feature<Iterator1,Feature>::value,
      check2 = check_iterator_feature<Iterator2,Feature>::value,
      value = list_search<typename extract_type_param<Params,FeaturesViaSecond>::type, Feature, absorbing_feature>::value
              ? check2 :
              list_contains<usual_or_features, Feature>::value
              ? check1 || check2
              : check1 && check2;
};

template <typename Params=void>
struct pair_coupler {
   typedef cons<end_sensitive, indexed> usual_or_features;

   template <typename Iterator1, typename Iterator2, typename ExpectedFeatures>
   struct defs {
      typedef typename list_search_all<ExpectedFeatures, usual_or_features, equivalent_features>::positive or_features;
      typedef typename list_search_all<ExpectedFeatures, usual_or_features, equivalent_features>::negative and_features;
      typedef typename list_search_all<Iterator1, or_features, check_iterator_feature>::positive2 first_can;
      typedef typename list_search_all<Iterator1, or_features, check_iterator_feature>::negative2 first_can_not;
      typedef typename if_else< check_iterator_feature<Iterator2,unlimited>::value, void, first_can_not >::type
         explicitly_via_second;
      typedef typename if_else< identical<explicitly_via_second,void>::value, Params,
                                typename concat_list<FeaturesViaSecond<explicitly_via_second>,Params>::type
                              >::type
         it_params;
      typedef iterator_pair<Iterator1, Iterator2, it_params> iterator;
      typedef typename if_else< check_iterator_feature<Iterator2,unlimited>::value,
                                ExpectedFeatures,
                                and_features >::type
         needed_features1;
      typedef typename if_else< check_iterator_feature<Iterator2,unlimited>::value,
                                and_features,
                                typename list_search_all<ExpectedFeatures, first_can, equivalent_features>::negative >::type
         needed_features2;
   };
};

template <typename IteratorPair, typename Operation, bool _partial>
class binary_transform_eval : public transform_iterator_base<IteratorPair,Operation>::type {
protected:
   typedef typename transform_iterator_base<IteratorPair,Operation>::type super;
public:
   typedef binary_helper<IteratorPair,Operation> helper;
   typedef typename helper::operation operation;
protected:
   operation op;

   typedef Operation op_arg_type;

   binary_transform_eval() {}

   template <typename Operation2>
   binary_transform_eval(const binary_transform_eval<typename iterator_traits<super>::iterator, Operation2, _partial>& it)
      : super(it), op(helper::create(it.op)) {}

   binary_transform_eval(const IteratorPair& cur_arg,
                         const op_arg_type& op_arg)
      : super(cur_arg),
        op(helper::create(op_arg)) {}
   binary_transform_eval(typename alt_constructor<IteratorPair>::arg_type& cur_arg,
                         const op_arg_type& op_arg)
      : super(cur_arg),
        op(helper::create(op_arg)) {}

   binary_transform_eval(const typename IteratorPair::first_type& first_arg,
                         const typename IteratorPair::second_type& second_arg,
                         const op_arg_type& op_arg)
      : super(first_arg,second_arg),
        op(helper::create(op_arg)) {}
   binary_transform_eval(const typename IteratorPair::first_type& first_arg,
                         typename alt_constructor<typename IteratorPair::second_type>::arg_type& second_arg,
                         const op_arg_type& op_arg)
      : super(first_arg,second_arg),
        op(helper::create(op_arg)) {}
   binary_transform_eval(typename alt_constructor<typename IteratorPair::first_type>::arg_type& first_arg,
                         const typename IteratorPair::second_type& second_arg,
                         const op_arg_type& op_arg)
      : super(first_arg,second_arg),
        op(helper::create(op_arg)) {}
   binary_transform_eval(typename alt_constructor<typename IteratorPair::first_type>::arg_type& first_arg,
                         typename alt_constructor<typename IteratorPair::second_type>::arg_type& second_arg,
                         const op_arg_type& op_arg)
      : super(first_arg,second_arg),
        op(helper::create(op_arg)) {}

   template <typename,typename,bool> friend class binary_transform_eval;
public:
   typedef typename operation::result_type reference;

   reference operator* () const
   {
      return op(*helper::get1(*this), *helper::get2(this->second));
   }

   typedef typename arrow_helper<reference>::pointer pointer;
   pointer operator-> () const { return arrow_helper<reference>::get(*this); }
};

template <typename IteratorPair, typename Operation, typename IndexOperation, bool _partial>
class binary_transform_eval<IteratorPair, pair<Operation, IndexOperation>, _partial>
   : public binary_transform_eval<IteratorPair, Operation, _partial> {
   typedef binary_transform_eval<IteratorPair, Operation, _partial> _super;
protected:
   typedef binary_helper<IteratorPair,IndexOperation> ihelper;
   typename ihelper::operation iop;

   typedef pair<Operation, IndexOperation> op_arg_type;

   binary_transform_eval() {}

   template <typename Operation2, typename IndexOperation2>
   binary_transform_eval(const binary_transform_eval<typename iterator_traits<IteratorPair>::iterator,
                                                     pair<Operation2, IndexOperation2>, _partial>& it)
      : _super(it),
        iop(ihelper::create(it.iop)) {}

   binary_transform_eval(const IteratorPair& cur_arg,
                         const op_arg_type& op_arg)
      : _super(cur_arg,op_arg.first),
        iop(ihelper::create(op_arg.second)) {}
   binary_transform_eval(typename alt_constructor<IteratorPair>::arg_type& cur_arg,
                         const op_arg_type& op_arg)
      : _super(cur_arg,op_arg.first),
        iop(ihelper::create(op_arg.second)) {}

   binary_transform_eval(const typename IteratorPair::first_type& first_arg,
                         const typename IteratorPair::second_type& second_arg,
                         const op_arg_type& op_arg)
      : _super(first_arg,second_arg,op_arg.first),
        iop(ihelper::create(op_arg.second)) {}
   binary_transform_eval(const typename IteratorPair::first_type& first_arg,
                         typename alt_constructor<typename IteratorPair::second_type>::arg_type& second_arg,
                         const op_arg_type& op_arg)
      : _super(first_arg,second_arg,op_arg.first),
        iop(ihelper::create(op_arg.second)) {}
   binary_transform_eval(typename alt_constructor<typename IteratorPair::first_type>::arg_type& first_arg,
                         const typename IteratorPair::second_type& second_arg,
                         const op_arg_type& op_arg)
      : _super(first_arg,second_arg,op_arg.first),
        iop(ihelper::create(op_arg.second)) {}
   binary_transform_eval(typename alt_constructor<typename IteratorPair::first_type>::arg_type& first_arg,
                         typename alt_constructor<typename IteratorPair::second_type>::arg_type& second_arg,
                         const op_arg_type& op_arg)
      : _super(first_arg,second_arg,op_arg.first),
        iop(ihelper::create(op_arg.second)) {}

   template <typename,typename,bool> friend class binary_transform_eval;
public:
   int index() const
   {
      return iop(*ihelper::get1(*this), *ihelper::get2(this->second));
   }
};

template <typename IteratorPair, typename IndexOperation, bool _partial>
class binary_transform_eval<IteratorPair, pair<nothing, IndexOperation>, _partial>
   : public transform_iterator_base<IteratorPair,IndexOperation>::type {
protected:
   typedef typename transform_iterator_base<IteratorPair,IndexOperation>::type super;

   typedef binary_helper<IteratorPair,IndexOperation> ihelper;
   typename ihelper::operation iop;

   typedef IndexOperation op_arg_type;

   binary_transform_eval() {}

   template <typename IndexOperation2>
   binary_transform_eval(const binary_transform_eval<typename iterator_traits<IteratorPair>::iterator,
                                                     pair<nothing, IndexOperation2>, _partial>& it)
      : super(it),
        iop(ihelper::create(it.iop)) {}

   binary_transform_eval(const IteratorPair& cur_arg,
                         const op_arg_type& op_arg)
      : super(cur_arg),
        iop(ihelper::create(op_arg)) {}
   binary_transform_eval(typename alt_constructor<IteratorPair>::arg_type& cur_arg,
                         const op_arg_type& op_arg)
      : super(cur_arg),
        iop(ihelper::create(op_arg)) {}

   binary_transform_eval(const typename IteratorPair::first_type& first_arg,
                         const typename IteratorPair::second_type& second_arg,
                         const op_arg_type& op_arg)
      : super(first_arg,second_arg),
        iop(ihelper::create(op_arg)) {}
   binary_transform_eval(const typename IteratorPair::first_type& first_arg,
                         typename alt_constructor<typename IteratorPair::second_type>::arg_type& second_arg,
                         const op_arg_type& op_arg)
      : super(first_arg,second_arg),
        iop(ihelper::create(op_arg)) {}
   binary_transform_eval(typename alt_constructor<typename IteratorPair::first_type>::arg_type& first_arg,
                         const typename IteratorPair::second_type& second_arg,
                         const op_arg_type& op_arg)
      : super(first_arg,second_arg),
        iop(ihelper::create(op_arg)) {}
   binary_transform_eval(typename alt_constructor<typename IteratorPair::first_type>::arg_type& first_arg,
                         typename alt_constructor<typename IteratorPair::second_type>::arg_type& second_arg,
                         const op_arg_type& op_arg)
      : super(first_arg,second_arg),
        iop(ihelper::create(op_arg)) {}

   template <typename,typename,bool> friend class binary_transform_eval;
public:
   int index() const
   {
      return iop(*ihelper::get1(*this), *ihelper::get2(this->second));
   }
};

template <typename IteratorPair, typename Operation, bool _partial=false>
class binary_transform_iterator
   : public binary_transform_eval<IteratorPair, Operation, _partial> {
   typedef binary_transform_eval<IteratorPair, Operation, _partial> _super;
protected:
   typedef IteratorPair super;
public:
   typedef typename deref<typename _super::reference>::type value_type;
   typedef binary_transform_iterator<typename iterator_traits<IteratorPair>::iterator,
                                     typename operation_cross_const_helper<Operation>::operation, _partial>
      iterator;
   typedef binary_transform_iterator<typename iterator_traits<IteratorPair>::const_iterator,
                                     typename operation_cross_const_helper<Operation>::const_operation, _partial>
      const_iterator;

   binary_transform_iterator() {}

   template <typename Operation2>
   binary_transform_iterator(const binary_transform_iterator<typename iterator_traits<super>::iterator, Operation2, _partial>& it)
      : _super(it) {}

   binary_transform_iterator(const IteratorPair& cur_arg,
                             const typename _super::op_arg_type& op_arg=typename _super::op_arg_type())
      : _super(cur_arg,op_arg) {}

   binary_transform_iterator(const typename IteratorPair::first_type& first_arg,
                             const typename IteratorPair::second_type& second_arg,
                             const typename _super::op_arg_type& op_arg=typename _super::op_arg_type())
      : _super(first_arg,second_arg,op_arg) {}
   binary_transform_iterator(const typename IteratorPair::first_type& first_arg,
                             typename alt_constructor<typename IteratorPair::second_type>::arg_type& second_arg,
                             const typename _super::op_arg_type& op_arg=typename _super::op_arg_type())
      : _super(first_arg,second_arg,op_arg) {}
   binary_transform_iterator(typename alt_constructor<typename IteratorPair::first_type>::arg_type& first_arg,
                             const typename IteratorPair::second_type& second_arg,
                             const typename _super::op_arg_type& op_arg=typename _super::op_arg_type())
      : _super(first_arg,second_arg,op_arg) {}
   binary_transform_iterator(typename alt_constructor<typename IteratorPair::first_type>::arg_type& first_arg,
                             typename alt_constructor<typename IteratorPair::second_type>::arg_type& second_arg,
                             const typename _super::op_arg_type& op_arg=typename _super::op_arg_type())
      : _super(first_arg,second_arg,op_arg) {}

   binary_transform_iterator& operator++ ()
   {
      super::operator++(); return *this;
   }
   const binary_transform_iterator operator++ (int)
   {
      binary_transform_iterator copy=*this;  operator++();  return copy;
   }

   binary_transform_iterator& operator-- ()
   {
      typedef typename enable_if<super, iterator_traits<super>::is_bidirectional>::type error_if_unimplemented __attribute__((unused));
      error_if_unimplemented::operator--();
      return *this;
   }
   const binary_transform_iterator operator-- (int)
   {
      binary_transform_iterator copy=*this;  operator--();  return copy;
   }

   binary_transform_iterator& operator+= (int i)
   {
      typedef typename enable_if<super, iterator_traits<super>::is_bidirectional>::type error_if_unimplemented __attribute__((unused));
      error_if_unimplemented::operator+=(i);
      return *this;
   }
   binary_transform_iterator& operator-= (int i)
   {
      typedef typename enable_if<super, iterator_traits<super>::is_random>::type error_if_unimplemented __attribute__((unused));
      error_if_unimplemented::operator-=(i);
      return *this;
   }
   const binary_transform_iterator operator+ (int i) const
   {
      binary_transform_iterator copy=*this;  return copy+=i;
   }
   const binary_transform_iterator operator- (int i) const
   {
      binary_transform_iterator copy=*this;  return copy-=i;
   }
   friend const binary_transform_iterator operator+ (int i, const binary_transform_iterator& it)
   {
      return it+i;
   }

   typename super::difference_type
   operator- (const iterator& it) const
   {
      typedef typename enable_if<super, iterator_traits<super>::is_random>::type error_if_unimplemented __attribute__((unused));
      return static_cast<const error_if_unimplemented&>(*this) - it;
   }
   typename super::difference_type
   operator- (typename assign_const<const_iterator, !identical<iterator,const_iterator>::value>::type& it) const
   {
      typedef typename enable_if<super, iterator_traits<super>::is_random>::type error_if_unimplemented __attribute__((unused));
      return static_cast<const error_if_unimplemented&>(*this) - it;
   }

protected:
   typename _super::reference _random(int i, cons<True, True>) const
   {
      return this->op(super::operator[](i), this->second[i]);
   }
   typename _super::reference _random(int i, cons<True, False>) const
   {
      return this->op(super::operator[](i), this->second+i);
   }
   typename _super::reference _random(int i, cons<False, True>) const
   {
      return this->op(static_cast<const typename IteratorPair::super&>(*this)+i, this->second[i]);
   }
   typename _super::reference _random(int i, cons<False, False>) const
   {
      return this->op(static_cast<const typename IteratorPair::super&>(*this)+i, this->second+i);
   }
public:
   typename super::reference operator[] (int i) const
   {
      typedef typename enable_if<typename _super::helper, iterator_traits<super>::is_random>::type error_if_unimplemented __attribute__((unused));
      return _random(i, cons< bool2type<error_if_unimplemented::first_data_arg>, bool2type<error_if_unimplemented::second_data_arg> >());
   }
};

template <typename IteratorPair, typename Operation, bool _partial, typename Feature>
struct check_iterator_feature<binary_transform_iterator<IteratorPair, Operation, _partial>,
                              Feature>
   : check_iterator_feature<IteratorPair,Feature> {};

template <typename IteratorPair, typename Operation, bool _partial>
struct check_iterator_feature<binary_transform_iterator<IteratorPair, Operation, _partial>, indexed> {
   static const bool value=is_instance2_of<Operation,pair>::value ||
                           check_iterator_feature<IteratorPair, indexed>::value;
};

template <typename Iterator>
struct has_partial_state : False {};

template <typename Params=void>
struct binary_transform_constructor {
   typedef Params params;
   template <typename IteratorPair, typename Operation, typename ExpectedFeatures>
   struct defs {
      static const bool
         _partial = extract_bool_param<Params, PartiallyDefined, has_partial_state<IteratorPair>::value>::value;

      typedef typename if_else<is_instance2_of<Operation,pair>::value,
                               typename list_search_all<ExpectedFeatures, indexed, equivalent_features>::negative,
                               ExpectedFeatures >::type
         needed_pair_features;
      typedef void needed_features1;
      typedef void needed_features2;
      typedef binary_transform_iterator<IteratorPair, Operation, _partial> iterator;
   };
};

template <typename Iterator1, typename Iterator2, typename Operation> inline
binary_transform_iterator<iterator_pair<Iterator1,Iterator2>, Operation>
make_binary_transform_iterator(Iterator1 first, Iterator2 second, const Operation& op)
{
   return binary_transform_iterator<iterator_pair<Iterator1,Iterator2>, Operation>(first,second,op);
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
} // end namespace polymake

#endif // POLYMAKE_INTERNAL_ITERATORS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
