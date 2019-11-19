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

#ifndef POLYMAKE_HEAP_H
#define POLYMAKE_HEAP_H

#include "polymake/vector"
#include "polymake/internal/comparators.h"
#include <cassert>

namespace pm {

/** Heap (priority queue).
    The queue is stored in a dynamic array, with element indices inducing an implicit binary tree structure
    (the child elements of the i-th element have indices 2*i+1 and 2*i+2.)

    \tparam Policy class defining the data types and housekeeping methods:

       value_type
         elements stored in the queue

       int position(value_type) const
         current position (index) of the element in the heap
       void update_position(value_type, int old, int new)
         change the current position of stored in/with the element;  -1 on any side means "none"
       <any_type> key(value_type)
         retrieve the key of the element
       <any_type> key_comparator() const
         retrieve the key comparator object

       keys, values, and comparator may be passed and/or returned by const reference if desired
*/
template <typename Policy>
class Heap : public Policy {
public:
   using typename Policy::value_type;

   // TODO: investigate whether std::deque performs better for large problem instances
   using queue_t = std::vector<value_type>;

   //! Create an empty heap
   template <typename... Args, typename=std::enable_if_t<std::is_constructible<Policy, Args...>::value>>
   explicit Heap(Args&&... args)
      : Policy(std::forward<Args>(args)...)
   {}

   //! Create an empty heap
   //! @param expected_qlen expected maximal heap size (helps to avoid extra reallocations)
   //! @param args optional arguments for constructing the base Policy
   template <typename... Args, typename=std::enable_if_t<std::is_constructible<Policy, Args...>::value>>
   explicit Heap(size_t expected_qlen, Args&&... args)
      : Policy(std::forward<Args>(args)...)
   {
      queue.reserve(expected_qlen);
   }

   const queue_t& get_queue() const { return queue; }

   bool empty() const { return queue.empty(); }

   void clear() { queue.clear(); }

   /// Add a new element or update the position of the existing one after a key increase/decrease.
   void push(const value_type& elem);

   /// The currently topmost element
   const value_type& top() const { return queue.front(); }

   /// Sift the topmost element down if its key has been increased.
   void update_top()
   {
      sift_down(0, 0, 0);
   }

   /// Remove the topmost element and return it, adjust the heap
   value_type pop()
   {
      assert(!queue.empty());
      value_type top=queue.front();
      assert(this->position(top)==0);
      sift_down(queue.size()-1, 0, 1);
      queue.pop_back();
      this->update_position(top, 0, -1);
      return top;
   }

   /// Remove the element
   void erase(const value_type& elem)
   {
      erase_at(this->position(elem));
   }

   /// Remove element at the given queue position
   value_type erase_at(int pos);

private:
   /// @param new_pos where to start looking for the new position
   void sift_down(int old_pos, int pos, int shrinking);

   queue_t queue;

#if POLYMAKE_DEBUG
public:
   bool sanity_check() const;
#endif
};

template <typename Policy>
void Heap<Policy>::push(const value_type& elem)
{
   const int old_pos=this->position(elem);
   bool moved=false;
   int pos=old_pos;
   if (old_pos < 0) {
      pos=queue.size();
      queue.push_back(elem);
   } else {
      assert(size_t(old_pos)<queue.size() && queue[old_pos]==elem);
   }

   const auto& k=this->key(elem);
   auto&& cmp=this->key_comparator();
   while (pos>0) {
      const int p_pos=(pos-1)/2;  // parent node in the heap tree
      if (cmp(this->key(queue[p_pos]), k) <= 0) break;
      this->update_position(queue[pos]=queue[p_pos], p_pos, pos);
      pos=p_pos;
      moved=true;
   }
   if (moved) {
      queue[pos]=elem;
      this->update_position(elem, old_pos, pos);
   } else if (old_pos>=0) {
      sift_down(old_pos, old_pos, 0);
   } else {
      this->update_position(elem, old_pos, pos);
   }
}

template <typename Policy>
void Heap<Policy>::sift_down(int old_pos, int pos, int shrinking)
{
   const int end=queue.size()-shrinking;
   const auto& k=this->key(queue[old_pos]);
   auto&& cmp=this->key_comparator();
   int c_pos;
   while ((c_pos=2*pos+1) < end) {
      if (c_pos+1 < end &&
          cmp(this->key(queue[c_pos+1]), this->key(queue[c_pos])) < 0) ++c_pos;
      if (cmp(k, this->key(queue[c_pos])) <= 0) break;
      this->update_position(queue[pos]=queue[c_pos], c_pos, pos);
      pos=c_pos;
   }
   if (pos != old_pos) {
      this->update_position(queue[pos]=queue[old_pos], old_pos, pos);
   }
}

template <typename Policy>
typename Heap<Policy>::value_type Heap<Policy>::erase_at(int pos)
{
   const value_type v=queue[pos];
   this->update_position(v, pos, -1);
   const int last_q=queue.size()-1;
   if (pos < last_q) {
      const auto& k=this->key(queue.back());
      auto&& cmp=this->key_comparator();
      bool bubble_up=false;
      int p_pos;
      while ((p_pos=(pos-1)/2) > 0) {
         if (cmp(k, this->key(queue[p_pos])) >= 0) break;
         this->update_position(queue[pos]=queue[p_pos], p_pos, pos);
         bubble_up=true;
         pos=p_pos;
      }
      if (bubble_up)
         this->update_position(queue[pos]=queue.back(), last_q, pos);
      else
         sift_down(last_q, pos, 1);
   }
   queue.pop_back();
   return v;
}

#if POLYMAKE_DEBUG
template <typename Policy>
bool Heap<Policy>::sanity_check() const
{
   bool OK=true;
   for (int i=0, iend=queue.size(); i<iend; ++i) {
      const value_type& el=queue[i];
      const int pos=this->position(el);
      if (pos != i) {
         std::cerr << "check(Heap): elem " << el << " has wrong index " << pos << " instead of " << i << std::endl;
         OK=false;
      }
      if (i>0) {
         int p=(i-1)/2;
         if (this->key_comparator()(this->key(el), this->key(queue[p]))<0) {
            std::cerr << "check(Heap): parent(" << el << ")=" << p << std::endl;
            OK=false;
         }
      }
   }
   return OK;
}
#endif

} // end namespace pm

namespace polymake {
   using pm::Heap;
}

#endif // POLYMAKE_HEAP_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
