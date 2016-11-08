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

/** @file AVL.h
    @brief This file contains the namespace pm::AVL
 */

#ifndef POLYMAKE_INTERNAL_AVL_H
#define POLYMAKE_INTERNAL_AVL_H

#include "polymake/internal/iterators.h"
#include "polymake/internal/comparators.h"

#if POLYMAKE_DEBUG
# include <iostream>
# include "polymake/GenericIO.h"
#endif

#include <memory>
#include <algorithm>
#include <cassert>

namespace pm {

/// special tags for find_nearest denoting the first and last occurrence of a given key in a multi-set
struct first_of_equal {};
struct last_of_equal {};

/// traits classes and such related to balanced trees
namespace AVL {

enum link_index { L=-1, P=0, R=1 };

/** Bit fields in \\ptr\\
    The nodes are assumed to be allocated at addresses aligned to the word boundary.
    Thus the lowest two bits of the address are always zero and so can be used for storing
    of various flags.
*/
enum Ptr_flags {
   NONE=0,
   SKEW=1,              // the subtree under this link is taller than the opposite one
   LEAF=2,              // the node pointed to is not the child of this node; it is a thread pointer
   END=SKEW|LEAF,       // the node pointed to is an apparent node in the tree head
   LINK=(-1UL & ~END)   // the rest bits hold the address
};

/// Pointer class
template <typename Node>
class Ptr {
private:
   unsigned long ptr;

public:
   explicit Ptr(Node* n=NULL) : ptr(reinterpret_cast<unsigned long>(n)) {}

   Ptr(Node *n, Ptr_flags flags) : ptr(reinterpret_cast<unsigned long>(n) | flags) {}

   Ptr& set(Node *n, Ptr_flags flags)
   {
      ptr = reinterpret_cast<unsigned long>(n) | flags;
      return *this;
   }

   Ptr& set(Ptr_flags flags)
   {
      (ptr &= LINK) |= flags;
      return *this;
   }

   Ptr& clear(Ptr_flags flags)
   {
      ptr &= ~flags;
      return *this;
   }

   Ptr& set(Node *n)
   {
      ptr = reinterpret_cast<unsigned long>(n) | (ptr & END);
      return *this;
   }

   operator Node* () const { return reinterpret_cast<Node*>(ptr & LINK); }
   Node* get_node() const { return reinterpret_cast<Node*>(ptr & LINK); }

   operator bool() const { return ptr; }
   bool operator! () const { return !ptr; }

   Ptr& operator= (Node *n)
   {
      ptr = reinterpret_cast<unsigned long>(n);
      return *this;
   }

   Ptr_flags leaf() const { return Ptr_flags(ptr & LEAF); }
   Ptr_flags skew() const { return Ptr_flags(ptr & SKEW); }
   bool skew_strict() const { return (ptr & END)==SKEW; }
   bool end() const { return (ptr & END)==END; }

   link_index direction() const
   {
      return link_index((signed long)(ptr << sizeof(ptr)*8-2) >> sizeof(ptr)*8-2);
   }

   Ptr& set_direction(Node *n, link_index X)
   {
      ptr = reinterpret_cast<unsigned long>(n) | (X & (SKEW|LEAF));
      return *this;
   }

   bool operator== (Node *n) const
   {
      return static_cast<Node*>(*this) == n;
   }

   bool operator== (const Ptr& p) const
   {
      return static_cast<Node*>(*this) == static_cast<Node*>(p);
   }

   bool operator!= (Node *n) const
   {
      return !(*this==n);
   }

   bool operator!= (const Ptr& p) const
   {
      return !(*this==p);
   }

  /// descend to the leaf in the given direction
   template <typename Traits>
   void traverse_to_leaf(const Traits& t, link_index Dir)
   {
      Ptr n2;
      while (! (n2=t.link(*this,Dir)).leaf())
         *this=n2;
   }

   template <typename Traits>
   Ptr& traverse(const Traits& t, link_index Dir)
   {
      *this=t.link(*this,Dir);
      if (!leaf()) traverse_to_leaf(t,link_index(-Dir));
      return *this;
   }

   /// move to the next (dir==R) or previous (dir==L) node.
   template <typename Traits>
   friend Ptr traverse(Ptr n, const Traits& t, link_index Dir)
   {
      return n.traverse(t,Dir);
   }
};

template <typename Traits> class tree;

template <typename Traits, link_index Dir>
class tree_iterator : public deref<Traits>::type {
public:
   typedef typename deref<Traits>::type traits;
   typedef bidirectional_iterator_tag iterator_category;
   typedef typename traits::Node value_type;
   typedef typename inherit_const<value_type, Traits>::type& reference;
   typedef typename inherit_const<value_type, Traits>::type* pointer;
   typedef ptrdiff_t difference_type;
   typedef tree_iterator<traits,Dir> iterator;
   typedef tree_iterator<const traits,Dir> const_iterator;
protected:
   /// pointer to the current node
   Ptr<value_type> cur;

   static const link_index Oppos=link_index(-Dir);
   typedef const tree_iterator<Traits,Oppos> reverse_iterator;
   typedef typename inherit_const<tree_iterator<traits,Oppos>, Traits>::type alt_reverse_iterator;
public:
   tree_iterator() {}
   tree_iterator(const traits& arg, const Ptr<value_type>& cur_arg)
      : traits(arg), cur(cur_arg) {}

   tree_iterator(const traits& arg, pointer cur_arg)
      : traits(arg), cur(cur_arg) {}

   tree_iterator(const iterator& it)
      : traits(it), cur(it.cur) {}

   tree_iterator(reverse_iterator& it)
      : traits(it), cur(it.cur) {}

   tree_iterator(alt_reverse_iterator& it)
      : traits(it), cur(it.cur) {}

   tree_iterator& operator= (const iterator& it)
   {
      static_cast<traits&>(*this)=it;
      cur=it.cur;
      return *this;
   }

   tree_iterator& operator= (reverse_iterator& it)
   {
      static_cast<traits&>(*this)=it;
      cur=it.cur;
      return *this;
   }

   tree_iterator& operator= (alt_reverse_iterator& it)
   {
      static_cast<traits&>(*this)=it;
      cur=it.cur;
      return *this;
   }

   reference operator* () const { return *cur; }
   pointer operator-> () const { return cur; }

   tree_iterator& operator++ ()
   {
      cur.traverse(*this,Dir);
      return *this;
   }

   tree_iterator& operator-- ()
   {
      cur.traverse(*this,link_index(-Dir));
      return *this;
   }

   const tree_iterator operator++ (int) { tree_iterator copy=*this; operator++(); return copy; }
   const tree_iterator operator-- (int) { tree_iterator copy=*this; operator--(); return copy; }

   template <link_index Dir2>
   bool operator== (const tree_iterator<traits,Dir2>& it) const { return cur==it.cur; }
   template <link_index Dir2>
   bool operator== (const tree_iterator<const traits,Dir2>& it) const { return cur==it.cur; }
   template <link_index Dir2>
   bool operator!= (const tree_iterator<traits,Dir2>& it) const { return cur!=it.cur; }
   template <link_index Dir2>
   bool operator!= (const tree_iterator<const traits,Dir2>& it) const { return cur!=it.cur; }

