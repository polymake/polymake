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

/** @file FacetList.h
    @brief Implementation of pm::FacetList and ... classes
 */

#ifndef POLYMAKE_FACET_LIST_H
#define POLYMAKE_FACET_LIST_H

#include "polymake/internal/sparse2d_ruler.h"
#include "polymake/internal/chunk_allocator.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/EmbeddedList.h"
#include "polymake/vector"
#include "polymake/list"
#include <cassert>

namespace pm {

class FacetList;
template <> class Cols<FacetList>;

namespace fl_internal {
   class facet;
   class vertex_list;
   class lex_ordered_vertex_list;
   class superset_iterator;
   template <typename Iterator, bool check_range = true> class subset_iterator;
   class Facet; class Table;
   typedef Int facet_id_t;
}

template <>
struct spec_object_traits<fl_internal::facet>
   : spec_object_traits<is_container> {
   static const bool is_always_const=true;
   static const IO_separator_kind IO_separator=IO_sep_inherit;
};
template <>
struct spec_object_traits<fl_internal::Facet>
   : spec_object_traits<is_container> {
   static const bool is_always_const=true;
   typedef fl_internal::facet masquerade_for;
};

template <> struct spec_object_traits<fl_internal::vertex_list>
   : spec_object_traits<is_container> {};
template <> struct spec_object_traits<fl_internal::lex_ordered_vertex_list>
   : spec_object_traits<is_container> {};

namespace fl_internal {

// element of a facet
struct cell {
   cell* head_cell;  // apparent head cell contained in the facet structure

   // links to neighbor cells in the same facet; =head_cell for the lowest/highest vertex
   ptr_pair<cell> facet;   // preceding vertex;  

   // links to other facets containing the same vertex:
   // prev = facet added after this one - new facets are always pushed at the front of the column list
   // next = facet added before this one
   ptr_pair<cell> col;

   // links to neighbor siblings in the prefix tree: facets containing same vertices up to here and differing in the next vertex
   ptr_pair<cell> lex;

   Int vertex;       // vertex number = element of the facet

   cell(cell* head_arg, Int vertex_arg)
      : head_cell(head_arg)
      , vertex(vertex_arg) {}
};

// iterator along a list of cells
template <ptr_pair<cell> cell::* links, bool _rev=false>
class cell_iterator
   : public embedded_list_iterator<cell, links, true, _rev> {
public:
   typedef cell_iterator iterator;
   typedef cell_iterator const_iterator;

   typedef embedded_list_iterator<cell, links, true, _rev> super;

   cell_iterator() {}

   cell_iterator(const cell* cur_arg, const cell* head_arg)
      : super(cur_arg)
      , head(head_arg)
   {
      assert(links == &cell::facet || !head_arg);
   }

   explicit cell_iterator(const cell* cur_arg)
      : super(cur_arg)
      , head(links == &cell::facet ? cur_arg->head_cell : nullptr) {}

   iterator& operator++ ()
   {
      super::operator++();
      return *this;
   }
   iterator& operator-- ()
   {
      super::operator--();
      return *this;
   }
   const iterator operator++ (int) { iterator copy(*this); operator++(); return copy; }
   const iterator operator-- (int) { iterator copy(*this); operator--(); return copy; }

   bool at_end() const { return this->cur == head; }

   Int index() const
   {
      return links == &cell::facet ? this->cur->vertex : get_facet()->get_id();
   }

   const facet* get_facet() const;

protected:
   const cell* head;   // apparent head cell of the facet or nullptr for vertical traversals

   // move to another facet following the given link
   // @return false if no cell exists in the given direction
   bool down(ptr_pair<cell> cell::* vertical_links)
   {
      assert(links == &cell::facet && links != vertical_links);
      const cell* next = (this->cur->*vertical_links).next;
      if (!next) return false;
      this->cur = next;
      head = this->cur->head_cell;
      return true;
   }

   template <ptr_pair<cell> cell::*, bool> friend class cell_iterator;
   friend class facet;
   friend class Table;
};

class facet {
public:
   explicit facet(facet_id_t id_arg=facet_id_t(-1))
      : cells(&cell::facet)
      , size_(0)
      , id(id_arg) {}

   facet(const facet& l, chunk_allocator& al);

   void unlink_cells(chunk_allocator& al);
   cell* push_back(Int vertex, chunk_allocator& al);

   typedef cell_iterator<&cell::facet, false> iterator;
   typedef iterator const_iterator;
   typedef cell_iterator<&cell::facet, true> reverse_iterator;
   typedef reverse_iterator const_reverse_iterator;
   typedef cell value_type;
   typedef const cell& reference;
   typedef reference const_reference;

   iterator begin() const
   {
      return iterator(cells.next, head_cell());
   }
   iterator end() const
   {
      return iterator(head_cell(), head_cell());
   }
   reverse_iterator rbegin() const
   {
      return reverse_iterator(cells.prev, head_cell());
   }
   reverse_iterator rend() const
   {
      return reverse_iterator(head_cell(), head_cell());
   }

   Int size() const { return size_; }
   bool empty() const { return size_ == 0; }

protected:
   ptr_pair<facet> list_ptrs;  // embedded list of all facets in FacetList
   ptr_pair<cell> cells;       // vertices comprising this facet
   Int size_;
   facet_id_t id;

   const cell* head_cell() const
   {
      return reverse_cast(&cells, &cell::facet);
   }
   cell* head_cell()
   {
      return reverse_cast(&cells, &cell::facet);
   }

public:
   static const facet* from_head_cell(const cell* head)
   {
      return reverse_cast(&(head->facet), &facet::cells);
   }

   static const facet* from_cell(const cell* c)
   {
      return from_head_cell(c->head_cell);
   }

   facet_id_t get_id() const { return id; }

   struct id2index {
      typedef facet argument_type;
      typedef Int result_type;
      result_type operator() (const facet& f) const { return f.id; }
   };

