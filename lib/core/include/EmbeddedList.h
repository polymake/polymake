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

#ifndef POLYMAKE_EMBEDDED_LIST_H
#define POLYMAKE_EMBEDDED_LIST_H

#include "polymake/internal/iterators.h"
#include <cassert>

namespace pm {

template <typename Class> struct ptr_pair;
template <typename Class, ptr_pair<Class> Class::* Ptrs> class EmbeddedList;
template <typename Class, ptr_pair<Class> Class::* Ptrs, bool _is_const, bool _rev> class embedded_list_iterator;

template <typename Class>
struct ptr_pair {
   Class* prev;
   Class* next;

   ptr_pair() : prev(NULL), next(NULL) {}

   explicit ptr_pair(Class* self) : prev(self), next(self) {}

   explicit ptr_pair(ptr_pair Class::*pptr)
   {
      prev=next=reverse_cast(this, pptr);
   }

   void exclude() { prev=NULL; next=NULL; }

   bool is_member() const { return next != NULL; }
};

template <typename Class, ptr_pair<Class> Class::* Ptrs, bool _is_const, bool _rev>
class embedded_list_iterator {
public:
   typedef bidirectional_iterator_tag iterator_category;
   typedef Class value_type;
   typedef typename assign_const<Class, _is_const>::type& reference;
   typedef typename assign_const<Class, _is_const>::type* pointer;
   typedef ptrdiff_t difference_type;
   typedef embedded_list_iterator<Class,Ptrs,false,_rev> iterator;
   typedef embedded_list_iterator<Class,Ptrs,true,_rev> const_iterator;

   embedded_list_iterator() {}
   embedded_list_iterator(pointer arg) : cur(arg) {}
   embedded_list_iterator(const iterator& it) : cur(it.cur) {}
   embedded_list_iterator(const embedded_list_iterator<Class,Ptrs,_is_const,!_rev>& it) : cur(it.cur) {}
   embedded_list_iterator(typename assign_const<embedded_list_iterator<Class,Ptrs,false,!_rev>, _is_const>::type& it) : cur(it.cur) {}

   embedded_list_iterator& operator= (const iterator& it) { cur=it.cur; return *this; }
   embedded_list_iterator& operator= (const embedded_list_iterator<Class,Ptrs,_is_const,!_rev>& it) { cur=it.cur; return *this; }
   embedded_list_iterator& operator= (typename assign_const<embedded_list_iterator<Class,Ptrs,false,!_rev>, _is_const>::type& it) { cur=it.cur; return *this; }

   reference operator* () const { return *cur; }
   pointer operator-> () const { return cur; }

   embedded_list_iterator& operator++ () { cur=_rev ? (cur->*Ptrs).prev : (cur->*Ptrs).next; return *this; }
   embedded_list_iterator& operator-- () { cur=_rev ? (cur->*Ptrs).next : (cur->*Ptrs).prev; return *this; }
   const embedded_list_iterator operator++ (int) { embedded_list_iterator copy(*this); operator++(); return copy; }
   const embedded_list_iterator operator-- (int) { embedded_list_iterator copy(*this); operator--(); return copy; }

   template <bool _is_const2, bool _rev2>
   bool operator== (const embedded_list_iterator<Class,Ptrs,_is_const2,_rev2>& it) const { return cur==it.cur; }
   template <bool _is_const2, bool _rev2>
   bool operator!= (const embedded_list_iterator<Class,Ptrs,_is_const2,_rev2>& it) const { return cur!=it.cur; }
protected:
   pointer cur;
   template <typename Class2, ptr_pair<Class2> Class2::* Ptrs2, bool,bool> friend class embedded_list_iterator;
};

template <typename Class, ptr_pair<Class> Class::* Ptrs>
class EmbeddedList {
protected:
   ptr_pair<Class> head;

   Class* end_node() const { return const_cast<Class*>(reverse_cast(&head, Ptrs)); }
public:
   typedef Class value_type;
   typedef Class& reference;
   typedef const Class& const_reference;

   EmbeddedList() : head(end_node()) {}

   typedef embedded_list_iterator<Class,Ptrs,false,false> iterator;
   typedef embedded_list_iterator<Class,Ptrs,true,false> const_iterator;
   typedef embedded_list_iterator<Class,Ptrs,false,true> reverse_iterator;
   typedef embedded_list_iterator<Class,Ptrs,true,true> const_reverse_iterator;

