/* Copyright (c) 1997-2016
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

#ifndef POLYMAKE_INTERNAL_SPARSE2D_H
#define POLYMAKE_INTERNAL_SPARSE2D_H

#include "polymake/internal/AVL.h"
#include "polymake/internal/tree_containers.h"
#include "polymake/internal/sparse2d_ruler.h"
#include "polymake/SelectedSubset.h"
#include "polymake/vector"

#include <stdexcept>
#if POLYMAKE_DEBUG
#  include <sstream>
#endif

namespace pm {

template <typename Matrix> class Rows;
template <typename Matrix> class Cols;

namespace sparse2d {

using pm::relocate;

template <typename E>
struct cell {
   typedef E mapped_type;

   int key;             /// sum of row and column indices
   AVL::Ptr<cell> links[6];
   E data;

   explicit cell(int i) : key(i), data() {}
   template <typename Data>
   cell(int i_arg, const Data& data_arg) : key(i_arg), data(data_arg) {}

   /// don't copy the tree links
   cell(const cell& o) : key(o.key), data(o.data) {}

   mapped_type& get_data() { return data; }
   const mapped_type& get_data() const { return data; }
};

template <>
struct cell<nothing> {
   typedef nothing mapped_type;

   int key;                     /// sum of row and column indices
   AVL::Ptr<cell> links[6];

   explicit cell(int i) : key(i) {}
   cell(int i, const nothing&) : key(i) {}

   /// don't copy the tree links
   cell(const cell& o) : key(o.key) {}

   const nothing& get_data() const
   {
      return pair<int,nothing>::second;
   }
};

enum restriction_kind { full, dying, only_rows, only_cols };

template <restriction_kind k>
using restriction_const = std::integral_constant<restriction_kind, k>;

typedef restriction_const<only_rows> rowwise;
typedef restriction_const<only_cols> columnwise;

template <typename TMatrix> inline
decltype(auto) lines(TMatrix& m, rowwise) { return pm::rows(m); }

template <typename TMatrix> inline
decltype(auto) lines(TMatrix& m, columnwise) { return pm::cols(m); }

template <typename E, bool symmetric=false, restriction_kind restriction=full> class Table;
template <typename Base, bool symmetric, restriction_kind restriction=full> class traits;
template <typename,typename,bool> struct asym_permute_entries;
template <typename Traits> struct sym_permute_entries;

template <typename E, bool _row_oriented, bool _symmetric>
class it_traits {
protected:
   int line_index;
public:
   static const bool row_oriented=_row_oriented, symmetric=_symmetric;
   typedef cell<E> Node;
   typedef it_traits<E,(!symmetric && !row_oriented),symmetric> cross_traits;

   AVL::Ptr<Node>& link(Node *n, AVL::link_index X) const
   {
      const int in_row= symmetric ? n->key > 2*line_index : row_oriented;
      return n->links[X-AVL::L + in_row*3];
   }

   it_traits(int index_arg=0) : line_index(index_arg) {}

   int get_line_index() const { return line_index; }
   const it_traits& get_it_traits() const { return *this; }
};

template <typename E, bool _row_oriented, bool _symmetric, restriction_kind restriction=full>
class traits_base : public it_traits<E, _row_oriented, _symmetric> {
public:
   typedef it_traits<E,_row_oriented,_symmetric> traits_for_iterator;
   typedef typename traits_for_iterator::Node Node;
protected:
   mutable AVL::Ptr<Node> root_links[3];

public:
   static const bool
      symmetric=_symmetric,
      row_oriented=_row_oriented,
      allow_multiple=false,
      cross_oriented= restriction!=only_rows && restriction!=only_cols ? !symmetric && !row_oriented : row_oriented,
      fixed_dim= _symmetric || restriction==(_row_oriented ? only_cols : only_rows);

   typedef E mapped_type;
   typedef AVL::tree< traits<traits_base, symmetric, restriction> > own_tree;
   typedef AVL::tree< traits<traits_base<E,cross_oriented,symmetric,restriction>, symmetric, restriction> > cross_tree;
   typedef ruler<own_tree, typename std::conditional<symmetric, nothing, void*>::type> own_ruler;
   typedef ruler<cross_tree, typename std::conditional<symmetric, nothing, void*>::type> cross_ruler;
protected:
   Node* head_node() const
   {
      return reinterpret_cast<Node*>(reinterpret_cast<char*>(const_cast<traits_base*>(this))
                                     - (!symmetric && row_oriented)*sizeof(root_links));
   }

   const own_ruler& get_cross_ruler(std::true_type) const
   {
      return own_ruler::reverse_cast(static_cast<const own_tree*>(this), this->line_index);
   }
   own_ruler& get_cross_ruler(std::true_type)
   {
      return own_ruler::reverse_cast(static_cast<own_tree*>(this), this->line_index);
   }
   const cross_ruler& get_cross_ruler(std::false_type) const
   {
      return *reinterpret_cast<const cross_ruler*>(get_cross_ruler(std::true_type()).prefix());
   }
   cross_ruler& get_cross_ruler(std::false_type)
   {
      return *reinterpret_cast<cross_ruler*>(get_cross_ruler(std::true_type()).prefix());
   }
   const cross_ruler& get_cross_ruler() const
   {
      return get_cross_ruler(bool_constant<symmetric>());
   }
   cross_ruler& get_cross_ruler()
   {
      return get_cross_ruler(bool_constant<symmetric>());
   }

   /** @retval <0 first visit of two
                0 first and only visit
               >0 second visit of two */
   int visit_by_copy(Node* n) const
   {
      if (symmetric)
         return 2*this->line_index - n->key;
      else
         return row_oriented ? -1 : 1;
   }

   void notify_add(Node*) {}
   void notify_remove(Node*) {}

