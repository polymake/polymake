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
   template <typename Iterator, bool check_range=true> class subset_iterator;
   class Facet; class Table;
   typedef unsigned long facet_id_t;
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

   int vertex;       // vertex number = element of the facet

   cell(cell* head_arg, int vertex_arg)
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
      assert(links == &cell::facet || head_arg == NULL);
   }

   explicit cell_iterator(const cell* cur_arg)
      : super(cur_arg)
      , head(links == &cell::facet ? cur_arg->head_cell : NULL) {}

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

   int index() const
   {
      return links == &cell::facet ? this->cur->vertex : get_facet()->get_id();
   }

   const facet* get_facet() const;

protected:
   const cell* head;   // apparent head cell of the facet or NULL for vertical traversals

   // move to another facet following the given link
   // @return false if no cell exists in the given direction
   bool down(ptr_pair<cell> cell::* vertical_links)
   {
      assert(links == &cell::facet && links != vertical_links);
      const cell* next=(this->cur->*vertical_links).next;
      if (next==NULL) return false;
      this->cur=next;
      head=this->cur->head_cell;
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
      , _size(0)
      , id(id_arg) {}

   facet(const facet& l, chunk_allocator& al);

   void unlink_cells(chunk_allocator& al);
   cell* push_back(int vertex, chunk_allocator& al);

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

   int size() const { return _size; }
   bool empty() const { return _size==0; }

protected:
   ptr_pair<facet> list_ptrs;  // embedded list of all facets in FacetList
   ptr_pair<cell> cells;       // vertices comprising this facet
   int _size;
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
      typedef int result_type;
      int operator() (const facet& f) const { return f.id; }
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
inline
const facet* cell_iterator<links, _rev>::get_facet() const
{
   return links == &cell::facet ? facet::from_head_cell(head) : facet::from_cell(this->cur);
}


class Facet
   : public modified_container_impl< Facet,
                                     list( Hidden<facet>,
                                           Operation< BuildUnaryIt<operations::index2element> > ) >,
   public GenericSet<Facet, int, operations::cmp> {
public:
   operations::cmp get_comparator() const { return operations::cmp(); }
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

   col_order_iterator(const cell* cur_arg = NULL)
      : super(cur_arg, NULL) {}

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

   lex_order_iterator(const cell* cur_arg = NULL);

   reference operator* () const
   {
      return reinterpret_cast<reference>(*Q.back().get_facet());
   }
   pointer operator-> () const { return &(operator*()); }

   iterator& operator++ ();
   const iterator operator++ (int) { iterator copy(*this); operator++(); return copy; }

   bool at_end() const { return Q.empty(); }

   bool operator== (const iterator& it) const
   {
      return equal(entire(Q), entire(it.Q));
   }

   int index() const
   {
      return Q.back().index();
   }
private:
   void scan_facet(const cell* cur);
};


// Heads of lists of cells pertaining to a certain vertex
class vertex_list {
public:
   explicit vertex_list(int vertex_arg)
      : vertex(vertex_arg)
      , first_col(NULL)
      , first_lex(NULL) {}

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
   int size() const
   {
      return count_it(begin());
   }
   bool empty() const
   {
      return first_col==NULL;
   }

   friend void relocate(vertex_list* from, vertex_list* to)
   {
      to->vertex=from->vertex;
      cell *first;
      if ((to->first_col=first=from->first_col) != NULL)
         first->col.prev=to->col_head_cell();
      if ((to->first_lex=first=from->first_lex) != NULL)
         first->lex.prev=to->lex_head_cell();
   }
protected:
   int vertex;
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
         : first_old(NULL)
         , last_old(NULL)
         , first_new(NULL)
         , last_new(NULL) {}

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
      if ((c->col.next=first_col) != NULL)
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
   int size() const
   {
      return count_it(begin());
   }
   bool empty() const
   {
      return first_lex == NULL;
   }
};

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
   int its_size;
   static const value_type empty_facet;
public:
   superset_iterator() {}

   template <typename Set>
   superset_iterator(const vertex_list* columns, const GenericSet<Set>& f, bool show_empty_facet)
   {
      its_size= iterator_traits<typename Set::const_iterator>::is_bidirectional
                ? f.top().size() : 0;
      for (typename Entire<Set>::const_iterator v=entire(f.top());  !v.at_end();  ++v) {
         its.push_back(columns[*v].begin());
         if (!iterator_traits<typename Set::const_iterator>::is_bidirectional)
            ++its_size;
      }
      if (its_size)
         valid_position();
      else
         cur=show_empty_facet ? &empty_facet : 0;
   }

   reference operator* () const { return *cur; }
   pointer operator-> () const { return cur; }

   bool exact_match() const
   {
      return its_size==cur->size();
   }

   iterator& operator++ () { valid_position(); return *this; }
   const iterator operator++ (int) { iterator copy(*this);  operator++();  return copy; }

   bool at_end() const { return cur==NULL; }

   bool operator== (const iterator& it) const { return cur==it.cur; }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   int index() const { return cur->get_id(); }
private:
   void valid_position();

   template <typename,bool> friend class subset_iterator;
};

