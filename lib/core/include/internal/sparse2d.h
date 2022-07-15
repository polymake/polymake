/* Copyright (c) 1997-2022
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

#pragma once

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

template <typename E>
struct cell {
   typedef E mapped_type;

   Int key;             /// sum of row and column indices
   AVL::Ptr<cell> links[6];
   E data;

   explicit cell(Int i) : key(i), data() {}
   template <typename Data>
   cell(Int i_arg, const Data& data_arg) : key(i_arg), data(data_arg) {}

   /// don't copy the tree links
   cell(const cell& o) : key(o.key), data(o.data) {}

   mapped_type& get_data() { return data; }
   const mapped_type& get_data() const { return data; }
};

template <>
struct cell<nothing> {
   typedef nothing mapped_type;

   Int key;                     /// sum of row and column indices
   AVL::Ptr<cell> links[6];

   explicit cell(Int i) : key(i) {}
   cell(Int i, const nothing&) : key(i) {}

   /// don't copy the tree links
   cell(const cell& o) : key(o.key) {}

   const nothing& get_data() const
   {
      return pair<Int, nothing>::second;
   }
};

enum restriction_kind { full, dying, only_rows, only_cols };

template <restriction_kind k>
using restriction_const = std::integral_constant<restriction_kind, k>;

typedef restriction_const<only_rows> rowwise;
typedef restriction_const<only_cols> columnwise;

template <typename TMatrix>
decltype(auto) lines(TMatrix& m, rowwise) { return pm::rows(m); }

template <typename TMatrix>
decltype(auto) lines(TMatrix& m, columnwise) { return pm::cols(m); }

template <typename E, bool symmetric=false, restriction_kind restriction=full> class Table;
template <typename Base, bool symmetric, restriction_kind restriction=full> class traits;
template <typename, typename, bool> struct asym_permute_entries;
template <typename Traits> struct sym_permute_entries;

template <typename E, bool is_row_oriented, bool is_symmetric>
class it_traits {
protected:
   Int line_index;
public:
   static constexpr bool row_oriented = is_row_oriented, symmetric = is_symmetric;
   using Node = cell<E>;
   using cross_traits = it_traits<E,(!symmetric && !row_oriented),symmetric>;

   AVL::Ptr<Node>& link(Node* n, AVL::link_index X) const
   {
      const int in_row = symmetric ? n->key > 2 * line_index : row_oriented;
      return n->links[X-AVL::L+in_row*3];
   }

   void prepare_move_between_trees(Node* n, const Int old_line_index, const Int new_line_index) const
   {
      if (is_symmetric) {
         const Int cross_index = n->key - old_line_index;
         if ((cross_index > old_line_index) != (cross_index > new_line_index)) {
            // element has leaped over the diagonal
            for (int i = 0; i < 3; ++i)
               std::swap(n->links[i], n->links[i+3]);
         }
      }
      n->key += new_line_index - old_line_index;
   }

   it_traits(Int index_arg = 0) : line_index(index_arg) {}

   Int get_line_index() const { return line_index; }
   const it_traits& get_it_traits() const { return *this; }
};

union ruler_prefix {
   Int cross_dim;   // during constrution of a restricted table
   char* cross_ruler;   // in a fully constructed 2-d grid

   template <typename Tree>
   void set_cross_ruler(ruler<Tree, ruler_prefix>* r)
   {
      cross_ruler = reinterpret_cast<char*>(r);
   }
   template <typename Tree>
   ruler<Tree, ruler_prefix>* get_cross_ruler()
   {
      return reinterpret_cast<ruler<Tree, ruler_prefix>*>(cross_ruler);
   }
   template <typename Tree>
   const ruler<Tree, ruler_prefix>* get_cross_ruler() const
   {
      return reinterpret_cast<const ruler<Tree, ruler_prefix>*>(cross_ruler);
   }
};

template <typename E, bool is_row_oriented, bool is_symmetric, restriction_kind restriction = full>
class traits_base : public it_traits<E, is_row_oriented, is_symmetric> {
public:
   typedef it_traits<E, is_row_oriented, is_symmetric> traits_for_iterator;
   typedef typename traits_for_iterator::Node Node;
protected:
   mutable AVL::Ptr<Node> root_links[3];

public:
   static constexpr bool
      symmetric = is_symmetric,
      row_oriented = is_row_oriented,
      allow_multiple = false,
      cross_oriented = restriction!=only_rows && restriction != only_cols ? !symmetric && !row_oriented : row_oriented,
      fixed_dim = is_symmetric || restriction == (is_row_oriented ? only_cols : only_rows);

   typedef E mapped_type;
   typedef AVL::tree< traits<traits_base, symmetric, restriction> > own_tree;
   typedef AVL::tree< traits<traits_base<E,cross_oriented,symmetric,restriction>, symmetric, restriction> > cross_tree;
   typedef ruler<own_tree, typename std::conditional<symmetric, nothing, ruler_prefix>::type> own_ruler;
   typedef ruler<cross_tree, typename std::conditional<symmetric, nothing, ruler_prefix>::type> cross_ruler;
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
      return *get_cross_ruler(std::true_type()).prefix().template get_cross_ruler<cross_tree>();
   }
   cross_ruler& get_cross_ruler(std::false_type)
   {
      return *get_cross_ruler(std::true_type()).prefix().template get_cross_ruler<cross_tree>();
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
   Int visit_by_copy(Node* n) const
   {
      if (symmetric)
         return 2 * this->line_index - n->key;
      else
         return row_oriented ? -1 : 1;
   }

   void notify_add(Node*) {}
   void notify_remove(Node*) {}

public:
   typedef Int arg_type;
   traits_base(Int index_arg) : traits_for_iterator(index_arg) {}

   const cross_tree& get_cross_tree(Int i) const
   {
      return get_cross_ruler()[i];
   }
   cross_tree& get_cross_tree(Int i)
   {
      return get_cross_ruler()[i];
   }
   
   template <typename, bool, restriction_kind> friend class Table;
};

template <typename Base, bool symmetric, restriction_kind restriction>
class traits : public Base {
public:
   using key_type = Int;
   using key_comparator_type = operations::cmp ;
   using typename Base::Node;
protected:
   key_comparator_type key_comparator;
   allocator node_allocator;

   Int key(const Node& n) const { return n.key - this->get_line_index(); }

   static typename Base::mapped_type& data(Node& n) { return n.data; }

   void insert_node_cross(Node *n, Int i, std::true_type)
   {
      this->get_cross_tree(i).insert_node(n);
   }
   void insert_node_cross(Node *, Int i, std::false_type)
   {
      Int& max_cross = this->get_cross_ruler(std::true_type()).prefix().cross_dim;
      if (i >= max_cross) max_cross = i+1;
   }

   Node* create_node(Int i)
   {
      Node* n = node_allocator.construct<Node>(i+this->get_line_index());
      insert_node_cross(n, i, bool_constant<restriction==full>());
      this->notify_add(n);
      return n;
   }

   template <typename Data>
   Node* create_node(Int i, Data&& data)
   {
      Node* n = node_allocator.construct<Node>(i+this->get_line_index(), std::forward<Data>(data));
      insert_node_cross(n, i, bool_constant<restriction==full>());
      this->notify_add(n);
      return n;
   }

   template <typename Data>
   Node* create_node(const pair<Int, Data>& p)
   {
      return create_node(p.first, p.second);
   }

   Node* create_node(const pair<Int, nothing>& p)
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
      const Int visit = this->visit_by_copy(n);
      Node* clone = visit <= 0 ? node_allocator.construct<Node>(*n) : n->links[1];
      if (visit < 0) {
         clone->links[1] = n->links[1];
         n->links[1] = clone;
      } else if (visit > 0) {
         n->links[1] = clone->links[1];
      }
      return clone;
   }

   static bool own_node(Node*)
   {
      return restriction != dying || Base::row_oriented;
   }

   Int max_size_impl(std::true_type) const
   {
      return this->get_cross_ruler().size();
   }
   Int max_size_impl(std::false_type) const
   {
      return this->get_cross_ruler(std::true_type()).prefix().cross_dim;
   }

public:
   template <typename... Args,
             typename = std::enable_if_t<std::is_constructible<Base, Args...>::value>>
   explicit traits(Args&&... init_args)
      : Base(std::forward<Args>(init_args)...) {}

   Node* create_free_node(Int i)
   {
      return node_allocator.construct<Node>(i);
   }

   template <typename Data>
   Node* create_free_node(Int i, Data&& data)
   {
      return node_allocator.construct<Node>(i, std::forward<Data>(data));
   }

   void destroy_node(Node* n)
   {
      remove_node_cross(n, bool_constant<restriction==full>());
      if (restriction != dying) this->notify_remove(n);
      node_allocator.destroy(n);
   }

   Int max_size() const
   {
      return max_size_impl(bool_constant<(restriction!=only_rows && restriction!=only_cols)>());
   }
   const key_comparator_type& get_comparator() const
   {
      return key_comparator;
   }

   template <typename, bool, restriction_kind> friend class Table;
   template <typename> friend struct sym_permute_entries;
   template <typename, typename, bool> friend struct asym_permute_entries;
};

template <typename Base, restriction_kind restriction>
class traits<Base, true, restriction>
   : public traits<Base, false, restriction> {
   using base_t = traits<Base, false, restriction>;
public:
   using typename base_t::Node;

protected:
   Node* insert_node(Node *n, Int i)
   {
      if (i != this->get_line_index())
         this->get_cross_tree(i).insert_node(n);
      this->notify_add(n);
      return n;
   }

   Node* create_node(Int i)
   {
      return insert_node(this->node_allocator.template construct<Node>(i+this->get_line_index()), i);
   }

   template <typename Data>
   Node* create_node(Int i, const Data& data)
   {
      return insert_node(this->node_allocator.template construct<Node>(i+this->get_line_index(), data), i);
   }

   template <typename Data>
   Node* create_node(const pair<Int, Data>& p)
   {
      return create_node(p.first, p.second);
   }

   Node* create_node(const pair<Int, nothing>& p)
   {
      return create_node(p.first);
   }

   void remove_node_cross(Node* n, std::true_type)
   {
      const Int l = this->get_line_index();
      const Int i = n->key - l;
      if (i != l) this->get_cross_tree(i).remove_node(n);
   }
   void remove_node_cross(Node*, std::false_type) {}

   bool own_node(Node *n) const
   {
      return restriction==full || n->key >= 2*this->get_line_index();
   }
public:
   template <typename... Args,
             typename=std::enable_if_t<std::is_constructible<base_t, Args...>::value>>
   explicit traits(Args&&... init_args)
      : base_t(std::forward<Args>(init_args)...) {}

   void destroy_node(Node* n)
   {
      remove_node_cross(n, bool_constant<restriction==full>());
      if (restriction != dying) this->notify_remove(n);
      this->node_allocator.destroy(n);
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
   typedef Int result_type;

   Int operator() (argument_type it) const
   {
      return it->key - it.get_line_index();
   }
};

template <typename row_ruler, typename col_ruler, bool restricted>
struct asym_permute_entries {
   col_ruler* C;

   typedef typename row_ruler::value_type tree_type;
   typedef typename tree_type::Node Node;

   static void relocate(tree_type *from, tree_type *to)
   {
      new(to) tree_type(std::move(*from));
   }

   void operator()(row_ruler*, row_ruler* R) const
   {
      if (!restricted) {
         for (auto ci = entire(*C);  !ci.at_end();  ++ci)
            ci->init();
         R->prefix().set_cross_ruler(C);
         C->prefix().set_cross_ruler(R);
      }
      Int r = 0;
      for (auto ri = entire(*R); !ri.at_end();  ++ri, ++r) {
         const Int old_r = ri->line_index;
         const Int rdiff = r - old_r;
         ri->line_index = r;
         for (auto e = ri->begin(); !e.at_end(); ++e) {
            Node* node = e.operator->();
            const Int c = node->key - old_r;
            node->key += rdiff;
            if (!restricted)
               (*C)[c].push_back_node(node);
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

template <typename Iterator>
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

   void copy_impl(const Table& t, Int add_r = 0, Int add_c = 0)
   {
      R = restriction != only_cols ? row_ruler::construct(*t.R, add_r) : nullptr;
      C = restriction != only_rows ? col_ruler::construct(*t.C, add_c) : nullptr;
      if (!restricted) {
         R->prefix().set_cross_ruler(C);
         C->prefix().set_cross_ruler(R);
      } else if (restriction == only_rows) {
         R->prefix().cross_dim = t.R->prefix().cross_dim;
      } else if (restriction == only_cols) {
         C->prefix().cross_dim = t.C->prefix().cross_dim;
      }
   }
public:
   Table()
      : R(row_ruler::construct(0))
      , C(col_ruler::construct(0))
   {
      R->prefix().set_cross_ruler(C);
      C->prefix().set_cross_ruler(R);
   }

   explicit Table(Int n)
      : R(restriction != only_cols ? row_ruler::construct(n) : nullptr)
      , C(restriction != only_rows ? col_ruler::construct(n) : nullptr)
   {
      if (!restricted)
         throw std::runtime_error("sparse2d::Table - both dimensions required in unrestricted mode");
      if (restriction == only_rows)
         R->prefix().cross_dim = 0;
      else
         C->prefix().cross_dim = 0;
   }

   Table(Int r, Int c)
      : R(restriction != only_cols ? row_ruler::construct(r) : nullptr)
      , C(restriction != only_rows ? col_ruler::construct(c) : nullptr)
   {
      if (restriction == only_rows) {
         R->prefix().cross_dim = c;
      } else if (restriction == only_cols) {
         C->prefix().cross_dim = r;
      } else {
         R->prefix().set_cross_ruler(C);
         C->prefix().set_cross_ruler(R);
      }
   }

   Table(const Table& t, Int add_r = 0, Int add_c = 0)
   {
      copy_impl(t, add_r, add_c);
   }
private:
   template <typename TRow_ruler, typename TCol_ruler>
   static TCol_ruler* take_over(TRow_ruler* R, TCol_ruler* C, std::false_type)
   {
      static_assert(restriction == full, "sparse2d::Table - moving between two restricted modes");
      C = TCol_ruler::construct(R->prefix().cross_dim);
      for (auto t = entire(*R);  !t.at_end();  ++t)
         for (auto e = t->begin(); !e.at_end(); ++e)
            (*C)[e->key - t->get_line_index()].push_back_node(e.operator->());
      R->prefix().set_cross_ruler(C);
      C->prefix().set_cross_ruler(R);
      return C;
   }

   template <typename TRow_ruler, typename TCol_ruler>
   static TCol_ruler* take_over(TRow_ruler* R, TCol_ruler* C, std::true_type) {}

public:
   Table(Table<E, symmetric, only_rows>&& t)
   {
      R = reinterpret_cast<row_ruler*>(t.R); t.R = nullptr;
      C = take_over(R, C, bool_constant<restriction==only_rows>());
   }

   Table(Table<E, symmetric, only_cols>&& t)
   {
      C = reinterpret_cast<col_ruler*>(t.C); t.C = nullptr;
      R = take_over(C, R, bool_constant<restriction==only_cols>());
   }

   ~Table()
   {
      if (restriction == full) {
         destroy_at(reinterpret_cast<restricted_Table*>(this));
      } else {
         if (restriction == only_cols && C || restriction == dying) col_ruler::destroy(C);
         if (restriction == only_rows && R || restriction == dying) row_ruler::destroy(R);
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

   Int rows() const
   {
      return restriction != only_cols ? R->size() : C->prefix().cross_dim;
   }
   Int cols() const
   {
      return restriction != only_rows ? C->size() : R->prefix().cross_dim;
   }

   void clear(Int r = 0, Int c = 0)
   {
      if (restriction == full) {
         reinterpret_cast<restricted_Table*>(this)->clear(r,c);
      } else if (restriction == only_rows) {
         R = row_ruler::resize_and_clear(R, r);
         R->prefix().cross_dim = 0;
      } else if (restriction == only_cols) {
         C = col_ruler::resize_and_clear(C, r);    // not a typo: the second argument must be omitted in this case
         C->prefix().cross_dim = 0;
      } else {
         R = row_ruler::resize_and_clear(R, r);
         C = col_ruler::resize_and_clear(C, c);
         R->prefix().set_cross_ruler(C);
         C->prefix().set_cross_ruler(R);
      }
   }

   struct shared_clear {
      Int r, c;
      shared_clear(Int r_arg, Int c_arg) : r(r_arg), c(c_arg) {}

      void operator() (void *p, const Table&) const { new(p) Table(r,c); }
      void operator() (Table& t) const { t.clear(r,c); }
   };

   void resize_rows(Int r)
   {
      if (restriction == only_rows && !R) {
         R = row_ruler::construct(r);
         R->prefix().cross_dim = 0;
      } else if (restriction == only_cols) {
         C->prefix().cross_dim = r;
      } else {
         R = row_ruler::resize(R, r);
      }
      if (!restricted) {
         R->prefix().set_cross_ruler(C);
         C->prefix().set_cross_ruler(R);
      }
   }

   void resize_cols(Int c)
   {
      if (restriction == only_cols && !C) {
         C = col_ruler::construct(c);
         C->prefix().cross_dim = 0;
      } else if (restriction == only_rows) {
         R->prefix().cross_dim = c;
      } else {
         C=col_ruler::resize(C, c);
      }
      if (!restricted) {
         R->prefix().set_cross_ruler(C);
         C->prefix().set_cross_ruler(R);
      }
   }

   struct shared_add_rows {
      Int n;
      shared_add_rows(Int n_arg) : n(n_arg) {}

      void operator() (void *p, const Table& t) const { new(p) Table(t, n, 0); }
      void operator() (Table& t) const { t.resize_rows(t.rows() + n); }
   };

   struct shared_add_cols {
      Int n;
      shared_add_cols(Int n_arg) : n(n_arg) {}

      void operator() (void *p, const Table& t) const { new(p) Table(t, 0, n); }
      void operator() (Table& t) const { t.resize_cols(t.cols() + n); }
   };

   void resize(Int r, Int c)
   {
      if (!restricted) {
         R = row_ruler::resize(R,r);
         C = col_ruler::resize(C,c);
         R->prefix().set_cross_ruler(C);
         C->prefix().set_cross_ruler(R);
      } else if (restricted) {
         throw std::runtime_error("sparse2d::Table::resize - exactly one non-zero dimension allowed in restricted mode");
      }
   }

protected:
   template <typename TRuler, typename number_consumer>
   static void squeeze_impl(TRuler* &R, const number_consumer& nc)
   {
      Int i = 0, inew = 0;
      for (auto t = R->begin(), end = R->end(); t != end; ++t, ++i) {
         using tree_type = pure_type_t<decltype(*t)>;
         if (t->size() != 0) {
            if (Int idiff = i - inew) {
               t->line_index = inew;
               for (auto e = entire(*t); !e.at_end(); ++e)
                  e->key -= idiff;
               new(&t[-idiff]) tree_type(std::move(*t));
            }
            nc(i, inew);  ++inew;
         } else {
            destroy_at(t.operator->());
         }
      }
      if (inew < i)
         R = TRuler::resize(R, inew, false);
   }
public:
   /** Remove the empty rows and columns.
       The remaining rows and columns are renumbered without gaps.
   */
   template <typename row_number_consumer, typename col_number_consumer>
   void squeeze(const row_number_consumer& rnc, const col_number_consumer& cnc)
   {
      if (restriction != only_cols)
         squeeze_impl(R, rnc);
      if (restriction != only_rows)
         squeeze_impl(C, cnc);
      if (!restricted) {
         R->prefix().set_cross_ruler(C);
         C->prefix().set_cross_ruler(R);
      }
   }

   template <typename row_number_consumer>
   void squeeze(const row_number_consumer& rnc) { squeeze(rnc, operations::binary_noop()); }

   void squeeze() { squeeze(operations::binary_noop(), operations::binary_noop()); }

   template <typename number_consumer>
   void squeeze_rows(const number_consumer& nc)
   {
      if (restriction==only_cols)
         throw std::runtime_error("squeeze_rows not allowed in restricted-to-columns mode");
      squeeze_impl(R, nc);
      if (!restricted) {
         R->prefix().set_cross_ruler(C);
         C->prefix().set_cross_ruler(R);
      }
   }

   void squeeze_rows() { squeeze_rows(operations::binary_noop()); }

   template <typename number_consumer>
   void squeeze_cols(const number_consumer& nc)
   {
      if (restriction==only_rows)
         throw std::runtime_error("squeeze_rows not allowed in restricted-to-rows mode");
      squeeze_impl(C, nc);
      if (!restricted) {
         R->prefix().set_cross_ruler(C);
         C->prefix().set_cross_ruler(R);
      }
   }

   void squeeze_cols() { squeeze_cols(operations::binary_noop()); }

   template <typename TPerm, typename TInverse>
   void permute_rows(const TPerm& perm, TInverse)
   {
      if (restriction == only_cols)
         throw std::runtime_error("sparse2d::Table::permute_rows - disabled in restricted mode");
      R = row_ruler::permute(R, perm, asym_permute_entries<row_ruler, col_ruler, restriction == only_rows>(C), TInverse());
   }

   template <typename TPerm, typename TInverse>
   void permute_cols(const TPerm& perm, TInverse)
   {
      if (restriction == only_rows)
         throw std::runtime_error("sparse2d::Table::permute_cols - disabled in restricted mode");
      C = col_ruler::permute(C, perm, asym_permute_entries<col_ruler, row_ruler, restriction == only_cols>(R), TInverse());
   }

   row_tree_type& row(Int i)             { return (*R)[i]; }
   const row_tree_type& row(Int i) const { return (*R)[i]; }
   col_tree_type& col(Int i)             { return (*C)[i]; }
   const col_tree_type& col(Int i) const { return (*C)[i]; }

   line<row_tree_type>& get_line(Int i, row_tree_type*) { return reinterpret_cast<line<row_tree_type>&>(row(i)); }
   const line<row_tree_type>& get_line(Int i, row_tree_type*) const { return reinterpret_cast<const line<row_tree_type>&>(row(i)); }
   line<col_tree_type>& get_line(Int i, col_tree_type*) { return reinterpret_cast<line<col_tree_type>&>(col(i)); }
   const line<col_tree_type>& get_line(Int i, col_tree_type*) const { return reinterpret_cast<const line<col_tree_type>&>(col(i)); }