public:
   typedef int arg_type;
   traits_base(int index_arg) : traits_for_iterator(index_arg) {}

   const cross_tree& get_cross_tree(int i) const
   {
      return get_cross_ruler()[i];
   }
   cross_tree& get_cross_tree(int i)
   {
      return get_cross_ruler()[i];
   }
   
   template <typename,bool,restriction_kind> friend class Table;
};

template <typename Base, bool symmetric, restriction_kind restriction>
class traits : public Base {
public:
   typedef int key_type;
   typedef operations::cmp key_comparator_type;
   typedef typename Base::Node Node;
   typedef std::allocator<Node> node_allocator_type;
protected:
   key_comparator_type key_comparator;
   node_allocator_type node_allocator;

   int key(const Node& n) const { return n.key - this->get_line_index(); }

   static typename Base::mapped_type& data(Node& n) { return n.data; }

   void insert_node_cross(Node *n, int i, std::true_type)
   {
      this->get_cross_tree(i).insert_node(n);
   }
   void insert_node_cross(Node *, int i, std::false_type)
   {
      long& max_cross=reinterpret_cast<long&>(this->get_cross_ruler(std::true_type()).prefix());
      if (i>=max_cross) max_cross=i+1;
   }

   Node* create_node(int i)
   {
      Node *n=new(node_allocator.allocate(1)) Node(i + this->get_line_index());
      insert_node_cross(n, i, bool_constant<restriction==full>());
      this->notify_add(n);
      return n;
   }

   template <typename Data>
   Node* create_node(int i, const Data& data)
   {
      Node *n=new(node_allocator.allocate(1)) Node(i + this->get_line_index(), data);
      insert_node_cross(n, i, bool_constant<restriction==full>());
      this->notify_add(n);
      return n;
   }

   template <typename Data>
   Node* create_node(const pair<int, Data>& p)
   {
      return create_node(p.first, p.second);
   }

   Node* create_node(const pair<int, nothing>& p)
   {
      return create_node(p.first);
   }

   void remove_node_cross(Node* n, std::true_type)
   {
      this->get_cross_tree(n->key - this->get_line_index()).remove_node(n);
   }
   void remove_node_cross(Node*, std::false_type) {}

   Node* clone_node(Node* n)
   {
      const int visit=this->visit_by_copy(n);
      Node *clone= visit<=0 ? new(node_allocator.allocate(1)) Node(*n) : n->links[1];
      if (visit<0) {
         clone->links[1]=n->links[1];
         n->links[1]=clone;
      } else if (visit>0) {
         n->links[1]=clone->links[1];
      }
      return clone;
   }

   static bool own_node(Node*)
   {
      return restriction != dying || Base::row_oriented;
   }

   int max_size_impl(std::true_type) const
   {
      return this->get_cross_ruler().size();
   }
   int max_size_impl(std::false_type) const
   {
      return reinterpret_cast<const long&>(this->get_cross_ruler(std::true_type()).prefix());
   }

public:
   traits() {}
   traits(typename Base::arg_type init_arg) : Base(init_arg) {}

   void destroy_node(Node* n)
   {
      remove_node_cross(n, bool_constant<restriction==full>());
      if (restriction != dying) this->notify_remove(n);
      node_allocator.destroy(n);
      node_allocator.deallocate(n,1);
   }

   int max_size() const
   {
      return max_size_impl(bool_constant<(restriction!=only_rows && restriction!=only_cols)>());
   }
   const key_comparator_type& get_comparator() const
   {
      return key_comparator;
   }

   friend void relocate(traits *from, traits *to)
   {
      pm::relocate(static_cast<Base*>(from), static_cast<Base*>(to), std::true_type());
   }

   template <typename, bool, restriction_kind> friend class Table;
   template <typename> friend struct sym_permute_entries;
   template <typename, typename, bool> friend struct asym_permute_entries;
};

template <typename Base, restriction_kind restriction>
class traits<Base, true, restriction> : public traits<Base, false, restriction> {
public:
   typedef typename traits<Base,false,restriction>::Node Node;
protected:
   Node* insert_node(Node *n, int i)
   {
      if (i != this->get_line_index())
         this->get_cross_tree(i).insert_node(n);
      this->notify_add(n);
      return n;
   }