   friend class Table;
   friend class superset_iterator;
   friend class fl_allocator;
private:
   // deleted
   facet(const facet&);
   void operator= (const facet&);
};


template <ptr_pair<cell> cell::* links, bool _rev>
const facet* cell_iterator<links, _rev>::get_facet() const
{
   return links == &cell::facet ? facet::from_head_cell(head) : facet::from_cell(this->cur);
}


class Facet
   : public modified_container_impl< Facet,
                                     mlist< HiddenTag<facet>,
                                            OperationTag< BuildUnaryIt<operations::index2element> > > >,
   public GenericSet<Facet, Int, operations::cmp> {
public:
   operations::cmp get_comparator() const { return operations::cmp(); }
   facet_id_t get_id() const { return this->hidden().get_id(); }
protected:
   Facet();
   ~Facet();
};

// Iterator over facets containing a given vertex.
// The facets are visited in reverse chronological order.
class col_order_iterator
   : public cell_iterator<&cell::col> {
   typedef cell_iterator<&cell::col> super;
public:
   typedef forward_iterator_tag iterator_category;
   typedef Facet value_type;
   typedef const Facet& reference;
   typedef const Facet* pointer;
   typedef col_order_iterator iterator;
   typedef iterator const_iterator;

   col_order_iterator(const cell* cur_arg = nullptr)
      : super(cur_arg, nullptr) {}

   reference operator* () const
   {
      return reinterpret_cast<reference>(*get_facet());
   }
   pointer operator-> () const { return &(operator*()); }

   iterator& operator++ () { super::operator++(); return *this; }
   const iterator operator++ (int) { iterator copy(*this); super::operator++(); return copy; }
};

//! Iterator over facets containing a given vertex as the lowest one.
//! Facets are visited in lexicographical order.
class lex_order_iterator {
protected:
   typedef cell_iterator<&cell::lex> cit;
   typedef std::list<cit> it_list;
   it_list Q;

public:
   typedef forward_iterator_tag iterator_category;
   typedef Facet value_type;
   typedef const Facet& reference;
   typedef const Facet* pointer;
   typedef ptrdiff_t difference_type;
   typedef lex_order_iterator iterator;
   typedef lex_order_iterator const_iterator;

   lex_order_iterator(const cell* cur_arg = nullptr);

   reference operator* () const
   {
      return reinterpret_cast<reference>(*Q.back().get_facet());
   }
   pointer operator-> () const { return &(operator*()); }

   iterator& operator++ ();
   const iterator operator++ (int) { iterator copy(*this); operator++(); return copy; }

   bool at_end() const { return Q.empty(); }

   bool operator== (const iterator& it) const { return Q==it.Q; }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   Int index() const
   {
      return Q.back().index();
   }
private:
   void scan_facet(const cell* cur);
};


// Heads of lists of cells pertaining to a certain vertex
class vertex_list {
public:
   explicit vertex_list(Int vertex_arg)
      : vertex(vertex_arg)
      , first_col(nullptr)
      , first_lex(nullptr) {}

   vertex_list(const vertex_list& l);
private:
   // deleted
   void operator=(const vertex_list&);

public:
   typedef col_order_iterator iterator;
   typedef iterator const_iterator;

   typedef Facet value_type;
   typedef const Facet& reference;
   typedef reference const_reference;

   iterator begin() const
   {
      return iterator(first_col);
   }
   iterator end() const
   {
      return iterator();
   }
   reference front() const
   {
      return reinterpret_cast<reference>(*facet::from_cell(first_col));
   }
   Int size() const
   {
      return count_it(begin());
   }
   bool empty() const
   {
      return !first_col;
   }

   friend void relocate(vertex_list* from, vertex_list* to)
   {
      to->vertex = from->vertex;
      cell* first;
      if ((to->first_col = first=from->first_col) != nullptr)
         first->col.prev = to->col_head_cell();
      if ((to->first_lex = first=from->first_lex) != nullptr)
         first->lex.prev = to->lex_head_cell();
   }
protected:
   Int vertex;
   cell* first_col;
   cell* first_lex;

   cell* col_head_cell() const
   {
      return reverse_cast(reverse_cast(const_cast<cell**>(&first_col), &ptr_pair<cell>::next), &cell::col);
   }
   cell* lex_head_cell() const
   {
      return reverse_cast(reverse_cast(const_cast<cell**>(&first_lex), &ptr_pair<cell>::next), &cell::lex);
   }

   // helper class inserting cells of a new facet into all lists
   class inserter {
   private:
      // deleted
      inserter(const inserter&);
      void operator=(const inserter&);
   public:
      inserter()
         : first_old(nullptr)
         , last_old(nullptr)
         , first_new(nullptr)
         , last_new(nullptr) {}

      // @retval true if the fork point reached
      bool push(vertex_list& column, cell* newc);

      // the new facet ended up as a prefix of an existing one
      bool new_facet_ended();

   private:
      void finalize();

      // cells of an existing facet that equals the new one so far
      cell* first_old;   // target cell of the incoming arc of the prefix tree
      cell* last_old;    // highest vertex coinciding with the new facet
      // cells of the new facet
      cell* first_new;   // of the same vertex as first_old
      cell* last_new;    // of the same vertex as last_old, that is, the new cell consumed last
   };

   // insert the cell at front of the column list
   cell* push_front(cell* c)
   {
      if ((c->col.next=first_col) != nullptr)
         first_col->col.prev=c;
      c->col.prev=col_head_cell();
      first_col=c;
      return c;
   }

