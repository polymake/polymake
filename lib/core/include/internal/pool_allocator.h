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

#ifndef POLYMAKE_INTERNAL_POOL_ALLOCATOR_H
#define POLYMAKE_INTERNAL_POOL_ALLOCATOR_H

// This file is a wrapper for gcc >= 3.4

#include <ext/pool_allocator.h>

namespace std {
template <typename T> class allocator;
}

namespace pm {
typedef std::allocator<char[1]> allocator;
}

#define __glibcxx_base_allocator __gnu_cxx::__pool_alloc

#endif // POLYMAKE_INTERNAL_POOL_ALLOCATOR_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
