/* Copyright (c) 1997-2014
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

#include "polymake/internal/lib_init.h"

#include <cstring>
#include <memory>
#include <cstdio>
#include <gmp.h>

namespace {

#if defined(__GNUC__)

__gnu_cxx::__pool_alloc<char> gmp_allocator;
void* pm_gmp_allocate(size_t n) { return gmp_allocator.allocate(n); }
void pm_gmp_deallocate(void* p, size_t n) { gmp_allocator.deallocate(reinterpret_cast<char*>(p), n); }

void* pm_gmp_reallocate(void* p, size_t old_sz, size_t new_sz)
{
   static const bool use_new=getenv("GLIBCPP_FORCE_NEW") || getenv("GLIBCXX_FORCE_NEW");
   const size_t align=8, limit=128;
   if (!use_new && ((old_sz+align-1)&~(align-1)) == ((new_sz+align-1)&~(align-1)) && new_sz < limit)
      return p;
   void* new_p=pm_gmp_allocate(new_sz);
   if (new_p) {
      std::memcpy(new_p, p, old_sz < new_sz ? old_sz : new_sz);
      pm_gmp_deallocate(p,old_sz);
   }
   return new_p;
}

#elif defined(__INTEL_COMPILER)

std::allocator<char> gmp_allocator;
void* pm_gmp_allocate(size_t n) { return gmp_allocator.allocate(n); }
void pm_gmp_deallocate(void* p, size_t n) { gmp_allocator.deallocate(reinterpret_cast<char*>(p), n); }

void* pm_gmp_reallocate(void* p, size_t old_sz, size_t new_sz)
{
   void* new_p=pm_gmp_allocate(new_sz);
   if (new_p) {
      std::memcpy(new_p, p, old_sz < new_sz ? old_sz : new_sz);
      pm_gmp_deallocate(p,old_sz);
   }
   return new_p;
}

#endif // __INTEL_COMPILER

}
namespace pm {

void init_gmp_memory_management()
{
   mp_set_memory_functions(pm_gmp_allocate, pm_gmp_reallocate, pm_gmp_deallocate);
}

}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
