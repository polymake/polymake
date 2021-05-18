/* Copyright (c) 1997-2021
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

   Author: Julian Pfeifle <julian.pfeifle@upc.edu>
--------------------------------------------------------------------------------
*/

#pragma once

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Map.h"
#include "polymake/Graph.h"
#include <vector>

namespace polymake { namespace graph {

      //The implementation is adapted from the dancing links implementation by
      //Julian Pfeifle, https://github.com/julian-upc/2015-algorithms/tree/master/dancing_links/c%2B%2B
      //
      //Donald E. Knuth, Dancing Links, arXiv:cs/0011047
      //Donald E. Knuth, The Art of Computer Programming, Volume 4, Fascicle 4, 24-31, 2006, Pearson Education Inc.





class ArcLinking {
private:
   // Properties that all cells have in common
   class IncidenceCellBase {
      friend class ArcLinking;
   public:
      IncidenceCellBase* up;
      IncidenceCellBase* down;
      Int id;
      Int tip;

      IncidenceCellBase() {}

      IncidenceCellBase(IncidenceCellBase* up_,
                        IncidenceCellBase* down_,
                        const Int id_,
                        const Int tip_)
         : up(up_)
         , down(down_)
         , id(id_)
         , tip(tip_)
      {}

      IncidenceCellBase(const Int id_, const Int tip_)
         : up(this)
         , down(this)
         , id(id_)
         , tip(tip_)
      {}

      IncidenceCellBase(const IncidenceCellBase&) = delete;
      IncidenceCellBase& operator=(const IncidenceCellBase&) = delete;
   };

public:
   class ColumnObject;

   // Cells representing arcs
   class IncidenceCell : public IncidenceCellBase {
      friend class ArcLinking;
   public:
      IncidenceCell* left;
      IncidenceCell* right;
      IncidenceCell* link;

      ColumnObject* list_header;

      IncidenceCell() {}

      IncidenceCell(IncidenceCell* left_,
                    IncidenceCell* right_,
                    IncidenceCellBase* up_,
                    IncidenceCellBase* down_,
                    ColumnObject* list_header_,
                    const Int id_,
                    const Int tip_)
         : IncidenceCellBase(up_, down_, id_, tip_)
         , left(left_)
         , right(right_)
         , link(nullptr)
         , list_header(list_header_)
      {}

      IncidenceCell(const IncidenceCell&) = delete;
      IncidenceCell &operator=(const IncidenceCell&) = delete;
   };

   // Bidirectional iterator traversing the IncidenceCells to which the ColumnObject points
   // While this iterator lives, changes to up and down pointers have to be made with caution (see definition of end()).
   class ColumnIterator {
      friend class ArcLinking;
   protected:
      const IncidenceCellBase& col;
      IncidenceCellBase* current;

   public:
      typedef std::bidirectional_iterator_tag iterator_category;
      typedef IncidenceCellBase* value_type;
      typedef value_type& reference;
      typedef value_type* pointer;
      typedef ptrdiff_t difference_type;

      typedef ColumnIterator iterator;

      ColumnIterator(ColumnObject& col_arg);

      reference operator* () { return current; }
      pointer operator-> () { return &current; }

      iterator& operator++ ()
      {
         current = current->down;
         return *this;
      }
      const iterator operator++ (int) { iterator copy(*this);  operator++();  return copy; }

      iterator& operator-- ()
      {
         current = current->up;
         return *this;
      }

      const iterator operator-- (int) { iterator copy(*this);  operator--();  return copy; }

      bool operator== (const iterator& it) const
      {
         return (current == it.current);
      }
      bool operator!= (const iterator& it) const { return !operator==(it); }
   };

   // Header cells functioning as anchors for outgoing arcs
   class ColumnObject : public IncidenceCellBase {
      friend class ColumnIterator;
      friend class ArcLinking;
   public:
      typedef ColumnIterator iterator;