template <typename Set, bool check_range>
class subset_iterator {
public:
   typedef forward_iterator_tag iterator_category;
   typedef facet value_type;
   typedef const value_type& reference;
   typedef const value_type* pointer;
   typedef ptrdiff_t difference_type;
   typedef subset_iterator iterator;
   typedef iterator const_iterator;
protected:
   typedef typename Entire<Set>::const_iterator set_iterator;
   typedef std::pair<facet::iterator, set_iterator> it_pair;
   typedef std::list<it_pair> it_list;

   const vertex_list* columns;
   int n_columns;
   set_iterator start;
   it_list Q;
   pointer cur;

public:
   subset_iterator() {}

   subset_iterator(const vertex_list* columns_arg, const GenericSet<Set>& f)
      : columns(columns_arg), start(entire(f.top()))
   {
      assert(!check_range);
      valid_position();
   }

   subset_iterator(const vertex_list* columns_arg, int n_columns_arg, const GenericSet<Set>& f)
      : columns(columns_arg), n_columns(n_columns_arg), start(entire(f.top()))
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

   int index() const { return cur->get_id(); }
private:
   void valid_position();
};

template <typename Set, bool check_range>
void subset_iterator<Set,check_range>::valid_position()
{
   for (;;) {
      while (!Q.empty()) {
         it_pair itp=Q.back();  Q.pop_back();
         bool match;
         do {
            if (itp.first->lex.next != NULL) {
               Q.push_back(it_pair(facet::iterator(itp.first->lex.next), itp.second));
            }
            if ((++itp.first).at_end()) {
               cur=itp.first.get_facet();
               return;
            }
            const int vertex_of_facet=itp.first->vertex;
            while ((match=!(++itp.second).at_end())) {
               const int vertex_of_query=*itp.second;
               if (vertex_of_query >= vertex_of_facet) {
                  match= vertex_of_query==vertex_of_facet; break;
               }
            }
         } while (match);
      }

      for (;;) {
         if (start.at_end() || check_range && *start >= n_columns) {
            cur=NULL; return;
         }
         cell* first_lex=columns[*start].first_lex;
         if (first_lex != NULL) {
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
struct check_iterator_feature<fl_internal::cell_iterator<links, _rev>, end_sensitive> : True {};
template <ptr_pair<fl_internal::cell> fl_internal::cell::* links, bool _rev>
struct check_iterator_feature<fl_internal::cell_iterator<links, _rev>, indexed> : True {};

template <> struct check_iterator_feature<fl_internal::col_order_iterator, end_sensitive> : True {};
template <> struct check_iterator_feature<fl_internal::col_order_iterator, indexed> : True {};

template <> struct check_iterator_feature<fl_internal::lex_order_iterator, end_sensitive> : True {};
template <> struct check_iterator_feature<fl_internal::lex_order_iterator, indexed> : True {};

template <> struct check_iterator_feature<fl_internal::superset_iterator, end_sensitive> : True {};
template <> struct check_iterator_feature<fl_internal::superset_iterator, indexed> : True {};

template <typename Set, bool check_range>
struct check_iterator_feature<fl_internal::subset_iterator<Set, check_range>, end_sensitive> : True {};
template <typename Set, bool check_range>
struct check_iterator_feature<fl_internal::subset_iterator<Set, check_range>, indexed> : True {};

namespace fl_internal {

class Table {
public:
   typedef sparse2d::ruler<vertex_list> col_ruler;

   typedef facet value_type;
   typedef const value_type& reference;
   typedef reference const_reference;
   typedef embedded_list_iterator<facet, &facet::list_ptrs, true, false> iterator;
   typedef embedded_list_iterator<facet, &facet::list_ptrs, true, true> reverse_iterator;
   typedef iterator const_iterator;
   typedef reverse_iterator const_reverse_iterator;

   explicit Table(size_t facet_size=sizeof(facet), int n_vertices=0);

   Table(const Table& c);

private:
   // deleted
   void operator= (const Table&);

public:
   ~Table() { col_ruler::destroy(columns); }

   //! make the table look empty
   void clear();

   //! remove all facets, keep the allocated number of vertices
   void clear_facets();

   int n_vertices() const { return columns->size(); }

   iterator begin() const { return iterator(facets.next); }
   iterator end() const { return iterator(end_facet()); }
   reverse_iterator rbegin() const { return reverse_iterator(facets.prev); }
   reverse_iterator rend() const { return reverse_iterator(end_facet()); }

   template <typename IndexConsumer>
   void squeeze(const IndexConsumer& ic)
   {
      int new_vertex=0;
      for (vertex_list *column=columns->begin(), *end=columns->end(); column!=end; ++column) {
         if (cell* c=column->first_col) {
            const int old_vertex=column->vertex;
            if (old_vertex != new_vertex) {
               do
                  c->vertex = new_vertex;
               while ((c=c->col.next) != NULL);
               vertex_list* column_new=column+(new_vertex - old_vertex);
               relocate(column, column_new);
               column_new->vertex=new_vertex;
            }
            ic(old_vertex, new_vertex);  ++new_vertex;
         }
      }
      if (new_vertex < n_vertices()) columns=col_ruler::resize(columns, new_vertex, false);

      if (next_id != _size) squeeze_ids();
   }

protected:
   template <typename Iterator>
   Table(size_t facet_size, Iterator src, False)
      : facet_alloc(facet_size)
      , cell_alloc(sizeof(cell))
      , facets(&facet::list_ptrs)
      , columns(col_ruler::construct(0))
      , _size(0)
      , next_id(0)
   {
      for (; !src.at_end(); ++src)
         insert(*src);
   }

   template <typename Iterator>
   Table(size_t facet_size, Iterator src, Iterator src_end, False)
      : facet_alloc(facet_size)
      , cell_alloc(sizeof(cell))
      , facets(&facet::list_ptrs)
      , columns(col_ruler::construct(0))
      , _size(0)
      , next_id(0)
   {
      for (; src!=src_end; ++src)
         insert(*src);
   }

   template <typename Iterator>
   Table(size_t facet_size, int n_vertices, Iterator src, False)
      : facet_alloc(facet_size)
      , cell_alloc(sizeof(cell))
      , facets(&facet::list_ptrs)
      , columns(col_ruler::construct(n_vertices))
      , _size(0)
      , next_id(0)
   {
      for (; !src.at_end(); ++src)
         insert_from_it(entire(*src), new_id());
   }

   template <typename Iterator>
   Table(size_t facet_size, Iterator src, True)
      : facet_alloc(facet_size)
      , cell_alloc(sizeof(cell))
      , facets(&facet::list_ptrs)
      , columns(col_ruler::construct(0))
      , _size(0)
      , next_id(0)
   {
      for (; !src.at_end(); ++src)
         push_back(*src);
   }

   template <typename Iterator>
   Table(size_t facet_size, int n_vertices, Iterator src, True)
      : facet_alloc(facet_size)
      , cell_alloc(sizeof(cell))
      , facets(&facet::list_ptrs)
      , columns(col_ruler::construct(n_vertices))
      , _size(0)
      , next_id(0)
   {
      for (; !src.at_end(); ++src)
         push_back_from_it(entire(*src));
   }

   const facet* end_facet() const
   {
      return reverse_cast(&facets, &facet::list_ptrs);
   }
   facet* end_facet()
   {
      return reverse_cast(&facets, &facet::list_ptrs);
   }

public:
   class LexOrdered_helper
      : public modified_container_impl< LexOrdered_helper,
                                        list( Container< col_ruler >,
                                              Operation< operations::reinterpret<lex_ordered_vertex_list> >,
                                              Hidden< Table > ) > {
   public:
      const col_ruler& get_container() const { return *hidden().columns; }
   };

protected:
   void push_back_new_facet(facet* f);
   void push_back_facet(facet* f);

   template <typename Iterator>
   void insert_cells(facet* f, Iterator src)
   {
      vertex_list::inserter inserter;
      int v;
      do {
         if (src.at_end()) {
            if (inserter.new_facet_ended()) return;
            erase_facet(*f);
            throw std::runtime_error("attempt to insert a duplicate or empty facet into FacetList");
         }
         v=*src;  ++src;
         if (identical<typename iterator_traits<Iterator>::iterator_category, input_iterator_tag>::value)
            extend_cols(v);
      } while (!inserter.push((*columns)[v], f->push_back(v, cell_alloc)));

      for (; !src.at_end(); ++src) {
         v=*src;
         if (identical<typename iterator_traits<Iterator>::iterator_category, input_iterator_tag>::value)
            extend_cols(v);
         (*columns)[v].push_front(f->push_back(v, cell_alloc));
      }
   }

   template <typename Iterator>
   facet* insert_from_it(Iterator src, facet_id_t nid)
   {
      facet* f=new(facet_alloc.allocate()) facet(nid);
      push_back_facet(f);
      ++_size;
      insert_cells(f, src);
      return f;
   }

   template <typename Iterator>
   void push_back_from_it(Iterator src)
   {
      int new_vertex=*src;
      facet* f=new(facet_alloc.allocate()) facet(new_id());
      cell* newc;
      cell* lex_prev;

      if ((*columns)[new_vertex].first_lex != NULL) {
         assert(facets.prev != end_facet());
         facet::const_iterator prev_facet_it=facets.prev->begin();
         assert(new_vertex == prev_facet_it->vertex);
         push_back_facet(f);
         do {
            newc=(*columns)[new_vertex].push_front(f->push_back(new_vertex, cell_alloc));
            lex_prev=const_cast<cell*>(prev_facet_it.cur);
            ++src;  ++prev_facet_it;
            if (prev_facet_it.at_end()) break;
            assert(!src.at_end());
            new_vertex=*src;
            assert(new_vertex >= prev_facet_it->vertex);
         } while (new_vertex == prev_facet_it->vertex);
      } else {
         push_back_facet(f);
         lex_prev=(*columns)[new_vertex].lex_head_cell();
         newc=(*columns)[new_vertex].push_front(f->push_back(new_vertex, cell_alloc));
      }

      newc->lex.prev=lex_prev;
      lex_prev->lex.next=newc;

      while (!(++src).at_end()) {
         new_vertex=*src;
         (*columns)[new_vertex].push_front(f->push_back(new_vertex, cell_alloc));
      }
      ++_size;
   }

public:
   template <typename Set>
   const facet* find_facet(const GenericSet<Set>& f) const
   {
      typename Entire<Set>::const_iterator v_it=entire(f.top());
      if (v_it.at_end()) return NULL;
      int v=*v_it;
      if (v>=n_vertices()) return NULL;

      const cell* c=(*columns)[v].first_lex;
      if (c==NULL) return NULL;

      for (facet::iterator cur(c); ;) {
         ++v_it;  ++cur;
         if (cur.at_end()) {
            if (v_it.at_end())
               return cur.get_facet();
            else
               break;
         }
         if (v_it.at_end()) break;
         v=*v_it;
         while (cur->vertex != v) {
            if (cur->vertex > v) return NULL;
            --cur;
            if (!cur.down(&cell::lex)) return NULL;
            ++cur;
         }
      }
      return NULL;
   }

   template <typename Set>
   facet* insert(const GenericSet<Set, int, operations::cmp>& f)
   {
      extend_cols(f.top().back());
      return insert_from_it(entire(f.top()), new_id());
   }

   template <typename Set>
   void push_back(const GenericSet<Set, int, operations::cmp>& f)
   {
      extend_cols(f.top().back());
      push_back_from_it(entire(f.top()));
   }

   template <typename Set>
   int erase(const GenericSet<Set, int, operations::cmp>& f)
   {
      const facet* fp=find_facet(f);
      return fp != NULL ? (erase_facet(*fp), 1) : 0;
   }

   void erase_facet(const facet& f);

   template <typename Set>
   superset_iterator findSupersets(const GenericSet<Set, int, operations::cmp>& f, bool show_empty_facet) const
   {
      return superset_iterator(columns->begin(), f, show_empty_facet);
   }

   template <typename Set, bool check_range>
   subset_iterator<Set, check_range> findSubsets(const GenericSet<Set, int, operations::cmp>& f, bool2type<check_range>) const
   {
      return subset_iterator<Set, check_range>(columns->begin(), n_vertices(), f);
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
   template <typename Set> static
   int back_or_nothing(const Set& f)
   {
      typename Entire<Set>::const_reverse_iterator last=entire(reversed(f));
      return last.at_end() ? -1 : *last;
   }

   template <typename Set, typename Consumer>
   int eraseSubsets(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer)
   {
      const size_t orig_size=_size;
      for (subset_iterator<Set> ss=findSubsets(f, True()); !ss.at_end(); ++ss) {
         consume_erased(ss, consumer);
         erase_facet(*ss);
      }
      return orig_size-_size;
   }

   template <typename Set, typename Consumer>
   int eraseSupersets(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer)
   {
      if (back_or_nothing(f.top()) >= n_vertices()) {
         return 0;
      }
      const size_t orig_size=_size;
      for (superset_iterator ss=findSupersets(f,false); !ss.at_end(); ++ss) {
         consume_erased(ss, consumer);
         erase_facet(*ss);
      }
      return orig_size-_size;
   }

   template <typename Set, bool can_extend, typename Consumer>
   facet* insertMax(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer,
                    bool2type<can_extend>)
   {
      int v_last;
      facet_id_t nid=new_id();
      if (can_extend && (v_last=back_or_nothing(f.top())) >= n_vertices()) {
         extend_cols(v_last);
      } else {
         superset_iterator ss=findSupersets(f, true);
         if (!ss.at_end()) return NULL;
      }

      for (subset_iterator<Set,false> ss=findSubsets(f, False());  !ss.at_end();  ++ss) {
         consume_erased(ss, consumer);
         erase_facet(*ss);
      }
      return insert_from_it(entire(f.top()), nid);
   }

   template <typename Set, bool can_extend, typename Consumer>
   facet* insertMin(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer,
                    bool2type<can_extend>)
   {
      bool accept=false;
      int v_last;
      facet_id_t nid=new_id();
      if (can_extend && (v_last=back_or_nothing(f.top())) >= n_vertices()) {
         extend_cols(v_last);
      } else {
         superset_iterator ss=findSupersets(f, true);
         if (!ss.at_end()) {
            if (ss.exact_match()) return NULL;
            do {
               consume_erased(ss,consumer);
               erase_facet(*ss); 
               accept=true;
            } while (! (++ss).at_end());
         }
      }

      if (!accept) {
         subset_iterator<Set,false> ss=findSubsets(f, False());
         if (!ss.at_end()) return NULL;
      }
      return insert_from_it(entire(f.top()), nid);
   }

protected:
   chunk_allocator facet_alloc;
   chunk_allocator cell_alloc;
   ptr_pair<facet> facets;
   col_ruler* columns;
   size_t _size;
   facet_id_t next_id;

   void extend_cols(int v)
   {
      if (v >= n_vertices()) columns=col_ruler::resize(columns, v+1);
   }

   facet_id_t new_id()
   {
      facet_id_t id=next_id;
      if (++next_id == 0) {
         id=squeeze_ids();
         ++next_id;
      }
      return id;
   }

   void skip_ids(int amount)
   {
      next_id+=amount;
   }

   facet_id_t squeeze_ids()
   {
      facet_id_t id=0;
      for (facet *f=facets.next, *fe=end_facet(); f != fe;  f=f->list_ptrs.next, ++id)
         f->id=id;
      return next_id=id;
   }

   friend class pm::FacetList;
   friend class pm::Cols<pm::FacetList>;
   template <typename> friend class pm::constructor;
#if POLYMAKE_DEBUG
   bool sanity_check() const;
#endif
};

} // end namespace fl_internal

template <>
class Cols<FacetList>
   : public redirected_container< Cols<FacetList>,
                                  list( Container< const fl_internal::Table::col_ruler >,
                                        Hidden< FacetList > ) > {
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
                                     list( Container< const fl_internal::Table >,
                                           Operation< pair< operations::reinterpret<fl_internal::Facet>, fl_internal::facet::id2index> > ) >,
     public matrix_col_methods<FacetList> {
protected:
   shared_object<fl_internal::Table, AliasHandler<shared_alias_handler> > table;

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
   explicit FacetList(int n_vertices=0)
      : table( constructor<fl_internal::Table(size_t, int)>(sizeof(fl_internal::facet), n_vertices) ) {}

   template <typename Iterator>
   explicit FacetList(Iterator src, typename enable_if_iterator<Iterator,end_sensitive>::type=0)
      : table( constructor<fl_internal::Table(size_t, Iterator&, False)>(sizeof(fl_internal::facet), src, False()) ) {}

   /** @brief Initialize the facets from the input sequence.
       The items obtained by dereferencing @a src must be sets of cardinals (of type GenericSet).
   */
   template <typename Iterator>
   FacetList(Iterator src, Iterator src_end)
      : table( constructor<fl_internal::Table(size_t, Iterator&, Iterator&, False)>(sizeof(fl_internal::facet), src, src_end, False()) ) {}

   /** @brief As above, but avoiding reallocation during the construction.
       The facets supplied by @a src may not contain vertices outside the range [0, @a n_vertices-1].
   */
   template <typename Iterator>
   FacetList(int n_vertices, Iterator src)
      : table( constructor<fl_internal::Table(size_t, int, Iterator&, False)>(sizeof(fl_internal::facet), n_vertices, src, False()) ) {}

   template <typename Set>
   FacetList(const GenericSet<Set, fl_internal::Facet::persistent_type, operations::cmp>& ps)
      : table( constructor<fl_internal::Table(size_t, typename Entire<Set>::const_iterator, True)>(sizeof(fl_internal::facet), entire(ps.top()), True()) ) {}

   template <typename Set>
   FacetList(int n_vertices, const GenericSet<Set, fl_internal::Facet::persistent_type, operations::cmp>& ps)
      : table( constructor<fl_internal::Table(size_t, int, typename Entire<Set>::const_iterator, True)>(sizeof(fl_internal::facet), n_vertices, entire(ps.top()), True()) ) {}

   template <typename Matrix>
   FacetList(const GenericIncidenceMatrix<Matrix>& m)
      : table( constructor<fl_internal::Table(size_t, int, typename Entire< Rows<Matrix> >::const_iterator, False)>(sizeof(fl_internal::facet), m.cols(), entire(rows(m)), False()) ) {}

   template <typename Container>
   FacetList(const Container& src,
             typename enable_if<void**, isomorphic_to_container_of<Container, Set<int>, is_set>::value>::type=0)
      : table( constructor<fl_internal::Table(size_t, typename Entire<Container>::const_iterator, False)>(sizeof(fl_internal::facet), entire(src), False()) ) {}

   /// Swap the contents of two lists in a most efficient way.
   void swap(FacetList& l) { table.swap(l.table); }

   /// Make the list empty, release all allocated resources.
   void clear() { table.apply(shared_clear()); }

   /// Return number of facets in list.
   int size() const { return table->_size; }
  
   /// True if empty.
   bool empty() const { return table->_size==0; }

   /// Returns the number of vertices.
   int n_vertices() const { return table->n_vertices(); }

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
   void skip_facet_id(int amount=1) { table->skip_ids(amount); }

   class LexOrdered
      : public cascade_impl< LexOrdered,
                             list( Container< fl_internal::Table::LexOrdered_helper >,
                                   CascadeDepth< int2type<2> >,
                                   Hidden< fl_internal::Table > ) >,
        public GenericSet< LexOrdered, fl_internal::Facet::persistent_type, operations::cmp> {
   protected:
      ~LexOrdered();
   };

   /**  Another view on the list, visiting the facets in lexicographical order.
    *   The result type is a @ref manipulation "masquerade reference" pointing to
    *   a GenericSet< GenericSet<int> >.
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
   template <typename Set>
   iterator insert(const GenericSet<Set, int, operations::cmp>& f)
   {
      if (POLYMAKE_DEBUG || !Unwary<Set>::value) {
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
   template <typename Set>
   void push_back(const GenericSet<Set, int, operations::cmp>& f)
   {
      if (POLYMAKE_DEBUG || !Unwary<Set>::value) {
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
   template <typename Set>
   int erase(const GenericSet<Set, int, operations::cmp>& f)
   {
      return table->erase(f);
   }

   /// Remove the facet pointed to by the given iterator.
   void erase(const iterator& where)
   {
      return table->erase_facet(reinterpret_cast<const fl_internal::facet&>(*where));
   }

   template <typename Set>
   iterator find(const GenericSet<Set, int, operations::cmp>& f) const
   {
      const fl_internal::facet* facet=table->find_facet(f);
      return facet != NULL ? iterator(fl_internal::Table::iterator(facet)) : end();
   }

   typedef unary_transform_iterator<fl_internal::superset_iterator, operations::reinterpret<fl_internal::Facet> >
      superset_iterator;

   template <typename Set>
   superset_iterator findSupersets(const GenericSet<Set, int, operations::cmp>& f) const
   {
      return superset_iterator(table->findSupersets(f, false));
   }

   template <typename Set>
   class subset_iterator
      : public unary_transform_iterator<fl_internal::subset_iterator<Set>, operations::reinterpret<fl_internal::Facet> > {
      typedef unary_transform_iterator<fl_internal::subset_iterator<Set>, operations::reinterpret<fl_internal::Facet> > _super;
   public:
      subset_iterator(const fl_internal::subset_iterator<Set>& it) : _super(it) {}
   };

   template <typename Set>
   subset_iterator<Set> findSubsets(const GenericSet<Set, int, operations::cmp>& f) const
   {
      return subset_iterator<Set>(table->findSubsets(f, True()));
   }

   /** @brief Erase all supersets of a given set.
       @return the number of facets actually removed.
   */
   template <typename Set>
   int eraseSupersets(const GenericSet<Set, int, operations::cmp>& f)
   {
      return table->eraseSupersets(f, black_hole<int>());
   }

   /** @brief Erase all supersets of a given set.
       @param consumer an output iterator swallowing all erased facets or their IDs
       @return the number of facets actually removed.
   */
   template <typename Set, typename Consumer>
   int eraseSupersets(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer)
   {
      return table->eraseSupersets(f, consumer);
   }

   /** @brief Erase all subsets of a given set.
    *  @return the number of facets actually removed.
   */
   template <typename Set>
   int eraseSubsets(const GenericSet<Set, int, operations::cmp>& f)
   {
      return table->eraseSubsets(f, black_hole<int>());
   }

   /** @brief Erase all subsets of a given set.
    *  @param consumer an output iterator swallowing all erased facets or their IDs
    *  @return the number of facets actually removed.
   */
   template <typename Set, typename Consumer>
   int eraseSubsets(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer)
   {
      return table->eraseSubsets(f, consumer);
   }

   /** Add a new facet @em{if and only if} there are no facets including it.
    *  If this holds, remove all facets that are included in the new one.
    *  The average operation costs are O(dim<sup>2</sup> deg).
    *
    *  @return @c true if the new facet was really included.
    */
   template <typename Set>
   bool insertMax(const GenericSet<Set, int, operations::cmp>& f)
   {
      return table->insertMax(f, black_hole<int>(), True()) != NULL;
   }

   /** Add a new facet @em{if and only if} there are no facets included in it.
    *  If this holds, remove all facets including the new one.
    *
    *  @return @c true if the new facet was really included.
    */
   template <typename Set>
   bool insertMin(const GenericSet<Set, int, operations::cmp>& f)
   {
      return table->insertMin(f, black_hole<int>(), True()) != NULL;
   }

   /** Add a new facet @em{if and only if} there are no facets including it.
    *  If this holds, remove all facets that are included in the new one.
    *  @param consumer an output iterator swallowing all erased facets or their IDs
    *
    *  @return @c true if the new facet was really included.
    */
   template <typename Set, typename Consumer>
   bool insertMax(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer)
   {
      return table->insertMax(f, consumer, True()) != NULL;
   }

   /** Add a new facet @em{if and only if} there are no facets included in it.
    *  If this holds, remove all facets including the new one.
    *  @param consumer an output iterator swallowing all erased facets or their IDs
    *
    *  @return @c true if the new facet was really included.
    */
   template <typename Set, typename Consumer>
   bool insertMin(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer)
   {
      return table->insertMin(f, consumer, True()) != NULL;
   }

   /** Slightly optimized versions of @see insertMax.  Assumes that the
       @c FacetList object already has all columns corresponding to the vertices
       of a new facet, and therefore does not need to be expanded.
   */
   template <typename Set>
   bool replaceMax(const GenericSet<Set, int, operations::cmp>& f)
   {
      if (POLYMAKE_DEBUG || !Unwary<Set>::value) {
         if (!set_within_range(f.top(), this->cols()))
            throw std::runtime_error("FacetList::replaceMax - invalid face");
      }
      return table->insertMax(f, black_hole<int>(), False()) != NULL;
   }

   /** Slightly optimized versions of @see insertMin.  Assumes that the
       @c FacetList object already has all columns corresponding to the vertices
       of a new facet, and therefore does not need to be expanded.
   */
   template <typename Set>
   bool replaceMin(const GenericSet<Set, int, operations::cmp>& f)
   {
      if (POLYMAKE_DEBUG || !Unwary<Set>::value) {
         if (!set_within_range(f.top(), this->cols()))
            throw std::runtime_error("FacetList::replaceMin - invalid face");
      }
      return table->insertMin(f, black_hole<int>(), False()) != NULL;
   }

   template <typename Set, typename Consumer>
   bool replaceMax(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer)
   {
      if (POLYMAKE_DEBUG || !Unwary<Set>::value) {
         if (!set_within_range(f.top(), this->cols()))
            throw std::runtime_error("FacetList::replaceMax - invalid face");
      }
      return table->insertMax(f, consumer, False()) != NULL;
   }

   template <typename Set, typename Consumer>
   bool replaceMin(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer)
   {
      if (POLYMAKE_DEBUG || !Unwary<Set>::value) {
         if (!set_within_range(f.top(), this->cols()))
            throw std::runtime_error("FacetList::replaceMin - invalid face");
      }
      return table->insertMin(f, consumer, False()) != NULL;
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

template <typename Set>
struct check_iterator_feature<FacetList::subset_iterator<Set>, end_sensitive> : True {};
template <typename Set>
struct check_iterator_feature<FacetList::subset_iterator<Set>, indexed> : True {};

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