   Node* create_node(int i)
   {
      return insert_node(new(this->node_allocator.allocate(1)) Node(i + this->get_line_index()), i);
   }

   template <typename Data>
   Node* create_node(int i, const Data& data)
   {
      return insert_node(new(this->node_allocator.allocate(1)) Node(i + this->get_line_index(), data), i);
   }

   template <typename Data>
   Node* create_node(const pair<int, Data>& p)
   {
      return create_node(p.first, p.second);
   }

   Node* create_node(const pair<int, nothing>& p)
   {
      return create_node(p.first);
   }

   void remove_node_cross(Node* n, std::true_type)
   {
      const int l=this->get_line_index(), i=n->key - l;
      if (i != l) this->get_cross_tree(i).remove_node(n);
   }
   void remove_node_cross(Node*, std::false_type) {}

   bool own_node(Node *n) const
   {
      return restriction==full || n->key >= 2*this->get_line_index();
   }
public:
   traits() {}
   traits(typename Base::arg_type init_arg) : traits<Base,false,restriction>(init_arg) {}

   void destroy_node(Node* n)
   {
      remove_node_cross(n, bool_constant<restriction==full>());
      if (restriction != dying) this->notify_remove(n);
      this->node_allocator.destroy(n);
      this->node_allocator.deallocate(n,1);
   }
};

template <typename CellRef>
struct cell_accessor {
   typedef CellRef argument_type;
   typedef typename deref<CellRef>::type::mapped_type mapped_type;
   typedef typename std::conditional<std::is_same<mapped_type, nothing>::value,
                                     const nothing&,
                                     typename inherit_ref<mapped_type,CellRef>::type>::type
      result_type;
   result_type operator() (typename function_argument<CellRef>::type c) const { return c.get_data(); }
};

template <typename IteratorRef>
struct cell_index_accessor {
   typedef IteratorRef argument_type;
   typedef const int result_type;

   result_type operator() (argument_type it) const
   {
      return it->key - it.get_line_index();
   }
};

template <typename row_ruler, typename col_ruler, bool restricted>
struct asym_permute_entries {
   col_ruler *C;

   typedef typename row_ruler::value_type tree_type;
   typedef typename tree_type::Node Node;

   static void relocate(tree_type *from, tree_type *to) { relocate_tree(from, to, std::true_type()); }

   void operator()(row_ruler*, row_ruler* R) const
   {
      if (!restricted) {
         for (typename Entire<col_ruler>::iterator ci=entire(*C);  !ci.at_end();  ++ci)
            ci->init();
         R->prefix()=C;
         C->prefix()=R;
      }
      int r=0;
      for (typename Entire<row_ruler>::iterator ri=entire(*R); !ri.at_end();  ++ri, ++r) {
         const int old_r=ri->line_index, rdiff=r-old_r;
         ri->line_index=r;
         for (typename tree_type::iterator e=ri->begin(); !e.at_end(); ++e) {
            Node *node=e.operator->();
            const int c=node->key-old_r;
            node->key += rdiff;
            if (!restricted) (*C)[c].push_back_node(node);
         }
      }
   }

   asym_permute_entries(col_ruler *C_arg) : C(C_arg) {}
};

template <typename Iterator> struct cross_direction_helper;

template <typename Traits, AVL::link_index Dir>
struct cross_direction_helper< AVL::tree_iterator<Traits,Dir> > {
   typedef AVL::tree_iterator<typename inherit_const<typename Traits::cross_traits, Traits>::type, Dir> iterator;
};

template <typename Iterator> inline
unary_transform_iterator<typename cross_direction_helper<Iterator>::iterator,
                         pair< BuildUnary<cell_accessor>, BuildUnaryIt<cell_index_accessor> > >
cross_direction(const unary_transform_iterator<Iterator, pair< BuildUnary<cell_accessor>, BuildUnaryIt<cell_index_accessor> > >& it)
{
   return typename cross_direction_helper<Iterator>::iterator(it.index(), it.Iterator::operator->());
}

template <typename Top, typename E, bool symmetric, restriction_kind restriction, typename LineFactory> class Rows;
template <typename Top, typename E, bool symmetric, restriction_kind restriction, typename LineFactory> class Cols;
template <typename Tree> class line;

template <typename E, bool symmetric, restriction_kind restriction>
class Table {
protected:
   static const bool restricted= restriction != full;
   typedef traits<traits_base<E,true,symmetric,restriction>, symmetric,restriction> row_tree_traits;
   typedef traits<traits_base<E,false,symmetric,restriction>, symmetric,restriction> col_tree_traits;
public:
   typedef AVL::tree<row_tree_traits> row_tree_type;
   typedef AVL::tree<col_tree_traits> col_tree_type;
   typedef typename std::conditional<restriction==only_cols, col_tree_type, row_tree_type>::type primary_tree_type;
   typedef typename row_tree_traits::own_ruler row_ruler;
   typedef typename col_tree_traits::own_ruler col_ruler;
protected:
   typedef Table<E,symmetric,dying> restricted_Table;