   friend class Table;
   template <typename,bool> friend class subset_iterator;
};

// masquerade for vertex_list, exposing the facets in lexicographical order
class lex_ordered_vertex_list
   : public vertex_list {
protected:
   lex_ordered_vertex_list();
   ~lex_ordered_vertex_list();
public:
   typedef lex_order_iterator iterator;
   typedef iterator const_iterator;
   typedef Facet value_type;
   typedef const Facet& reference;
   typedef reference const_reference;

   iterator begin() const
   {
      return iterator(first_lex);
   }
   iterator end() const
   {
      return iterator();
   }
   reference front() const
   {
      return reinterpret_cast<reference>(*facet::from_cell(first_lex));
   }
   Int size() const
   {
      return count_it(begin());
   }
   bool empty() const
   {
      return first_lex == nullptr;
   }
};

typedef sparse2d::ruler<vertex_list> col_ruler;

class superset_iterator {
public:
   typedef forward_iterator_tag iterator_category;
   typedef facet value_type;
   typedef const value_type& reference;
   typedef const value_type* pointer;
   typedef ptrdiff_t difference_type;
   typedef superset_iterator iterator;
   typedef iterator const_iterator;
protected:
   typedef cell_iterator<&cell::col> it_type;
   typedef std::list<it_type> it_list;

   it_list its;
   pointer cur;
   Int its_size;
   static const value_type empty_facet;
public:
   superset_iterator() {}

   template <typename TSet>
   superset_iterator(col_ruler::const_iterator columns, const GenericSet<TSet>& f, bool show_empty_facet)
   {
      its_size = iterator_traits<typename TSet::const_iterator>::is_bidirectional
                 ? f.top().size() : 0;
      for (auto v = entire(f.top());  !v.at_end();  ++v) {
         its.push_back(columns[*v].begin());
         if (!iterator_traits<typename TSet::const_iterator>::is_bidirectional)
            ++its_size;
      }
      if (its_size != 0)
         valid_position();
      else
         cur = show_empty_facet ? &empty_facet : nullptr;
   }

   reference operator* () const { return *cur; }
   pointer operator-> () const { return cur; }

   bool exact_match() const
   {
      return its_size == cur->size();
   }

   iterator& operator++ () { valid_position(); return *this; }
   const iterator operator++ (int) { iterator copy(*this);  operator++();  return copy; }

   bool at_end() const { return cur == nullptr; }

   bool operator== (const iterator& it) const { return cur == it.cur; }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   Int index() const { return cur->get_id(); }
private:
   void valid_position();

   template <typename,bool> friend class subset_iterator;
};

template <typename TSet, bool check_range>
class subset_iterator {
public:
   using iterator_category = forward_iterator_tag;
   using value_type = facet;
   using reference = const value_type&;
   using pointer = const value_type*;
   using difference_type = ptrdiff_t;
   using iterator = subset_iterator;
   using const_iterator = iterator;
protected:
   using set_iterator = typename ensure_features<TSet, end_sensitive>::const_iterator;
   using it_pair = std::pair<facet::iterator, set_iterator>;
   using it_list = std::list<it_pair>;

   col_ruler::const_iterator columns;
   Int n_columns;
   set_iterator start;
   it_list Q;
   pointer cur;

public:
   subset_iterator() {}

   subset_iterator(col_ruler::const_iterator columns_arg, const GenericSet<TSet>& f)
      : columns(columns_arg)
      , start(entire(f.top()))
   {
      assert(!check_range);
      valid_position();
   }

   subset_iterator(col_ruler::const_iterator columns_arg, Int n_columns_arg, const GenericSet<TSet>& f)
      : columns(columns_arg)
      , n_columns(n_columns_arg)
      , start(entire(f.top()))
   {
      valid_position();
   }

   reference operator* () const { return *cur; }
   pointer operator-> () const { return cur; }

   iterator& operator++ () { valid_position(); return *this; }
   const iterator operator++ (int) { iterator copy(*this);  operator++();  return copy; }

   bool at_end() const { return !cur; }

   bool operator== (const iterator& it) const { return cur==it.cur; }
   bool operator!= (const iterator& it) const { return cur!=it.cur; }

   bool operator== (const superset_iterator& it) const { return cur==it.cur; }
   bool operator!= (const superset_iterator& it) const { return cur!=it.cur; }

   Int index() const { return cur->get_id(); }
private:
   void valid_position();
};

template <typename TSet, bool check_range>
void subset_iterator<TSet, check_range>::valid_position()
{
   for (;;) {
      while (!Q.empty()) {
         it_pair itp = Q.back();  Q.pop_back();
         bool match;
         do {
            if (itp.first->lex.next != nullptr) {
               Q.push_back(it_pair(facet::iterator(itp.first->lex.next), itp.second));
            }
            if ((++itp.first).at_end()) {
               cur = itp.first.get_facet();
               return;
            }
            const Int vertex_of_facet = itp.first->vertex;
            while ((match = !(++itp.second).at_end())) {
               const Int vertex_of_query = *itp.second;
               if (vertex_of_query >= vertex_of_facet) {
                  match = vertex_of_query == vertex_of_facet;
                  break;
               }
            }
         } while (match);
      }

      for (;;) {
         if (start.at_end() || check_range && *start >= n_columns) {
            cur = nullptr;
            return;
         }
         cell* first_lex = columns[*start].first_lex;
         if (first_lex != nullptr) {
            Q.push_back(it_pair(facet::iterator(first_lex), start));
            ++start;
            break;
         }
         ++start;
      }
   }
}

} // end namespace fl_internal;

template <ptr_pair<fl_internal::cell> fl_internal::cell::* links, bool _rev>
struct check_iterator_feature<fl_internal::cell_iterator<links, _rev>, end_sensitive> : std::true_type {};
template <ptr_pair<fl_internal::cell> fl_internal::cell::* links, bool _rev>
struct check_iterator_feature<fl_internal::cell_iterator<links, _rev>, indexed> : std::true_type {};

template <> struct check_iterator_feature<fl_internal::col_order_iterator, end_sensitive> : std::true_type {};
template <> struct check_iterator_feature<fl_internal::col_order_iterator, indexed> : std::true_type {};

template <> struct check_iterator_feature<fl_internal::lex_order_iterator, end_sensitive> : std::true_type {};
template <> struct check_iterator_feature<fl_internal::lex_order_iterator, indexed> : std::true_type {};

template <> struct check_iterator_feature<fl_internal::superset_iterator, end_sensitive> : std::true_type {};
template <> struct check_iterator_feature<fl_internal::superset_iterator, indexed> : std::true_type {};

template <typename TSet, bool check_range>
struct check_iterator_feature<fl_internal::subset_iterator<TSet, check_range>, end_sensitive> : std::true_type {};
template <typename TSet, bool check_range>
struct check_iterator_feature<fl_internal::subset_iterator<TSet, check_range>, indexed> : std::true_type {};

namespace fl_internal {

class Table {
public:
   typedef facet value_type;
   typedef const value_type& reference;
   typedef reference const_reference;
   typedef embedded_list_iterator<facet, &facet::list_ptrs, true, false> iterator;
   typedef embedded_list_iterator<facet, &facet::list_ptrs, true, true> reverse_iterator;
   typedef iterator const_iterator;
   typedef reverse_iterator const_reverse_iterator;

