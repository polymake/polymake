/* Copyright (c) 1997-2019
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

#ifndef POLYMAKE_INTERNAL_TREE_ITERATORS_H
#define POLYMAKE_INTERNAL_TREE_ITERATORS_H

#include "polymake/internal/iterators.h"

namespace pm {

#if defined(__GLIBCXX__)

template <typename Tp>
struct iterator_cross_const_helper< std::_Rb_tree_iterator<Tp>, true> {
   typedef std::_Rb_tree_iterator<Tp> iterator;
   typedef std::_Rb_tree_const_iterator<Tp> const_iterator;
};

template <typename Tp>
struct iterator_cross_const_helper< std::_Rb_tree_const_iterator<Tp>, true> {
   typedef std::_Rb_tree_iterator<Tp> iterator;
   typedef std::_Rb_tree_const_iterator<Tp> const_iterator;
};

#elif defined(_LIBCPP_VERSION)

template <typename Tp, typename NodePtr, typename DiffType>
struct iterator_cross_const_helper< std::__tree_iterator<Tp,NodePtr,DiffType>, true> {
   typedef std::__tree_iterator<Tp,NodePtr,DiffType> iterator;
   typedef std::__tree_const_iterator<Tp,NodePtr,DiffType> const_iterator;
};

template <typename Tp, typename NodePtr, typename DiffType>
struct iterator_cross_const_helper< std::__tree_const_iterator<Tp,NodePtr,DiffType>, true> {
   typedef std::__tree_iterator<Tp,NodePtr,DiffType> iterator;
   typedef std::__tree_const_iterator<Tp,NodePtr,DiffType> const_iterator;
};

#endif

}
#endif // POLYMAKE_INTERNAL_TREE_ITERATORS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
