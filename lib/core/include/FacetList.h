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
#include "polymake/IncidenceMatrix.h"
#include "polymake/CascadedContainer.h"
#include "polymake/list"
#include <cassert>

namespace pm {

class FacetList;
template <> class Cols<FacetList>;

namespace facet_list {
   template <bool standalone=false> class facet;
   class vertex_list;
   class lex_ordered_vertex_list;
   class superset_iterator;
   template <typename Iterator, bool check_range=true> class subset_iterator;
   class Facet; class Table;
}

template <bool standalone>
struct spec_object_traits< facet_list::facet<standalone> >
   : spec_object_traits<is_container> {
   static const bool is_always_const=true;
   static const IO_separator_kind IO_separator=IO_sep_inherit;
};
template <>
struct spec_object_traits<facet_list::Facet>
   : spec_object_traits<is_container> {
   static const bool is_always_const=true;
   typedef facet_list::facet<> masquerade_for;
};

template <> struct spec_object_traits<facet_list::vertex_list>
   : spec_object_traits<is_container> {};
template <> struct spec_object_traits<facet_list::lex_ordered_vertex_list>
   : spec_object_traits<is_container> {};

namespace facet_list {

struct cell;

union cell_pointer {
   cell *ptr;
   unsigned long key;

   cell_pointer() : ptr(0) {}
   explicit cell_pointer(cell *p) : ptr(p) {}
   explicit cell_pointer(unsigned long k) : key(k) {}
   cell_pointer(cell *p1, cell *p2) : ptr(p1) { cell_pointer cp2(p2); key^=cp2.key; }
   cell_pointer(cell *p, unsigned long k) : ptr(p) { key^=k; }

   cell_pointer& operator= (cell *p) { ptr=p; return *this; }
   cell_pointer& operator= (unsigned long k) { key=k; return *this; }

   cell_pointer& operator^= (const cell_pointer& cp) { key ^= cp.key; return *this; }
   cell_pointer& operator^= (unsigned long k) { key ^= k; return *this; }

   cell_pointer operator^ (const cell_pointer& cp) const { return cell_pointer(key ^ cp.key); }
   cell* operator^ (unsigned long k2) const { cell_pointer p(key ^ k2); return p.ptr; }
   unsigned long operator^ (cell *p2) const { cell_pointer p(p2); return p.key ^ key; }

   const cell* operator-> () const { return ptr; }
};

struct cell {
   /// vertex number XOR facet list addr
   cell_pointer key;

   /// facet list (=row)
   cell *row_prev, *row_next,
   /// vertex list (=column)
        *col_prev, *col_next,
   /// lexicographical list
        *lex_prev, *lex_next;

   explicit cell(unsigned long k)
      : key(k), lex_prev(0), lex_next(0) {}

   cell(cell *p, unsigned long k)
      : key(p,k), lex_prev(0), lex_next(0) {}

   cell(const cell_pointer& cp)
      : key(cp), lex_prev(0), lex_next(0) {}
};

extern std::allocator<cell> cell_allocator;

template <bool _forward>
class facet_list_iterator {
public:
   typedef bidirectional_iterator_tag iterator_category;
   typedef cell value_type;
   typedef const cell& reference;
   typedef const cell* pointer;
   typedef ptrdiff_t difference_type;
   typedef facet_list_iterator iterator;
   typedef facet_list_iterator const_iterator;

   facet_list_iterator() {}

   facet_list_iterator(cell *cur_arg, cell *head_arg)
      : head(head_arg), cur(cur_arg) {}

   facet_list_iterator(cell *cur_arg, unsigned long index_arg)
      : head(cur_arg->key ^ index_arg), cur(cur_arg) {}

   reference operator* () const { return *cur; }
   pointer operator-> () const { return cur; }

   iterator& operator++ ()
   {
      cur= _forward ? cur->row_next : cur->row_prev;
      return *this;
   }
   iterator& operator-- ()
   {
      cur= _forward ? cur->row_prev : cur->row_next;
      return *this;
   }
   const iterator operator++ (int) { iterator copy(*this); operator++(); return copy; }
   const iterator operator-- (int) { iterator copy(*this); operator--(); return copy; }

   bool at_end() const { return cur==head.ptr; }

   template <bool _forward2>
   bool operator== (const facet_list_iterator<_forward2>& it) const { return cur==it.cur; }

   template <bool _forward2>
   bool operator!= (const facet_list_iterator<_forward2>& it) const { return !operator==(it); }

   int index() const
   {
      return head.key ^ cur->key.key;
   }
protected:
   cell_pointer head;
   cell *cur;

   bool down(cell* cell::* next_ptr)
   {
      cell *next=cur->*next_ptr;
      if (!next) return false;
      head ^= cur->key ^ next->key;
      cur=next;
      return true;
   }

