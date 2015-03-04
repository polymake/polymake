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
   : public modified_tree< Map<K,D,Comparator>,
                           list( Container< AVL::tree< AVL::traits<K,D,Comparator> > >,
                                 Operation< BuildUnary<AVL::node_accessor> > ) > {
   typedef modified_tree<Map> _super;
protected:
   typedef AVL::tree< AVL::traits<K,D,Comparator> > tree_type;
   shared_object<tree_type, AliasHandler<shared_alias_handler> > tree;

   friend Map& make_mutable_alias(Map& alias, Map& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }
public:
   typedef K key_type;
   typedef D mapped_type;
   typedef Comparator key_comparator_type;

   tree_type& get_container() { return *tree; }
   const tree_type& get_container() const { return *tree; }

	/// Create an empty Map. Initialize the element comparator with its default constructor.
   Map() {}

	/// Create an empty Map with non-default comparator.
   explicit Map(const Comparator& cmp_arg)
      : tree( make_constructor(cmp_arg, (tree_type*)0) ) {}

	/// Construct from an iterator.
   template <typename Iterator>
   Map(Iterator src, Iterator src_end)
      : tree( make_constructor(src, src_end, (tree_type*)0) ) {}

   template <typename Iterator>
   explicit Map(Iterator src, typename enable_if_iterator<Iterator,end_sensitive>::type=0)
      : tree( make_constructor(src, (tree_type*)0) ) {}

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

   /** @brief Associative search.
   
	Find the data element associated with the given key. If it doesn't exist so far, it will be created with the default constructor.

   Note that the type of the search key is not necessarily the same as of the map entries.
   It suffices that both are comparable with each other.
   */
   template <typename Key>
   typename assoc_helper<Map,Key>::type operator[] (const Key& k)
   {
      return assoc_helper<Map,Key>::doit(*this,k);
   }

   /** @brief Associative search (const).
	Find the data element associated with the given key. If it doesn't exist so far, it will raise an exception.

   Note that the type of the search key is not necessarily the same as of the map entries.
   It suffices that both are comparable with each other.
   */
   template <typename Key>
   typename assoc_helper<Map,Key>::const_type operator[] (const Key& k) const
   {
      return assoc_helper<Map,Key>::doit(*this,k);
   }

#if POLYMAKE_DEBUG
   void check(const char* label) const { tree->check(label); }
   void tree_dump() const { tree->dump(); }

   ~Map() { POLYMAKE_DEBUG_METHOD(Map,dump); }
   void dump(False) const { cerr << "elements are not printable" << std::flush; }
   void dump(True) const { cerr << *this << std::flush; }
   void dump() const { dump(bool2type<is_printable<K>::value && is_printable<D>::value>()); }
#endif
};

template <typename K, typename D, typename Comparator>
struct spec_object_traits< Map<K,D,Comparator> >
   : spec_object_traits<is_container> {
   static const IO_separator_kind IO_separator=IO_sep_inherit;
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
