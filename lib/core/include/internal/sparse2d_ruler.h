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

#ifndef POLYMAKE_INTERNAL_SPARSE2D_RULER_H
#define POLYMAKE_INTERNAL_SPARSE2D_RULER_H

#include "polymake/internal/iterators.h"

namespace pm { namespace sparse2d {

template <typename Container, typename prefix_data=nothing>
class ruler
   : public plain_array< ruler<Container, prefix_data>, Container > {
protected:
   Int alloc_size;
   pair<Int, prefix_data> size_and_prefix;
   Container containers[1];

   static constexpr Int min_alloc = 20;

   friend class plain_array< ruler<Container, prefix_data>, Container >;

   Container* get_data() { return containers; }
   const Container* get_data() const { return containers; }

   static size_t total_size(size_t n)
   {
      return sizeof(ruler)-sizeof(Container)+n*sizeof(Container);
   }

   void init(Int n)
   {
      Container* cur = containers + size_and_prefix.first;
      for (Int i = size_and_prefix.first; i < n; ++i, ++cur)
         construct_at(cur, i);
      size_and_prefix.first = n;
   }

   static ruler* allocate(Int n)
   {
      allocator alloc;
      ruler* r = reinterpret_cast<ruler*>(alloc.allocate(total_size(n)));
      r->alloc_size = n;
      if (!std::is_pod<prefix_data>::value && !std::is_same<prefix_data, nothing>::value)
         construct_at(&r->size_and_prefix.second);
      r->size_and_prefix.first = 0;
      return r;
   }

   static void deallocate(ruler *r)
   {
      allocator alloc;
      alloc.deallocate(reinterpret_cast<allocator::value_type*>(r), total_size(r->alloc_size));
   }

   void destroy_containers()
   {
      for (typename ruler::reverse_iterator cur=this->rbegin(), end=this->rend();  cur != end;  ++cur)
         destroy_at(cur.operator->());
   }

public:
   static ruler* construct(Int n)
   {
      ruler* r = allocate(n);
      r->init(n);
      return r;
   }

   static ruler* construct(const ruler& r2, Int add = 0)
   {
      Int n = r2.size();
      ruler* r = allocate(n+add);
      Container* cur = r->containers;
      Container* end = cur + n;
      for (auto src = r2.begin();  cur < end;  ++cur, ++src)
         construct_at(cur, *src);
      for (end += add; cur < end; ++cur, ++n)
         construct_at(cur, n);
      r->size_and_prefix.first = n;
      return r;
   }

   template <typename Iterator>
   static ruler* construct(Int n, Iterator src)
   {
      ruler* r = allocate(n);
      Int i = 0;
      for (Container *cur = r->containers, *end = cur + n;  cur != end;  ++cur, ++src, ++i)
         construct_at(cur, i, *src);
      r->size_and_prefix.first = n;
      return r;
   }

   static void destroy(ruler* r)
   {
      r->destroy_containers();
      deallocate(r);
   }

   /* Extending: creates new objects;
                 allocates additional space anticipating further extensions (at least for 20 items or ~20% of old size)
      Truncating: deletes trailing objects;
                  allocates the exact amount of space
   */
   static ruler* resize(ruler *old, Int n, const bool should_delete = true)
   {
      Int n_alloc = n;
      Int diff = n_alloc - old->alloc_size;
      if (diff <= 0) {
         if (n > old->size_and_prefix.first) {  // extending, but still enough spare space
            old->init(n);
            return old;
         }

         // truncating
         if (should_delete) {
            Container* cur = old->containers + old->size_and_prefix.first;
            Container* first = old->containers + n;
            while (cur > first)
               destroy_at(--cur);
         }
         old->size_and_prefix.first = n;

         if (-diff <= std::max(old->alloc_size/5, +min_alloc)) {       // the savings are not significant
            return old;
         }
      } else {  // extending, need realloc
         assign_max(diff, +min_alloc);
         assign_max(diff, old->alloc_size/5);
         n_alloc = old->alloc_size + diff;
      }

      ruler* r = allocate(n_alloc);
      for (auto src = old->begin(), src_end = old->end(), dst = r->begin();  src != src_end;  ++src, ++dst)
         new(dst.operator->()) Container(std::move(*src));
      r->size_and_prefix = old->size_and_prefix;

      deallocate(old);
      r->init(n);
      return r;
   }

   static ruler* resize_and_clear(ruler* r, Int n)
   {
      Container* cur = r->containers + r->size_and_prefix.first;
      Container* first = r->containers;
      while (cur > first)
         destroy_at(--cur);

      Int n_alloc = n;
      Int diff = n_alloc - r->alloc_size, m = std::max(r->alloc_size/5, +min_alloc);
      
      if (diff <= 0 ? -diff > m : (n_alloc = r->alloc_size + std::max(diff, m), true)) {
         deallocate(r);
         r = allocate(n_alloc);
      } else {
         r->size_and_prefix.first = 0;
      }
      r->init(n);
      return r;
   }

   /// perm[i]==j => !inverse: old[j] moves to new[i]
   ///                inverse: old[i] moves to new[j]
   template <typename TPerm, typename PermuteEntries, bool inverse>
   static ruler* permute(ruler* old, const TPerm& perm, PermuteEntries&& perm_entries, bool_constant<inverse>)
   {
      Int n = old->size_and_prefix.first;
      ruler* r = allocate(n);
      auto perm_it = perm.begin();
      for (Container *src = old->containers, *dst = r->containers, *end = dst + n;  dst != end;  ++dst, ++perm_it)
         perm_entries.relocate(src + (inverse ? 0 : *perm_it), dst + (inverse ? *perm_it : 0));
      r->size_and_prefix = old->size_and_prefix;
      perm_entries(old, r);
      deallocate(old);
      return r;
   }

   prefix_data& prefix() { return size_and_prefix.second; }
   const prefix_data& prefix() const { return size_and_prefix.second; }

   static ruler& reverse_cast(Container* cur, Int i)
   {
      return *pm::reverse_cast(cur, i, &ruler::containers);
   }

   static const ruler& reverse_cast(const Container* cur, Int i)
   {
      return *pm::reverse_cast(cur, i, &ruler::containers);
   }

   Int size() const { return size_and_prefix.first; }
   Int max_size() const { return alloc_size; }
};

} // end namespace sparse2d

template <typename Container, typename prefix_data>
struct spec_object_traits< sparse2d::ruler<Container, prefix_data> > : spec_object_traits<is_container> {
   static constexpr int is_resizeable = 0;    // since it has no standard resize() method
};

}

#endif // POLYMAKE_INTERNAL_SPARSE2D_RULER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