   template <bool> friend class facet;
   template <bool> friend class facet_list_iterator;
   friend class Table;
};

template <bool standalone>
class facet {
public:
   explicit facet(unsigned int id_arg=-1U)
      : _size(0), id(id_arg), last(_end_cell()), first(last) {}

   facet(const facet& l)
      : _size(l._size), id(l.id)
   {
      cell *prev_clone=_end_cell();
      if (_size) {
         cell *end=l._end_cell();
         const cell_pointer key_mask(prev_clone,end);
         for (cell *orig=l.first;  orig!=end;  orig=orig->row_next) {
            cell *clone=new(cell_allocator.allocate(1)) cell(orig->key ^ key_mask);
            clone->col_prev=orig->col_prev;
            orig->col_prev=clone;
            prev_clone->row_next=clone;
            clone->row_prev=prev_clone;
            prev_clone=clone;
         }
         prev_clone->row_next=_end_cell();
      } else {
         first=prev_clone;
      }
      last=prev_clone;
   }

   // we don't use it at all, but std::list requires its elements be assignable
   facet& operator= (const facet&)
   {
      throw std::runtime_error("FacetList::facet::operator= forbidden");
   }

public:
   ~facet();

   cell* push_back(int index)
   {
      cell* c=new(cell_allocator.allocate(1)) cell(_end_cell(), index);
      c->row_next=_end_cell();
      c->row_prev=last;
      last->row_next=c;
      last=c;
      ++_size;
      return c;
   }

   cell* push_front(int index)
   {
      cell* c=new(cell_allocator.allocate(1)) cell(_end_cell(), index);
      c->row_next=first;
      c->row_prev=_end_cell();
      first->row_prev=c;
      first=c;
      ++_size;
      return c;
   }

   typedef facet_list_iterator<true> iterator;
   typedef iterator const_iterator;
   typedef facet_list_iterator<false> reverse_iterator;
   typedef reverse_iterator const_reverse_iterator;
   typedef cell value_type;
   typedef const cell& reference;
   typedef reference const_reference;

   iterator begin() const
   {
      return iterator(first, _end_cell());
   }
   iterator end() const
   {
      return iterator(_end_cell(), _end_cell());
   }
   reverse_iterator rbegin() const
   {
      return reverse_iterator(last, _end_cell());
   }
   reverse_iterator rend() const
   {
      return reverse_iterator(_end_cell(), _end_cell());
   }

   int size() const { return _size; }
   bool empty() const { return !_size; }

protected:
   int _size;
   unsigned int id;
   cell *last, *first;

   cell* _end_cell() const
   {
      return reverse_cast(const_cast<cell**>(&last), &cell::row_prev);
   }
public:
   static const facet* from_end_cell(const cell *c)
   {
      return reverse_cast(&(c->row_prev), &facet::last);
   }

   static const facet* from_cell(const cell *c, unsigned long col)
   {
      return from_end_cell(c->key^col);
   }

   template <bool _forward>
   static const facet* from_iterator(const facet_list_iterator<_forward>& it)
   {
      return from_end_cell(it.head.ptr);
   }

   unsigned int get_id() const { return id; }

   struct id2index {
      typedef facet argument_type;
      typedef int result_type;
      int operator() (const facet& f) const { return f.id; }
   };

   friend class Table;
   friend class superset_iterator;
};

template <> facet<true>::~facet();
template <> facet<false>::~facet();

class Facet
   : public modified_container_impl< Facet,
                                     list( Hidden< facet<> >,
                                           Operation< BuildUnaryIt<operations::index2element> > ) >,
   public GenericSet<Facet, int, operations::cmp> {
public:
   operations::cmp get_comparator() const { return operations::cmp(); }
protected:
   Facet();
   ~Facet();
};

template <cell* cell::* next_ptr>
class column_iterator {
protected:
   const cell *cur;
   int col;
public:
   typedef forward_iterator_tag iterator_category;
   typedef cell value_type;
   typedef const cell& reference;
   typedef const cell* pointer;
   typedef ptrdiff_t difference_type;
   typedef column_iterator iterator;
   typedef column_iterator const_iterator;

   column_iterator() {} 
   column_iterator(const cell* cur_arg, int col_arg)
      : cur(cur_arg), col(col_arg) {}

   reference operator* () const { return *cur; }
   pointer operator-> () const { return cur; }

   iterator& operator++ ()
   {
      cur=cur->*next_ptr;
      return *this;
   }
   const iterator operator++ (int) { iterator copy(*this); operator++(); return copy; }

