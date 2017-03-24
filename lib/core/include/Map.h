/* Copyright (c) 1997-2017
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


/** @file Map.h
    @brief Implementation of pm::Map class
*/


#ifndef POLYMAKE_MAP_H
#define POLYMAKE_MAP_H

#include "polymake/internal/AVL.h"
#include "polymake/internal/tree_containers.h"
#include "polymake/internal/shared_object.h"
#include "polymake/internal/assoc.h"

namespace pm {

/** @class Map
    @brief Associative array based on AVL::tree.
    
   It differs from the standard @c std::map in the implementation: it uses
   an AVL::tree</a> instead of the red-black tree. The tree is attached via a smart pointer with @ref refcounting "reference counting".

*/


template <typename K, typename D, typename Comparator=operations::cmp>
class Map
   : public modified_tree< Map<K, D, Comparator>,
                           mlist< ContainerTag< AVL::tree< AVL::traits<K, D, Comparator> > >,
                                  OperationTag< BuildUnary<AVL::node_accessor> > > > {
protected:
   typedef AVL::tree< AVL::traits<K,D,Comparator> > tree_type;
   shared_object<tree_type, AliasHandlerTag<shared_alias_handler>> tree;

   friend Map& make_mutable_alias(Map& alias, Map& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }
public:
   typedef K key_type;
   typedef D mapped_type;
   typedef Comparator key_comparator_type;

   static_assert(!std::is_same<Comparator, operations::cmp_unordered>::value, "comparator must define a total ordering");
   static_assert(!std::is_same<Comparator, operations::cmp>::value || is_ordered<K>::value, "keys must have a total ordering");

   tree_type& get_container() { return *tree; }
   const tree_type& get_container() const { return *tree; }

   /// Create an empty Map. Initialize the element comparator with its default constructor.
   Map() {}

   /// Create an empty Map with non-default comparator.
   explicit Map(const Comparator& cmp_arg)
      : tree(cmp_arg) {}

   /// Construct from an iterator.
   template <typename Iterator>
   Map(Iterator&& src, Iterator&& src_end)
      : tree(make_iterator_range(src, src_end)) {}

   template <typename Iterator>
   explicit Map(Iterator&& src,
                typename std::enable_if<assess_iterator<Iterator, check_iterator_feature, end_sensitive>::value,
                                        void**>::type=nullptr)
      : tree(std::forward<Iterator>(src)) {}

   /// Clear all contents.
   void clear() { tree.apply(shared_clear()); }

   /** @brief Swap content with another Map in an efficient way.
       @param m the other Map
   */
   void swap(Map& m) { tree.swap(m.tree); }

   friend void relocate(Map* from, Map* to)
   {
      relocate(&from->tree, &to->tree);
   }

   bool exists(const key_type& k) const
   {
      return tree->exists(k);
   }

   /** @brief Associative search.
   
       Find the data element associated with the given key. If it doesn't exist so far, it will be created with the default constructor.

       Note that the type of the search key is not necessarily the same as of the map entries.
       It suffices that both are comparable with each other.

       k can also be a container with keys; the result will be a sequence of corresponding values.
   */
   template <typename TKeys>
   typename assoc_helper<Map, TKeys>::result_type operator[] (const TKeys& k)
   {
      return assoc_helper<Map, TKeys>()(*this, k);
   }

   /** @brief Associative search (const).
       Find the data element associated with the given key. If it doesn't exist so far, it will raise an exception.

       Note that the type of the search key is not necessarily the same as of the map entries.
       It suffices that both are comparable with each other.

       k can also be a container with keys; the result will be a sequence of corresponding values.
   */
   template <typename TKeys>
   typename assoc_helper<const Map, TKeys>::result_type operator[] (const TKeys& k) const
   {
      return assoc_helper<const Map, TKeys>()(*this, k);
   }

   /// synonym for const key lookup
   template <typename TKeys>
   typename assoc_helper<const Map, TKeys>::result_type map(const TKeys& k) const
   {
      return assoc_helper<const Map, TKeys>()(*this, k);
   }

   friend
   bool operator== (const Map& l, const Map& r)
   {
      return l.size()==r.size() && equal_ranges(entire(l), r.begin());
   }

   friend
   bool operator!= (const Map& l, const Map& r)
   {
      return !(l==r);
   }

#if POLYMAKE_DEBUG
   void check(const char* label) const { tree->check(label); }

private:
   void dump(std::false_type) const { cerr << "elements are not printable" << std::flush; }
   void dump(std::true_type) const { cerr << *this << std::flush; }
public:
   void dump() const __attribute__((used)) { dump(bool_constant<is_printable<K>::value && is_printable<D>::value>()); }
#endif
};

template <typename K, typename D, typename Comparator>
struct spec_object_traits< Map<K,D,Comparator> >
   : spec_object_traits<is_container> {
   static const IO_separator_kind IO_separator=IO_sep_inherit;
};

template <typename K, typename D, typename Comparator>
struct choose_generic_object_traits< Map<K, D, Comparator>, false, false > :
      spec_object_traits< Map<K,D,Comparator> > {
   typedef void generic_type;
   typedef is_map generic_tag;
   typedef Map<K,D,Comparator> persistent_type;
};

} // end namespace pm

namespace polymake {
   using pm::Map;
}

namespace std {
   template <typename K, typename D, typename Comparator> inline
   void swap(pm::Map<K,D,Comparator>& m1, pm::Map<K,D,Comparator>& m2) { m1.swap(m2); }
}

#endif // POLYMAKE_MAP_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