      ColumnObject* left;
      ColumnObject* right;
      Int size;
      ColumnObject(ColumnObject* left_,
                   ColumnObject* right_,
                   IncidenceCellBase* up_,
                   IncidenceCellBase* down_,
                   const Int id_)
         : IncidenceCellBase(up_, down_, id_, -1)
         , left(left_)
         , right(right_)
         , size(0)
      {}

      ColumnObject(const Int id_)
         : IncidenceCellBase(id_, -1)
         , left(this)
         , right(this)
         , size(0)
      {}

      ColumnObject(const ColumnObject&) = delete;

      ColumnObject &operator=(const ColumnObject&) = delete;

      iterator begin() { return iterator( *this ); }

      iterator end() { return --iterator( *this ); }

      iterator rbegin() { return ----iterator( *this ); }

      iterator rend() { return --iterator( *this ); }
   };

private:
   ColumnObject* h;
   Int rows;
   Map<Int, ColumnObject*> column_object_of_id;

public:
   ArcLinking()
      : h(new ColumnObject(-1))
      , rows(0)
      , column_object_of_id()
   {
      column_object_of_id[-1] = h;
   }

   explicit ArcLinking(const std::vector<Int>& ids)
      : ArcLinking()
   {
      append_column_objects(ids);
   }

   explicit ArcLinking(Int n) : ArcLinking()
   {
      std::vector<Int> ids;
      for (Int i = 0; i < n; ++i)
         ids.push_back(i);
      append_column_objects(ids);
   }

   ArcLinking(const Graph<Undirected>& G, Array<IncidenceCell*>& a ) : ArcLinking(G.nodes())
   {
      Int i = 0;
      for (auto eit = entire(edges(G)); !eit.at_end(); ++eit, ++i) {
         std::vector<std::tuple<Int, Int, Int>> row;
         row.push_back(std::make_tuple(eit.to_node(), i, eit.from_node()));
         row.push_back(std::make_tuple(eit.from_node(), i, eit.to_node()));
         a[i] = append_row(row);
      }
   }

   ArcLinking(const ArcLinking&) = delete;

   ArcLinking &operator=(const ArcLinking&) = delete;

   //appends column objects to the right
   //the ids must be unique
   void append_column_objects(const std::vector<Int>& ids) {
      ColumnObject* current_column_object{h};
      for (const auto& i : ids) {
         insert_column_object(current_column_object, h, i);
         current_column_object = current_column_object->right;
         column_object_of_id[i] = current_column_object;
      }
   }

   void insert_column_object(ColumnObject* left,
                             ColumnObject* right,
                             const Int id) {
      ColumnObject* x = new ColumnObject(id);
      x->left = left;
      x->right = right;
      x->up = x;
      x->down = x;
      right->left = x;
      left->right = x;
      ++h->size;
   }

   //appends a row of IncidenceCells, where each tuple in the vector has the form <list_header,id,tip>
   IncidenceCell* append_row(const std::vector<std::tuple<Int, Int, Int>>& elements) {
      auto eit = elements.cbegin();
      ColumnObject* row_header_col = column_object_of_id[std::get<0>(*eit)];
      IncidenceCell* row_header = new IncidenceCell(nullptr,
                                                    nullptr,
                                                    row_header_col->up,
                                                    row_header_col,
                                                    row_header_col,
                                                    std::get<1>(*eit),
                                                    std::get<2>(*eit));
      row_header->left = row_header->right = row_header;
      row_header_col->up->down = row_header;
      row_header_col->up = row_header;
      ++row_header_col->size;
      while (++eit != elements.cend()) {
         ColumnObject* list_header = column_object_of_id[std::get<0>(*eit)];
         IncidenceCell* x = new IncidenceCell(
            row_header->left,
            row_header,
            list_header->up,
            list_header,
            list_header,
            std::get<1>(*eit),
            std::get<2>(*eit));
         x->up->down = x->down->up = x->right->left = x->left->right = x;
         ++list_header->size;
      }
      ++rows;
      return row_header;
   }