   bool at_end() const { return !cur; }
   bool operator== (const iterator& it) const { return cur==it.cur; }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   const facet<>* get_facet() const
   {
      return facet<>::from_end_cell(cur->key^col);
   }

   template <bool _forward>
   facet_list_iterator<_forward> get_facet_iterator(bool2type<_forward>) const
   {
      return facet_list_iterator<_forward>(cur, *get_facet());
   }

   int get_col() const { return col; }

   int index() const { return get_facet()->get_id(); }
};

class col_order_iterator : public column_iterator<&cell::col_next> {
   typedef column_iterator<&cell::col_next> super;
public:
   typedef Facet value_type;
   typedef const Facet& reference;
   typedef const Facet* pointer;
   typedef col_order_iterator iterator;
   typedef iterator const_iterator;

   col_order_iterator() {}
   col_order_iterator(const cell* cur_arg, int col_arg)
      : super(cur_arg, col_arg) {}

   reference operator* () const
   {
      return reinterpret_cast<reference>(*get_facet());
   }
   pointer operator-> () const { return &(operator*()); }

   iterator& operator++ () { super::operator++(); return *this; }
   const iterator operator++ (int) { iterator copy(*this); super::operator++(); return copy; }
};

class lex_order_iterator {
protected:
   typedef column_iterator<&cell::lex_next> cit;
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

   lex_order_iterator() {}
   lex_order_iterator(const cell* cur_arg, int col);

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
   void scan_facet(const cell* cur, int col);

   void push(int);
};

class vertex_list {
public:
   explicit vertex_list(int index_arg)
      : vertex_index(index_arg), first_col(0), first_lex(0) {}

   vertex_list(const vertex_list& l);
private:
   void operator=(const vertex_list&);

public:
   typedef col_order_iterator iterator;
   typedef iterator const_iterator;

   typedef Facet value_type;
   typedef const Facet& reference;
   typedef reference const_reference;

   iterator begin() const
   {
      return iterator(first_col, vertex_index);
   }
   iterator end() const
   {
      return iterator(0, vertex_index);
   }
   reference front() const
   {
      return *begin();
   }
   int size() const
   {
      return count_it(begin());
   }
   bool empty() const
   {
      return !first_col;
   }

   friend void relocate(vertex_list* from, vertex_list* to)
   {
      to->vertex_index=from->vertex_index;
      cell *first;
      if ((to->first_col=first=from->first_col))
         first->col_prev=to->_col_head_cell();
      if ((to->first_lex=first=from->first_lex))
         first->lex_prev=to->_lex_head_cell();
   }
protected:
   int vertex_index;
   cell *first_col, *first_lex;

   cell* _col_head_cell() const
   {
      return reverse_cast(const_cast<cell**>(&first_col), &cell::col_next);
   }
   cell* _lex_head_cell() const
   {
      return reverse_cast(const_cast<cell**>(&first_lex), &cell::lex_next);
   }

   class inserter {
   private:
      inserter(const inserter&);
      void operator=(const inserter&);
   public:
      explicit inserter()
         : first_old(0), first_new(0), last_old(0), last_new(0) {}

      /// @retval true if the fork point reached
      bool push(vertex_list& column, cell *newc);

   protected:
      cell *first_old, *first_new, *last_old, *last_new;
      cell_pointer old_key_mask;
   };

   cell* push_front(cell *c)
   {
      if ((c->col_next=first_col))
         first_col->col_prev=c;
      c->col_prev=_col_head_cell();
      first_col=c;
      return c;
   }

   friend class Table;
   template <typename,bool> friend class subset_iterator;
};

class lex_ordered_vertex_list : public vertex_list {
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
      return iterator(first_lex, vertex_index);
   }
   iterator end() const
   {
      return iterator(0, vertex_index);
   }
   reference front() const
   {
      return reinterpret_cast<reference>(*facet<>::from_cell(first_lex, vertex_index));
   }
   int size() const
   {
      return count_it(begin());
   }
   bool empty() const
   {
      return !first_lex;
   }
};

class superset_iterator {
public:
   typedef forward_iterator_tag iterator_category;
   typedef facet<> value_type;
   typedef const value_type& reference;
   typedef const value_type* pointer;
   typedef ptrdiff_t difference_type;
   typedef superset_iterator iterator;
   typedef iterator const_iterator;
protected:
   typedef column_iterator<&cell::col_next> it_type;
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

   bool at_end() const { return !cur; }

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
   typedef facet<> value_type;
   typedef const value_type& reference;
   typedef const value_type* pointer;
   typedef ptrdiff_t difference_type;
   typedef subset_iterator iterator;
   typedef iterator const_iterator;
protected:
   typedef facet_list_iterator<true> it_type;
   typedef typename Entire<Set>::const_iterator second_it_type;
   typedef std::pair<it_type, second_it_type> it_pair;
   typedef std::list<it_pair> it_list;

