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

#ifndef POLYMAKE_INTERNAL_INDEX_DISPENSER_H
#define POLYMAKE_INTERNAL_INDEX_DISPENSER_H

#include "polymake/internal/iterators.h"
#include "polymake/vector"
#include <cassert>

namespace pm {

template <typename Traits>
class IndexDispenser : protected Traits {
   std::vector<int> indices;
   int start, first_free;

   void init(int reserve_size)
   {
      if (reserve_size) {
         copy_range(entire(sequence(1, reserve_size-1)), indices.begin());
         indices.back()=-1;
         first_free=0;
      } else {
         first_free=-1;
      }
   }

   int lease()
   {
      if (first_free<0) {
         first_free=indices.size();
         indices.push_back(-1);
         Traits::resize(first_free+start+1);
      }
      int i=first_free;
      first_free=indices[i];
      return i+start;
   }

   void reclaim(int i)
   {
      i-=start;
      indices[i]=first_free;
      first_free=i;
   }

public:
   explicit IndexDispenser(int start_arg=0, int reserve_size=0)
      : indices(reserve_size), start(start_arg)
   {
      assert(reserve_size>=0);
      init(reserve_size);
   }

   IndexDispenser(const Traits& super_arg, int start_arg=0, int reserve_size=0)
      : Traits(super_arg), indices(reserve_size), start(start_arg)
   {
      assert(reserve_size>=0);
      init(reserve_size);
   }

   void clear(int new_start=0, int new_size=0)
   {
      indices.resize(new_size);
      start=new_start;
      init(new_size);
   }

   class agent {
      mutable IndexDispenser *master;
      int i;

      friend class IndexDispenser;

      agent(IndexDispenser& m)
         : master(&m)
      {
         i=m.lease();
      }

   public:
      agent() : master(0) { }
      operator int() const { return i; }

      agent& operator= (IndexDispenser& m)
      {
         if (master) master->reclaim(i);
         master=&m;
         i=m.lease();
         return *this;
      }

      agent& operator= (const agent& other)
      {
         master=other.master; i=other.i;
         other.master=NULL;
      }

      ~agent() { if (master) master->reclaim(i); }
   };

   agent get() { return *this; }
};

} // end namespace pm

namespace polymake {
   using pm::IndexDispenser;
}

#endif // POLYMAKE_INTERNAL_INDEX_DISPENSER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