   row_ruler* R;
   col_ruler* C;

   void copy_impl(const Table& t, int add_r=0, int add_c=0)
   {
      R= restriction != only_cols ? row_ruler::construct(*t.R,add_r) : 0;
      C= restriction != only_rows ? col_ruler::construct(*t.C,add_c) : 0;
      if (!restricted) {
         R->prefix()=C;
         C->prefix()=R;
      } else if (restriction == only_rows) {
         R->prefix()=t.R->prefix();
      } else if (restriction == only_cols) {
         C->prefix()=t.C->prefix();
      }
   }
public:
   Table()
      : R(row_ruler::construct(0))
      , C(col_ruler::construct(0))
   {
      R->prefix()=C;
      C->prefix()=R;
   }

   explicit Table(std::nullptr_t)
   {
      if (!restricted)
         throw std::runtime_error("sparse2d::Table - both dimensions required in unrestricted mode");
      R=nullptr; C=nullptr;
   }

   explicit Table(int n)
      : R(restriction != only_cols ? row_ruler::construct(n) : 0)
      , C(restriction != only_rows ? col_ruler::construct(n) : 0)
   {
      if (!restricted)
         throw std::runtime_error("sparse2d::Table - both dimensions required in unrestricted mode");
      if (restriction == only_rows)
         R->prefix()=nullptr;
      else
         C->prefix()=nullptr;
   }

   Table(int r, int c)
      : R(restriction != only_cols ? row_ruler::construct(r) : 0)
      , C(restriction != only_rows ? col_ruler::construct(c) : 0)
   {
      if (restriction == only_rows) {
         reinterpret_cast<long&>(R->prefix())=c;
      } else if (restriction == only_cols) {
         reinterpret_cast<long&>(C->prefix())=r;
      } else {
         R->prefix()=C;
         C->prefix()=R;
      }
   }

   Table(const Table& t, int add_r=0, int add_c=0)
   {
      copy_impl(t, add_r, add_c);
   }
private:
   template <typename TRow_ruler, typename TCol_ruler>
   static TCol_ruler* take_over(TRow_ruler* R, TCol_ruler* C, std::false_type)
   {
      static_assert(restriction==full, "sparse2d::Table - moving between two restricted modes");
      C=TCol_ruler::construct(reinterpret_cast<const long&>(R->prefix()));
      for (auto t=entire(*R);  !t.at_end();  ++t)
         for (auto e=t->begin(); !e.at_end(); ++e)
            (*C)[e->key - t->get_line_index()].push_back_node(e.operator->());
      R->prefix()=C;
      C->prefix()=R;
      return C;
   }

   template <typename TRow_ruler, typename TCol_ruler>
   static TCol_ruler* take_over(TRow_ruler* R, TCol_ruler* C, std::true_type) {}

public:
   Table(Table<E, symmetric, only_rows>&& t)
   {
      R=reinterpret_cast<row_ruler*>(t.R); t.R=nullptr;
      C=take_over(R, C, bool_constant<restriction==only_rows>());
   }

   Table(Table<E, symmetric, only_cols>&& t)
   {
      C=reinterpret_cast<col_ruler*>(t.C); t.C=nullptr;
      R=take_over(C, R, bool_constant<restriction==only_cols>());
   }

   ~Table()
   {
      if (restriction==full) {
         destroy_at(reinterpret_cast<restricted_Table*>(this));
      } else {
         if (restriction==only_cols && C || restriction==dying) col_ruler::destroy(C);
         if (restriction==only_rows && R || restriction==dying) row_ruler::destroy(R);
      }
   }

   Table& operator= (const Table& t)
   {
      this->~Table();
      new(this) Table(t);
      return *this;
   }

   Table& operator= (Table<E, symmetric, only_rows>&& t)
   {
      this->~Table();
      new(this) Table(t);
      return *this;
   }

   Table& operator= (Table<E, symmetric, only_cols>&& t)
   {
      this->~Table();
      new(this) Table(t);
      return *this;
   }

   void swap(Table& t)
   {
      std::swap(R,t.R);
      std::swap(C,t.C);
   }

   int rows() const
   {
      return restriction != only_cols ? R->size() : reinterpret_cast<const long&>(C->prefix());
   }
   int cols() const
   {
      return restriction != only_rows ? C->size() : reinterpret_cast<const long&>(R->prefix());
   }

   void clear(int r=0, int c=0)
   {
      if (restriction==full) {
         reinterpret_cast<restricted_Table*>(this)->clear(r,c);
      } else if (restriction==only_rows) {
         R=row_ruler::resize_and_clear(R,r);
         R->prefix()=nullptr;
      } else if (restriction==only_cols) {
         C=col_ruler::resize_and_clear(C,r);    // not a typo: the second argument must be omitted in this case
         C->prefix()=nullptr;
      } else {
         R=row_ruler::resize_and_clear(R,r);
         C=col_ruler::resize_and_clear(C,c);
         R->prefix()=C;
         C->prefix()=R;
      }
   }