   explicit Table(size_t facet_size = sizeof(facet), Int n_vertices = 0);

   Table(const Table& c);

private:
   void operator= (const Table&) = delete;

public:
   ~Table() { col_ruler::destroy(columns); }

   //! make the table look empty
   void clear();

   //! remove all facets, keep the allocated number of vertices
   void clear_facets();

   Int n_vertices() const { return columns->size(); }

   iterator begin() const { return iterator(facets.next); }
   iterator end() const { return iterator(end_facet()); }
   reverse_iterator rbegin() const { return reverse_iterator(facets.prev); }
   reverse_iterator rend() const { return reverse_iterator(end_facet()); }

   template <typename IndexConsumer>
   void squeeze(const IndexConsumer& ic)
   {
      Int new_vertex = 0;
      for (auto column_it = columns->begin(), end = columns->end();  column_it != end;  ++column_it) {
         if (cell* c = column_it->first_col) {
            const Int old_vertex = column_it->vertex;
            if (old_vertex != new_vertex) {
               do
                  c->vertex = new_vertex;
               while ((c = c->col.next) != nullptr);
               vertex_list* column_new = &column_it[new_vertex - old_vertex];
               relocate(column_it.operator->(), column_new);
               column_new->vertex = new_vertex;
            }
            ic(old_vertex, new_vertex);  ++new_vertex;
         }
      }
      if (new_vertex < n_vertices()) columns=col_ruler::resize(columns, new_vertex, false);

      if (next_id != size_) squeeze_ids();
   }

   template <typename Iterator>
   Table(size_t facet_size, Iterator src, std::false_type)
      : facet_alloc(facet_size)
      , cell_alloc(sizeof(cell))
      , facets(&facet::list_ptrs)
      , columns(col_ruler::construct(0))
      , size_(0)
      , next_id(0)
   {
      for (; !src.at_end(); ++src)
         insert(*src);
   }

   template <typename Iterator>
   Table(size_t facet_size, Int n_vertices, Iterator src, std::false_type)
      : facet_alloc(facet_size)
      , cell_alloc(sizeof(cell))
      , facets(&facet::list_ptrs)
      , columns(col_ruler::construct(n_vertices))
      , size_(0)
      , next_id(0)
   {
      for (; !src.at_end(); ++src)
         insert_from_it(entire_range(*src), new_id());
   }

   template <typename Iterator>
   Table(size_t facet_size, Iterator src, std::true_type)
      : facet_alloc(facet_size)
      , cell_alloc(sizeof(cell))
      , facets(&facet::list_ptrs)
      , columns(col_ruler::construct(0))
      , size_(0)
      , next_id(0)
   {
      for (; !src.at_end(); ++src)
         push_back(*src);
   }

   template <typename Iterator>
   Table(size_t facet_size, Int n_vertices, Iterator src, std::true_type)
      : facet_alloc(facet_size)
      , cell_alloc(sizeof(cell))
      , facets(&facet::list_ptrs)
      , columns(col_ruler::construct(n_vertices))
      , size_(0)
      , next_id(0)
   {
      for (; !src.at_end(); ++src)
         push_back_from_it(entire_range(*src));
   }

   const facet* end_facet() const
   {
      return reverse_cast(&facets, &facet::list_ptrs);
   }
   facet* end_facet()
   {
      return reverse_cast(&facets, &facet::list_ptrs);
   }

   class LexOrdered_helper
      : public modified_container_impl< LexOrdered_helper,
                                        mlist< ContainerTag< col_ruler >,
                                               OperationTag< operations::reinterpret<lex_ordered_vertex_list> >,
                                               HiddenTag< Table > > > {
   public:
      const col_ruler& get_container() const { return *hidden().columns; }
   };

protected:
   void push_back_new_facet(facet* f);
   void push_back_facet(facet* f);

   template <typename Iterator>
   void insert_cells(facet* f, Iterator&& src)
   {
      vertex_list::inserter inserter;
      Int v;
      do {
         if (src.at_end()) {
            if (inserter.new_facet_ended()) return;
            erase_facet(*f);
            throw std::runtime_error("attempt to insert a duplicate or empty facet into FacetList");
         }
         v = *src;  ++src;
         if (std::is_same<typename iterator_traits<Iterator>::iterator_category, input_iterator_tag>::value)
            extend_cols(v);
      } while (!inserter.push((*columns)[v], f->push_back(v, cell_alloc)));

      for (; !src.at_end(); ++src) {
         v = *src;
         if (std::is_same<typename iterator_traits<Iterator>::iterator_category, input_iterator_tag>::value)
            extend_cols(v);
         (*columns)[v].push_front(f->push_back(v, cell_alloc));
      }
   }

   template <typename Iterator>
   facet* insert_from_it(Iterator&& src, facet_id_t nid)
   {
      facet* f = new(facet_alloc.allocate()) facet(nid);
      push_back_facet(f);
      ++size_;
      insert_cells(f, std::move(src));
      return f;
   }

