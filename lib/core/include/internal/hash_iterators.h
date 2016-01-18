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

#ifndef POLYMAKE_INTERNAL_HASH_ITERATORS_H
#define POLYMAKE_INTERNAL_HASH_ITERATORS_H

#include "polymake/internal/comparators.h"

namespace pm {

#if defined(__GLIBCXX__) && defined(__cplusplus) && __cplusplus >= 201103L && !defined(PM_FORCE_TR1)

template <typename Value, bool __constant_iterators, bool __cache>
struct iterator_cross_const_helper< std::__detail::_Node_iterator<Value,__constant_iterators,__cache>, true> {
   typedef std::__detail::_Node_iterator<Value,__constant_iterators,__cache> iterator;
   typedef std::__detail::_Node_const_iterator<Value,__constant_iterators,__cache> const_iterator;
};

template <typename Value, bool __constant_iterators, bool __cache>
struct iterator_cross_const_helper< std::__detail::_Node_const_iterator<Value,__constant_iterators,__cache>, true> {
   typedef std::__detail::_Node_iterator<Value,__constant_iterators,__cache> iterator;
   typedef std::__detail::_Node_const_iterator<Value,__constant_iterators,__cache> const_iterator;
};

#elif defined(__GLIBCXX__)

   template <typename Value, bool __constant_iterators, bool __cache>
   struct iterator_cross_const_helper< std::tr1::__detail::_Hashtable_iterator<Value,__constant_iterators,__cache>, true> {
      typedef std::tr1::__detail::_Hashtable_iterator<Value,__constant_iterators,__cache> iterator;
      typedef std::tr1::__detail::_Hashtable_const_iterator<Value,__constant_iterators,__cache> const_iterator;
   };

   template <typename Value, bool __constant_iterators, bool __cache>
   struct iterator_cross_const_helper< std::tr1::__detail::_Hashtable_const_iterator<Value,__constant_iterators,__cache>, true> {
      typedef std::tr1::__detail::_Hashtable_iterator<Value,__constant_iterators,__cache> iterator;
      typedef std::tr1::__detail::_Hashtable_const_iterator<Value,__constant_iterators,__cache> const_iterator;
   };

#elif defined(_LIBCPP_VERSION)

template <typename Iterator>
struct iterator_cross_const_helper<std::__1::__hash_map_iterator<Iterator>,true> {
   typedef std::__1::__hash_map_iterator<typename iterator_traits<Iterator>::iterator> iterator;
   typedef std::__1::__hash_map_const_iterator<typename iterator_traits<Iterator>::const_iterator> const_iterator;
};
template <typename Iterator>
struct iterator_cross_const_helper<std::__1::__hash_map_const_iterator<Iterator>,true> {
   typedef std::__1::__hash_map_iterator<typename iterator_traits<Iterator>::iterator> iterator;
   typedef std::__1::__hash_map_const_iterator<typename iterator_traits<Iterator>::const_iterator> const_iterator;
};

template <typename Iterator>
struct iterator_cross_const_helper<std::__1::__hash_iterator<Iterator>,true> {
   typedef std::__1::__hash_iterator<typename iterator_traits<Iterator>::iterator> iterator;
   typedef std::__1::__hash_const_iterator<typename iterator_traits<Iterator>::const_iterator> const_iterator;
};
template <typename Iterator>
struct iterator_cross_const_helper<std::__1::__hash_const_iterator<Iterator>,true> {
   typedef std::__1::__hash_iterator<typename iterator_traits<Iterator>::iterator> iterator;
   typedef std::__1::__hash_const_iterator<typename iterator_traits<Iterator>::const_iterator> const_iterator;
};

#endif

   template <typename Key, typename Params>
   struct hash_table_cmp_adapter {
      typedef typename extract_type_param<Params, Comparator, operations::cmp>::type key_comparator;
      typedef operations::cmp2eq<key_comparator, Key> type;
   };

} // end namespace pm

#endif // POLYMAKE_INTERNAL_HASH_ITERATORS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
