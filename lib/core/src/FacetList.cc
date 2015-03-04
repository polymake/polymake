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

#if POLYMAKE_DEBUG
#  include <vector>
#endif

#include "polymake/FacetList.h"

namespace pm { namespace facet_list {

std::allocator<cell> cell_allocator;

template <>
facet<false>::~facet()
{
   cell *cur=first, *end=_end_cell();
   while (cur != end) {
      cell *next=cur->row_next;
      cell_allocator.deallocate(cur,1);
      cur=next;
   }
}

static inline
cell* delete_cell(cell *c)
{
   cell *prev=c->row_prev;
   if ((c->col_prev->col_next=c->col_next))
      c->col_next->col_prev=c->col_prev;
   cell_allocator.deallocate(c,1);
   return prev;
}

template <>
facet<true>::~facet()
{
   cell *cur=last, *end=_end_cell(), *below, *above;
   while (cur != end) {
      below=cur->lex_next;  above=cur->lex_prev;
      cur=delete_cell(cur);
      if (above) goto ABOVE;
      if (below) {
         below->lex_prev=0;
         break;
      }
   }
   while (cur != end) {
      below=below->row_prev;  above=cur->lex_prev;
      if ((below->lex_next=cur->lex_next))
         below->lex_next->lex_prev=below;
      cur=delete_cell(cur);
      if (above) goto ABOVE;
   }
   return;
 ABOVE:
   if ((above->lex_next=below))
      below->lex_prev=above;
   while (cur != end)
      cur=delete_cell(cur);
}

vertex_list::vertex_list(const vertex_list& l)
   : vertex_index(l.vertex_index)
{
   for (cell *orig=l.first_col;  orig;  orig=orig->col_next) {
      if (orig->lex_next) {
         cell *clone=orig->col_prev, *next_clone=orig->lex_next->col_prev;
         clone->lex_next=next_clone;
         next_clone->lex_prev=clone;
      }
   }
   if (l.first_lex) {
      cell *clone=l.first_lex->col_prev;
      first_lex=clone;
      clone->lex_prev=_lex_head_cell();
   } else {
      first_lex=0;
   }

   cell *prev_clone=_col_head_cell();
   for (cell *orig=l.first_col;  orig;  orig=orig->col_next) {
      cell *clone=orig->col_prev;
      orig->col_prev=clone->col_prev;
      prev_clone->col_next=clone;
      clone->col_prev=prev_clone;
      prev_clone=clone;
   }
   prev_clone->col_next=0;
}

bool vertex_list::inserter::push(vertex_list& column, cell *newc)
{
   column.push_front(newc);

   if (!first_new) {
      // the starting vertex of a facet
      if (!column.first_lex) {
         // we have the first facet with the given starting vertex: ready
         column.first_lex=newc;
         newc->lex_prev=column._lex_head_cell();
         return true;
      }
      // there are already facets with this starting vertex: keep on comparing
      first_old=last_old=column.first_lex;
      first_new=last_new=newc;
      (old_key_mask=first_old->key) ^= column.vertex_index;
      return false;
   }

   for (;;) {
      cell *next_old=last_old->row_next;
      assert(next_old != old_key_mask.ptr);
      int old_v=next_old->key.key ^ old_key_mask.key, new_v=column.vertex_index;
      if (old_v==new_v) {
         // still equal prefixes: keep on comparing
         last_old=next_old;
         last_new=newc;
         return false;
      }
      if (old_v>new_v) break;

      if (!(first_old=last_old->lex_next)) {
         // no more facets with this prefix: hang below and ready
         last_old->lex_next=last_new;
         last_new->lex_prev=last_old;
         return true;
      }
      old_key_mask ^= first_old->key ^ last_old->key;
      last_old=first_old;
      first_new=last_new;
   }

   (first_new->lex_prev=first_old->lex_prev)->lex_next=first_new;
   first_old->lex_prev=0;
   for (; first_old != last_old;  first_old=first_old->row_next, first_new=first_new->row_next) {
      if ((first_new->lex_next=first_old->lex_next))
         first_new->lex_next->lex_prev=first_new;
      first_old->lex_next=0;
   }
   last_new->lex_next=last_old;
   last_old->lex_prev=last_new;
   return true;
}

inline
void lex_order_iterator::scan_facet(const cell* cur, int col)
{
   cell *end=cur->key ^ col;
   while ((cur=cur->row_next) != end)
      if (cur->lex_next)
         Q.push_back(cit(cur, cur->key ^ end));
}

lex_order_iterator::lex_order_iterator(const cell* cur, int col)
{
   if (!cur) return;
   Q.push_back(cit(cur,col));
   scan_facet(cur,col);
}

lex_order_iterator& lex_order_iterator::operator++ ()
{
   do {
      cit& last_it=Q.back();
      ++last_it;
      if (!last_it.at_end()) {
         scan_facet(last_it.operator->(), last_it.get_col());
         break;
      }
      Q.pop_back();
   } while (!Q.empty());

   return *this;
}

void superset_iterator::valid_position()
{
   it_list::iterator start_it=its.begin(), end_it=its.end(), it=start_it;
   if (it->at_end()) {
      cur=0;  return;
   }
   cur=it->get_facet();  ++(*it);
   pointer next;
   while (((++it)==end_it ? it=its.begin() : it) != start_it) {
      do {
         if (it->at_end()) {
            cur=0;  return;
         }
         next=it->get_facet();  ++(*it);
      } while (next->id > cur->id);
      if (next->id < cur->id) {
         start_it=it;  cur=next;
      }
   }
}

const facet<> superset_iterator::empty_facet(0);

#if POLYMAKE_DEBUG

static
PlainPrinter<>& _show_facet(PlainPrinter<>& os, const facet<>& f, bool& shown)
{
   if (!shown) {
      shown=true;
      os << "Table::check - facet " << reinterpret_cast<const Facet&>(f) << endl;
   }
   return os;
}

void Table::check() const
{
   size_t n_f=0;
   std::vector<int> hist(n_vertices());
   for (Entire<facet_list>::const_iterator f=entire(_facets);  !f.at_end(); ++f, ++n_f) {
      int n_e=0;
      int prev_i=-1;
      bool shown=false, lex_prev_seen=false;
      for (Entire< facet<> >::const_iterator e=entire(*f); !e.at_end(); ++e, ++n_e) {
         int i=e.index();
         if (i<=prev_i)
            _show_facet(cerr, *f,shown) << "order violation" << endl;
         prev_i=i;
         ++hist[i];

         if (!e->col_prev)
            _show_facet(cerr, *f,shown) << "col_prev[" << i << "]=NULL" << endl;
         else if (e->col_prev->col_next != e.operator->())
            _show_facet(cerr, *f,shown) << "col_prev[" << i << "] mismatch" << endl;
         else if (e->col_next && e->col_next->col_prev != e.operator->())
            _show_facet(cerr, *f,shown) << "col_next[" << i << "] mismatch" << endl;

         if (e->lex_prev && e->lex_prev->lex_next != e.operator->())
            _show_facet(cerr, *f,shown) << "lex_prev[" << i << "] mismatch" << endl;
         else if (e->lex_next && e->lex_next->lex_prev != e.operator->())
            _show_facet(cerr, *f,shown) << "lex_next[" << i << "] mismatch" << endl;

         if (lex_prev_seen && e->lex_prev)
            _show_facet(cerr, *f,shown) << "lex_prev[" << i << "] multiple" << endl;
         if (e->lex_prev) lex_prev_seen=true;
         if (!lex_prev_seen && e->lex_next)
            _show_facet(cerr, *f,shown) << "lex_next[" << i << "] before lex_prev" << endl;

         if (e->lex_next) {
            column_iterator<&cell::lex_next> ci(e->lex_next, i);
            if (reinterpret_cast<const Facet&>(*f) >= reinterpret_cast<const Facet&>(*ci.get_facet()))
               _show_facet(cerr, *f,shown) << "lexical order violation with "
                                           << reinterpret_cast<const Facet&>(*ci.get_facet()) << endl;
            else if (incl(reinterpret_cast<const Facet&>(*f), reinterpret_cast<const Facet&>(*ci.get_facet())) != 2)
               _show_facet(cerr, *f,shown) << "inclusion independence violated with "
                                           << reinterpret_cast<const Facet&>(*ci.get_facet()) << endl;
         }
      }
      if (n_e != f->size())
         _show_facet(cerr, *f,shown) << "size mismatch: " << n_e << "!=" << f->size() << endl;
   }

   if (n_f != _size)
      cerr << "Table::check - total size violation: " << n_f << "!=" << _size << endl;

   for (Entire<col_ruler>::const_iterator ci=entire(*columns); !ci.at_end(); ++ci) {
      int n_e=0;
      for (Entire<vertex_list>::const_iterator e=entire(*ci); !e.at_end(); ++e, ++n_e) ;
      if (n_e != hist[ci->vertex_index])
         cerr << "Table::check - column counter[" << ci->vertex_index << "] mismatch: " << n_e << "!=" << hist[ci->vertex_index] << endl;
      for (column_iterator<&cell::lex_next> e(ci->first_lex, ci->vertex_index); !e.at_end(); ++e)
         if (e->row_prev != e.get_facet()->_end_cell())
            cerr << "Table::check - first_lex[" << ci->vertex_index << "] linked with "
                 << reinterpret_cast<const Facet&>(*e.get_facet()) << endl;
   }
}
#endif

} } // end namespace pm::facet_list

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