   template <typename Iterator>
   void push_back_from_it(Iterator&& src)
   {
      Int new_vertex = *src;
      facet* f = new(facet_alloc.allocate()) facet(new_id());
      cell* newc;
      cell* lex_prev;

      if ((*columns)[new_vertex].first_lex != nullptr) {
         assert(facets.prev != end_facet());
         facet::const_iterator prev_facet_it = facets.prev->begin();
         assert(new_vertex == prev_facet_it->vertex);
         push_back_facet(f);
         do {
            newc = (*columns)[new_vertex].push_front(f->push_back(new_vertex, cell_alloc));
            lex_prev = const_cast<cell*>(prev_facet_it.cur);
            ++src;  ++prev_facet_it;
            if (prev_facet_it.at_end()) break;
            assert(!src.at_end());
            new_vertex = *src;
            assert(new_vertex >= prev_facet_it->vertex);
         } while (new_vertex == prev_facet_it->vertex);
      } else {
         push_back_facet(f);
         lex_prev = (*columns)[new_vertex].lex_head_cell();
         newc = (*columns)[new_vertex].push_front(f->push_back(new_vertex, cell_alloc));
      }

      newc->lex.prev = lex_prev;
      lex_prev->lex.next = newc;

      while (!(++src).at_end()) {
         new_vertex = *src;
         (*columns)[new_vertex].push_front(f->push_back(new_vertex, cell_alloc));
      }
      ++size_;
   }

public:
   template <typename TSet>
   const facet* find_facet(const GenericSet<TSet>& f) const
   {
      auto v_it = entire(f.top());
      if (v_it.at_end()) return nullptr;
      Int v = *v_it;
      if (v >= n_vertices()) return nullptr;

      const cell* c = (*columns)[v].first_lex;
      if (!c) return nullptr;

      for (facet::iterator cur(c); ;) {
         ++v_it;  ++cur;
         if (cur.at_end()) {
            if (v_it.at_end())
               return cur.get_facet();
            else
               break;
         }
         if (v_it.at_end()) break;
         v = *v_it;
         while (cur->vertex != v) {
            if (cur->vertex > v) return nullptr;
            --cur;
            if (!cur.down(&cell::lex)) return nullptr;
            ++cur;
         }
      }
      return nullptr;
   }

   template <typename TSet>
   facet* insert(const GenericSet<TSet, Int, operations::cmp>& f)
   {
      extend_cols(f.top().back());
      return insert_from_it(entire(f.top()), new_id());
   }

   template <typename TSet>
   void push_back(const GenericSet<TSet, Int, operations::cmp>& f)
   {
      extend_cols(f.top().back());
      push_back_from_it(entire(f.top()));
   }

   template <typename TSet>
   Int erase(const GenericSet<TSet, Int, operations::cmp>& f)
   {
      const facet* fp = find_facet(f);
      return fp ? (erase_facet(*fp), 1) : 0;
   }

   void erase_facet(const facet& f);

   template <typename TSet>
   superset_iterator findSupersets(const GenericSet<TSet, Int, operations::cmp>& f, bool show_empty_facet) const
   {
      return superset_iterator(columns->begin(), f, show_empty_facet);
   }

   template <typename TSet, bool check_range>
   subset_iterator<TSet, check_range> findSubsets(const GenericSet<TSet, Int, operations::cmp>& f, bool_constant<check_range>) const
   {
      return subset_iterator<TSet, check_range>(columns->begin(), n_vertices(), f);
   }

protected:
   template <typename Iterator, typename Consumer, typename Model>
   static void consume_erased(const Iterator& ss, Consumer& consumer, Model)
   {
      *consumer = reinterpret_cast<const Facet&>(*ss); ++consumer;
   }

   template <typename Iterator, typename Consumer>
   static void consume_erased(const Iterator& ss, Consumer& consumer, is_scalar)
   {
      *consumer = ss.index(); ++consumer;
   }

   template <typename Iterator, typename Consumer>
   static void consume_erased(const Iterator& ss, Consumer& consumer)
   {
      consume_erased(ss, consumer, typename object_traits<typename iterator_traits<Consumer>::value_type>::model());
   }

   template <typename Iterator, typename Data>
   static void consume_erased(const Iterator&, black_hole<Data>&) {}

public:
   template <typename TSet> static
   Int back_or_nothing(const TSet& f)
   {
      auto last=entire<reversed>(f);
      return last.at_end() ? -1 : *last;
   }

   template <typename TSet, typename Consumer>
   Int eraseSubsets(const GenericSet<TSet, Int, operations::cmp>& f, Consumer consumer)
   {
      const Int orig_size = size_;
      for (subset_iterator<TSet> ss = findSubsets(f, std::true_type()); !ss.at_end(); ++ss) {
         consume_erased(ss, consumer);
         erase_facet(*ss);
      }
      return orig_size - size_;
   }

   template <typename TSet, typename Consumer>
   Int eraseSupersets(const GenericSet<TSet, Int, operations::cmp>& f, Consumer consumer)
   {
      if (back_or_nothing(f.top()) >= n_vertices()) {
         return 0;
      }
      const Int orig_size = size_;
      for (superset_iterator ss = findSupersets(f,false); !ss.at_end(); ++ss) {
         consume_erased(ss, consumer);
         erase_facet(*ss);
      }
      return orig_size - size_;
   }

   template <typename TSet, bool can_extend, typename Consumer>
   facet* insertMax(const GenericSet<TSet, Int, operations::cmp>& f, Consumer consumer,
                    bool_constant<can_extend>)
   {
      Int v_last;
      facet_id_t nid = new_id();
      if (can_extend && (v_last = back_or_nothing(f.top())) >= n_vertices()) {
         extend_cols(v_last);
      } else {
         superset_iterator ss = findSupersets(f, true);
         if (!ss.at_end())
            return nullptr;
      }

      for (subset_iterator<TSet, false> ss = findSubsets(f, std::false_type());  !ss.at_end();  ++ss) {
         consume_erased(ss, consumer);
         erase_facet(*ss);
      }
      return insert_from_it(entire(f.top()), nid);
   }