   bool at_end() const { return cur.end(); }

   template <typename,link_index> friend class tree_iterator;
   template <typename> friend class tree;
};

/** @class tree
    @brief balanced binary search tree

    An AVL tree is a kind of balanced binary search tree, allowing for
    logarithmic time element find, insert, and delete operations.  AVL trees
    are thoroughly discussed in D.E. Knuth's The Art of Computer Programming,
    Vol. III, 6.2.3, pp 465ff;
    http://www-cs-staff.stanford.edu/~knuth/taocp.html

    AVL trees play the same role in the polymake library that the red-black
    trees have in STL: a basis for the associative container classes. The
    current implementation differs from the red-black trees in following
    details:

    - In the leaf nodes so called thread pointers to the lexicographically
      next and previous tree nodes are maintained. This reduces the amortized
      tree traversal time by approximately 50%.

    - The key lookup procedure uses the comparator functor operations::cmp
      instead of @c std::less, achieving the acceleration by 1.5 times in
      average for non-trivial key types.

    - A tree created from a preordered key sequence is kept as a double-linked
      list up to the first random access operation (key search). Since some
      container objects are accessed only in sequential traversal loops during
      all their lifetime, this eliminates the overhead of rebalancing the tree
      at its creation stage. The list-to-tree conversion takes linear time and
      don't involve rebalancing at all.

 */

template <typename Traits>
class tree : public Traits {
public:
   typedef Traits tree_traits;
   typedef tree tree_type;
   typedef typename Traits::Node Node;
   typedef typename Traits::key_type key_type;
   typedef typename Traits::key_comparator_type key_comparator_type;
   typedef typename Traits::node_allocator_type node_allocator_type;
protected:
   typedef AVL::Ptr<Node> Ptr;

private:
   /// direct assignment is forbidden
   void operator= (const tree&) = delete;

   /** Recursive procedure making an exact copy of the tree.
       @param n        root node of the subtree to be cloned
       @param lthread  previous node in the new tree
       @param rthread  next node in the new tree
       @return         the root node of the subtree copy.
   */
   Node* clone_tree(Node *n, Ptr lthread, Ptr rthread)
   {
      Node *copy=this->clone_node(n);
      if (this->link(n,L).leaf()) {
         if (!lthread) {
            this->link(this->head_node(),R).set(copy,LEAF);
            lthread=end_node();
         }
         this->link(copy,L)=lthread;
      } else {
         Node *lc=clone_tree(this->link(n,L), lthread, Ptr(copy,LEAF));
         this->link(copy,L).set(lc, this->link(n,L).skew());
         this->link(lc,P).set_direction(copy,L);
      }
      if (this->link(n,R).leaf()) {
         if (!rthread) {
            this->link(this->head_node(),L).set(copy,LEAF);
            rthread=end_node();
         }
         this->link(copy,R)=rthread;
      } else {
         Node *rc=clone_tree(this->link(n,R), Ptr(copy,LEAF), rthread);
         this->link(copy,R).set(rc, this->link(n,R).skew());
         this->link(rc,P).set_direction(copy,R);
      }
      return copy;
   }

   /** Recursive procedure converting the list fragment to the AVL subtree.
       @param left left neighbour of the leftmost leaf of the subtree to be built
       @return pair(root, rightmost leaf) of the subtree
   */
   pair<Node*,Node*> treeify(Node *left, int n) const
   {
      if (n<=2) {
         Node *cur=this->link(left,R);
         if (n==2) {
            left=cur;
            cur=this->link(left,R);
            this->link(cur,L).set(left,SKEW);
            this->link(left,P).set_direction(cur,L);
         }
         return pair<Node*,Node*>(cur,cur);
      }
      pair<Node*, Node*> subtree=treeify(left, (n-1)/2);
      Node *root=this->link(subtree.second,R);
      this->link(root,L)=subtree.first;
      this->link(subtree.first,P).set_direction(root,L);
      subtree=treeify(root, n/2);
      this->link(root,R).set(subtree.first, Ptr_flags(!(n&(n-1))) /* SKEW if n is a power of 2 */);
      this->link(subtree.first,P).set_direction(root,R);
      return pair<Node*, Node*>(root,subtree.second);
   }

   /** Converts the list structure to the AVL tree.
       The conversion is performed by relinking some pointers,
       the nodes are neither copied nor moved around in the storage.
   */
   void treeify() const
   {
      Node *root=treeify(this->head_node(),n_elem).first;
      this->link(this->head_node(),P)=root;
      this->link(root,P).set_direction(this->head_node(),P);
   }

   Ptr root_node() const { return this->link(this->head_node(),P); }
   Ptr first()     const { return this->link(this->head_node(),R); }
   Ptr last()      const { return this->link(this->head_node(),L); }
   Ptr end_node()  const { return Ptr(this->head_node(), END); }

   friend class AVL::tree_iterator<typename Traits::traits_for_iterator, R>;
   friend class AVL::tree_iterator<const typename Traits::traits_for_iterator, R>;
   friend class AVL::tree_iterator<typename Traits::traits_for_iterator, L>;
   friend class AVL::tree_iterator<const typename Traits::traits_for_iterator, L>;

public:
   void* allocate_node() { return this->node_allocator.allocate(1); }

   /// true if object is really a tree (and not a list)
   bool tree_form() const { return root_node(); }

   typedef typename Traits::traits_for_iterator traits_for_iterator;
   typedef AVL::tree_iterator<traits_for_iterator, R> iterator;
   typedef AVL::tree_iterator<const traits_for_iterator, R> const_iterator;
   typedef AVL::tree_iterator<traits_for_iterator, L> reverse_iterator;
   typedef AVL::tree_iterator<const traits_for_iterator, L> const_reverse_iterator;

   typedef Node value_type;
   typedef Node& reference;
   typedef const Node& const_reference;

   iterator begin()
   {
      return iterator(this->get_it_traits(), first());
   }
   iterator end()
   {
      return iterator(this->get_it_traits(), end_node());
   }
   const_iterator begin() const
   {
      return const_iterator(this->get_it_traits(), first());
   }
   const_iterator end() const
   {
      return const_iterator(this->get_it_traits(), end_node());
   }
   reverse_iterator rbegin()
   {
      return reverse_iterator(this->get_it_traits(), last());
   }
   reverse_iterator rend()
   {
      return reverse_iterator(this->get_it_traits(), end_node());
   }
   const_reverse_iterator rbegin() const
   {
      return const_reverse_iterator(this->get_it_traits(), last());
   }
   const_reverse_iterator rend() const
   {
      return const_reverse_iterator(this->get_it_traits(), end_node());
   }

private:
   template <typename Key, typename Comparator>
   pair<Ptr,link_index> _do_find_descend(const Key& k, const Comparator& comparator) const
   {
      Ptr cur;
      if (!tree_form()) {
         // quick test whether one of the list ends matches
         cur=last();
         cmp_value c=comparator(k, this->key(*cur));
         if (c>=0 ||
             n_elem==1 ||
             (cur=first(), c=comparator(k, this->key(*cur)))<=0)
            return pair<Ptr,link_index>(cur,link_index(c));
         treeify();
      }
      link_index Down;
      cur=root_node();
      typename Diligent<const Key&>::type K=diligent(k);
      while (1) {
         Down=link_index(comparator(K, this->key(*cur)));
         if (Down==P ||
             this->link(cur,Down).leaf()) break;
         cur=this->link(cur,Down);
      }
      return pair<Ptr,link_index>(cur,Down);
   }