   struct shared_clear {
      int r, c;
      shared_clear(int r_arg, int c_arg) : r(r_arg), c(c_arg) {}

      void operator() (void *p, const Table&) const { new(p) Table(r,c); }
      void operator() (Table& t) const { t.clear(r,c); }
   };

   void resize_rows(int r)
   {
      if (restriction == only_rows && !R) {
         R=row_ruler::construct(r);
         R->prefix()=nullptr;
      } else if (restriction == only_cols) {
         reinterpret_cast<long&>(C->prefix())=r;
      } else {
         R=row_ruler::resize(R,r);
      }
      if (!restricted) {
         R->prefix()=C;
         C->prefix()=R;
      }
   }

   void resize_cols(int c)
   {
      if (restriction == only_cols && !C) {
         C=col_ruler::construct(c);
         C->prefix()=nullptr;
      } else if (restriction == only_rows) {
         reinterpret_cast<long&>(R->prefix())=c;
      } else {
         C=col_ruler::resize(C,c);
      }
      if (!restricted) {
         R->prefix()=C;
         C->prefix()=R;
      }
   }

   struct shared_add_rows {
      int n;
      shared_add_rows(int n_arg) : n(n_arg) {}

      void operator() (void *p, const Table& t) const { new(p) Table(t,n,0); }
      void operator() (Table& t) const { t.resize_rows(t.rows()+n); }
   };

   struct shared_add_cols {
      int n;
      shared_add_cols(int n_arg) : n(n_arg) {}

      void operator() (void *p, const Table& t) const { new(p) Table(t,0,n); }
      void operator() (Table& t) const { t.resize_cols(t.cols()+n); }
   };

   void resize(int r, int c)
   {
      if (!restricted) {
         R=row_ruler::resize(R,r);
         C=col_ruler::resize(C,c);
         R->prefix()=C;
         C->prefix()=R;
      } else {
         if (restricted)
            throw std::runtime_error("sparse2d::Table::resize - exactly one non-zero dimension allowed in restricted mode");
      }
   }

protected:
   template <typename TRuler, typename number_consumer>
   static void _squeeze(TRuler* &R, const number_consumer& nc)
   {
      int i=0, inew=0;
      for (auto t=R->begin(), end=R->end(); t!=end; ++t, ++i) {
         if (t->size()) {
            if (int idiff=i-inew) {
               t->line_index=inew;
               for (auto e=entire(*t); !e.at_end(); ++e)
                  e->key -= idiff;
               relocate_tree(t.operator->(), &t[-idiff], std::true_type());
            }
            nc(i, inew);  ++inew;
         } else {
            destroy_at(t.operator->());
         }
      }
      if (inew < i) R=TRuler::resize(R,inew,false);
   }
public:
   /** Remove the empty rows and columns.
       The remaining rows and columns are renumbered without gaps.
   */
   template <typename row_number_consumer, typename col_number_consumer>
   void squeeze(const row_number_consumer& rnc, const col_number_consumer& cnc)
   {
      if (restriction != only_cols) _squeeze(R, rnc);
      if (restriction != only_rows) _squeeze(C, cnc);
      if (!restricted) R->prefix()=C, C->prefix()=R;
   }

   template <typename row_number_consumer>
   void squeeze(const row_number_consumer& rnc) { squeeze(rnc, operations::binary_noop()); }

   void squeeze() { squeeze(operations::binary_noop(), operations::binary_noop()); }

   template <typename number_consumer>
   void squeeze_rows(const number_consumer& nc)
   {
      if (restriction==only_cols)
         throw std::runtime_error("squeeze_rows not allowed in restricted-to-columns mode");
      _squeeze(R, nc);
      if (!restricted) R->prefix()=C, C->prefix()=R;
   }

   void squeeze_rows() { squeeze_rows(operations::binary_noop()); }

   template <typename number_consumer>
   void squeeze_cols(const number_consumer& nc)
   {
      if (restriction==only_rows)
         throw std::runtime_error("squeeze_rows not allowed in restricted-to-rows mode");
      _squeeze(C, nc);
      if (!restricted) R->prefix()=C, C->prefix()=R;
   }

   void squeeze_cols() { squeeze_cols(operations::binary_noop()); }

   template <typename Iterator, typename _inverse>
   void permute_rows(Iterator perm, _inverse)
   {
      if (restriction==only_cols)
         throw std::runtime_error("sparse2d::Table::permute_rows - disabled in restricted mode");
      R=row_ruler::permute(R, perm, asym_permute_entries<row_ruler, col_ruler, restriction==only_rows>(C), _inverse());
   }

   template <class Iterator, typename _inverse>
   void permute_cols(Iterator perm, _inverse)
   {
      if (restriction==only_rows)
         throw std::runtime_error("sparse2d::Table::permute_cols - disabled in restricted mode");
      C=col_ruler::permute(C, perm, asym_permute_entries<col_ruler, row_ruler, restriction==only_cols>(R), _inverse());
   }