   template <typename TSet, bool can_extend, typename Consumer>
   facet* insertMin(const GenericSet<TSet, Int, operations::cmp>& f, Consumer consumer,
                    bool_constant<can_extend>)
   {
      bool accept = false;
      Int v_last;
      facet_id_t nid = new_id();
      if (can_extend && (v_last = back_or_nothing(f.top())) >= n_vertices()) {
         extend_cols(v_last);
      } else {
         superset_iterator ss = findSupersets(f, true);
         if (!ss.at_end()) {
            if (ss.exact_match())
               return nullptr;
            do {
               consume_erased(ss,consumer);
               erase_facet(*ss); 
               accept = true;
            } while (! (++ss).at_end());
         }
      }

      if (!accept) {
         subset_iterator<TSet, false> ss=findSubsets(f, std::false_type());
         if (!ss.at_end())
            return nullptr;
      }
      return insert_from_it(entire(f.top()), nid);
   }

protected:
   chunk_allocator facet_alloc;
   chunk_allocator cell_alloc;
   ptr_pair<facet> facets;
   col_ruler* columns;
   Int size_;
   facet_id_t next_id;

   void extend_cols(Int v)
   {
      if (v >= n_vertices())
         columns=col_ruler::resize(columns, v+1);
   }

   facet_id_t new_id()
   {
      facet_id_t id=next_id;
      if (++next_id == 0) {
         id = squeeze_ids();
         ++next_id;
      }
      return id;
   }

   void skip_ids(Int amount)
   {
      next_id += amount;
   }

   facet_id_t squeeze_ids()
   {
      facet_id_t id = 0;
      for (facet *f = facets.next, *fe = end_facet(); f != fe;  f=f->list_ptrs.next, ++id)
         f->id = id;
      return next_id=id;
   }