   iterator begin() { return head.next; }
   iterator end() { return end_node(); }
   const_iterator begin() const { return head.next; }
   const_iterator end() const { return end_node(); }

   reverse_iterator rbegin() { return head.prev; }
   reverse_iterator rend() { return end_node(); }
   const_reverse_iterator rbegin() const { return head.prev; }
   const_reverse_iterator rend() const { return end_node(); }

   reference front() { return *head.next; }
   reference back() { return *head.prev; }
   const_reference front() const { return *head.next; }
   const_reference back() const { return *head.prev; }

   bool empty() const { return head.next==end_node(); }
   int size() const { return count_it(entire(*this)); }

   void push_front(Class& obj)
   {
      Class *const first=head.next;
      if (first==&obj) return;
      if ((obj.*Ptrs).is_member()) _remove(&obj);
      head.next=&obj;
      (first->*Ptrs).prev=&obj;
      (obj.*Ptrs).next=first;
      (obj.*Ptrs).prev=end_node();
   }

   void push_back(Class& obj)
   {
      Class *const last=head.prev;
      if (last==&obj) return;
      if ((obj.*Ptrs).is_member()) _remove(&obj);
      head.prev=&obj;
      (last->*Ptrs).next=&obj;
      (obj.*Ptrs).prev=last;
      (obj.*Ptrs).next=end_node();
   }

   void pop_front()
   {
      Class *const lost=head.next;
      Class *const first=(lost->*Ptrs).next;
      head.next=first;
      (first->*Ptrs).prev=end_node();
      (lost->*Ptrs).exclude();
   }

   void pop_back()
   {
      Class *const lost=head.prev;
      Class *const last=(lost->*Ptrs).prev;
      head.prev=last;
      (last->*Ptrs).next=end_node();
      (lost->*Ptrs).exclude();
   }

   iterator insert(const iterator& where, Class& obj)
   {
      assert(!(obj.*Ptrs).is_member());
      Class *const next=where.operator->();
      Class *const prev=(next->*Ptrs).prev;
      (next->*Ptrs).prev=&obj;
      (prev->*Ptrs).next=&obj;
      (obj.*Ptrs).next=next;
      (obj.*Ptrs).prev=prev;
      return &obj;
   }

protected:
   Class* _remove(Class *lost)
   {
      Class *const next=(lost->*Ptrs).next;
      Class *const prev=(lost->*Ptrs).prev;
      (next->*Ptrs).prev=prev;
      (prev->*Ptrs).next=next;
      return next;
   }
public:
   void remove(Class& lost)
   {
      _remove(&lost);
      (lost.*Ptrs).exclude();
   }

   iterator erase(const iterator& where)
   {
      Class *const lost=where.operator->();
      Class *const next=_remove(lost);
      (lost->*Ptrs).exclude();
      return next;
   }

   void clear()
   {
      for (iterator it=begin(), e=end(); it!=e; ) {
         Class& obj=*it; ++it;
         (obj.*Ptrs).exclude();
      }
      head.next=head.prev=end_node();
   }

   void swap(EmbeddedList& l2)
   {
      Class *const f1=head.next;
      Class *const b1=head.prev;
      Class *const f2=l2.head.next;
      Class *const b2=l2.head.prev;
      Class *const e1=end_node();
      Class *const e2=l2.end_node();
      if (f1 != e1) {
         (f1->*Ptrs).prev=(b1->*Ptrs).next=e2;
         l2.head.next=f1; l2.head.prev=b1;
      } else {
         l2.head.next=l2.head.prev=e2;
      }
      if (f2 != e2) {
         (f2->*Ptrs).prev=(b2->*Ptrs).next=e1;
         head.next=f2; head.prev=b2;
      } else {
         head.next=head.prev=e1;
      }
   }
};

template <typename Class, ptr_pair<Class> Class::* Ptrs>
struct spec_object_traits< EmbeddedList<Class,Ptrs> > : spec_object_traits<is_container> {
   static const IO_separator_kind IO_separator=IO_sep_inherit;
};

} // end namespace pm

namespace std {
   template <typename Class, pm::ptr_pair<Class> Class::* Ptrs>
   void swap(pm::EmbeddedList<Class,Ptrs>& l1, pm::EmbeddedList<Class,Ptrs>& l2) { l1.swap(l2); }
}

namespace polymake {
   using pm::EmbeddedList;
}

#endif // POLYMAKE_EMBEDDED_LIST_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