   template <typename Key, typename Comparator>
   pair<Ptr,link_index> find_descend(const Key& k, const Comparator& comparator) const
   {
      return _do_find_descend(k,comparator);
   }

   template <typename Key, typename Data, typename Comparator>
   pair<Ptr,link_index> find_descend(const pair<Key,Data>& p, const Comparator& comparator) const
   {
      return _do_find_descend(pair_init_first<key_type>(p),comparator);
   }

   Node* insert_first(Node *n)
   {
      this->link(this->head_node(),L)=this->link(this->head_node(),R).set(n,LEAF);
      this->link(n,R)=this->link(n,L)=end_node();
      n_elem=1;
      return n;
   }

   template <typename Key>
   void assign_node(Node&, const Key&, std::false_type) {}

   template <typename Key, typename Data>
   void assign_node(Node& n, const pair<Key, Data>& p, std::true_type)
   {
      Traits::data(n)=p.second;
   }

   struct assign_op {
      template <typename Dst, typename Src>
      void operator() (Dst& dst, const Src& src) const { dst=src; }
   };

   template <typename Key>
   Node* find_insert(const Key& k)
   {
      if (!n_elem) return insert_first(this->create_node(k));
      pair<Ptr,link_index> p=find_descend(k, this->key_comparator);
      if (p.second==P) {
         assign_node(*p.first, k, bool_constant<isomorphic_to_first<key_type, Key>::value>());
         return p.first;
      }
      ++n_elem;
      Node *n=this->create_node(k);
      insert_rebalance(n,p.first,p.second);
      return n;
   }

   template <typename Key, typename Data, typename Operation>
   Node* find_insert(const Key& k, const Data& d, const Operation& op)
   {
      if (!n_elem) return insert_first(this->create_node(k,d));
      pair<Ptr,link_index> p=find_descend(k, this->key_comparator);
      if (p.second==P) {
         op(Traits::data(*p.first), d);
         return p.first;
      }
      ++n_elem;
      Node *n=this->create_node(k,d);
      insert_rebalance(n, p.first, p.second);
      return n;
   }

   void insert_rebalance(Node *n, Node *parent, link_index Down);
   void remove_rebalance(Node *n);

   template <typename Key, typename Comparator>
   Ptr find_node(const Key& k, const Comparator& comparator) const
   {
      if (!n_elem) return end_node();
      pair<Ptr,link_index> p=find_descend(k, comparator);
      return p.second==P ? p.first : end_node();
   }

private:
   void find_last_equal(Ptr& cur, link_index Dir) const
   {
      Ptr next;
      while (!this->link(cur,Dir).end()) {
         next=traverse(cur,*this,Dir);
         if (this->key_comparator(this->key(*cur), this->key(*next)) == cmp_eq)
            cur=next;
         else
            break;
      }
   }

public:
   template <typename Key, typename RelOp>
   Ptr find_nearest_node(const Key& k, const RelOp&) const
   {
      if (n_elem) {
         if (is_instance_of<Key, maximal>::value) {
            if (is_among<RelOp, polymake::operations::le, polymake::operations::lt>::value)
               return last();
            else
               return end_node();
         }
         if (is_instance_of<Key, minimal>::value) {
            if (is_among<RelOp, polymake::operations::ge, polymake::operations::gt>::value)
               return first();
            else
               return end_node();
         }

         pair<Ptr,link_index> p=find_descend(k, this->key_comparator);
         switch (p.second) {
         case P:
            if (Traits::allow_multiple) {
               if (is_among<RelOp, last_of_equal, polymake::operations::ge, polymake::operations::gt>::value)
                  find_last_equal(p.first, R);
               if (is_among<RelOp, first_of_equal, polymake::operations::le, polymake::operations::lt>::value)
                  find_last_equal(p.first, L);
            }
            if (std::is_same<RelOp, polymake::operations::gt>::value)
               p.first.traverse(*this,R);
            if (std::is_same<RelOp, polymake::operations::lt>::value)
               p.first.traverse(*this,L);
            return p.first;

         case R:
            if (is_among<RelOp, polymake::operations::le, polymake::operations::lt>::value)
               return p.first;
            if (is_among<RelOp, polymake::operations::ge, polymake::operations::gt>::value)
               return p.first.traverse(*this,R);
            break;

         case L:
            if (is_among<RelOp, polymake::operations::ge, polymake::operations::gt>::value)
               return p.first;
            if (is_among<RelOp, polymake::operations::le, polymake::operations::lt>::value)
               return p.first.traverse(*this,L);
            break;
         }
      }
      return end_node();
   }

   template <typename Key, typename RelOp, typename Comparator>
   Ptr find_nearest_node(const Key& k, const RelOp&, const Comparator& comparator) const
   {
      Ptr found=end_node();
      const link_index Left= is_among<RelOp, polymake::operations::ge, polymake::operations::gt>::value ? L : R,
                       Right=link_index(-Left);
      link_index Down;
      if (n_elem) {
         typename unary_op_builder< operations::fix2<cmp_value, RelOp>, const cmp_value*>::operation good(cmp_eq);

         if (!tree_form()) {    // we have the list form - test the ends
            if (good(comparator(this->key(*this->link(this->head_node(),Right)), k)))
               // the first node satisfies
               return this->link(this->head_node(),Right);
            if (n_elem==1 || !good(comparator(this->key(*this->link(this->head_node(),Left)), k)))
               // the last node does not satisfy
               return found;
            treeify();
         }
         typename Diligent<const Key&>::type K=diligent(k);
         Ptr cur=root_node();
         do {
            const cmp_value diff=comparator(this->key(*cur), K);
            if (good(diff)) {
               found=cur;
               Down=Left;
            } else {
               Down=Right;
            }
         } while (!(cur=this->link(cur,Down)).leaf());
      }
      return found;
   }

   // Down==L : before next
   // Down==R : after next
   Node* insert_node_at(Ptr next, link_index Down, Node *n)
   {
#ifndef NDEBUG
      {
         const Ptr other= traverse(next,*this,Down),
                     lft= Down==L ? other : next,
                     rgt= Down==L ? next : other;
         assert(lft.end() || this->key_comparator(this->key(*lft), this->key(*n)) <= (Traits::allow_multiple ? cmp_eq : cmp_lt));
         assert(rgt.end() || this->key_comparator(this->key(*n), this->key(*rgt)) <= (Traits::allow_multiple ? cmp_eq : cmp_lt));
      }
#endif
      ++n_elem;
      if (!tree_form()) {
         Ptr other=this->link(next,Down);
         this->link(n,Down)=other;
         this->link(n,link_index(-Down))=next;
         this->link(other,link_index(-Down))=this->link(next,Down).set(n,LEAF);
      } else {
         if (next.end()) {
            next=this->link(next,Down);
            Down=link_index(-Down);
         } else if (! this->link(next,Down).leaf()) {
            next.traverse(*this,Down);
            Down=link_index(-Down);
         }
         insert_rebalance(n,next,Down);
      }
      return n;
   }