   friend class pm::FacetList;
   friend class pm::Cols<pm::FacetList>;
#if POLYMAKE_DEBUG
   bool sanity_check() const;
#endif
};

} // end namespace fl_internal

template <>
class Cols<FacetList>
   : public redirected_container< Cols<FacetList>,
                                  mlist< ContainerTag< const fl_internal::col_ruler >,
                                         HiddenTag< FacetList > > > {
protected:
   ~Cols();
public:
   inline const container& get_container() const;
};

/** @class FacetList

    This is a collection of sets of integral numbers from a closed contiguous
    range [0..n-1], as one usually has to deal with when modeling simplicial
    complexes and related mathematical objects.  Thus we will refer to the
    contained sets as @em facets, and to the set elements as @em vertices.

    The class can efficiently maintain the property of all facets being
    mutually inclusion-free, although this property is not mandatory,
    it can be violated by the application if needed.
    The primary design goal of this class is efficient search, insertion,
    and removal of facets included in or including a given vertex set.

    The data structure is a rectangular grid, similar to IncidenceMatrix,
    interwoven with a forest of suffix trees, indexed with the vertex number.
    This also provides the lexicographical facet ordering as a pleasant side
    effect.  The whole thing is attached to a smart pointer with @ref
    refcounting "reference counting".

    For complexity statements below we (ab-)use the term @em dimension
    (abbreviated as @em dim) for the number of vertices in a facet.
    Conversely, the @em degree (abbreviated as @em deg) of a vertex is the
    number of facets containing it.

    FacetList implements STL's reversible container interface.  During the
    iteration the facets appear in the chronological order, that is, as they
    were added to the list.  Unlike @c std::list, the size() method runs in
    constant time.

    The elements of the list (facets) are of type GenericSet and implement the
    reversible container interface, too.  However, there is no random access or
    cheap existence checks for single vertices; facets have to be scanned
    sequentially.

    The iterators over the facet list have an additional method @c index()
    retrieving the unique integer id assigned to each facet when it was
    inserted into the FacetList.  A new id is generated by each call to any of
    the insert() methods, regardless whether the facet is eventually inserted
    or discarded as being dependent.  Using methods squeeze() and
    skip_facet_id() you can affect the id generated for the next facet.
 */

class FacetList
   : public modified_container_impl< FacetList,
                                     mlist< ContainerTag< const fl_internal::Table >,
                                            OperationTag< pair< operations::reinterpret<fl_internal::Facet>, fl_internal::facet::id2index> > > >
   , public matrix_col_methods<FacetList> {
protected:
   shared_object<fl_internal::Table, AliasHandlerTag<shared_alias_handler>> table;

   friend FacetList& make_mutable_alias(FacetList& alias, FacetList& owner)
   {
      alias.table.make_mutable_alias(owner.table);
      return alias;
   }

public:
   const container& get_container() const { return *table; }

   /** @brief Create an empty list.
      
      Allocate the internal data structures capable of handling sets of
      vertices from the range [0 .. @a n_vertices-1] in advance.  The vertex range can be
      dynamically expanded later, by @c insert* and @c push_back %operations,
      with reallocation costs O(@a n_vertices).
   */
   explicit FacetList(Int n_vertices = 0)
      : table(sizeof(fl_internal::facet), n_vertices) {}

   template <typename Iterator>
   explicit FacetList(Iterator&& src,
                      std::enable_if_t<assess_iterator<Iterator, check_iterator_feature, end_sensitive>::value &&
                                       assess_iterator_value<Iterator, isomorphic_types, Set<Int>>::value,
                                       void**> =nullptr)
      : table(sizeof(fl_internal::facet), std::forward<Iterator>(src), std::false_type()) {}

   /** @brief Initialize the facets from the input sequence.
       The items obtained by dereferencing @a src must be sets of cardinals (of type GenericSet).
   */
   template <typename Iterator>
   FacetList(Iterator&& src, Iterator&& src_end)
      : table(sizeof(fl_internal::facet), make_iterator_range(src, src_end), std::false_type()) {}

   /** @brief As above, but avoiding reallocation during the construction.
       The facets supplied by @a src may not contain vertices outside the range [0, @a n_vertices-1].
   */
   template <typename Iterator>
   FacetList(Int n_vertices, Iterator&& src,
             std::enable_if_t<assess_iterator<Iterator, check_iterator_feature, end_sensitive>::value &&
                              assess_iterator_value<Iterator, isomorphic_types, Set<Int>>::value,
                              std::nullptr_t> = nullptr)
      : table(sizeof(fl_internal::facet), n_vertices, std::forward<Iterator>(src), std::false_type()) {}

   template <typename TSet>
   FacetList(const GenericSet<TSet, fl_internal::Facet::persistent_type, operations::cmp>& ps)
      : table(sizeof(fl_internal::facet), entire(ps.top()), std::true_type()) {}

   template <typename TSet>
   FacetList(Int n_vertices, const GenericSet<TSet, fl_internal::Facet::persistent_type, operations::cmp>& ps)
      : table(sizeof(fl_internal::facet), n_vertices, entire(ps.top()), std::true_type()) {}

   template <typename TMatrix>
   FacetList(const GenericIncidenceMatrix<TMatrix>& m)
      : table(sizeof(fl_internal::facet), m.cols(), entire(rows(m)), std::false_type()) {}

   template <typename Container, typename=std::enable_if_t<isomorphic_to_container_of<Container, Set<Int>, is_set>::value>>
   FacetList(const Container& src)
      : table(sizeof(fl_internal::facet), entire(src), std::false_type()) {}

   /// Swap the contents of two lists in a most efficient way.
   void swap(FacetList& l) { table.swap(l.table); }

   /// Make the list empty, release all allocated resources.
   void clear() { table.apply(shared_clear()); }

   /// Return number of facets in list.
   Int size() const { return table->size_; }
  
   /// True if empty.
   bool empty() const { return table->size_==0; }

   /// Returns the number of vertices.
   Int n_vertices() const { return table->n_vertices(); }

   /// Renumber the facet ids consequently, starting with 0, thus eliminating the gaps.
   void squeeze()
   {
      table->squeeze(operations::binary_noop());
   }

   /// If you want to gather the old ids, pass a binary functor as @a index_consumer.
   template <typename IndexConsumer>
   void squeeze(const IndexConsumer& ic)
   {
      table->squeeze(ic);
   }

   /** @brief Make an artificial gap in the generated facet id sequence.
    *  The facet inserted next will have an id @a amount greater than it would have had without this call.
    */
   void skip_facet_id(Int amount = 1) { table->skip_ids(amount); }

   class LexOrdered
      : public cascade_impl< LexOrdered,
                             mlist< ContainerTag< fl_internal::Table::LexOrdered_helper >,
                                    CascadeDepth< int_constant<2> >,
                                    HiddenTag< fl_internal::Table > > >,
        public GenericSet< LexOrdered, fl_internal::Facet::persistent_type, operations::cmp> {
   protected:
      ~LexOrdered();
   };

   /**  Another view on the list, visiting the facets in lexicographical order.
    *   The result type is a @ref manipulation "masquerade reference" pointing to
    *   a GenericSet< GenericSet<Int> >.
    */
   friend const LexOrdered& lex_ordered(const FacetList& c)
   {
      return reinterpret_cast<const LexOrdered&>(*c.table);
   }

   /** @brief Add a new facet without checking the inclusion relation to the existing facets.
    *
    *  It is allowed to insert a new facet being a subset or superset of existing facets.
    *  However, insertion of an empty set or of a duplicate facet is forbidden.
    *
    *  The operation costs are O(dim + deg).
    */
   template <typename TSet>
   iterator insert(const GenericSet<TSet, Int, operations::cmp>& f)
   {
      if (POLYMAKE_DEBUG || is_wary<TSet>()) {
         if (f.top().empty())
            throw std::runtime_error("FacetList::insert - empty facet");
      }
      return iterator(fl_internal::Table::iterator(table->insert(f)));
   }

   /** @brief Add a facet to the list with least efforts.
    *
    *  This method is primarily thought of as an construction aid, if none of
    *  the explicit constructors above suites, and enables the use of the
    *  convenient std::back_inserter.  The operation costs are O(dim).
    *
    *  The given facet must be lexicographically greater than all facets added
    *  before.  This can only be checked in debugging mode.
    */
   template <typename TSet>
   void push_back(const GenericSet<TSet, Int, operations::cmp>& f)
   {
      if (POLYMAKE_DEBUG || is_wary<TSet>()) {
         if (f.top().empty())
            throw std::runtime_error("FacetList::push_back - empty facet");
      }
      table->push_back(f);
   }

   /** @brief Find the facet equal to the given vertex set and remove it from the list.
    *
    *  Returns 1 if a facet was removed or 0 if no matching facet was found.
    *
    *  The operation costs are O(dim + deg).
    */
   template <typename TSet>
   Int erase(const GenericSet<TSet, Int, operations::cmp>& f)
   {
      return table->erase(f);
   }

   /// Remove the facet pointed to by the given iterator.
   void erase(const iterator& where)
   {
      return table->erase_facet(reinterpret_cast<const fl_internal::facet&>(*where));
   }

   template <typename TSet>
   iterator find(const GenericSet<TSet, Int, operations::cmp>& f) const
   {
      const fl_internal::facet* facet = table->find_facet(f);
      return facet ? iterator(fl_internal::Table::iterator(facet)) : end();
   }

   typedef unary_transform_iterator<fl_internal::superset_iterator, operations::reinterpret<fl_internal::Facet> >
      superset_iterator;

   template <typename TSet>
   superset_iterator findSupersets(const GenericSet<TSet, Int, operations::cmp>& f) const
   {
      return superset_iterator(table->findSupersets(f, false));
   }

   template <typename TSet>
   class subset_iterator
      : public unary_transform_iterator<fl_internal::subset_iterator<TSet>, operations::reinterpret<fl_internal::Facet> > {
      typedef unary_transform_iterator<fl_internal::subset_iterator<TSet>, operations::reinterpret<fl_internal::Facet> > base_t;
   public:
      subset_iterator(const fl_internal::subset_iterator<TSet>& it) : base_t(it) {}
   };

   template <typename TSet>
   subset_iterator<TSet> findSubsets(const GenericSet<TSet, Int, operations::cmp>& f) const
   {
      return subset_iterator<TSet>(table->findSubsets(f, std::true_type()));
   }

   /** @brief Erase all supersets of a given set.
       @return the number of facets actually removed.
   */
   template <typename TSet>
   Int eraseSupersets(const GenericSet<TSet, Int, operations::cmp>& f)
   {
      return table->eraseSupersets(f, black_hole<Int>());
   }

   /** @brief Erase all supersets of a given set.
       @param consumer an output iterator swallowing all erased facets or their IDs
       @return the number of facets actually removed.
   */
   template <typename TSet, typename Consumer>
   Int eraseSupersets(const GenericSet<TSet, Int, operations::cmp>& f, Consumer consumer)
   {
      return table->eraseSupersets(f, consumer);
   }

   /** @brief Erase all subsets of a given set.
    *  @return the number of facets actually removed.
   */
   template <typename TSet>
   Int eraseSubsets(const GenericSet<TSet, Int, operations::cmp>& f)
   {
      return table->eraseSubsets(f, black_hole<Int>());
   }

   /** @brief Erase all subsets of a given set.
    *  @param consumer an output iterator swallowing all erased facets or their IDs
    *  @return the number of facets actually removed.
   */
   template <typename TSet, typename Consumer>
   Int eraseSubsets(const GenericSet<TSet, Int, operations::cmp>& f, Consumer consumer)
   {
      return table->eraseSubsets(f, consumer);
   }

   /** Add a new facet @em{if and only if} there are no facets including it.
    *  If this holds, remove all facets that are included in the new one.
    *  The average operation costs are O(dim<sup>2</sup> deg).
    *
    *  @return @c true if the new facet was really included.
    */
   template <typename TSet>
   bool insertMax(const GenericSet<TSet, Int, operations::cmp>& f)
   {
      return table->insertMax(f, black_hole<Int>(), std::true_type());
   }

   /** Add a new facet @em{if and only if} there are no facets included in it.
    *  If this holds, remove all facets including the new one.
    *
    *  @return @c true if the new facet was really included.
    */
   template <typename TSet>
   bool insertMin(const GenericSet<TSet, Int, operations::cmp>& f)
   {
      return table->insertMin(f, black_hole<Int>(), std::true_type());
   }

   /** Add a new facet @em{if and only if} there are no facets including it.
    *  If this holds, remove all facets that are included in the new one.
    *  @param consumer an output iterator swallowing all erased facets or their IDs
    *
    *  @return @c true if the new facet was really included.
    */
   template <typename TSet, typename Consumer>
   bool insertMax(const GenericSet<TSet, Int, operations::cmp>& f, Consumer consumer)
   {
      return table->insertMax(f, consumer, std::true_type());
   }

   /** Add a new facet @em{if and only if} there are no facets included in it.
    *  If this holds, remove all facets including the new one.
    *  @param consumer an output iterator swallowing all erased facets or their IDs
    *
    *  @return @c true if the new facet was really included.
    */
   template <typename TSet, typename Consumer>
   bool insertMin(const GenericSet<TSet, Int, operations::cmp>& f, Consumer consumer)
   {
      return table->insertMin(f, consumer, std::true_type());
   }

   /** Slightly optimized versions of @see insertMax.  Assumes that the
       @c FacetList object already has all columns corresponding to the vertices
       of a new facet, and therefore does not need to be expanded.
   */
   template <typename TSet>
   bool replaceMax(const GenericSet<TSet, Int, operations::cmp>& f)
   {
      if (POLYMAKE_DEBUG || is_wary<TSet>()) {
         if (!set_within_range(f.top(), this->cols()))
            throw std::runtime_error("FacetList::replaceMax - invalid face");
      }
      return table->insertMax(f, black_hole<Int>(), std::false_type());
   }

   /** Slightly optimized versions of @see insertMin.  Assumes that the
       @c FacetList object already has all columns corresponding to the vertices
       of a new facet, and therefore does not need to be expanded.
   */
   template <typename TSet>
   bool replaceMin(const GenericSet<TSet, Int, operations::cmp>& f)
   {
      if (POLYMAKE_DEBUG || is_wary<TSet>()) {
         if (!set_within_range(f.top(), this->cols()))
            throw std::runtime_error("FacetList::replaceMin - invalid face");
      }
      return table->insertMin(f, black_hole<Int>(), std::false_type());
   }

   template <typename TSet, typename Consumer>
   bool replaceMax(const GenericSet<TSet, Int, operations::cmp>& f, Consumer consumer)
   {
      if (POLYMAKE_DEBUG || is_wary<TSet>()) {
         if (!set_within_range(f.top(), this->cols()))
            throw std::runtime_error("FacetList::replaceMax - invalid face");
      }
      return table->insertMax(f, consumer, std::false_type());
   }

   template <typename TSet, typename Consumer>
   bool replaceMin(const GenericSet<TSet, Int, operations::cmp>& f, Consumer consumer)
   {
      if (POLYMAKE_DEBUG || is_wary<TSet>()) {
         if (!set_within_range(f.top(), this->cols()))
            throw std::runtime_error("FacetList::replaceMin - invalid face");
      }
      return table->insertMin(f, consumer, std::false_type());
   }

   friend class Cols<FacetList>;

#if POLYMAKE_DEBUG
   bool sanity_check() const;
   void dump() const;
#endif
};

const Cols<FacetList>::container&
Cols<FacetList>::get_container() const
{
   return *hidden().table->columns;
}

template <>
struct spec_object_traits<FacetList::LexOrdered>
   : spec_object_traits<is_container> {
   static const bool is_always_const=true;
   typedef FacetList masquerade_for;
};

template <typename TSet>
struct check_iterator_feature<FacetList::subset_iterator<TSet>, end_sensitive> : std::true_type {};
template <typename TSet>
struct check_iterator_feature<FacetList::subset_iterator<TSet>, indexed> : std::true_type {};

} // end namespace pm

namespace std {
   inline
   void swap(pm::FacetList& l1, pm::FacetList& l2) { l1.swap(l2); }
}

namespace polymake {
   using pm::FacetList;
}

#endif // POLYMAKE_FACET_LIST_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
