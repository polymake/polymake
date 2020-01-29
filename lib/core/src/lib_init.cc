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

#include "polymake/internal/pool_allocator.h"

#include <cstring>
#include <memory>
#include <cstdlib>
#include <cassert>
#include <gmp.h>

extern "C" {
   void* __gmp_default_allocate(size_t);
}

namespace pm {

// clang also defines __GNUC__, this is only for libstdc++
#if defined(__GLIBCXX__)

namespace {

class pool_allocator_constants : public __gnu_cxx::__pool_alloc_base {
public:
   static constexpr size_t align = _S_align, limit = _S_max_bytes;
};

}

void* pm::allocator::reallocate(void* p, size_t old_sz, size_t new_sz)
{
   if (!p) {
      assert(old_sz == 0);
      return allocate(new_sz);
   }
   static const bool use_new = getenv("GLIBCPP_FORCE_NEW") || getenv("GLIBCXX_FORCE_NEW");
   constexpr size_t align_mask = pool_allocator_constants::align-1;
   if (!use_new && ((old_sz+align_mask) & ~align_mask) == ((new_sz+align_mask) & ~align_mask) && new_sz < pool_allocator_constants::limit)
      return p;
   void* new_p = allocate(new_sz);
   if (new_p) {
      std::memcpy(new_p, p, old_sz < new_sz ? old_sz : new_sz);
      deallocate(p, old_sz);
   }
   return new_p;
}

#else  // libc++

void* pm::allocator::reallocate(void* p, size_t old_sz, size_t new_sz)
{
   void* new_p = allocate(new_sz);
   if (new_p) {
      std::memcpy(new_p, p, old_sz < new_sz ? old_sz : new_sz);
      deallocate(p,old_sz);
   }
   return new_p;
}

#endif

namespace {

allocator gmp_allocator;
void* pm_gmp_allocate(size_t n) { return gmp_allocator.allocate(n); }
void pm_gmp_deallocate(void* p, size_t n) { gmp_allocator.deallocate(p, n); }
void* pm_gmp_reallocate(void* p, size_t old_sz, size_t new_sz) { return gmp_allocator.reallocate(p, old_sz, new_sz); }

void init_gmp_memory_management() __attribute__((constructor));

void init_gmp_memory_management()
{
   // switch to custom GMP allocators only if no other component did it before
   typedef void* (*alloc_t)(size_t);
   alloc_t was_alloc;
   mp_get_memory_functions(&was_alloc, nullptr, nullptr);
   if (was_alloc == &__gmp_default_allocate)
      mp_set_memory_functions(pm_gmp_allocate, pm_gmp_reallocate, pm_gmp_deallocate);
}

}
}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
