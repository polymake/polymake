/* Copyright (c) 1997-2020
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

#ifndef POLYMAKE_INTERNAL_HASH_ITERATORS_H
#define POLYMAKE_INTERNAL_HASH_ITERATORS_H

#include "polymake/internal/comparators.h"
#include "polymake/meta_list.h"

namespace pm {

#if defined(__GLIBCXX__)

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

#elif defined(_LIBCPP_VERSION)

template <typename Iterator>
struct iterator_cross_const_helper<std::__hash_map_iterator<Iterator>, true> {
   typedef std::__hash_map_iterator<typename iterator_traits<Iterator>::iterator> iterator;
   typedef std::__hash_map_const_iterator<typename iterator_traits<Iterator>::const_iterator> const_iterator;
};
template <typename Iterator>
struct iterator_cross_const_helper<std::__hash_map_const_iterator<Iterator>, true> {
   typedef std::__hash_map_iterator<typename iterator_traits<Iterator>::iterator> iterator;
   typedef std::__hash_map_const_iterator<typename iterator_traits<Iterator>::const_iterator> const_iterator;
};

#if _LIBCPP_VERSION >= 3900 || (defined(__APPLE__) && __clang_major__ >= 8 && _LIBCPP_VERSION >= 3700 )

template <typename NodePtr>
struct iterator_cross_const_helper<std::__hash_iterator<NodePtr>, true> {
   typedef std::__hash_iterator<NodePtr> iterator;
   typedef std::__hash_const_iterator<NodePtr> const_iterator;
};
template <typename NodePtr>
struct iterator_cross_const_helper<std::__hash_const_iterator<NodePtr>, true> {
   typedef std::__hash_iterator<NodePtr> iterator;
   typedef std::__hash_const_iterator<NodePtr> const_iterator;
};

#else

template <typename Iterator>
struct iterator_cross_const_helper<std::__hash_iterator<Iterator>, true> {
   typedef std::__hash_iterator<typename iterator_traits<Iterator>::iterator> iterator;
   typedef std::__hash_const_iterator<typename iterator_traits<Iterator>::const_iterator> const_iterator;
};
template <typename Iterator>
struct iterator_cross_const_helper<std::__hash_const_iterator<Iterator>, true> {
   typedef std::__hash_iterator<typename iterator_traits<Iterator>::iterator> iterator;
   typedef std::__hash_const_iterator<typename iterator_traits<Iterator>::const_iterator> const_iterator;
};

#endif
#endif

template <typename Key, typename... TParams>
struct hash_table_cmp_adapter {
   typedef typename mlist_wrap<TParams...>::type params;
   typedef typename mtagged_list_extract<params, ComparatorTag>::type key_comparator;
   typedef typename std::conditional<std::is_same<key_comparator, void>::value, std::equal_to<Key>, operations::cmp2eq<key_comparator, Key>>::type type;
};

} // end namespace pm

#endif // POLYMAKE_INTERNAL_HASH_ITERATORS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