   row_tree_type& row(int i)             { return (*R)[i]; }
   const row_tree_type& row(int i) const { return (*R)[i]; }
   col_tree_type& col(int i)             { return (*C)[i]; }
   const col_tree_type& col(int i) const { return (*C)[i]; }

   line<row_tree_type>& get_line(int i, row_tree_type*) { return reinterpret_cast<line<row_tree_type>&>(row(i)); }
   const line<row_tree_type>& get_line(int i, row_tree_type*) const { return reinterpret_cast<const line<row_tree_type>&>(row(i)); }
   line<col_tree_type>& get_line(int i, col_tree_type*) { return reinterpret_cast<line<col_tree_type>&>(col(i)); }
   const line<col_tree_type>& get_line(int i, col_tree_type*) const { return reinterpret_cast<const line<col_tree_type>&>(col(i)); }

#if POLYMAKE_DEBUG
   void check() const
   {
      if (restriction != only_cols)
         for (int r=0, rend=this->rows(); r<rend; ++r) {
            std::ostringstream label;
            label << "row " << r << ": ";
            (*R)[r].check(label.str().c_str());
         }
      if (restriction != only_cols)
         for (int c=0, cend=this->cols(); c<cend; ++c) {
            std::ostringstream label;
            label << "column " << c << ": ";
            (*C)[c].check(label.str().c_str());
         }
   }
#endif // POLYMAKE_DEBUG

   template <typename, typename, bool, restriction_kind, typename> friend class Rows;
   template <typename, typename, bool, restriction_kind, typename> friend class Cols;
   template <typename, bool, restriction_kind> friend class Table;
};

template <typename Traits>
struct sym_permute_entries : public Traits {
   typedef typename Traits::tree_type tree_type;
   typedef typename Traits::ruler ruler;
   typedef typename tree_type::Node Node;

   static void relocate(tree_type *from, tree_type *to) { relocate_tree(from, to, std::false_type()); }

   static void complete_cross_links(ruler* R)
   {
      int r=0;
      for (typename Entire<ruler>::iterator ri=entire(*R); !ri.at_end();  ++ri, ++r) {
         for (typename tree_type::iterator e=Traits::tree(*ri).begin(); !e.at_end(); ++e) {
            Node *node=e.operator->();
            const int c=node->key-r;
            if (c!=r) Traits::tree((*R)[c]).push_back_node(node);
         }
      }
   }

   void operator()(ruler* Rold, ruler* R) const
   {
      // unfortunately I can't reuse the line_index entries in both old and new rulers,
      // as the iterators always need correct values there
      const int n=R->size();
      std::vector<int> perm(n), inv_perm(n);

      int r=0;
      for (typename Entire<ruler>::iterator ri=entire(*R);  !ri.at_end();  ++ri, ++r) {
         tree_type& t=Traits::tree(*ri);
         perm[r]=t.line_index;
         inv_perm[t.line_index]=r;
         t.line_index=r;
      }

      for (r=0; r<n; ++r) {
         const int old_r=perm[r];
         for (typename tree_type::iterator e=Traits::tree((*Rold)[old_r]).begin(); !e.at_end(); ) {
            Node *node=e.operator->();  ++e;
            const int old_c=node->key-old_r, c=inv_perm[old_c];
            if (old_r!=old_c) Traits::tree((*Rold)[old_c]).unlink_node(node);
            node->key=r+c;
            Traits::tree((*R)[std::max(r,c)]).push_back_node(node);
         }
      }

      complete_cross_links(R);
   }

   template <typename Perm, typename InvPerm>
   static void copy(const ruler *Rold, ruler *R, const Perm& perm, const InvPerm& inv_perm)
   {
      const int n=R->size();
      typename Perm::const_iterator p=perm.begin();
      for (int r=0; r<n; ++r, ++p) {
         const int old_r=*p;
         for (typename tree_type::const_iterator e=Traits::tree((*Rold)[old_r]).begin(); !e.at_end(); ++e) {
            const Node *node=e.operator->();
            const int old_c=node->key-old_r, c=inv_perm[old_c];
            if (c>=r) {
               tree_type& t=Traits::tree((*R)[c]);
               t.push_back_node(new(t.allocate_node()) Node(r+c, node->get_data()));
            }
         }
      }

      complete_cross_links(R);
   }
};

template <typename E, restriction_kind restriction>
class Table<E, true, restriction> {
protected:
   static const bool restricted= restriction==dying;
   typedef traits<traits_base<E,false,true,restriction>, true, restriction> row_tree_traits;
   typedef row_tree_traits col_tree_traits;
public:
   typedef AVL::tree<row_tree_traits> row_tree_type;
   typedef row_tree_type col_tree_type;
   typedef row_tree_type primary_tree_type;
   typedef typename row_tree_traits::own_ruler row_ruler;
   typedef row_ruler col_ruler;
protected:
   typedef Table<E,true,dying> restricted_Table;
   row_ruler *R;
public:
   Table() : R(row_ruler::construct(0)) {}

   explicit Table(int r, int=0)
      : R(row_ruler::construct(r)) {}