   const vertex_list* columns;
   int n_columns;
   second_it_type start;
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
            if (itp.first->lex_next) {
               Q.push_back(it_pair(it_type(itp.first->lex_next, itp.first.index()), itp.second));
            }
            if ((++itp.first).at_end()) {
               cur=facet<>::from_end_cell(itp.first.operator->());
               return;
            }
            int v_f=itp.first.index();
            while ((match=!(++itp.second).at_end())) {
               int v=*itp.second;
               if (v>=v_f) {
                  match= v==v_f; break;
               }
            }
         } while (match);
      }

      for (;;) {
         if (start.at_end() || check_range && *start>=n_columns) {
            cur=0; return;
         }
         cell *first_lex=columns[*start].first_lex;
         if (first_lex) {
            Q.push_back(it_pair(it_type(first_lex, *start), start));
            ++start;
            break;
         }
         ++start;
      }
   }
}

} // end namespace facet_list;

template <bool _forward>
struct check_iterator_feature<facet_list::facet_list_iterator<_forward>, end_sensitive> : True {};
template <bool _forward>
struct check_iterator_feature<facet_list::facet_list_iterator<_forward>, indexed> : True {};

template <facet_list::cell* facet_list::cell::* next_ptr>
struct check_iterator_feature<facet_list::column_iterator<next_ptr>, end_sensitive> : True {};
template <facet_list::cell* facet_list::cell::* next_ptr>
struct check_iterator_feature<facet_list::column_iterator<next_ptr>, indexed> : True {};

template <> struct check_iterator_feature<facet_list::col_order_iterator, end_sensitive> : True {};
template <> struct check_iterator_feature<facet_list::col_order_iterator, indexed> : True {};

template <> struct check_iterator_feature<facet_list::lex_order_iterator, end_sensitive> : True {};
template <> struct check_iterator_feature<facet_list::lex_order_iterator, indexed> : True {};

template <> struct check_iterator_feature<facet_list::superset_iterator, end_sensitive> : True {};
template <> struct check_iterator_feature<facet_list::superset_iterator, indexed> : True {};

template <typename Set, bool check_range>
struct check_iterator_feature<facet_list::subset_iterator<Set,check_range>, end_sensitive> : True {};
template <typename Set, bool check_range>
struct check_iterator_feature<facet_list::subset_iterator<Set,check_range>, indexed> : True {};

namespace facet_list {

/** @class Table
    @brief abstraction of FacetList
*/
class Table {
public:
   typedef std::list< facet<> > facet_list;
   typedef sparse2d::ruler<vertex_list> col_ruler;

   explicit Table(int n_vertices=0)
      : columns(col_ruler::construct(n_vertices)), _size(0), next_id(0) {}

   Table(const Table& c)
      : _facets(c._facets), columns(col_ruler::construct(*c.columns)), _size(c._size), next_id(c.next_id) {}

   ~Table() { col_ruler::destroy(columns); }

   void clear()
   {
      _facets.clear();
      columns=col_ruler::resize(columns,0);
   }

   int n_vertices() const { return columns->size(); }

   template <typename IndexConsumer>
   void squeeze(IndexConsumer ic)
   {
      int cnew=0;
      for (vertex_list *column=columns->begin(), *end=columns->end(); column!=end; ++column) {
         if (cell *c=column->first_col) {
            int cold=column->vertex_index;
            if (unsigned long diff = cold ^ cnew) {
               do
                  c->key ^= diff;
               while ((c=c->col_next));
               vertex_list *column_new=column+(cnew-cold);
               relocate(column, column_new);
               column_new->vertex_index=cnew;
            }
            *ic++=cold; ++cnew;
         }
      }
      if (cnew < n_vertices()) columns=col_ruler::resize(columns,cnew,false);

      if (next_id != _size) squeeze_ids();
   }

protected:
   template <typename Iterator>
   Table(Iterator src, False)
      : columns(col_ruler::construct(0)), _size(0), next_id(0)
   {
      for (; !src.at_end(); ++src)
         insert(*src);
   }

   template <typename Iterator>
   Table(Iterator src, Iterator src_end, False)
      : columns(col_ruler::construct(0)), _size(0), next_id(0)
   {
      for (; src!=src_end; ++src)
         insert(*src);
   }

   template <typename Iterator>
   Table(int n_vertices, Iterator src, False)
      : columns(col_ruler::construct(n_vertices)), _size(0), next_id(0)
   {
      for (; !src.at_end(); ++src)
         _insert(entire(*src));
   }

