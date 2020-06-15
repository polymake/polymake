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

#ifndef POLYMAKE_INTERNAL_POOL_ALLOCATOR_H
#define POLYMAKE_INTERNAL_POOL_ALLOCATOR_H

#include <cstddef>
#include <memory>

#if defined(__GLIBCXX__)
# include <ext/pool_allocator.h>
# define PM_ALLOCATOR_BASE __gnu_cxx::__pool_alloc
#else 
# define PM_ALLOCATOR_BASE std::allocator
#endif

namespace pm {

class allocator : public PM_ALLOCATOR_BASE<char> {
   using base_t = PM_ALLOCATOR_BASE<char>;
public:
   void* allocate(std::size_t n)
   {
      return base_t::allocate(n, nullptr);
   }
   void deallocate(void* p, std::size_t n)
   {
      base_t::deallocate(reinterpret_cast<char*>(p), n);
   }
   void* reallocate(void* p, std::size_t old_sz, std::size_t new_sz);

   template <typename Data, typename... Args>
   Data* construct(Args&&... args)
   {
      return new(allocate(sizeof(Data))) Data(std::forward<Args>(args)...);
   }

   template <typename Data>
   void destroy(Data* p)
   {
      p->~Data();
      deallocate(p, sizeof(Data));
   }
};

}

#undef PM_ALLOCATOR_BASE

#endif // POLYMAKE_INTERNAL_POOL_ALLOCATOR_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