   template <typename Key>
   iterator insert_impl(const Key& k, int_constant<0>)
   {
      return iterator(this->get_it_traits(), find_insert(k));
   }

   template <typename Key, typename Data>
   iterator insert_impl(const Key& k, const Data& data, int_constant<0>)
   {
      return iterator(this->get_it_traits(), find_insert(k, data, assign_op()));
   }

   template <typename Key, typename Data, typename Operation>
   iterator insert_impl(const Key& k, const Data& data, const Operation& op, int_constant<0>)
   {
      return iterator(this->get_it_traits(), find_insert(k, data, op));
   }

   template <typename Key, typename Data>
   iterator insert_impl(const pair<Key, Data>& p, int_constant<1>)
   {
      return iterator(this->get_it_traits(), find_insert(p.first, p.second, assign_op()));
   }

   template <typename Key, typename Data, typename Operation>
   iterator insert_impl(const pair<Key, Data>& p, const Operation& op, int_constant<1>)
   {
      return iterator(this->get_it_traits(), find_insert(p.first, p.second, op));
   }

   template <typename Key>
   iterator insert_impl(const iterator& pos, const Key& k, int_constant<2>)
   {
      return iterator(this->get_it_traits(), insert_node_at(pos.cur, L, this->create_node(k)));
   }

   template <typename Key, typename Data>
   iterator insert_impl(const iterator& pos, const Key& k, const Data& data, int_constant<2>)
   {
      return iterator(this->get_it_traits(), insert_node_at(pos.cur, L, this->create_node(k, data)));
   }

   template <typename Key>
   reverse_iterator insert_impl(const reverse_iterator& pos, const Key& k, int_constant<2>)
   {
      return reverse_iterator(this->get_it_traits(), insert_node_at(pos.cur, R, this->create_node(k)));
   }

   template <typename Key, typename Data>
   reverse_iterator insert_impl(const reverse_iterator& pos, const Key& k, const Data& data, int_constant<2>)
   {
      return reverse_iterator(this->get_it_traits(), insert_node_at(pos.cur, R, this->create_node(k, data)));
   }

   template <typename Key>
   iterator insert_new_impl(const Key& k, int_constant<0>)
   {
      return iterator(this->get_it_traits(), insert_node(this->create_node(k)));
   }

   template <typename Key, typename Data>
   iterator insert_new_impl(const Key& k, const Data& data, int_constant<0>)
   {
      return iterator(this->get_it_traits(), insert_node(this->create_node(k, data)));
   }

   template <typename Key, typename Data>
   iterator insert_new_impl(const pair<Key, Data>& p, int_constant<1>)
   {
      return iterator(this->get_it_traits(), insert_node(this->create_node(p.first, p.second)));
   }

   template <typename Key>
   void erase_impl(const Key& k, int_constant<0>)
   {
      if (empty()) return;
      pair<Ptr,link_index> p=find_descend(k, this->key_comparator);
      if (p.second!=P) return;
      remove_node(p.first);
      this->destroy_node(p.first);
   }

   void erase_impl(const iterator& pos, int_constant<2>)
   {
      this->destroy_node(remove_node(pos.cur));
   }

   void erase_impl(const reverse_iterator& pos, int_constant<2>)
   {
      this->destroy_node(remove_node(pos.cur));
   }

   void erase_impl(iterator pos, const iterator& end, int_constant<2>)
   {
      while (pos != end) {
         Node *n=pos.cur;  ++pos;
         this->destroy_node(remove_node(n));
      }
   }

   void erase_impl(reverse_iterator pos, const reverse_iterator& end, int_constant<2>)
   {
      while (pos != end) {
         Node *n=pos.cur;  ++pos;
         this->destroy_node(remove_node(n));
      }
   }

   void swap_nodes_list_form(Node *l, Node *r)
   {
      Ptr pl=this->link(l,L), pr=this->link(r,L);
      std::swap(this->link(pl,R), this->link(pr,R));
      this->link(l,L)=pr;  this->link(r,L)=pl;
      pl=this->link(l,R);  pr=this->link(r,R);
      std::swap(this->link(pl,L), this->link(pr,L));
      this->link(l,R)=pr;  this->link(r,R)=pl;
   }

public:
   template <typename Key>
   iterator find(const Key& k)
   {
      return iterator(this->get_it_traits(), find_node(k,this->key_comparator));
   }

   template <typename Key>
   const_iterator find(const Key& k) const
   {
      return const_iterator(this->get_it_traits(), find_node(k,this->key_comparator));
   }

   template <typename Key, typename Comparator>
   iterator find(const Key& k, const Comparator& comparator)
   {
      return iterator(this->get_it_traits(), find_node(k,comparator));
   }

   template <typename Key, typename Comparator>
   const_iterator find(const Key& k, const Comparator& comparator) const
   {
      return const_iterator(this->get_it_traits(), find_node(k,comparator));
   }

   template <typename Key, typename RelOp>
   iterator find_nearest(const Key& k, const RelOp& relop)
   {
      return iterator(this->get_it_traits(), find_nearest_node(k,relop));
   }

   template <typename Key, typename RelOp>
   const_iterator find_nearest(const Key& k, const RelOp& relop) const
   {
      return const_iterator(this->get_it_traits(), find_nearest_node(k,relop));
   }

   template <typename Key, typename RelOp, typename Comparator>
   iterator find_nearest(const Key& k, const RelOp& relop, const Comparator& comparator)
   {
      return iterator(this->get_it_traits(), find_nearest_node(k,relop,comparator));
   }

   template <typename Key, typename RelOp, typename Comparator>
   const_iterator find_nearest(const Key& k, const RelOp& relop, const Comparator& comparator) const
   {
      return const_iterator(this->get_it_traits(), find_nearest_node(k,relop,comparator));
   }

   template <typename Key>
   bool exists(const Key& k) const
   {
      return ! find_node(k,this->key_comparator).end();
   }

   template <typename Arg>
   struct insert_arg_helper {
      static const bool is_reverse_iterator=is_derived_from<Arg, reverse_iterator>::value;
      static const bool is_iterator=is_derived_from<Arg, iterator>::value || is_reverse_iterator;
      static const bool is_pair=isomorphic_to_first<key_type, Arg>::value;
      static const int d=is_iterator*2+is_pair;
      typedef int_constant<d> discr;
      typedef typename std::conditional<is_reverse_iterator, reverse_iterator, iterator>::type return_type;
   };

   template <typename First>
   typename insert_arg_helper<First>::return_type
   insert(const First& first_arg)
   {
      return insert_impl(first_arg, typename insert_arg_helper<First>::discr());
   }

   template <typename First, typename Second>
   typename insert_arg_helper<First>::return_type
   insert(const First& first_arg, const Second& second_arg)
   {
      return insert_impl(first_arg, second_arg, typename insert_arg_helper<First>::discr());
   }

