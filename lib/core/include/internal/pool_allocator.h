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

#ifndef POLYMAKE_INTERNAL_POOL_ALLOCATOR_H
#define POLYMAKE_INTERNAL_POOL_ALLOCATOR_H

// make sure macros for version library tests are loaded
#include <cstddef>

#if defined(__GLIBCXX__)

#include <ext/pool_allocator.h>

namespace std {
template <typename T> class allocator;
}

#elif defined(_LIBCPP_VERSION)

#include <memory>

#endif

namespace pm {
typedef std::allocator<char[1]> allocator;
}

#ifdef __GLIBCXX__
#define __glibcxx_base_allocator __gnu_cxx::__pool_alloc
#endif

#endif // POLYMAKE_INTERNAL_POOL_ALLOCATOR_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
