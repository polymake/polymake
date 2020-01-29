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

#ifndef POLYMAKE_INTERNAL_TREE_CONTAINERS_H
#define POLYMAKE_INTERNAL_TREE_CONTAINERS_H

#include "polymake/internal/modified_containers.h"

namespace pm {

template <typename Top, typename Params=typename Top::manipulator_params,
          bool is_modified=mtagged_list_extract<Params, OperationTag>::is_specified>
class modified_tree_impl
   : public modified_container_impl<Top, Params> {
   using base_t = modified_container_impl<Top, Params>;
public:
   using typename base_t::iterator;
   using typename base_t::const_iterator;
   using typename base_t::reverse_iterator;
   using typename base_t::const_reverse_iterator;
   using tree_type = typename base_t::container::tree_type;
protected:
   iterator finalize(typename tree_type::iterator&& it)
   {
      return iterator(std::move(it), this->manip_top().get_operation());
   }
   const_iterator finalize(typename tree_type::const_iterator&& it) const
   {
      return const_iterator(std::move(it), this->manip_top().get_operation());
   }
   reverse_iterator finalize(typename tree_type::reverse_iterator&& it)
   {
      return reverse_iterator(std::move(it), this->manip_top().get_operation());
   }
   const_reverse_iterator finalize(typename tree_type::const_reverse_iterator&& it) const
   {
      return const_reverse_iterator(std::move(it), this->manip_top().get_operation());
   }
};

template <typename Top, typename Params>
class modified_tree_impl<Top, Params, false>
   : public redirected_container<Top, Params> {
   using base_t = redirected_container<Top, Params>;
public:
   using typename base_t::iterator;
   using typename base_t::const_iterator;
   using typename base_t::reverse_iterator;
   using typename base_t::const_reverse_iterator;
protected:
   iterator finalize(iterator&& it) { return std::move(it); }
   const_iterator finalize(const_iterator&& it) const { return std::move(it); }
   reverse_iterator finalize(reverse_iterator&& it) { return std::move(it); }
   const_reverse_iterator finalize(const_reverse_iterator&& it) const { return std::move(it); }
};

template <typename Top, typename Params=typename Top::manipulator_params>
class modified_tree
   : public modified_tree_impl<Top, Params> {
   using base_t = modified_tree_impl<Top, Params>;
public:
   using typename base_t::iterator;
   using typename base_t::const_iterator;
   using typename base_t::reverse_iterator;
   using typename base_t::const_reverse_iterator;
   using tree_type = typename base_t::container::tree_type;

   template <typename Key>
   iterator find(const Key& k)
   {
      return this->finalize(this->manip_top().get_container().find(k));
   }
   template <typename Key>
   const_iterator find(const Key& k) const
   {
      return this->finalize(this->manip_top().get_container().find(k));
   }
   template <typename Key, typename Comparator>
   iterator find(const Key& k, const Comparator& comparator)
   {
      return this->finalize(this->manip_top().get_container().find(k,comparator));
   }
   template <typename Key, typename Comparator>
   const_iterator find(const Key& k, const Comparator& comparator) const
   {
      return this->finalize(this->manip_top().get_container().find(k,comparator));
   }

   template <typename Key, typename RelOp>
   iterator find_nearest(const Key& k, const RelOp& relop)
   {
      return this->finalize(this->manip_top().get_container().find_nearest(k,relop));
   }
   template <typename Key, typename RelOp>
   const_iterator find_nearest(const Key& k, const RelOp& relop) const
   {
      return this->finalize(this->manip_top().get_container().find_nearest(k,relop));
   }
   template <typename Key, typename RelOp, typename Comparator>
   iterator find_nearest(const Key& k, const RelOp& relop, const Comparator& comparator)
   {
      return this->finalize(this->manip_top().get_container().find_nearest(k,relop,comparator));
   }
   template <typename Key, typename RelOp, typename Comparator>
   const_iterator find_nearest(const Key& k, const RelOp& relop, const Comparator& comparator) const
   {
      return this->finalize(this->manip_top().get_container().find_nearest(k,relop,comparator));
   }

   template <typename Key>
   bool exists(const Key& k) const
   {
      return this->manip_top().get_container().exists(k);
   }

   // synonym for sets
   template <typename Key>
   bool contains(const Key& k) const { return exists(k); }

   // TODO: align the naming with STL as per C++17

   template <typename... Args>
   auto insert(Args&&... args)
   {
      return this->finalize(this->manip_top().get_container().insert(std::forward<Args>(args)...));
   }

   template <typename... Args>
   auto emplace(Args&&... args)
   {
      return this->finalize(this->manip_top().get_container().insert_new(std::forward<Args>(args)...));
   }

   template <typename... Args>
   void erase(Args&&... args)
   {
      this->manip_top().get_container().erase(std::forward<Args>(args)...);
   }

   template <typename Key>
   iterator toggle(const Key& k)
   {
      return this->finalize(this->manip_top().get_container().toggle(k));
   }

   template <typename Key>
   void push_front(const Key& k)
   {
      this->manip_top().get_container().push_front(k);
   }
   template <typename Key, typename Data>
   void push_front(const Key& k, const Data& data)
   {
      this->manip_top().get_container().push_front(k,data);
   }

   template <typename Key>
   void push_back(const Key& k)
   {
      this->manip_top().get_container().push_back(k);
   }
   template <typename Key, typename Data>
   void push_back(const Key& k, const Data& data)
   {
      this->manip_top().get_container().push_back(k,data);
   }
   void pop_front()
   {
      this->manip_top().get_container().pop_front();
   }
   void pop_back()
   {
      this->manip_top().get_container().pop_back();
   }

   bool tree_form() const
   {
      return this->manip_top().get_container().tree_form();
   }
   Int max_size() const
   {
      return this->manip_top().get_container().max_size();
   }
   void clear()
   {
      this->manip_top().get_container().clear();
   }

   const typename tree_type::key_comparator_type&
   get_comparator() const
   {
      return this->manip_top().get_container().get_comparator();
   }
};

} // end namespace pm

#endif // POLYMAKE_INTERNAL_TREE_CONTAINERS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
