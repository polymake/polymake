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

/** @file FaceMap.h
    @brief This file contains the namespace face_map
 */

#ifndef POLYMAKE_FACE_MAP_H
#define POLYMAKE_FACE_MAP_H

#include "polymake/internal/AVL.h"
#include "polymake/GenericSet.h"
#include "polymake/vector"

namespace pm {

/** @namespace face_map
    @brief implements the face map data structure due to Kaibel and Pfetsch
 */

namespace face_map {

template <typename Traits> class tree_traits;

/** The template parameter @a Traits should be a traits class defining all necessary information
    about the data to be stored in the FaceMap data structure.
**/
template <typename Traits>
struct node {
   typedef AVL::tree< tree_traits<Traits> > tree_type;
   
   AVL::Ptr<node> links[3];
   
   /// element type
   typename Traits::key_type key;
   typename Traits::mapped_type data;

   // tree for the next face element
   tree_type *descend;

   node(typename function_argument<typename Traits::key_type>::type key_arg)
      : key(key_arg), data(Traits::initialize_data()), descend(0) { }

   template <typename TreeAllocator>
   node(const node& o, TreeAllocator& tree_allocator)
      : key(o.key), data(o.data), descend(0)
   {
      if (o.descend)
         descend=new(tree_allocator.allocate(1)) tree_type(*o.descend);
   }
};

template <typename Traits>
struct it_traits {
   typedef node<Traits> Node;

   static AVL::Ptr<Node>& link(Node* n, AVL::link_index X) { return n->links[X-AVL::L]; }

   const it_traits& get_it_traits() const { return *this; }
};

template <typename Traits>
class tree_traits : public it_traits<Traits> {
public:
   typedef it_traits<Traits> traits_for_iterator;
   typedef typename traits_for_iterator::Node Node;
   typedef typename Traits::key_type key_type;
   typedef typename Traits::key_comparator_type key_comparator_type;
   static const bool allow_multiple=false;
   typedef std::allocator<Node> node_allocator_type;
protected:
   mutable AVL::Ptr<Node> root_links[3];
   key_comparator_type key_comparator;
   node_allocator_type node_allocator;

   Node* head_node() const { return reinterpret_cast<Node*>(&root_links[0]); }

   static const key_type& key(const Node& n) { return n.key; }

   static typename Traits::mapped_type& data(Node& n) { return n.data; }

   Node* create_node(typename function_argument<key_type>::type key_arg)
   {
      return new(node_allocator.allocate(1)) Node(key_arg);
   }

   Node* clone_node(Node* n)
   {
      typename node_allocator_type::template rebind<typename Node::tree_type>::other tree_allocator(node_allocator);
      return new(node_allocator.allocate(1)) Node(*n, tree_allocator);
   }

   void destroy_node(Node* n)
   {
      if (n->descend) {
         typename node_allocator_type::template rebind<typename Node::tree_type>::other tree_allocator(node_allocator);
         tree_allocator.destroy(n->descend);
         tree_allocator.deallocate(n->descend,1);
      }
      node_allocator.destroy(n);
      node_allocator.deallocate(n,1);
   }

   static bool own_node(Node*) { return true; }

public:
   typedef const key_comparator_type& arg_type;

   tree_traits() { }
   tree_traits(arg_type cmp_arg) : key_comparator(cmp_arg) { }

   static int max_size() { return std::numeric_limits<int>::max(); }
   const key_comparator_type& get_comparator() const { return key_comparator; }
};

template <typename Traits>
struct accessor {
   typedef typename AVL::tree< tree_traits<Traits> >::const_iterator argument_type;
   typedef const typename Traits::key_type& result_type;

   result_type operator() (const argument_type& it) const
   {
      return it->key;
   }
};

template <typename Traits>
class element
   : public modified_container_impl< element<Traits>,
                                     mlist< ContainerTag< typename std::vector< typename AVL::tree< tree_traits<Traits> >::const_iterator > >,
                                            OperationTag< accessor<Traits> > > >,
     public GenericSet< element<Traits>, typename Traits::key_type, typename Traits::key_comparator_type> {
protected:
   typedef AVL::tree< tree_traits<Traits> > tree_type;
   typedef typename tree_type::const_iterator elem_iterator;
   std::vector<elem_iterator> ptr;
public:
   const std::vector<elem_iterator>& get_container() const { return ptr; }

   element() { }
   element(int size_arg) : ptr(size_arg) { }
};

template <typename Traits>
class Iterator : protected element<Traits> {
   typedef AVL::tree< tree_traits<Traits> > tree_type;
public:
   typedef typename Traits::mapped_type mapped_type;
   typedef forward_iterator_tag iterator_category;
   typedef element<Traits> value_type;
   typedef const value_type& reference;
   typedef const value_type* pointer;
   typedef ptrdiff_t difference_type;
   typedef Iterator iterator;
   typedef Iterator const_iterator;

   Iterator() { }

   explicit Iterator(typename value_type::elem_iterator cur, int face_size=0)
      : value_type(face_size>0 ? face_size : 1), max_depth(face_size-1)
   {
      this->ptr[0]=cur;
      if (cur.at_end()) return;
      if (max_depth>=0)
         find_to_depth(0);
      else
         find_descend(cur);
   }