   //before calling the destructor, the initial linking must be restored
   ~ArcLinking() {
      for (auto column_it = entire(column_object_of_id); !column_it.at_end(); ++column_it) {
         IncidenceCellBase* current_cell = (*column_it).second->down;
         IncidenceCellBase* next = nullptr;
         while (current_cell != (*column_it).second) {
            next = current_cell->down;
            delete static_cast<IncidenceCell*>(current_cell);
            current_cell = next;
         }
         delete (*column_it).second;
      }
   }

   ColumnObject* get_column_object(Int i) const
   {
      return column_object_of_id[i];
   }

   Int get_rows() const
   {
      return rows;
   }

   static Set<Int> ids_of_column(ColumnObject* c)
   {
      Set<Int> ids;
      for (auto it = c->begin(); it != c->end(); ++it)
         ids += (*it)->id;
      return ids;
   }

   static IncidenceCell* reverse(IncidenceCell* i)
   {
      return i->right;
   }

   static void delete_incidenceCell(IncidenceCell* i)
   {
      i->up->down = i->down;
      i->down->up = i->up;
      --i->list_header->size;
   }

   static void undelete_incidenceCell(IncidenceCell* i)
   {
      i->up->down = i;
      i->down->up = i;
      ++i->list_header->size;
   }

   static void delete_row_exclusive(IncidenceCell* r)
   {
      IncidenceCell* i = r->right;
      while (i != r) {
         delete_incidenceCell(i);
         i = i->right;
      }
   }

   static void undelete_row_exclusive(IncidenceCell* r)
   {
      IncidenceCell* i = r->left;
      while (i != r) {
         undelete_incidenceCell(i);
         i = i->left;
      }
   }

   static void delete_row(IncidenceCell* r)
   {
      delete_incidenceCell(r);
      delete_row_exclusive(r);
   }

   static void undelete_row(IncidenceCell* r)
   {
      undelete_row_exclusive(r);
      undelete_incidenceCell(r);
   }

   //deletes all arcs between u to v, bends all arcs with tip being u to v and hangs the list of u into v
   IncidenceCell* contract_edge(ColumnObject* u, ColumnObject* v) {
      IncidenceCell* g = nullptr;
      for (auto it = u->begin(); it != u->end(); ++it) {
         if ((*it)->tip == v->id) {
               delete_row(static_cast<IncidenceCell*>(*it));
               static_cast<IncidenceCell*>(*it)->link = g;
               g = static_cast<IncidenceCell*>(*it);
         } else {
            reverse(static_cast<IncidenceCell*>(*it))->tip = v->id;
         }
      }
      hang_in(u,v);
      return g;
   }

   //reverts contract_edge()
   void expand_edge(ColumnObject* u, ColumnObject* v, IncidenceCell* l) {
      hang_out(u,v);
      for (auto it = u->rbegin(); it != u->rend(); --it) {
         reverse(static_cast<IncidenceCell*>(*it))->tip = u->id;
      }
      while (l != nullptr) {
         undelete_row(reverse(l));
         l = l->link;
      }
   }

   void hang_in(ColumnObject* a, ColumnObject* b) {
      a->up->down = b->down;
      b->down->up = a->up;
      a->down->up = b;
      b->down = a->down;
      b->size += a->size;
   }

   void hang_out(ColumnObject* a, ColumnObject* b) {
      b->down = a->up->down;
      b->down->up = b;
      a->up->down = a;
      a->down->up = a;
      b->size -= a->size;
   }

   void cover_column(ColumnObject* c) {
      c->right->left = c->left;
      c->left->right = c->right;
      for (auto i = c->begin(); i != c->end(); ++i) {
         delete_row_exclusive(static_cast<IncidenceCell*>(*i));
      }
   }

   void uncover_column(ColumnObject* c) {
      for (auto i = c->rbegin(); i != c->rend(); --i) {
         delete_row_exclusive(static_cast<IncidenceCell*>(*i));
      }
      c->right->left = c;
      c->left->right = c;
   }
};

ArcLinking::ColumnIterator::ColumnIterator(ColumnObject& col_arg)
      : col(col_arg), current(col_arg.down)
   {}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