   Table(const Table& t, int add_r=0)
      : R(row_ruler::construct(*t.R,add_r)) {}

   ~Table()
   {
      if (restricted)
         row_ruler::destroy(R);
      else
         destroy_at(reinterpret_cast<restricted_Table*>(this));
   }

   Table& operator= (const Table& t)
   {
      this->~Table();
      new(this) Table(t);
      return *this;
   }

   void swap(Table& t) { std::swap(R,t.R); }

   int rows() const { return R->size(); }
   int cols() const { return this->rows(); }

   void clear(int r=0) { R=row_ruler::resize_and_clear(R,r); }

   void resize_rows(int r) { R=row_ruler::resize(R,r); }
   void resize_cols(int r) { resize_rows(r); }

   struct shared_clear {
      int r;
      shared_clear(int r_arg, int=0) : r(r_arg) {}

      void operator() (void *p, const Table&) const { new(p) Table(r); }
      void operator() (Table& t) const { t.clear(r); }
   };

   struct shared_add_rows {
      int n;
      shared_add_rows(int n_arg) : n(n_arg) {}

      void operator() (void *p, const Table& t) const { new(p) Table(t,n); }
      void operator() (Table& t) const { t.resize_rows(t.rows()+n); }
   };

   void resize() { resize_rows(this->rows()); }
   void resize(int r, int=0) { resize_rows(r); }

   template <typename row_number_consumer>
   void squeeze(const row_number_consumer& rnc)
   {
      int r=0, rnew=0;
      for (auto t=R->begin(), end=R->end(); t!=end; ++t, ++r) {
         if (t->size()) {
            if (int rdiff=r-rnew) {
               const int diag=2*r;
               for (auto e=entire(*t); !e.at_end(); ) {
                  cell<E>& c=*e; ++e;
                  c.key -= rdiff << (c.key==diag);
               }
               t->line_index=rnew;
               relocate_tree(&*t, &t[-rdiff], std::true_type());
            }
            rnc(r, rnew);  ++rnew;
         } else {
            destroy_at(t.operator->());
         }
      }
      if (rnew < this->rows()) R=row_ruler::resize(R, rnew, false);
   }

   void squeeze() { squeeze(operations::binary_noop()); }

   template <typename row_number_consumer>
   void squeeze_rows(const row_number_consumer& rnc) { squeeze(rnc); }

   void squeeze_rows() { squeeze(); }

   template <typename row_number_consumer>
   void squeeze_cols(const row_number_consumer& rnc) { squeeze(rnc); }

   void squeeze_cols() { squeeze(); }

   struct perm_traits {
      typedef row_ruler ruler;
      typedef row_tree_type tree_type;
      static tree_type& tree(tree_type& t) { return t; }
      static const tree_type& tree(const tree_type& t) { return t; }
   };
   typedef sym_permute_entries<perm_traits> permute_entries;

   template <typename Iterator, typename _inverse>
   void permute_rows(Iterator perm, _inverse)
   {
      R=row_ruler::permute(R, perm, permute_entries(), _inverse());
   }

   template <class Iterator, typename _inverse>
   void permute_cols(Iterator perm, _inverse)
   {
      permute_rows(perm, _inverse());
   }

   template <typename Perm, typename InvPerm>
   void copy_permuted(const Table& src, const Perm& perm, const InvPerm& inv_perm)
   {
      permute_entries::copy(src.R, R, perm, inv_perm);
   }

   row_tree_type&       row(int i)       { return (*R)[i]; }
   const row_tree_type& row(int i) const { return (*R)[i]; }
   col_tree_type&       col(int i)       { return row(i); }
   const col_tree_type& col(int i) const { return row(i); }

   line<row_tree_type>& get_line(int i, row_tree_type*) { return reinterpret_cast<line<row_tree_type>&>(row(i)); }
   const line<row_tree_type>& get_line(int i, row_tree_type*) const { return reinterpret_cast<const line<row_tree_type>&>(row(i)); }

#if POLYMAKE_DEBUG
   void check() const
   {
      for (int r=0, rend=this->rows(); r<rend; ++r) {
         std::ostringstream label;
         label << "row " << r << ": ";
         (*R)[r].check(label.str().c_str());
      }
   }
#endif

   template <typename, typename, bool, restriction_kind, typename> friend class Rows;
};

template <typename Tree>
class line
   : public modified_tree< line<Tree>,
                           mlist< OperationTag< pair< BuildUnary<cell_accessor>, BuildUnaryIt<cell_index_accessor> > >,
                                  HiddenTag< Tree > > > {
protected:
   ~line();
public:
   int get_line_index() const { return this->hidden().get_line_index(); }
   int dim() const { return this->max_size(); }
};

template <typename TreeRef=void>
struct line_index_accessor {
   typedef TreeRef argument_type;
   typedef TreeRef first_argument_type;
   typedef void second_argument_type;
   typedef const int result_type;

   result_type operator() (TreeRef t) const { return t.get_line_index(); }

   template <typename Iterator2>
   result_type operator() (TreeRef t, const Iterator2&) const { return operator()(t); }
};