   template <typename First, typename Second, typename Third>
   typename insert_arg_helper<First>::return_type
   insert(const First& first_arg, const Second& second_arg, const Third& third_arg)
   {
      return insert_impl(first_arg, second_arg, third_arg, typename insert_arg_helper<First>::discr());
   }

   template <typename First>
   typename std::enable_if<Traits::allow_multiple, typename insert_arg_helper<First>::return_type>::type
   insert_new(const First& first_arg)
   {
      return insert_new_impl(first_arg, typename insert_arg_helper<First>::discr());
   }

   template <typename First, typename Second>
   typename std::enable_if<Traits::allow_multiple, typename insert_arg_helper<First>::return_type>::type
   insert_new(const First& first_arg, const Second& second_arg)
   {
      return insert_new_impl(first_arg, second_arg, typename insert_arg_helper<First>::discr());
   }
   

   /// Insert a given node in the tree corresponding to the value of its key field.
   /// @return NULL if the key of the given node already occurs in the tree, //n// otherwise.
   Node* insert_node(Node *n)
   {
      if (n_elem) {
         pair<Ptr,link_index> p=find_descend(this->key(*n), this->key_comparator);
         if (Traits::allow_multiple) {
            if (p.second == P) {
               if (tree_form()) {
                  if (this->link(p.first, L).leaf()) {
                     p.second=L;
                  } else if (this->link(p.first, R).leaf()) {
                     p.second=R;
                  } else if (this->link(p.first, L).skew()) {
                     p.first.traverse(*this,R);
                     p.second=L;
                  } else {
                     p.first.traverse(*this,L);
                     p.second=R;
                  }
               } else {
                  p.second=R;
               }
            }
         } else if (p.second == P) {
            return NULL;
         }
         ++n_elem;
         insert_rebalance(n,p.first,p.second);
      } else {
         insert_first(n);
      }
      return n;
   }

   /** Delete an element if it exists.
       Delete the element the iterator points to. */
   template <typename Key_or_Iterator>
   void erase(const Key_or_Iterator& k_or_it)
   {
      erase_impl(k_or_it, typename insert_arg_helper<Key_or_Iterator>::discr());
   }

   template <typename Iterator>
   void erase(const Iterator& pos, const Iterator& last)
   {
      erase_impl(pos, last, typename insert_arg_helper<Iterator>::discr());
   }

   /** Remove the node with a given address.
       The Node object is not destroyed.
       @return address of the removed node
   */
   Node* remove_node(Node *n)
   {
      --n_elem;
      if (!tree_form()) {
         Ptr next=this->link(n,R),
             prev=this->link(n,L);
         this->link(next,L)=prev;
         this->link(prev,R)=next;
      } else {
         remove_rebalance(n);
      }
      return n;
   }

   // This operation destroys the balancedness and should be used with uttermost caution
   Node* unlink_node(Node *n)
   {
      --n_elem;
      Ptr next=this->link(n,R),
          prev=this->link(n,L);
      if (prev.leaf()) {
         if (next.leaf()) {
            this->link(next,L)=prev;
            this->link(prev,R)=next;
         } else {
            Ptr parent=this->link(n,P);
            this->link(parent,parent.direction())=next;
            this->link(next,P)=parent;
            next.traverse_to_leaf(*this,L);
            this->link(next,L)=prev;
         }
      } else {
         Ptr parent=this->link(n,P);
         this->link(parent,parent.direction())=prev;
         this->link(prev,P)=parent;
         prev.traverse_to_leaf(*this,R);
         this->link(prev,R)=next;
         if (!next.leaf()) {
            this->link(next,P).set_direction(prev,R);
            next.traverse_to_leaf(*this,L);
            this->link(next,L).set(prev,LEAF);
         }
      }
      return n;
   }

   /// The key of the node has been changed: move it to the proper position.
   /// The new key must not violate the uniqueness requirement.
   void update_node(Node *n);

   template <typename Key>
   iterator toggle(const Key& k)
   {
      if (!n_elem) {
         return iterator(this->get_it_traits(), insert_first(this->create_node(k)));
      }
      pair<Ptr,link_index> p=find_descend(k, this->key_comparator);
      if (p.second==P) {
         this->destroy_node(remove_node(p.first));
         return end();
      }
      ++n_elem;
      Node *n=this->create_node(k);
      insert_rebalance(n,p.first,p.second);
      return iterator(this->get_it_traits(), Ptr(n));
   }

   /// Return the current number of nodes.
   int size() const { return n_elem; }
   bool empty() const { return n_elem==0; }

   template <typename Key>
   void push_front(const Key& k)
   {
      insert_node_at(end_node(), R, this->create_node(k));
   }

   template <typename Key, typename Data>
   void push_front(const Key& k, const Data& data)
   {
      insert_node_at(end_node(), R, this->create_node(k,data));
   }

   void push_front_node(Node *n)
   {
      insert_node_at(end_node(), R, n);
   }

   template <typename Key>
   void push_back(const Key& k)
   {
      insert_node_at(end_node(), L, this->create_node(k));
   }

   template <typename Key, typename Data>
   void push_back(const Key& k, const Data& data)
   {
      insert_node_at(end_node(), L, this->create_node(k,data));
   }

   void push_back_node(Node *n)
   {
      insert_node_at(end_node(), L, n);
   }

   reference front() { return *first(); }
   const_reference front() const { return *first(); }
   reference back() { return *last(); }
   const_reference back() const { return *last(); }

   void pop_front()
   {
      this->destroy_node(remove_node(first()));
   }

   void pop_back()
   {
      this->destroy_node(remove_node(last()));
   }

   /** Makes the tree look like empty.
       Nodes are not destroyed.
   */
   void init()
   {
      this->link(this->head_node(),L)=this->link(this->head_node(),R)=end_node();
      this->link(this->head_node(),P)=NULL;
      n_elem=0;
   }

private:
   template <bool force>
   void destroy_nodes(bool_constant<force>)
   {
      Ptr n=last();
      do {
         if (!(force || this->own_node(n))) break;
         Node *to_delete=n;
         n.traverse(*this,L);
         this->destroy_node(to_delete);
      } while (!n.end());
   }

public:
   /// Make the tree empty, destroy the nodes.
   void clear()
   {
      if (n_elem) {
         destroy_nodes(std::true_type());
         init();
      }
   }

   /// Creates an empty tree.
   tree() { init(); }

   tree(typename Traits::arg_type traits_arg) : Traits(traits_arg) { init(); }

   /** Create a copy of another tree.
       The current form (list or tree) is also inherited.
   */
   tree(const tree& t)
      : Traits(t)
   {
      if (t.tree_form()) {
         n_elem=t.n_elem;
         Node *root=clone_tree(t.root_node(), Ptr(), Ptr());
         this->link(this->head_node(),P)=root;
         this->link(root,P).set_direction(this->head_node(),P);
      } else {          // list form
         init();
         for (Ptr n=t.first(); !n.end(); n=t.link(n,R))
            insert_node_at(end_node(), L, this->clone_node(n));
      }
   }

private:
   template <typename Iterator>
   void fill_impl(Iterator& src, std::true_type)
   {
      for (; !src.at_end(); ++src)
         push_back(src.index(), *src);
   }