#if POLYMAKE_DEBUG
   void check() const
   {
      if (restriction != only_cols)
         for (Int r = 0, rend = this->rows(); r < rend; ++r) {
            std::ostringstream label;
            label << "row " << r << ": ";
            (*R)[r].check(label.str().c_str());
         }
      if (restriction != only_cols)
         for (Int c = 0, cend = this->cols(); c < cend; ++c) {
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
struct sym_permute_entries
   : public Traits {

   using typename Traits::tree_t;
   using typename Traits::ruler;
   using typename Traits::entry_t;
   using Node = typename tree_t::Node;

   static void relocate(tree_t* from, tree_t* to)
   {
      new tree_t(*from, AVL::copy_without_nodes());
   }

   void complete_cross_links(ruler* R)
   {
      Int r = 0;
      for (entry_t& entry : *R) {
         for (auto e = this->tree(entry).begin(); !e.at_end(); ++e) {
            Node* node = e.operator->();
            const Int c = node->key-r;
            if (c != r)
               this->tree((*R)[c]).push_back_node(node);
         }
         ++r;
      }
   }

   void operator()(ruler* Rold, ruler* R)
   {
      const Int n = R->size();
      inv_perm_store.resize(n, -1);

      Int r = 0;
      for (entry_t& entry : *R) {
         if (this->is_alive(entry)) {
            tree_t& t = this->tree(entry);
            inv_perm_store[t.line_index] = r;
            t.line_index = r;
         } else {
            this->handle_dead_entry(entry, r);
         }
         ++r;
      }

      for (Int old_r = 0; old_r < n; ++old_r) {
         r = inv_perm_store[old_r];
         if (r >= 0) {
            auto& old_tree = Traits::tree((*Rold)[old_r]);
            for (auto e = old_tree.begin(); !e.at_end(); ) {
               Node* node = e.operator->();  ++e;
               const Int old_c = node->key - old_r;
               // don't move nodes above the main diagonal,
               // otherwise subsequent trees will look corrupted
               if (old_c <= old_r) {
                  const Int c = inv_perm_store[old_c];
                  node->key = r+c;
                  Traits::tree((*R)[std::max(r,c)]).insert_node(node);
               } else {
                  break;
               }
            }
            old_tree.init();
         }
      }

      complete_cross_links(R);
      this->finalize_dead_entries();
   }

   template <typename Perm, typename InvPerm>
   void copy(const ruler* R_src, ruler* R_dst, const Perm& perm, const InvPerm& inv_perm)
   {
      const Int n = R_dst->size();
      auto p_it = perm.begin();
      for (Int dst_r = 0; dst_r < n; ++dst_r, ++p_it) {
         const Int src_r = *p_it;
         const entry_t& src_entry = (*R_src)[src_r];
         if (this->is_alive(src_entry)) {
            for (auto e = this->tree(src_entry).begin(); !e.at_end(); ++e) {
               const Node* node = e.operator->();
               const Int src_c = node->key - src_r;
               const Int dst_c = inv_perm[src_c];
               if (dst_c >= dst_r) {
                  tree_t& t = this->tree((*R_dst)[dst_c]);
                  t.insert_node(t.create_free_node(dst_r+dst_c, node->get_data()));
               }
            }
         } else {
            this->handle_dead_entry((*R_dst)[dst_r], dst_r);
         }
      }

      complete_cross_links(R_dst);
      this->finalize_dead_entries();
   }

   std::vector<Int> inv_perm_store;
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

   explicit Table(Int r, Int = 0)
      : R(row_ruler::construct(r)) {}

   Table(const Table& t, Int add_r = 0)
      : R(row_ruler::construct(*t.R, add_r)) {}

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

   Int rows() const { return R->size(); }
   Int cols() const { return this->rows(); }

   void clear(Int r = 0) { R = row_ruler::resize_and_clear(R, r); }

   void resize_rows(Int r) { R = row_ruler::resize(R, r); }
   void resize_cols(Int r) { resize_rows(r); }

   struct shared_clear {
      Int r;
      shared_clear(Int r_arg, Int = 0) : r(r_arg) {}

      void operator() (void *p, const Table&) const { new(p) Table(r); }
      void operator() (Table& t) const { t.clear(r); }
   };

   struct shared_add_rows {
      Int n;
      shared_add_rows(Int n_arg) : n(n_arg) {}

      void operator() (void *p, const Table& t) const { new(p) Table(t, n); }
      void operator() (Table& t) const { t.resize_rows(t.rows() + n); }
   };

   void resize() { resize_rows(this->rows()); }
   void resize(Int r, Int = 0) { resize_rows(r); }

   template <typename row_number_consumer>
   void squeeze(const row_number_consumer& rnc)
   {
      Int r = 0, rnew = 0;
      for (auto t = R->begin(), end = R->end(); t != end; ++t, ++r) {
         if (t->size() != 0) {
            if (Int rdiff = r-rnew) {
               const Int diag = 2 * r;
               for (auto e = entire(*t); !e.at_end(); ) {
                  cell<E>& c = *e; ++e;
                  c.key -= rdiff << (c.key == diag);
               }
               t->line_index = rnew;
               new(&t[-rdiff]) row_tree_type(std::move(*t));
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
      typedef row_tree_type tree_t;
      typedef tree_t entry_t;
      static tree_t& tree(tree_t& t) { return t; }
      static const tree_t& tree(const tree_t& t) { return t; }
      static bool is_alive (const tree_t& t) { return true; }
      static void handle_dead_entry(tree_t&, Int) {}
      static void finalize_dead_entries() {}
   };
   typedef sym_permute_entries<perm_traits> permute_entries;

   template <typename TPerm, typename _inverse>
   void permute_rows(const TPerm& perm, _inverse)
   {
      R=row_ruler::permute(R, perm, permute_entries(), _inverse());
   }

   template <typename TPerm, typename _inverse>
   void permute_cols(const TPerm& perm, _inverse)
   {
      permute_rows(perm, _inverse());
   }

   template <typename TPerm, typename TInvPerm>
   void copy_permuted(const Table& src, const TPerm& perm, const TInvPerm& inv_perm)
   {
      permute_entries::copy(src.R, R, perm, inv_perm);
   }

   row_tree_type&       row(Int i)       { return (*R)[i]; }
   const row_tree_type& row(Int i) const { return (*R)[i]; }
   col_tree_type&       col(Int i)       { return row(i); }
   const col_tree_type& col(Int i) const { return row(i); }

   line<row_tree_type>& get_line(Int i, row_tree_type*) { return reinterpret_cast<line<row_tree_type>&>(row(i)); }
   const line<row_tree_type>& get_line(Int i, row_tree_type*) const { return reinterpret_cast<const line<row_tree_type>&>(row(i)); }

#if POLYMAKE_DEBUG
   void check() const
   {
      for (Int r = 0, rend = this->rows(); r < rend; ++r) {
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
   Int get_line_index() const { return this->hidden().get_line_index(); }
   Int dim() const { return this->max_size(); }
};

template <typename TreeRef=void>
struct line_index_accessor {
   typedef TreeRef argument_type;
   typedef TreeRef first_argument_type;
   typedef void second_argument_type;
   typedef Int result_type;

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
   void resize(Int n)
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
   void resize(Int n)
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

template <typename Line>
lower_triangle_line<Line>& select_lower_triangle(Line& l)
{
   return reinterpret_cast<lower_triangle_line<Line>&>(l);
}

template <typename Line>
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
   template <typename E, bool symmetric, pm::sparse2d::restriction_kind restriction>
   void swap(pm::sparse2d::Table<E,symmetric,restriction>& t1, pm::sparse2d::Table<E,symmetric,restriction>& t2)
   { t1.swap(t2); }
} // end namespace std

namespace polymake {
   using pm::sparse2d::only_rows;
   using pm::sparse2d::only_cols;
   using pm::sparse2d::rowwise;
   using pm::sparse2d::columnwise;
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
