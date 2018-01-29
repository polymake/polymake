/* Copyright (c) 1997-2018
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

#ifndef POLYMAKE_INTERNAL_TREE_CONTAINERS_H
#define POLYMAKE_INTERNAL_TREE_CONTAINERS_H

#include "polymake/internal/modified_containers.h"

namespace pm {

template <typename Top, typename TParams=typename Top::manipulator_params,
          bool TModified=mtagged_list_extract<TParams, OperationTag>::is_specified>
class modified_tree_impl
   : public modified_container_impl<Top, TParams> {
   typedef modified_container_impl<Top, TParams> base_t;
public:
   using typename base_t::iterator;
   using typename base_t::const_iterator;
   using typename base_t::reverse_iterator;
   using typename base_t::const_reverse_iterator;
   typedef typename base_t::container::tree_type tree_type;
protected:
   iterator finalize(typename tree_type::iterator it)
   {
      return iterator(it, this->manip_top().get_operation());
   }
   const_iterator finalize(typename tree_type::const_iterator it) const
   {
      return const_iterator(it, this->manip_top().get_operation());
   }
   reverse_iterator finalize(typename tree_type::reverse_iterator it)
   {
      return reverse_iterator(it, this->manip_top().get_operation());
   }
   const_reverse_iterator finalize(typename tree_type::const_reverse_iterator it) const
   {
      return const_reverse_iterator(it, this->manip_top().get_operation());
   }
};

template <typename Top, typename TParams>
class modified_tree_impl<Top, TParams, false>
   : public redirected_container<Top, TParams> {
   typedef redirected_container<Top, TParams> base_t;
public:
   using typename base_t::iterator;
   using typename base_t::const_iterator;
   using typename base_t::reverse_iterator;
   using typename base_t::const_reverse_iterator;
protected:
   iterator finalize(iterator it) { return it; }
   const_iterator finalize(const_iterator it) const { return it; }
   reverse_iterator finalize(reverse_iterator it) { return it; }
   const_reverse_iterator finalize(const_reverse_iterator it) const { return it; }
};

template <typename Top, typename TParams=typename Top::manipulator_params>
class modified_tree
   : public modified_tree_impl<Top, TParams> {
   typedef modified_tree_impl<Top, TParams> base_t;
public:
   using typename base_t::iterator;
   using typename base_t::const_iterator;
   using typename base_t::reverse_iterator;
   using typename base_t::const_reverse_iterator;
   typedef typename base_t::container::tree_type tree_type;

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

protected:
   template <typename First>
   struct insert_helper {
      static const bool is_reverse_iterator=tree_type::template insert_arg_helper<First>::is_reverse_iterator;
      typedef typename std::conditional<is_reverse_iterator, typename base_t::reverse_iterator, iterator>::type return_type;
   };
public:
   template <typename First>
   typename insert_helper<First>::return_type
   insert(const First& first_arg)
   {
      return this->finalize(this->manip_top().get_container().insert(first_arg));
   }
   template <typename First, typename Second>
   typename insert_helper<First>::return_type
   insert(const First& first_arg, const Second& second_arg)
   {
      return this->finalize(this->manip_top().get_container().insert(first_arg,second_arg));
   }
   template <typename First, typename Second, typename Third>
   typename insert_helper<First>::return_type
   insert(const First& first_arg, const Second& second_arg, const Third& third_arg)
   {
      return this->finalize(this->manip_top().get_container().insert(first_arg,second_arg,third_arg));
   }

   template <typename Key_or_Iterator>
   void erase(const Key_or_Iterator& k_or_it)
   {
      this->manip_top().get_container().erase(k_or_it);
   }

   template <typename Iterator>
   void erase(const Iterator& pos, const Iterator& end)
   {
      this->manip_top().get_container().erase(pos,end);
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
   int max_size() const
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