   template <typename Iterator>
   void fill_impl(Iterator& src, std::false_type)
   {
      for (; !src.at_end(); ++src)
         push_back(*src);
   }

   template <typename Iterator>
   void fill_impl(Iterator&& src)
   {
      fill_impl(src, bool_constant< check_iterator_feature<Iterator, indexed>::value &&
                                    isomorphic_types<typename iterator_traits<Iterator>::value_type, typename Node::mapped_type>::value >());
   }
public:
   template <typename Iterator>
   explicit tree(Iterator&& src,
                 typename std::enable_if<assess_iterator<Iterator, check_iterator_feature, end_sensitive>::value, void**>::type=nullptr)
   {
      init();
      fill_impl(ensure_private_mutable(std::forward<Iterator>(src)));
   }

   ~tree() { if (n_elem) destroy_nodes(std::false_type()); }

   void swap(tree& t)
   {
      if (this==&t) return;
      std::swap(static_cast<Traits&>(*this), static_cast<Traits&>(t));
      std::swap(n_elem, t.n_elem);
   }

   template <bool with_nodes>
   friend void relocate_tree(tree* from, tree* to, bool_constant<with_nodes>)
   {
      relocate(static_cast<Traits*>(from), static_cast<Traits*>(to));
      if (with_nodes && from->n_elem) {
         to->n_elem=from->n_elem;
         to->link(to->first(),L)=to->link(to->last(),R)=to->end_node();
         if (to->tree_form())
            to->link(to->root_node(),P).set_direction(to->head_node(), P);
      } else {
         to->init();
      }
   }

   friend void relocate(tree* from, tree* to)
   {
      relocate_tree(from, to, std::true_type());
   }

   template <typename Iterator,
             typename=typename std::enable_if<assess_iterator<Iterator, check_iterator_feature, end_sensitive>::value>::type>
   void assign(Iterator&& src)
   {
      clear();
      fill_impl(ensure_private_mutable(std::forward<Iterator>(src)));
   }

#if POLYMAKE_DEBUG
private:
   struct check_ret {
      /// height and weight of the visited subtree
      int h, w;
   };

   check_ret
   check_tree(Ptr n, const char *prefix, bool is_leftmost, bool is_rightmost) const
   {
      check_ret l={ 0, 0 }, r={ 0, 0 };
      Ptr left=this->link(n,L),
          right=this->link(n,R);
      if (left.end()) {
         if (!is_leftmost) cerr << prefix << "bad END <-- " << this->key(*n) << endl;
         else if (this->link(left,R) != n) cerr << prefix << "bad LEFTMOST" << endl;
      } else {
         if (this->key_comparator(this->key(*left),this->key(*n)) > (Traits::allow_multiple ? cmp_eq : cmp_lt))
            cerr << prefix << "order violation: " << this->key(*left) << " <-- " << this->key(*n) << endl;
         if (!left.leaf()) {
            if (this->link(left,P) != n || this->link(left,P).direction() != L)
               cerr << prefix << this->key(*left) << " : P broken" << endl;
            l=check_tree(left,prefix,is_leftmost,false);
         }
      }
      if (right.end()) {
         if (!is_rightmost) cerr << prefix << this->key(*n) << " --> bad END" << endl;
         else if (this->link(right,L) != n) cerr << prefix << "bad RIGHTMOST" << endl;
      } else {
         if (this->key_comparator(this->key(*n),this->key(*right)) > (Traits::allow_multiple ? cmp_eq : cmp_lt))
            cerr << prefix << "order violation: " << this->key(*n) << " --> " << this->key(*right) << endl;
         if (!right.leaf()) {
            if (this->link(right,P) != n || this->link(right,P).direction() != R)
               cerr << prefix << this->key(*right) << " : P broken" << endl;
            r=check_tree(right,prefix,false,is_rightmost);
         }
      }
      if (l.h-r.h<-1 || l.h-r.h>1) {
         cerr << prefix << "disbalance: (" << l.h << ") <-- " << this->key(*n) << " --> (" << r.h << ")" << endl;
      }
      if (left.skew_strict()) {
         if (l.h<=r.h) cerr << prefix << "SKEW <-- " << this->key(*n) << endl;
      } else {
         if (l.h>r.h) cerr << prefix << "!SKEW <-- " << this->key(*n) << endl;
      }
      if (right.skew_strict()) {
         if (r.h<=l.h) cerr << prefix << this->key(*n) << "--> SKEW" << endl;
      } else {
         if (r.h>l.h) cerr << prefix << this->key(*n) << "--> !SKEW" << endl;
      }
      r.h=std::max(r.h,l.h)+1;
      r.w=r.w+l.w+1;
      return r;
   }

   /// Dump the subtree.
   void dump_tree(Ptr n, int depth) const
   {
      if (! this->link(n,R).leaf())
         dump_tree(this->link(n,R), depth+3);
      cerr << std::setw(depth) << (n.skew() ? "*" : " ") << std::setw(0) << this->key(*n) << endl;
      if (! this->link(n,L).leaf())
         dump_tree(this->link(n,L), depth+3);
   }
public:
   /** Check the integrity of the tree.
       @param prefix a string to be prepended to each error message
   */
   void check(const char *prefix) const
   {
      int n=0;
      if (this->link(this->head_node(),P).direction() != P)
         cerr << prefix << "root pointer - wrong direction" << endl;
      if (Ptr root=root_node()) {
         if (this->link(root,P) != this->head_node() || this->link(root,P).direction() != P)
            cerr << prefix << "root node " << this->key(*root_node()) << " : P broken" << endl;
         n=(check_tree(root,prefix,true,true)).w;
      } else {
         Ptr cur=first(), prev=end_node();
         while (!cur.end()) {
            if (!this->link(cur,L).leaf())
               cerr << prefix << this->key(*cur) << " L !leaf" << endl;
            if (this->link(cur,L) != prev)
               cerr << prefix << this->key(*cur) << " L broken" << endl;
            if (!prev.end() && this->key_comparator(this->key(*prev),this->key(*cur)) > (Traits::allow_multiple ? cmp_eq : cmp_lt))
               cerr << prefix << "order violation: " << this->key(*prev) << " --> " << this->key(*cur) << endl;
            prev=cur;
            if (!this->link(cur,R).leaf())
               cerr << prefix << this->key(*cur) << " R !leaf" << endl;
            cur=this->link(cur,R);
            ++n;
         }
         if (prev != last()) cerr << prefix << "RIGHTMOST broken" << endl;
      }
      if (n != n_elem)
      cerr << prefix << "element count=" << n_elem << " != " << n << endl;
   }

