/* Copyright (c) 1997-2023
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

#include "polymake/internal/chunk_allocator.h"

namespace pm {

chunk_allocator::chunk_allocator(size_t obj_size_arg, size_t n_objects_in_chunk_arg)
   : obj_size((obj_size_arg+7UL) & ~7UL)  // round up to a multiple of sizeof(double)
   , n_objects_in_chunk(n_objects_in_chunk_arg
                        ? n_objects_in_chunk_arg
                        : (default_chunk_size-sizeof(char*)) / obj_size)
   , free_obj(nullptr)
   , last_obj(nullptr)
   , chunk_end(nullptr)
{}

void* chunk_allocator::allocate()
{
   void* result;
   if (free_obj) {
      result=free_obj;
      free_obj=*reinterpret_cast<char**>(free_obj);
   } else if (last_obj != chunk_end) {
      result=last_obj;
      last_obj+=obj_size;
   } else {
      const size_t chunk_length=sizeof(char*) + obj_size * n_objects_in_chunk;
      char* new_chunk=new char[chunk_length];
      *reinterpret_cast<char**>(new_chunk)=chunk_end;
      chunk_end=new_chunk+chunk_length;
      result=new_chunk+sizeof(char*);
      last_obj=new_chunk+sizeof(char*)+obj_size;
   }
   return result;
}

void chunk_allocator::reclaim(void* p)
{
   char* obj=reinterpret_cast<char*>(p);
   *reinterpret_cast<char**>(obj)=free_obj;
   free_obj=obj;
}

void chunk_allocator::release()
{
   const size_t chunk_length=sizeof(char*) + obj_size * n_objects_in_chunk;
   while (chunk_end) {
      char* chunk=chunk_end-chunk_length;
      chunk_end=*reinterpret_cast<char**>(chunk);
      delete[] chunk;
   }
}

void chunk_allocator::clear()
{
   release();
   free_obj=nullptr;
   last_obj=nullptr;
   chunk_end=nullptr;
}

}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
