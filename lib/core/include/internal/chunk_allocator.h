/* Copyright (c) 1997-2022
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

#pragma once

#include <cstdlib>

namespace pm {

//! Maintains a list of private memory chunks of fixed size.
class chunk_allocator {
public:
   static const size_t default_chunk_size=4096;

   explicit chunk_allocator(size_t obj_size_arg, size_t n_objects_in_chunk_arg = 0);

   void* allocate();
   void reclaim(void* p);
   // give all chunks back to the system
   void clear();

   ~chunk_allocator() { release(); }

   size_t get_object_size() const { return obj_size; }

protected:
   void release();

   size_t obj_size;
   size_t n_objects_in_chunk;
   char* free_obj;
   char* last_obj;
   char* chunk_end;

private:
   // deleted
   chunk_allocator(const chunk_allocator&);
   void operator= (const chunk_allocator&);
};

}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