   void dump() const
   {
      if (tree_form()) {
         dump_tree(root_node(),1);
      } else {
         for (Ptr cur=first(); !cur.end(); cur.traverse(*this,R))
            cerr << this->key(*cur) << ", ";
      }
      cerr << endl;
   }
#endif // POLYMAKE_DEBUG

private:
   /// current number of nodes
   int n_elem;
};


template <typename Traits>
void tree<Traits>::insert_rebalance(Node *n, Node *parent, link_index Tonew)
{
   link_index Oppos=link_index(-Tonew);
   this->link(n,Oppos).set(parent,LEAF);

   if (!tree_form()) {
      this->link((this->link(n,Tonew)=this->link(parent,Tonew)), Oppos).set(n,LEAF);
      this->link(parent,Tonew).set(n,LEAF);
      return;
   }

   if ((this->link(n,Tonew)=this->link(parent,Tonew)).end())
      this->link(this->head_node(),Oppos).set(n,LEAF);
   this->link(n,P).set_direction(parent,Tonew);
   if (this->link(parent,Oppos).skew_strict()) {
      this->link(parent,Oppos).clear(SKEW);
      this->link(parent,Tonew)=n;
      return;
   }

   this->link(parent,Tonew).set(n,SKEW);
   Node *root=root_node();
   while (parent != root) {
      n=parent;
      parent=this->link(n,P);
      Tonew=this->link(n,P).direction();
      Oppos=link_index(-Tonew);

      if (this->link(parent,Tonew).skew()) {
         Node *grandparent=this->link(parent,P);
         link_index Down=this->link(parent,P).direction();

         if (this->link(n,Tonew).skew_strict()) {       // single rotation
            if (this->link(n,Oppos).leaf())
               this->link(parent,Tonew).set(n,LEAF);
            else
               this->link((this->link(parent,Tonew)=(Node*)this->link(n,Oppos)),P).set_direction(parent,Tonew);

            this->link(grandparent,Down).set(n);
            this->link(n,P).set_direction(grandparent,Down);
            this->link(parent,P).set_direction(n,Oppos);
            this->link(n,Tonew).clear(SKEW);
            this->link(n,Oppos)=parent;
         } else {                               // double rotation
            Node *n2=this->link(n,Oppos);
            if (this->link(n2,Tonew).leaf()) {
               this->link(n,Oppos).set(n2,LEAF);
            } else {
               this->link((this->link(n,Oppos)=(Node*)this->link(n2,Tonew)),P).set_direction(n,Oppos);
               this->link(parent,Oppos).set( this->link(n2,Tonew).skew() );
            }
            if (this->link(n2,Oppos).leaf()) {
               this->link(parent,Tonew).set(n2,LEAF);
            } else {
               this->link((this->link(parent,Tonew)=(Node*)this->link(n2,Oppos)),P).set_direction(parent,Tonew);
               this->link(n,Tonew).set( this->link(n2,Oppos).skew() );
            }

            this->link(grandparent,Down).set(n2);
            this->link(n2,P).set_direction(grandparent,Down);
            this->link(n2,Tonew)=n;
            this->link(n,P).set_direction(n2,Tonew);
            this->link(n2,Oppos)=parent;
            this->link(parent,P).set_direction(n2,Oppos);
         }
         break;
      }

      if (this->link(parent,Oppos).skew()) {
         this->link(parent,Oppos).clear(SKEW);
         break;
      }

      this->link(parent,Tonew).set(SKEW);
   }
}

template <typename Traits>
void tree<Traits>::remove_rebalance(Node *n)
{
   if (!n_elem) {               // the last element removed - tree got empty
      this->link(this->head_node(),L)=this->link(this->head_node(),R)=end_node();
      this->link(this->head_node(),P)=NULL;
      return;
   }

   Node *lf=n;
   Node *parent=this->link(n,P);
   link_index Down=this->link(n,P).direction();
   if (! this->link(n,L).leaf() && ! this->link(n,R).leaf()) {
      link_index Descend;                       // the node to be removed has both subtrees, find the
      Node *neighbor_leaf;                      // lexicographically adjacent as replacement
      if (this->link(n,L).skew()) {
         neighbor_leaf=traverse(Ptr(n),*this,R);
         Descend=L;
      } else {
         neighbor_leaf=traverse(Ptr(n),*this,L);
         Descend=R;
      }
      link_index Oppos=link_index(-Descend), lf_Down=Descend;
      while (1) {
         lf=this->link(lf,lf_Down);
         if (this->link(lf,Oppos).leaf()) break;
         lf_Down=Oppos;
      }
      this->link(neighbor_leaf,Descend).set(lf,LEAF);

      this->link(parent,Down).set(lf);
      this->link((this->link(lf,Oppos)=this->link(n,Oppos)), P).set_direction(lf,Oppos);

      if (lf_Down==Descend) {
         if (!this->link(n,Descend).skew() && this->link(lf,Descend).skew_strict())
            this->link(lf,Descend).clear(SKEW);
         this->link(lf,P).set_direction(parent,Down);
         parent=lf;
      } else {
         Node *lf_parent=this->link(lf,P);
         if (this->link(lf,Descend).leaf()) {
            this->link(lf_parent,lf_Down).set(lf,LEAF);
         } else {
            Node *n2=this->link(lf,Descend);
            this->link(lf_parent,lf_Down).set(n2);
            this->link(n2,P).set_direction(lf_parent,lf_Down);
         }
         this->link((this->link(lf,Descend)=this->link(n,Descend)), P).set_direction(lf,Descend);
         this->link(lf,P).set_direction(parent,Down);
         parent=lf_parent;
      }
      Down=lf_Down;
   } else {
      link_index Toleaf= this->link(n,L).leaf() ? L : R;
      link_index Oppos=link_index(-Toleaf);
      if (this->link(n,Oppos).leaf()) {
         if ((this->link(parent,Down)=this->link(n,Down)).end())
            this->link(this->head_node(),link_index(-Down)).set(parent,LEAF);
      } else {
         Node *n2=this->link(n,Oppos);
         this->link(parent,Down).set(n2);
         this->link(n2,P).set_direction(parent,Down);
         if ((this->link(n2,Toleaf)=this->link(n,Toleaf)).end())
            this->link(this->head_node(),Oppos).set(n2,LEAF);
      }
   }

   // rebalance the tree
   while (parent != this->head_node()) {
      Node *cur=parent;
      link_index Todel=Down, Oppos=link_index(-Todel);
      parent=this->link(cur,P);
      Down=this->link(cur,P).direction();
      if (this->link(cur,Todel).skew_strict()) {
         this->link(cur,Todel).clear(SKEW);
      } else if (this->link(cur,Oppos).skew_strict()) {
         Node *n1=this->link(cur,Oppos);
         if (this->link(n1,Todel).skew()) {        // double rotation
            Node *n2=this->link(n1,Todel);
            if (this->link(n2,Todel).leaf()) {
               this->link(cur,Oppos).set(n2,LEAF);
            } else {
               this->link((this->link(cur,Oppos)=(Node*)this->link(n2,Todel)),P).set_direction(cur,Oppos);
               this->link(n1,Oppos).set( this->link(n2,Todel).skew() );
            }
            if (this->link(n2,Oppos).leaf()) {
               this->link(n1,Todel).set(n2,LEAF);
            } else {
               this->link((this->link(n1,Todel)=(Node*)this->link(n2,Oppos)),P).set_direction(n1,Todel);
               this->link(cur,Todel).set( this->link(n2,Oppos).skew() );
            }

            this->link(parent,Down).set(n2);
            this->link(n2,P).set_direction(parent,Down);
            this->link(n2,Todel)=cur;
            this->link(cur,P).set_direction(n2,Todel);
            this->link(n2,Oppos)=n1;
            this->link(n1,P).set_direction(n2,Oppos);
         } else {                                       // single rotation
            if (this->link(n1,Todel).leaf())
               this->link(cur,Oppos).set(n1,LEAF);
            else
               this->link((this->link(cur,Oppos)=this->link(n1,Todel)),P).set_direction(cur,Oppos);

            this->link(parent,Down).set(n1);
            this->link(n1,P).set_direction(parent,Down);
            this->link(n1,Todel)=cur;
            this->link(cur,P).set_direction(n1,Todel);
            if (this->link(n1,Oppos).skew_strict()) {
               this->link(n1,Oppos).clear(SKEW);
            } else {
               this->link(n1,Todel).set(SKEW);
               this->link(cur,Oppos).set(SKEW);
               break;
            }
         }
      } else if (! this->link(cur,Oppos).leaf()) {
         this->link(cur,Oppos).set(SKEW);
         break;
      }
   }
}

template <typename Traits>
void tree<Traits>::update_node(Node *n)
{
   if (n_elem<=1) return;

   if (tree_form()) {
      Ptr l(n), r(n);
      l.traverse(*this,L); r.traverse(*this,R);
      if ((!l.end() && this->key_comparator(this->key(*l), this->key(*n))==cmp_gt) ||
          (!r.end() && this->key_comparator(this->key(*r), this->key(*n))==cmp_lt)) {
         --n_elem;
         remove_rebalance(n);
         insert_node(n);
      }

   } else {
      Ptr next(n);
      do
         next=this->link(next,L);
      while (!next.end() && this->key_comparator(this->key(*next), this->key(*n)) == cmp_gt);

      next=this->link(next,R);
      if (next != n) {
         swap_nodes_list_form(next,n);
         return;
      }

      do
         next=this->link(next,R);
      while (!next.end() && this->key_comparator(this->key(*n), this->key(*next)) == cmp_gt);

      next=this->link(next,L);
      if (next != n) {
         swap_nodes_list_form(n,next);
      }
   }
}

/** Standard node of an AVL tree
    @tmplparam K type of the key field
    @tmplparam D type of the data field
*/
template <typename K, typename D>
struct node {
   typedef D mapped_type;
   typedef pair<const K, D> value_type;