   reference operator* () const { return *this; }
   pointer operator-> () const { return this; }
   const mapped_type& data() const { return this->ptr.back()->data; }

   Iterator& operator++ ()
   {
      if (max_depth>=0) {
         int depth=max_depth;
         while ((++this->ptr[depth]).at_end()) {
            if (--depth<0) return *this;
         }
         find_to_depth(depth);
      } else {
         while (! this->ptr.back()->descend) {
            while ((++this->ptr.back()).at_end()) {
               if (this->ptr.size()==1) return *this;
               this->ptr.pop_back();
            }
            if (!Traits::unvisited(this->ptr.back()->data)) return *this;
         }
         find_descend(this->ptr.back());
      }
      return *this;
   }
   const Iterator operator++ (int) { Iterator copy=*this; operator++(); return copy; }

   bool operator== (const Iterator& it) const { return this->ptr==it.ptr; }
   bool operator!= (const Iterator& it) const { return !operator==(it); }
   bool at_end() const { return this->ptr.front().at_end(); }
protected:
   // precondition: ! cur.at_end()
   void find_descend(typename value_type::elem_iterator cur)
   {
      while (Traits::unvisited(cur->data)) {
         cur=cur->descend->begin();
         this->ptr.push_back(cur);
      }
   }

   // precondition: ! ptr[depth].at_end()
   void find_to_depth(int depth)
   {
      typename value_type::elem_iterator cur=this->ptr[depth];
      while (depth < max_depth || Traits::unvisited(cur->data)) {
         while (true) {
            if (this->ptr[depth].at_end()) {
               if (--depth<0) return;
            } else if (depth < max_depth && this->ptr[depth]->descend) {
               break;
            }
            ++this->ptr[depth];
         }
         cur=this->ptr[depth]->descend->begin();
         this->ptr[++depth]=cur;
      }
   }

   int max_depth;
};

template <typename Vertex=int>
struct index_traits {
   /// search key in the AVL trees
   typedef Vertex key_type;
   typedef operations::cmp key_comparator_type;
   /// data associated with the face, here: the node index in the HasseDiagram
   typedef int mapped_type;
   /// initial value: non-existing node
   static int initialize_data() { return -1; }
   /// face is unvisiited if no node assigned
   static bool unvisited(int k) { return k==-1; }
};

} // end namespace face_map

template <typename Traits>
struct check_iterator_feature<face_map::Iterator<Traits>, end_sensitive> : std::true_type { };

/** @class FaceMap

    A special case of an associative container, whose keys are objects of type
    GenericSet.  It is implemented as a recursively nested AVL tree: The
    topmost tree uses the first set element as a local search key, and his
    nodes contain second-level trees, using in turn the second set element as
    a local search key, and so on.

   The main purpose of the FaceMap class is to collect faces from a polytope
   face lattice or a simplicial complex, hence the name.  Concept due to
   Pfetsch and Kaibel.
**/

template <typename Traits=face_map::index_traits<int> >
class FaceMap {
   typedef AVL::tree< face_map::tree_traits<Traits> > tree_type;
   typedef typename tree_type::node_allocator_type::template rebind<tree_type>::other subtree_allocator_type;
   subtree_allocator_type subtree_allocator;
public:
   typedef typename Traits::key_type key_type;
   typedef typename Traits::mapped_type mapped_type;
   typedef typename Traits::key_comparator_type key_comparator_type;

   FaceMap() : data_of_empty_face(Traits::initialize_data()) { }

   typedef face_map::Iterator<Traits> iterator;
   typedef iterator const_iterator;
   typedef typename iterator::value_type value_type;
   typedef typename iterator::reference reference;
   typedef reference const_reference;

   iterator begin() const { return iterator(head.begin()); }
   iterator end() const { return iterator(head.end()); }

   iterator begin_of_dim(int d) const { return iterator(head.begin(), d+1); }
   iterator end_of_dim(int d) const { return iterator(head.end(), d+1); }

   template <typename Set2>
   mapped_type& operator[] (const GenericSet<Set2, key_type, key_comparator_type>& face)
   {
      if (!face.top().empty()) {
         tree_type *current_tree=&head;
         typename Entire<Set2>::const_iterator f_i=entire(face.top());

         while (true) {
            // invariants: current_tree!=0, !f_i.at_end()
            typename tree_type::iterator current_node=current_tree->insert(*f_i);
            ++f_i;
            if (f_i.at_end()) return current_node->data;
            if (!current_node->descend)
               current_node->descend=new(subtree_allocator.allocate(1)) tree_type();
            current_tree=current_node->descend;
         }
      }
      return data_of_empty_face;
   }

   bool empty() const { return head.empty(); }

   /// Caution: counting via full enumeration!
   int size() const { return count_it(begin()); }
   int faces_of_dim(int d) const { return count_it(begin_of_dim(d)); }

   void clear() { head.clear(); }
private:
   tree_type head;
   mapped_type data_of_empty_face;
};

template <typename Traits>
struct spec_object_traits< FaceMap<Traits> > : spec_object_traits<is_container> { };

} // end namespace pm

namespace polymake {
   using pm::FaceMap;
}

#endif // POLYMAKE_FACE_MAP_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
