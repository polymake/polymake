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

#ifndef POLYMAKE_INTERNAL_SPARSE2D_RULER_H
#define POLYMAKE_INTERNAL_SPARSE2D_RULER_H

#include "polymake/pair.h"

namespace pm { namespace sparse2d {

template <typename Container, typename prefix_data=nothing>
class ruler {
protected:
   int _alloc_size;
   pair<int, prefix_data> size_and_prefix;
   Container containers[1];

   static size_t total_size(size_t n)
   {
      return sizeof(ruler)-sizeof(Container)+n*sizeof(Container);
   }

   void init(int n)
   {
      Container *cur=containers+size_and_prefix.first;
      for (int i=size_and_prefix.first; i<n; ++i, ++cur)
         new(cur) Container(i);
      size_and_prefix.first=n;
   }

   static ruler* allocate(int n)
   {
      allocator alloc;
      ruler *r=reinterpret_cast<ruler*>(alloc.allocate(total_size(n)));
      r->_alloc_size=n;
      if (!is_pod<prefix_data>::value && !identical<prefix_data,nothing>::value)
         new(&r->size_and_prefix.second) prefix_data;
      r->size_and_prefix.first=0;
      return r;
   }

   static void deallocate(ruler *r)
   {
      allocator alloc;
      alloc.deallocate(reinterpret_cast<allocator::value_type*>(r), total_size(r->_alloc_size));
   }
public:
   static ruler* construct(int n)
   {
      ruler *r=allocate(n);
      r->init(n);
      return r;
   }

   static ruler* construct(const ruler& r2, int add=0)
   {
      int n=r2.size();
      ruler *r=allocate(n+add);
      Container *cur=r->containers, *end=cur+n;
      for (const Container *src=r2.begin();  cur<end;  ++cur, ++src)
         new(cur) Container(*src);
      for (end+=add; cur<end; ++cur, ++n)
         new(cur) Container(n);
      r->size_and_prefix.first=n;
      return r;
   }

   template <typename Iterator>
   static ruler* construct(int n, Iterator src)
   {
      ruler *r=allocate(n);
      int i=0;
      for (Container *cur=r->containers, *end=cur+n;  cur != end;  ++cur, ++src, ++i)
         new(cur) Container(i,*src);
      r->size_and_prefix.first=n;
      return r;
   }

   static void destroy(ruler *r)
   {
      Container *cur=r->end(), *first=r->begin();
      while (cur > first)
         std::_Destroy(--cur);
      deallocate(r);
   }

   /* Extending: creates new objects;
                 allocates additional space anticipating further extensions (at least for 20 items or ~20% of old size)
      Truncating: deletes trailing objects;
                  allocates the exact amount of space
   */
   static ruler* resize(ruler *old, int n, bool _do_delete=true)
   {
      int n_alloc=n;
      int diff=n_alloc - old->_alloc_size;
      if (diff<=0) {
         if (n > old->size_and_prefix.first) {  // extending, but still enough spare space
            old->init(n);
            return old;
         }

         // truncating
         if (_do_delete) {
            Container *cur=old->containers+old->size_and_prefix.first, *first=old->containers+n;
            while (cur > first)
               std::_Destroy(--cur);
         }
         old->size_and_prefix.first=n;

         if (-diff <= std::max(20, old->_alloc_size/5)) {       // the savings are not significant
            return old;
         }
      } else {  // extending, need realloc
         diff=std::max(std::max(diff, 20), old->_alloc_size/5);
         n_alloc=old->_alloc_size+diff;
      }

      ruler *r=allocate(n_alloc);
      for (Container *src=old->begin(), *src_end=old->end(), *dst=r->containers;  src!=src_end;  ++src, ++dst)
         relocate(src,dst);
      r->size_and_prefix=old->size_and_prefix;

      deallocate(old);
      r->init(n);
      return r;
   }

   static ruler* resize_and_clear(ruler *r, int n)
   {
      Container *cur=r->containers+r->size_and_prefix.first, *first=r->containers;
      while (cur > first)
         std::_Destroy(--cur);

      int n_alloc=n;
      int diff=n_alloc - r->_alloc_size, m=std::max(20, r->_alloc_size/5);
      
      if (diff<=0 ? -diff>m : (n_alloc=r->_alloc_size+std::max(diff, m), true)) {
         deallocate(r);
         r=allocate(n_alloc);
      } else {
         r->size_and_prefix.first=0;
      }
      r->init(n);
      return r;
   }

   /// perm[i]==j => !inverse: old[j] moves to new[i]
   ///                inverse: old[i] moves to new[j]
   template <typename Iterator, typename PermuteEntries, bool inverse>
   static ruler* permute(ruler *old, Iterator perm, const PermuteEntries& perm_entries, bool2type<inverse>)
   {
      int n=old->size_and_prefix.first;
      ruler *r=allocate(n);
      for (Container *src=old->containers, *dst=r->containers, *end=dst+n;  dst!=end;  ++dst, ++perm)
         perm_entries.relocate(src+(inverse ? 0 : *perm), dst+(inverse ? *perm : 0));
      r->size_and_prefix.first=n;
      perm_entries(old,r);
      deallocate(old);
      return r;
   }

   prefix_data& prefix() { return size_and_prefix.second; }
   const prefix_data& prefix() const { return size_and_prefix.second; }

   static ruler& reverse_cast(Container* cur, int i)
   {
      return *pm::reverse_cast(cur,i,&ruler::containers);
   }

   static const ruler& reverse_cast(const Container* cur, int i)
   {
      return *pm::reverse_cast(cur,i,&ruler::containers);
   }

   typedef Container value_type;
   typedef value_type& reference;
   typedef const value_type& const_reference;
   typedef value_type* iterator;
   typedef const value_type* const_iterator;
   typedef std::reverse_iterator<iterator> reverse_iterator;
   typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

   int size() const { return size_and_prefix.first; }
   int empty() const { return size()==0; }
   int max_size() const { return _alloc_size; }

   iterator begin() { return containers; }
   iterator end() { return containers + size(); }
   const_iterator begin() const { return containers; }
   const_iterator end() const { return containers + size(); }

   reverse_iterator rbegin() { return reverse_iterator(end()); }
   reverse_iterator rend() { return reverse_iterator(begin()); }
   const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
   const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

   reference front() { return containers[0]; }
   reference back() { return containers[size()-1]; }
   reference operator[] (int i) { return containers[i]; }
   const_reference front() const { return containers[0]; }
   const_reference back() const { return containers[size()-1]; }
   const_reference operator[] (int i) const { return containers[i]; }
};

} // end namespace sparse2d

template <typename Container, typename prefix_data>
struct spec_object_traits< sparse2d::ruler<Container,prefix_data> > : spec_object_traits<is_container> {
   static const int is_resizeable=0;    // since it has no standard resize() method
};

}

#endif // POLYMAKE_INTERNAL_SPARSE2D_RULER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