   template <typename Iterator>
   Table(Iterator src, True)
      : columns(col_ruler::construct(0)), _size(0), next_id(0)
   {
      for (; !src.at_end(); ++src)
         push_back(*src);
   }

   template <typename Iterator>
   Table(int n_vertices, Iterator src, True)
      : columns(col_ruler::construct(n_vertices)), _size(0), next_id(0)
   {
      for (; !src.at_end(); ++src)
         _push_back(entire(*src));
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
   template <typename Iterator>
   void _insert(Iterator src, unsigned int nid)
   {
      _facets.push_back(facet<>(nid));
      facet<>& newf=_facets.back();

      vertex_list::inserter inserter;
      int v;
      do {
         assert(!src.at_end());
         v=*src;  ++src;
         if (identical<typename iterator_traits<Iterator>::iterator_category, input_iterator_tag>::value)
            _extend(v);
      } while (!inserter.push((*columns)[v], newf.push_back(v)));

      for (; !src.at_end(); ++src) {
         v=*src;
         if (identical<typename iterator_traits<Iterator>::iterator_category, input_iterator_tag>::value)
            _extend(v);
         (*columns)[v].push_front(newf.push_back(v));
      }
      ++_size;
   }

   template <typename Iterator>
   void _insert(Iterator src)
   {
      _insert(src, new_id());
   }

   template <typename Iterator>
   void _push_back(Iterator src)
   {
      int v=*src;
      facet<> *newf;
      cell *newc, *lex_prev;

      if ((*columns)[v].first_lex) {
         facet<>::const_iterator old_it=_facets.back().begin();
         assert(old_it.index()==v);
         _facets.push_back(facet<>(new_id()));
         newf=&_facets.back();
         do {
            newc=(*columns)[v].push_front(newf->push_back(v));
            lex_prev=const_cast<cell*>(old_it.cur);
            ++src; ++old_it;
            assert(!src.at_end() && !old_it.at_end());
            v=*src;
            assert(v >= old_it.index());
         } while (v==old_it.index());
      } else {
         _facets.push_back(facet<>(new_id()));
         newf=&_facets.back();
         lex_prev=(*columns)[v]._lex_head_cell();
         newc=(*columns)[v].push_front(newf->push_back(v));
      }

      newc->lex_prev=lex_prev;
      lex_prev->lex_next=newc;

      while (!(++src).at_end()) {
         v=*src;
         (*columns)[v].push_front(newf->push_back(v));
      }
      ++_size;
   }

   template <typename Set>
   const facet<>* _find(const GenericSet<Set>& f) const
   {
      typename Entire<Set>::const_iterator v_it=entire(f.top());
      if (v_it.at_end()) return 0;
      int v=*v_it;
      if (v>=n_vertices()) return 0;

      cell *c=(*columns)[v].first_lex;
      if (!c) return 0;

      for (facet<>::iterator cur(c,v); ;) {
         ++v_it;  ++cur;
         if (cur.at_end()) {
            if (v_it.at_end())
               return facet<>::from_iterator(cur);
            else
               break;
         }
         if (v_it.at_end()) break;
         v=*v_it;
         int vc;
         while ((vc=cur.index()) != v) {
            if (vc>v) return 0;
            --cur;
            if (!cur.down(&cell::lex_next)) return 0;
            ++cur;
         }
      }
      return 0;
   }

   void _erase(const facet<>& f)
   {
      typedef std::list< facet<true> > list;
      typedef list::iterator list_iterator;
      typedef std_list_traits< facet<true> > traits;
      traits::Node *n=traits::to_node(reinterpret_cast<facet<true>*>(const_cast<facet<>*>(&f)));
      reinterpret_cast<list&>(_facets).erase(list_iterator(n));
      --_size;
   }
public:
   template <typename Set>
   void insert(const GenericSet<Set, int, operations::cmp>& f)
   {
      _extend(f.top().back());
      _insert(entire(f.top()));
   }

   template <typename Set>
   void push_back(const GenericSet<Set, int, operations::cmp>& f)
   {
      _extend(f.top().back());
      _push_back(entire(f.top()));
   }

   template <typename Set>
   int erase(const GenericSet<Set, int, operations::cmp>& f)
   {
      const facet<>* fp=_find(f);
      return fp ? (_erase(*fp), 1) : 0;
   }

   void erase(const facet_list::const_iterator& where)
   {
      typedef std::list< facet<true> > list;
      typedef list::iterator list_iterator;
      reinterpret_cast<list&>(_facets).erase(reinterpret_cast<const list_iterator&>(where));
      --_size;
   }

   template <typename Set>
   facet_list::const_iterator
   find(const GenericSet<Set, int, operations::cmp>& f) const
   {
      const facet<>* fp=_find(f);
      if (fp)
         return facet_list::const_iterator(std_list_traits< facet<> >::to_node(fp));
      return _facets.end();
   }

   template <typename Set>
   superset_iterator findMax(const GenericSet<Set, int, operations::cmp>& f, bool show_empty_facet) const
   {
      return superset_iterator(columns->begin(), f, show_empty_facet);
   }

   template <typename Set, bool check_range>
   subset_iterator<Set, check_range> findMin(const GenericSet<Set, int, operations::cmp>& f, bool2type<check_range>) const
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
   int eraseMax(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer)
   {
      int cnt=_size;
      for (subset_iterator<Set> ss=findMin(f, True()); !ss.at_end(); ++ss) {
         consume_erased(ss,consumer);
         _erase(*ss);
      }
      return cnt-_size;
   }

   template <typename Set, typename Consumer>
   int eraseMin(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer)
   {
      if (back_or_nothing(f.top()) >= n_vertices()) {
         return 0;
      }
      int cnt=_size;
      for (superset_iterator ss=findMax(f,false); !ss.at_end(); ++ss) {
         consume_erased(ss,consumer);
         _erase(*ss);
      }
      return cnt-_size;
   }

   template <typename Set, bool can_extend, typename Consumer>
   bool insertMax(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer,
                  bool2type<can_extend>)
   {
      int v_last;
      unsigned int nid=new_id();
      if (can_extend && (v_last=back_or_nothing(f.top())) >= n_vertices()) {
         _extend(v_last);
      } else {
         superset_iterator ss=findMax(f,true);
         if (!ss.at_end()) return false;
      }

      for (subset_iterator<Set,false> ss=findMin(f, False());  !ss.at_end();  ++ss) {
         consume_erased(ss, consumer);
         _erase(*ss);
      }
      _insert(entire(f.top()), nid);
      return true;
   }

   template <typename Set, bool can_extend, typename Consumer>
   bool insertMin(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer,
                  bool2type<can_extend>)
   {
      bool accept=false;
      int v_last;
      unsigned int nid=new_id();
      if (can_extend && (v_last=back_or_nothing(f.top())) >= n_vertices()) {
         _extend(v_last);
      } else {
         superset_iterator ss=findMax(f,true);
         if (!ss.at_end()) {
            if (ss.exact_match()) return false;
            do {
               consume_erased(ss,consumer);
               _erase(*ss); 
               accept=true;
            } while (! (++ss).at_end());
         }
      }

      if (!accept) {
         subset_iterator<Set,false> ss=findMin(f, False());
         if (!ss.at_end()) return false;
      }
      _insert(entire(f.top()), nid);
      return true;
   }

protected:
   facet_list _facets;
   sparse2d::ruler<vertex_list> *columns;
   unsigned int _size, next_id;

   void _extend(int v)
   {
      if (v >= n_vertices()) columns=col_ruler::resize(columns, v+1);
   }

   unsigned int new_id()
   {
      unsigned int id=next_id;
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

   unsigned int squeeze_ids()
   {
      unsigned int id=0;
      for (Entire<facet_list>::iterator f=entire(_facets); !f.at_end(); ++f, ++id)
         f->id=id;
      return next_id=id;
   }

   friend class pm::FacetList;
   friend class pm::Cols<pm::FacetList>;
   template <typename> friend class pm::constructor;
#if POLYMAKE_DEBUG
   void check() const;
#endif
};

} // end namespace facet_list

template <>
class Cols<FacetList>
   : public redirected_container< Cols<FacetList>,
                                  list( Container< const facet_list::Table::col_ruler >,
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

    The main invariant is that all facets are mutually inclusion-free.  The
    primary design goal of this class is effective search, insertion, and
    removal of facets included in or including a given vertex set.

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
    reversible container interface, too.

    The iterators over the facet list have an additional method @c index()
    retrieving the unique integer id assigned to each facet when it was being
    inserted into the FacetList.  A new id is generated by each call to any of
    the insert() methods, regardless whether the facet is eventually inserted
    or discarded as being dependent.  Using methods squeeze() and
    skip_facet_id() you can take effect on the id generated for the next
    facet.
 */

class FacetList
   : public modified_container_impl< FacetList,
                                     list( Container< const facet_list::Table::facet_list >,
                                           Operation< pair< operations::reinterpret<facet_list::Facet>, facet_list::facet<>::id2index> > ) >,
     public matrix_col_methods<FacetList> {
protected:
   shared_object<facet_list::Table, AliasHandler<shared_alias_handler> > table;

   friend FacetList& make_mutable_alias(FacetList& alias, FacetList& owner)
   {
      alias.table.make_mutable_alias(owner.table);
      return alias;
   }

public:
   const container& get_container() const { return table->_facets; }

  /** @brief Create an empty list.
      
      Allocate the internal data structures capable of handling sets of
      vertices from the range [0 .. @a n_vertices-1] in advance.  The vertex range can be
      dynamically expanded later, by @c insert* and @c push_back %operations,
      with reallocation costs O(@a n_vertices).
  */
   explicit FacetList(int n_vertices=0)
      : table( constructor<facet_list::Table(int)>(n_vertices) ) {}

   template <typename Iterator>
   explicit FacetList(Iterator src, typename enable_if_iterator<Iterator,end_sensitive>::type=0)
      : table( constructor<facet_list::Table(Iterator&, False)>(src, False()) ) {}

  /** @brief Initialize the facets from the input sequence.
      The items obtained by dereferencing @a src must be sets of cardinals (of type GenericSet).
  */
   template <typename Iterator>
   FacetList(Iterator src, Iterator src_end)
      : table( constructor<facet_list::Table(Iterator&, Iterator&, False)>(src, src_end, False()) ) {}

  /** @brief As above, but avoiding reallocation during the construction.
      The facets supplied by @a src may not contain vertices outside the range [0, @a n_vertices-1].
  */
   template <typename Iterator>
   FacetList(int n_vertices, Iterator src)
      : table( constructor<facet_list::Table(int, Iterator&, False)>(n_vertices, src, False()) ) {}

   template <typename Set>
   FacetList(const GenericSet<Set, facet_list::Facet::persistent_type, operations::cmp>& ps)
      : table( constructor<facet_list::Table(typename Entire<Set>::const_iterator, True)>(entire(ps.top()), True()) ) {}

   template <typename Set>
   FacetList(int n_vertices, const GenericSet<Set, facet_list::Facet::persistent_type, operations::cmp>& ps)
      : table( constructor<facet_list::Table(int, typename Entire<Set>::const_iterator, True)>(n_vertices, entire(ps.top()), True()) ) {}

   template <typename Matrix>
   FacetList(const GenericIncidenceMatrix<Matrix>& m)
      : table( constructor<facet_list::Table(int, typename Entire< Rows<Matrix> >::const_iterator, False)>(m.cols(), entire(rows(m)), False()) ) {}

   template <typename Container>
   FacetList(const Container& src,
             typename enable_if<void**, isomorphic_to_container_of<Container, Set<int>, is_set>::value>::type=0)
      : table( constructor<facet_list::Table(typename Entire<Container>::const_iterator, False)>(entire(src), False()) ) {}

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
      table->squeeze(black_hole<int>());
   }

   /// If you want to gather the old ids, pass an output iterator as @a index_consumer.
   template <typename IndexConsumer>
   void squeeze(IndexConsumer ic)
   {
      table->squeeze(ic);
   }

   /** @brief Make an artificial gap in the generated facet id sequence.
    *  The facet inserted next will have an id @a amount greater than it would have had without this call.
   */
   void skip_facet_id(int amount=1) { table->skip_ids(amount); }

   class LexOrdered
      : public cascade_impl< LexOrdered,
                             list( Container< facet_list::Table::LexOrdered_helper >,
                                   CascadeDepth< int2type<2> >,
                                   Hidden< facet_list::Table > ) >,
        public GenericSet< LexOrdered, facet_list::Facet::persistent_type, operations::cmp> {
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
    *  It should be guaranteed by the application logic, that the
    *  non-inclusion invariant is not violated!  There is no debugging mode
    *  that could detect this.
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
      table->insert(f);
      return --(const_cast<const FacetList*>(this)->table->_facets.end());
   }

   /** @brief Add a facet to the list.
    *
    *  This method is primarily thought of as an construction aid, if none of
    *  the explicit constructors above suites, and enables the use of the
    *  convenient std::back_inserter.  The operation costs are O(dim).
    *
    *  The given facet must be lexicographically greater than all facets added
    *  before.
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
      return table->erase(where);
   }

   template <typename Set>
   iterator find(const GenericSet<Set, int, operations::cmp>& f) const
   {
      return iterator(table->find(f));
   }

   typedef unary_transform_iterator<facet_list::superset_iterator, operations::reinterpret<facet_list::Facet> >
      iteratorMax;

   template <typename Set>
   iteratorMax findMax(const GenericSet<Set, int, operations::cmp>& f) const
   {
      return iteratorMax(table->findMax(f, false));
   }

   template <typename Set>
   class iteratorMin
      : public unary_transform_iterator<facet_list::subset_iterator<Set>, operations::reinterpret<facet_list::Facet> > {
      typedef unary_transform_iterator<facet_list::subset_iterator<Set>, operations::reinterpret<facet_list::Facet> > _super;
   public:
      iteratorMin(const facet_list::subset_iterator<Set>& it) : _super(it) {}
   };

   template <typename Set>
   iteratorMin<Set> findMin(const GenericSet<Set, int, operations::cmp>& f) const
   {
      return iteratorMin<Set>(table->findMin(f, True()));
   }

   /** @brief The same as insertMax(), but without adding any new facet.
       Returns the number of facets actually removed.
   */
   template <typename Set>
   int eraseMax(const GenericSet<Set, int, operations::cmp>& f)
   {
      return table->eraseMax(f, black_hole<int>());
   }

   /** @brief The same as insertMin(), but without adding any new facet.
       Returns the number of facets actually removed.
   */
   template <typename Set>
   int eraseMin(const GenericSet<Set, int, operations::cmp>& f)
   {
      return table->eraseMin(f, black_hole<int>());
   }

   template <typename Set, typename Consumer>
   int eraseMax(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer)
   {
      return table->eraseMax(f, consumer);
   }

   template <typename Set, typename Consumer>
   int eraseMin(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer)
   {
      return table->eraseMin(f, consumer);
   }

  /** Add a new facet @em{if and only if} there are no facets including it.
   *  If this holds, remove also all facets that are included in the new one.
   *  Return @c true if the new facet was really included.
   *
   *  The average operation costs are O(dim<sup>2</sup> deg).
   */
   template <typename Set>
   bool insertMax(const GenericSet<Set, int, operations::cmp>& f)
   {
      return table->insertMax(f, black_hole<int>(), True());
   }

   /** The opposite of insertMax: add a new facet @em{if and only if}
    *  there are no facets included in it, remove all facets including the new facet.
    */
   template <typename Set>
   bool insertMin(const GenericSet<Set, int, operations::cmp>& f)
   {
      return table->insertMin(f, black_hole<int>(), True());
   }

  /** Same as insertMax(const GenericSet<Set, int, operations::cmp>& f) except
      for a second argument.  This iterator gathers the facets removed by
      the operation.  It must implement the output iterator interface; its @c
      value_type must be either int, if it is collecting the facet @em id's,
      or GenericSet<...,int>, if it is collecting the complete facets.
  */
   template <typename Set, typename Consumer>
   bool insertMax(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer)
   {
      return table->insertMax(f, consumer, True());
   }

  /// Opposite of the above.
   template <typename Set, typename Consumer>
   bool insertMin(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer)
   {
      return table->insertMin(f, consumer, True());
   }

  /** Slightly optimized versions of @c insertMax.  Assumes that the @c
      FacetList object already has all columns corresponding to the vertices of a
      new facet, and therefore does not need to be expanded.
  */
   template <typename Set>
   bool replaceMax(const GenericSet<Set, int, operations::cmp>& f)
   {
      if (POLYMAKE_DEBUG || !Unwary<Set>::value) {
         if (!set_within_range(f.top(), this->cols()))
            throw std::runtime_error("FacetList::replaceMax - invalid face");
      }
      return table->insertMax(f, black_hole<int>(), False());
   }

  /// Opposite of the above.
   template <typename Set>
   bool replaceMin(const GenericSet<Set, int, operations::cmp>& f)
   {
      if (POLYMAKE_DEBUG || !Unwary<Set>::value) {
         if (!set_within_range(f.top(), this->cols()))
            throw std::runtime_error("FacetList::replaceMin - invalid face");
      }
      return table->insertMin(f, black_hole<int>(), False());
   }

  /// ... with output consumer
   template <typename Set, typename Consumer>
   bool replaceMax(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer)
   {
      if (POLYMAKE_DEBUG || !Unwary<Set>::value) {
         if (!set_within_range(f.top(), this->cols()))
            throw std::runtime_error("FacetList::replaceMax - invalid face");
      }
      return table->insertMax(f, consumer, False());
   }

  /// ... with output consumer
   template <typename Set, typename Consumer>
   bool replaceMin(const GenericSet<Set, int, operations::cmp>& f, Consumer consumer)
   {
      if (POLYMAKE_DEBUG || !Unwary<Set>::value) {
         if (!set_within_range(f.top(), this->cols()))
            throw std::runtime_error("FacetList::replaceMin - invalid face");
      }
      return table->insertMin(f, consumer, False());
   }

   friend class Cols<FacetList>;

#if POLYMAKE_DEBUG
   void check() const { table->check(); }
   ~FacetList() { POLYMAKE_DEBUG_METHOD(FacetList,dump); }
   void dump() const { cerr << *this << std::flush; }
#endif
};

const Cols<FacetList>::container&
Cols<FacetList>::get_container() const
{
   return *hidden().table->columns;
}

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