   /// links in the tree
   Ptr<node> links[3];
   /// key and data fields
   value_type key_and_data;

   template <typename Key>
   explicit node(const Key& key_arg)
      : key_and_data(key_arg,D()) {}

   template <typename Key, typename Data>
   explicit node(const pair<Key,Data>& p)
      : key_and_data(p) {}

   template <typename Key, typename Data>
   node(const Key& key_arg, const Data& data_arg)
      : key_and_data(key_arg,data_arg) {}

   template <typename Key>
   node(const Key& key_arg, const nothing&)
      : key_and_data(key_arg,D()) {}

   /// doesn't copy the tree links
   node(const node& o) : key_and_data(o.key_and_data) {}
};

/** Key-data pair access filter for @c node.
    Key field is referred to as @c first, data field as @c second.
*/
template <typename NodeRef, bool no_data=std::is_same<typename deref<NodeRef>::type::mapped_type, nothing>::value>
struct node_accessor_impl {
   typedef NodeRef argument_type;
   typedef typename inherit_ref<typename deref<NodeRef>::type::value_type, NodeRef>::type result_type;
   result_type operator() (argument_type n) const { return n.key_and_data; }
};

template <typename NodeRef>
struct node_accessor_impl<NodeRef, true> {
   typedef NodeRef argument_type;
   typedef typename inherit_ref<typename deref<NodeRef>::type::value_type::first_type, NodeRef>::type result_type;
   result_type operator() (argument_type n) const { return n.key_and_data.first; }
};

template <typename NodeRef>
struct node_accessor : node_accessor_impl<NodeRef> {};

template <typename K, typename D, typename Comparator>
struct it_traits {
   typedef node<K,D> Node;

   static Ptr<Node>& link(Node *n, link_index X) { return n->links[X-L]; }

   const it_traits& get_it_traits() const { return *this; }
};

template <typename K, typename D, typename Comparator>
class traits : public it_traits<K,D,Comparator> {
public:
   typedef it_traits<K,D,Comparator> traits_for_iterator;
   typedef typename traits_for_iterator::Node Node;
   typedef K key_type;
   typedef Comparator key_comparator_type;
   static const bool allow_multiple=false;
   typedef std::allocator<Node> node_allocator_type;

protected:
   mutable Ptr<Node> root_links[3];
   key_comparator_type key_comparator;
   node_allocator_type node_allocator;

   Node* head_node() const { return reinterpret_cast<Node*>(&root_links[0]); }

   static const K& key(const Node& n) { return n.key_and_data.first; }

   static D& data(Node& n) { return n.key_and_data.second; }

   template <typename Key, typename Data>
   Node* create_from_pair(const pair<Key, Data>& arg, std::false_type)
   {
      return new(node_allocator.allocate(1)) Node(arg.first, arg.second);
   }

   template <typename Key1, typename Key2>
   Node* create_from_pair(const pair<Key1, Key2>& arg, std::true_type)
   {
      return new(node_allocator.allocate(1)) Node(arg, nothing());
   }

   template <typename Key1, typename Key2>
   Node* create_node(const pair<Key1, Key2>& arg)
   {
      return create_from_pair(arg, bool_constant<isomorphic_types<pair<Key1, Key2>, key_type>::value>());
   }

   template <typename Key>
   Node* create_node(const Key& key_arg)
   {
      return new(node_allocator.allocate(1)) Node(key_arg);
   }

   template <typename Key, typename Data>
   Node* create_node(const Key& key_arg, const Data& data_arg)
   {
      return new(node_allocator.allocate(1)) Node(key_arg, data_arg);
   }

   Node* clone_node(Node *n)
   {
      return new(node_allocator.allocate(1)) Node(*n);
   }

   void destroy_node(Node *n)
   {
      node_allocator.destroy(n);
      node_allocator.deallocate(n,1);
   }

   static bool own_node(Node*) { return true; }

public:
   typedef const key_comparator_type& arg_type;

   traits() {}
   explicit traits(arg_type cmp_arg) : key_comparator(cmp_arg) {}

   static int max_size()
   {
      return (unsigned int)-1/sizeof(Node);
   }

   const key_comparator_type& get_comparator() const { return key_comparator; }
};

} // end namespace AVL

template <typename Traits, AVL::link_index Dir>
struct check_iterator_feature<AVL::tree_iterator<Traits,Dir>, end_sensitive> : std::true_type {};

template <typename Traits, AVL::link_index Dir>
struct iterator_reversed< AVL::tree_iterator<Traits,Dir> > {
   typedef AVL::tree_iterator<Traits, AVL::link_index(-Dir)> type;

   static
   const AVL::tree_iterator<Traits,Dir>& revert(const type& it)
   {
      return reinterpret_cast<const AVL::tree_iterator<Traits,Dir>&>(it);
   }
};

} // end namespace pm

namespace std {
   template <typename Traits>
   inline void swap(pm::AVL::tree<Traits>& t1, pm::AVL::tree<Traits>& t2) { t1.swap(t2); }
}

#endif // POLYMAKE_INTERNAL_AVL_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
