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

#include "polymake/FacetList.h"
#include "polymake/list"

namespace pm { namespace fl_internal {

namespace {

// unlink the cell from the column list and destroy it
// @return the predecessor in the column list
cell* delete_cell(cell* c, chunk_allocator& al)
{
   cell* prev = c->facet.prev;
   if (cell* col_next = (c->col.prev->col.next = c->col.next))
      col_next->col.prev = c->col.prev;
   al.reclaim(c);
   return prev;
}

// unlink the cell from the prefix tree,
// destroy it and all predecessors in the facet
void delete_facet_prefix(cell* c, cell* below, chunk_allocator& al)
{
   if ((c->lex.prev->lex.next = below) != nullptr)
      below->lex.prev = c->lex.prev;

   const cell* const head = c->head_cell;
   do c = delete_cell(c, al); while (c != head);      
}

void store_cell_copy(cell* orig, cell* copy)
{
   copy->col.prev = orig->col.prev;
   orig->col.prev = copy;
}

cell* cell_copy(cell* c) { return c->col.prev; }

cell* forget_cell_copy(cell* orig)
{
   cell* copy = orig->col.prev;
   orig->col.prev = copy->col.prev;
   return copy;
}

}

// copy constructor creates new cells and links them properly in the row direction.
// column and prefix-tree links are established later, in vertex_list copy constructor
facet::facet(const facet& l, chunk_allocator& al)
   : size_(l.size_)
   , id(l.id)
{
   cell* const head = head_cell();
   cell* prev_copy = head;
   if (size_ != 0) {
      const cell* end = l.head_cell();
      for (cell* orig = l.cells.next;  orig != end;  orig = orig->facet.next) {
         cell* copy = new(al.allocate()) cell(head, orig->vertex);
         store_cell_copy(orig, copy);
         prev_copy->facet.next = copy;
         copy->facet.prev = prev_copy;
         prev_copy = copy;
      }
      prev_copy->facet.next = head;
   } else {
      cells.next = head;
   }
   cells.prev = prev_copy;
}

void facet::unlink_cells(chunk_allocator& al)
{
   cell* cur = cells.prev;
   cell* const head = head_cell();
   cell* below = nullptr;
   while (cur != head) {
      below = cur->lex.next;
      if (cur->lex.prev != nullptr) {
         delete_facet_prefix(cur, below, al);
         return;
      }
      cur = delete_cell(cur, al);
      if (below != nullptr) {
         below->lex.prev = nullptr;
         break;
      }
   }
   // the lexicographically next facet (pointed by below) must inherit all out-arcs in the prefix tree
   for (;;) {
      assert(cur != head);
      below = below->facet.prev;
      if ((below->lex.next = cur->lex.next) != nullptr)
         below->lex.next->lex.prev = below;
      if (cur->lex.prev != nullptr) {
         delete_facet_prefix(cur, below, al);
         return;
      }
      cur = delete_cell(cur, al);
   }
}

cell* facet::push_back(Int vertex, chunk_allocator& al)
{
   cell* const head = head_cell();
   cell* c = new(al.allocate()) cell(head, vertex);
   c->facet.next = head;
   c->facet.prev = cells.prev;
   cells.prev->facet.next = c;
   cells.prev = c;
   ++size_;
   return c;
}

vertex_list::vertex_list(const vertex_list& l)
   : vertex(l.vertex)
{
   // reminder: facet copy constructor linked the cloned cells to source cells' col.prev
   //           and saved the original pointers in clones' col.prev
   for (cell* orig = l.first_col;  orig != nullptr;  orig = orig->col.next) {
      if (orig->lex.next != nullptr) {
         cell* copy = cell_copy(orig);
         cell* next_copy = cell_copy(orig->lex.next);
         copy->lex.next = next_copy;
         next_copy->lex.prev = copy;
      }
   }
   if (l.first_lex != nullptr) {
      cell* copy = cell_copy(l.first_lex);
      first_lex = copy;
      copy->lex.prev = lex_head_cell();
   } else {
      first_lex = nullptr;
   }

   cell* prev_copy = col_head_cell();
   for (cell* orig = l.first_col;  orig != nullptr;  orig = orig->col.next) {
      cell* copy = forget_cell_copy(orig);
      prev_copy->col.next = copy;
      copy->col.prev = prev_copy;
      prev_copy = copy;
   }
   prev_copy->col.next = nullptr;
}

bool vertex_list::inserter::push(vertex_list& column, cell* newc)
{
   column.push_front(newc);

   if (first_new == nullptr) {
      // the starting vertex of a facet
      if (column.first_lex == nullptr) {
         // we have the first facet with the given starting vertex: ready
         column.first_lex = newc;
         newc->lex.prev = column.lex_head_cell();
         return true;
      }
      // there are already facets with this starting vertex: keep on comparing
      first_old = last_old = column.first_lex;
      first_new = last_new = newc;
      return false;
   }

   for (;;) {
      cell* next_old = last_old->facet.next;
      if (next_old != last_old->head_cell) {
         if (next_old->vertex == column.vertex) {
            // still equal prefixes: keep on comparing
            last_old = next_old;
            last_new = newc;
            break;
         }
         if (next_old->vertex > column.vertex) {
            // the new facet is lexicographically lower than the old one
            finalize();
            return true;
         }
      }
      if ((first_old=last_old->lex.next) == nullptr) {
         // the new facet is lexicographically following the old one and there are no other facets with the same prefix:
         // hang the new facet below the old one and ready
         last_old->lex.next = last_new;
         last_new->lex.prev = last_old;
         return true;
      }
      // proceed with the lexicographically next existing facet
      last_old = first_old;
      first_new = last_new;
   }
   return false;
}

bool vertex_list::inserter::new_facet_ended()
{
   // reject empty and duplicate facets
   if (last_old != nullptr && last_old->facet.next != nullptr) {
      finalize();
      return true;
   }
   return false;
}

void vertex_list::inserter::finalize()
{
   // the new facet is lexicographically preceding the old one: take over all out-arcs in the prefix tree
   (first_new->lex.prev = first_old->lex.prev)->lex.next = first_new;
   first_old->lex.prev = nullptr;
   for (; first_old != last_old;  first_old = first_old->facet.next, first_new = first_new->facet.next) {
      if ((first_new->lex.next = first_old->lex.next) != nullptr)
         first_new->lex.next->lex.prev = first_new;
      first_old->lex.next = nullptr;
   }
   last_new->lex.next = last_old;
   last_old->lex.prev = last_new;
}

void lex_order_iterator::scan_facet(const cell* cur)
{
   const cell* const head = cur->head_cell;
   while ((cur = cur->facet.next) != head)
      if (cur->lex.next != nullptr)
         Q.push_back(cit(cur));
}

lex_order_iterator::lex_order_iterator(const cell* cur)
{
   if (cur != nullptr) {
      Q.push_back(cit(cur));
      scan_facet(cur);
   }
}

lex_order_iterator& lex_order_iterator::operator++ ()
{
   do {
      cit& last_it = Q.back();
      ++last_it;
      if (!last_it.at_end()) {
         scan_facet(last_it.operator->());
         break;
      }
      Q.pop_back();
   } while (!Q.empty());

   return *this;
}

void superset_iterator::valid_position()
{
   it_list::iterator start_it = its.begin(), end_it = its.end(), it = start_it;
   if (it->at_end()) {
      cur=nullptr;  return;
   }
   cur = it->get_facet();  ++(*it);
   pointer next;
   while (((++it) == end_it ? it = its.begin() : it) != start_it) {
      do {
         if (it->at_end()) {
            cur = nullptr;  return;
         }
         next = it->get_facet();  ++(*it);
      } while (next->id > cur->id);
      if (next->id < cur->id) {
         start_it = it;  cur = next;
      }
   }
}

Table::Table(size_t facet_size, Int n_vertices)
   : facet_alloc(facet_size)
   , cell_alloc(sizeof(cell))
   , facets(&facet::list_ptrs)
   , columns(col_ruler::construct(n_vertices))
   , size_(0)
   , next_id(0)
{}

Table::Table(const Table& c)
   : facet_alloc(c.facet_alloc.get_object_size())
   , cell_alloc(sizeof(cell))
   , facets(&facet::list_ptrs)
   , size_(c.size_)
   , next_id(c.next_id)
{
   for (const_iterator f = c.begin(), fe = c.end();  f != fe;  ++f)
      push_back_facet(new(facet_alloc.allocate()) facet(*f, cell_alloc));

   columns = col_ruler::construct(*c.columns);
}

void Table::push_back_new_facet(facet* f)
{
   f->id = new_id();
   push_back_facet(f);
   ++size_;
}

void Table::push_back_facet(facet* f)
{
   f->list_ptrs.next = end_facet();
   f->list_ptrs.prev = facets.prev;
   facets.prev->list_ptrs.next = f;
   facets.prev = f;
}

void Table::erase_facet(const facet& f)
{
   facet* fp = const_cast<facet*>(&f);
   fp->unlink_cells(cell_alloc);
   facet* prev = fp->list_ptrs.prev;
   facet* next = fp->list_ptrs.next;
   prev->list_ptrs.next = next;
   next->list_ptrs.prev = prev;
   facet_alloc.reclaim(fp);
   --size_;
}

void Table::clear()
{
   facet_alloc.clear();
   cell_alloc.clear();
   size_ = 0;
   facets.next = facets.prev = end_facet();
   columns = col_ruler::resize(columns, 0);
}

void Table::clear_facets()
{
   facet_alloc.clear();
   cell_alloc.clear();
   size_ = 0;
   facets.next = facets.prev = end_facet();
   next_id = 0;
   for (auto col_it = entire(*columns); !col_it.at_end();  ++col_it) {
      col_it->first_col = nullptr;
      col_it->first_lex = nullptr;
   }
}

#if POLYMAKE_DEBUG

namespace {

PlainPrinter<>& show_facet(PlainPrinter<>& os, const facet& f, bool& shown)
{
   if (!shown) {
      shown = true;
      os << "Table::check - facet " << reinterpret_cast<const Facet&>(f) << endl;
   }
   return os;
}

}

bool Table::sanity_check() const
{
   bool OK = true;
   Int n_f = 0;
   std::vector<Int> hist(n_vertices());
   for (const_iterator f = begin(), fe = end();  f != fe;  ++f, ++n_f) {
      Int n_e = 0;
      Int prev_i = -1;
      bool shown = false, lex_prev_seen = false;
      for (auto e = entire(*f); !e.at_end(); ++e, ++n_e) {
         Int i = e.index();
         if (i <= prev_i)
            show_facet(cerr, *f, shown) << "order violation" << endl;
         prev_i = i;
         ++hist[i];

         if (e->col.prev == nullptr)
            show_facet(cerr, *f, shown) << "col.prev[" << i << "]=NULL" << endl;
         else if (e->col.prev->col.next != e.operator->())
            show_facet(cerr, *f, shown) << "col.prev[" << i << "] mismatch" << endl;
         else if (e->col.next != nullptr && e->col.next->col.prev != e.operator->())
            show_facet(cerr, *f, shown) << "col.next[" << i << "] mismatch" << endl;

         if (e->lex.prev != nullptr && e->lex.prev->lex.next != e.operator->())
            show_facet(cerr, *f, shown) << "lex.prev[" << i << "] mismatch" << endl;
         else if (e->lex.next && e->lex.next->lex.prev != e.operator->())
            show_facet(cerr, *f, shown) << "lex.next[" << i << "] mismatch" << endl;

         if (e->lex.prev != nullptr) {
            if (lex_prev_seen)
               show_facet(cerr, *f, shown) << "lex.prev[" << i << "] multiple" << endl;
            else
               lex_prev_seen=true;
         }
         if (e->lex.next != nullptr) {
            if (!lex_prev_seen) {
               show_facet(cerr, *f, shown) << "lex.next[" << i << "] before lex.prev" << endl;
            } else {
               cell_iterator<&cell::lex> ci(e->lex.next);
               if (reinterpret_cast<const Facet&>(*f) >= reinterpret_cast<const Facet&>(*ci.get_facet()))
                  show_facet(cerr, *f, shown) << "lexical order violation with "
                                              << reinterpret_cast<const Facet&>(*ci.get_facet()) << endl;
               else if (incl(reinterpret_cast<const Facet&>(*f), reinterpret_cast<const Facet&>(*ci.get_facet())) != 2)
                  show_facet(cerr, *f, shown) << "inclusion independence violated with "
                                              << reinterpret_cast<const Facet&>(*ci.get_facet()) << endl;
            }
         }
      }
      if (n_e != f->size())
         show_facet(cerr, *f, shown) << "size mismatch: " << n_e << "!=" << f->size() << endl;

      if (shown) OK = false;
   }

   if (n_f != size_) {
      OK = false;
      cerr << "Table::check - total size violation: " << n_f << "!=" << size_ << endl;
   }
   for (auto ci = entire(*columns); !ci.at_end(); ++ci) {
      Int n_e = 0;
      for (auto e = entire(*ci); !e.at_end(); ++e, ++n_e) ;
      if (n_e != hist[ci->vertex]) {
         cerr << "Table::check - column counter[" << ci->vertex << "] mismatch: " << n_e << "!=" << hist[ci->vertex] << endl;
         OK = false;
      }
      for (cell_iterator<&cell::lex> e(ci->first_lex); !e.at_end(); ++e) {
         if (e->facet.prev != e.get_facet()->head_cell()) {
            cerr << "Table::check - first_lex[" << ci->vertex << "] linked with "
                 << reinterpret_cast<const Facet&>(*e.get_facet()) << endl;
            OK = false;
         }
      }
   }
   return OK;
}
#endif

} // end namespace pm::fl_internal

#if POLYMAKE_DEBUG
bool FacetList::sanity_check() const { return table->sanity_check(); }
void FacetList::dump() const { cerr << *this << std::flush; }
#endif

}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