template <>
struct line_index_accessor<void> : operations::incomplete {};

} // end namespace sparse2d

template <typename Iterator, typename Reference>
struct unary_op_builder< sparse2d::line_index_accessor<>, Iterator, Reference>
   : empty_op_builder< sparse2d::line_index_accessor<Reference> > {};

template <typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder< sparse2d::line_index_accessor<>, Iterator1, Iterator2, Reference1, Reference2>
   : empty_op_builder< sparse2d::line_index_accessor<Reference1> > {};

namespace sparse2d {

template <typename Top, typename E, bool TSymmetric, restriction_kind TRestriction, typename LineFactory>
class Rows
   : public modified_container_impl< pm::Rows<Top>,
                                     mlist< ContainerTag< typename Table<E, TSymmetric, TRestriction>::row_ruler >,
                                            OperationTag< pair< LineFactory, line_index_accessor<> > >,
                                            HiddenTag< Top > > > {
public:
   typename Rows::container& get_container()
   {
      return *this->hidden().get_table().R;
   }
   const typename Rows::container& get_container() const
   {
      return *this->hidden().get_table().R;
   }
   void resize(int n)
   {
      this->hidden().get_table().resize_rows(n);
   }
};

template <typename Top, typename E, bool TSymmetric, restriction_kind TRestriction, typename LineFactory>
class Cols
   : public modified_container_impl< pm::Cols<Top>,
                                     mlist< ContainerTag< typename Table<E, false, TRestriction>::col_ruler >,
                                            OperationTag< pair< LineFactory, line_index_accessor<> > >,
                                            HiddenTag< Top > > > {
public:
   typename Cols::container& get_container()
   {
      return *this->hidden().get_table().C;
   }
   const typename Cols::container& get_container() const
   {
      return *this->hidden().get_table().C;
   }
   void resize(int n)
   {
      this->hidden().get_table().resize_cols(n);
   }
};

template <typename Top, typename E, restriction_kind TRestriction, typename LineFactory>
class Cols<Top, E, true, TRestriction, LineFactory> : public Rows<Top, E, true, TRestriction, LineFactory> {};

template <typename IteratorRef>
struct lower_triangle_selector {
   typedef IteratorRef argument_type;
   typedef bool result_type;

   bool operator() (argument_type it) const
   {
      return it.index() <= it.get_line_index();
   }
};

template <typename Line>
class lower_triangle_line
   : public modified_container_impl< lower_triangle_line<Line>,
                                     mlist< IteratorConstructorTag< input_truncator_constructor >,
                                            OperationTag< BuildUnaryIt< lower_triangle_selector > >,
                                            MasqueradedTop > > {
   typedef modified_container_impl<lower_triangle_line> base_t;
protected:
   ~lower_triangle_line();
public:
   template <typename First>
   typename base_t::iterator insert(const First& first_arg)
   {
      return this->hidden().insert(first_arg);
   }
   template <typename First, typename Second>
   typename base_t::iterator insert(const First& first_arg, const Second& second_arg)
   {
      return this->hidden().insert(first_arg,second_arg);
   }
   template <typename First, typename Second, typename Third>
   typename base_t::iterator insert(const First& first_arg, const Second& second_arg, const Third& third_arg)
   {
      return this->hidden().insert(first_arg,second_arg,third_arg);
   }
   template <typename Key_or_Iterator>
   void erase(const Key_or_Iterator& k_or_it)
   {
      return this->hidden().erase(k_or_it);
   }
};

template <typename Line> inline
lower_triangle_line<Line>& select_lower_triangle(Line& l)
{
   return reinterpret_cast<lower_triangle_line<Line>&>(l);
}

template <typename Line> inline
const lower_triangle_line<Line>& select_lower_triangle(const Line& l)
{
   return reinterpret_cast<const lower_triangle_line<Line>&>(l);
}

template <typename TreeRef>
struct line_params {
   typedef typename deref<TreeRef>::type tree_type;
   typedef line<tree_type> container;
   typedef typename std::conditional< attrib<TreeRef>::is_reference,
                                      mlist< ContainerTag< typename inherit_const<container, TreeRef>::type > >,
                                      mlist< ContainerTag<container>, HiddenTag<tree_type> > >::type
      type;
};

} // end namespace sparse2d

template <typename TreeRef>
struct check_container_feature<sparse2d::line<TreeRef>, pure_sparse> : std::true_type {};

} // end namespace pm

namespace std {
   template <typename E, bool symmetric, pm::sparse2d::restriction_kind restriction> inline
   void swap(pm::sparse2d::Table<E,symmetric,restriction>& t1, pm::sparse2d::Table<E,symmetric,restriction>& t2)
   { t1.swap(t2); }
} // end namespace std

namespace polymake {
   using pm::sparse2d::only_rows;
   using pm::sparse2d::only_cols;
   using pm::sparse2d::rowwise;
   using pm::sparse2d::columnwise;
}

#endif // POLYMAKE_INTERNAL_SPARSE2D_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
