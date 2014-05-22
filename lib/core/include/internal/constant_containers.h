/* Copyright (c) 1997-2014
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

#ifndef POLYMAKE_INTERNAL_CONSTANT_CONTAINERS_H
#define POLYMAKE_INTERNAL_CONSTANT_CONTAINERS_H

#include "polymake/internal/shared_object.h"

#include <cassert>

namespace pm {

template <typename T>
class single_value_iterator {
protected:
   alias<T> value;
   bool _at_end;

   static const bool is_const=!attrib<T>::is_reference || attrib<T>::is_const;
   template <typename> friend class single_value_iterator;
public:
   typedef random_access_iterator_tag iterator_category;
   typedef typename deref<T>::type value_type;
   typedef typename assign_const<value_type, is_const>::type& reference;
   typedef typename assign_const<value_type, is_const>::type* pointer;
   typedef ptrdiff_t difference_type;
   typedef typename if_else<attrib<T>::is_reference, single_value_iterator<typename attrib<T>::minus_const>,
                                                     single_value_iterator>::type
      iterator;
   typedef typename if_else<attrib<T>::is_reference, single_value_iterator<typename attrib<T>::plus_const>,
                                                     single_value_iterator>::type
      const_iterator;

   typedef typename if_else<identical<iterator, single_value_iterator>::value, type2type<T>,
                            const alias<typename attrib<T>::minus_const> >::type
      alt_alias_arg_type;
   typedef single_value_iterator<typename attrib<T>::toggle_const> cmp_iterator;

   single_value_iterator() : _at_end(true) {}
   single_value_iterator(typename alias<T>::arg_type arg)
      : value(arg), _at_end(false) {}
   single_value_iterator(const alias<T>& arg)
      : value(arg), _at_end(false) {}
   single_value_iterator(alt_alias_arg_type& arg)
      : value(arg), _at_end(false) {}
   single_value_iterator(const iterator& it)
      : value(it.value), _at_end(it._at_end) {}

   reference operator* () const { return *value; }
   pointer operator-> () const { return &operator*(); }

   single_value_iterator& operator++ () { _at_end^=1; return *this; }
   const single_value_iterator operator++ (int) { single_value_iterator copy=*this; operator++(); return copy; }
   single_value_iterator& operator-- () { return operator++(); }
   const single_value_iterator operator-- (int) { single_value_iterator copy=*this; operator--(); return copy; }

   single_value_iterator& operator+= (int i) { _at_end^=i&1; return *this; }
   single_value_iterator& operator-= (int i) { return operator+=(i); }
   const single_value_iterator operator+ (int i) const { single_value_iterator copy=*this; return copy+=i; }
   friend const single_value_iterator operator+ (int i, const single_value_iterator& me) { return me+i; }
   const single_value_iterator operator- (int i) const { single_value_iterator copy=*this; return copy+=i; }

   difference_type operator- (const single_value_iterator& it) const { return _at_end - it._at_end; }
   bool operator== (const single_value_iterator& it) const { return _at_end==it._at_end; }
   bool operator!= (const single_value_iterator& it) const { return !operator==(it); }
   bool operator< (const single_value_iterator& it) const { return _at_end < it._at_end; }
   bool operator> (const single_value_iterator& it) const { return it.operator<(*this); }
   bool operator<= (const single_value_iterator& it) const { return true; }
   bool operator>= (const single_value_iterator& it) const { return true; }

   difference_type operator- (const cmp_iterator& it) const { return _at_end - it._at_end; }
   bool operator== (const cmp_iterator& it) const { return _at_end==it._at_end; }
   bool operator!= (const cmp_iterator& it) const { return !operator==(it); }
   bool operator< (const cmp_iterator& it) const { return _at_end < it._at_end; }
   bool operator> (const cmp_iterator& it) const { return it.operator<(*this); }
   bool operator<= (const cmp_iterator& it) const { return true; }
   bool operator>= (const cmp_iterator& it) const { return true; }

   bool at_end() const { return _at_end; }
   int index() const { return 0; }
   void rewind() { _at_end=false; }
};

template <typename Iterator>
class single_position_iterator :
   public Iterator {
   template <typename> friend class single_position_iterator;
public:
   typedef bidirectional_iterator_tag iterator_category;
   typedef single_position_iterator<typename iterator_traits<Iterator>::iterator> iterator;
   typedef single_position_iterator<typename iterator_traits<Iterator>::const_iterator> const_iterator;

   single_position_iterator() : _at_end(-1) {}

   single_position_iterator(Iterator it) : Iterator(it), _at_end(-it.at_end()) {}

   single_position_iterator(const iterator& it) : Iterator(it), _at_end(it._at_end) {}

   single_position_iterator& operator++ () { ++_at_end; return *this; }
   single_position_iterator& operator-- () { --_at_end; return *this; }

   const single_position_iterator operator++ (int) { single_position_iterator copy(*this);  operator++();  return copy; }
   const single_position_iterator operator-- (int) { single_position_iterator copy(*this);  operator--();  return copy; }

   bool at_end() const { return _at_end; }

   bool operator== (const const_iterator& it) const { return _at_end == it._at_end; }
   bool operator== (const iterator& it)       const { return _at_end == it._at_end; }
   bool operator!= (const const_iterator& it) const { return !operator==(it); }
   bool operator!= (const iterator& it)       const { return !operator==(it); }

   void rewind() { _at_end &= ~1; }

protected:
   // 0: valid position, 1: after valid position, <0: no valid position at all (empty sequence)
   int _at_end;

private:
   // hide these if the original iterator is a random-access one
   void operator+=(int);
   void operator-=(int);
   void operator+ (int);
   void operator- (int);
   void operator[](int);
};

template <typename T>
class constant_value_iterator {
protected:
   mutable alias<T> value;

   static const bool is_const=!attrib<T>::is_reference || attrib<T>::is_const;
   template <typename> friend class constant_value_iterator;
public:
   typedef random_access_iterator_tag iterator_category;
   typedef typename deref<T>::type value_type;
   typedef typename assign_const<value_type, is_const>::type& reference;
   typedef typename assign_const<value_type, is_const>::type* pointer;
   typedef ptrdiff_t difference_type;
   typedef typename if_else<attrib<T>::is_reference, constant_value_iterator<typename attrib<T>::minus_const>,
                                                     constant_value_iterator>::type
      iterator;
   typedef typename if_else<attrib<T>::is_reference, constant_value_iterator<typename attrib<T>::plus_const>,
                                                     constant_value_iterator>::type
      const_iterator;

   typedef typename if_else<identical<iterator, constant_value_iterator>::value, type2type<T>,
                            const alias<typename attrib<T>::minus_const> >::type
      alt_alias_arg_type;
   typedef constant_value_iterator<typename attrib<T>::toggle_const> cmp_iterator;

   constant_value_iterator() {}
   constant_value_iterator(typename alias<T>::arg_type arg)
      : value(arg) {}
   constant_value_iterator(const alias<T>& arg)
      : value(arg) {}
   constant_value_iterator(alt_alias_arg_type& arg)
      : value(arg) {}
   constant_value_iterator(const iterator& it)
      : value(it.value) {}

   reference operator* () const { return *value; }
   pointer operator-> () const { return &operator*(); }
   reference operator[] (int) const { return *value; }

   constant_value_iterator& operator++ () { return *this; }
   const constant_value_iterator& operator++ (int) { return *this; }
   constant_value_iterator& operator-- () { return *this; }
   const constant_value_iterator& operator-- (int) { return *this; }
   constant_value_iterator& operator+= (int) { return *this; }
   constant_value_iterator& operator-= (int) { return *this; }
   const constant_value_iterator& operator+ (int) const { return *this; }
   const constant_value_iterator& operator- (int) const { return *this; }
   friend const constant_value_iterator& operator+ (int, const constant_value_iterator& me) { return me; }

   difference_type operator- (const constant_value_iterator& it) const { return this!=&it; }
   bool operator== (const constant_value_iterator& it) const { return this==&it; }
   bool operator!= (const constant_value_iterator& it) const { return !operator==(it); }
   bool operator< (const constant_value_iterator& it) const { return !operator==(it); }
   bool operator> (const constant_value_iterator& it) const { return !operator==(it); }
   bool operator<= (const constant_value_iterator& it) const { return true; }
   bool operator>= (const constant_value_iterator& it) const { return true; }

   difference_type operator- (const cmp_iterator& it) const { return reinterpret_cast<const cmp_iterator*>(this)!=&it; }
   bool operator== (const cmp_iterator& it) const { return reinterpret_cast<const cmp_iterator*>(this)==&it; }
   bool operator!= (const cmp_iterator& it) const { return !operator==(it); }
   bool operator< (const cmp_iterator& it) const { return !operator==(it); }
   bool operator> (const cmp_iterator& it) const { return !operator==(it); }
   bool operator<= (const cmp_iterator& it) const { return true; }
   bool operator>= (const cmp_iterator& it) const { return true; }

   void rewind() {}
   void contract(bool, int, int) {}
};

template <typename T>
class constant_pointer_iterator : public ptr_wrapper<T> {
   typedef ptr_wrapper<T> _super;
public:
   typedef constant_pointer_iterator<typename attrib<T>::minus_const> iterator;
   typedef constant_pointer_iterator<typename attrib<T>::plus_const> const_iterator;
   template <typename> friend class constant_pointer_iterator;

   constant_pointer_iterator() {}
   constant_pointer_iterator(T& arg) : _super(&arg) {}
   constant_pointer_iterator(const iterator& it) : _super(it.cur) {};

   constant_pointer_iterator& operator++ () { return *this; }
   const constant_pointer_iterator& operator++ (int) { return *this; }
   constant_pointer_iterator& operator-- () { return *this; }
   const constant_pointer_iterator& operator-- (int) { return *this; }
   constant_pointer_iterator& operator+= (int) { return *this; }
   constant_pointer_iterator& operator-= (int) { return *this; }
   const constant_pointer_iterator& operator+ (int) { return *this; }
   const constant_pointer_iterator& operator- (int) { return *this; }
   friend const constant_pointer_iterator& operator+ (int, const constant_pointer_iterator& me) { return me; }

   ptrdiff_t operator- (const constant_pointer_iterator& it) const { return (this->cur != 0) != (it.cur != 0); }
   void rewind() {}
   void contract(bool, int, int) {}
};

template <typename T>
struct check_iterator_feature<single_value_iterator<T>, end_sensitive> : True {};

template <typename T>
struct check_iterator_feature<single_value_iterator<T>, indexed> : True {};

template <typename T>
struct check_iterator_feature<single_value_iterator<T>, rewindable> : True {};

template <typename Iterator>
struct check_iterator_feature<single_position_iterator<Iterator>, end_sensitive> : True {};

template <typename Iterator>
struct check_iterator_feature<single_position_iterator<Iterator>, indexed> : check_iterator_feature<Iterator, indexed> {};

template <typename Iterator>
struct check_iterator_feature<single_position_iterator<Iterator>, rewindable> : True {};

template <typename T>
struct check_iterator_feature<constant_value_iterator<T>, unlimited> : True {};

template <typename T>
struct check_iterator_feature<constant_value_iterator<T>, contractable> : True {};

template <typename T>
struct check_iterator_feature<constant_value_iterator<T>, rewindable> : True {};

template <typename T>
struct check_iterator_feature<constant_pointer_iterator<T>, unlimited> : True {};

template <typename T>
struct check_iterator_feature<constant_pointer_iterator<T>, contractable> : True {};

template <typename T>
struct check_iterator_feature<constant_pointer_iterator<T>, rewindable> : True {};

template <typename T, bool _sparse=false>
class single_value_container : public alias<T> {
   typedef alias<T> _super;
protected:
   single_value_container() {}
public:
   typedef typename if_else<identical_minus_const_ref<T, typename _super::arg_type>::value,
                            type2type<T>&, typename attrib<T>::plus_const_ref>::type
      alt_arg_type;
   single_value_container(typename _super::arg_type arg) : _super(arg) {}
   single_value_container(alt_arg_type arg) : _super(arg) {}
protected:
   single_value_container(const _super& arg) : _super(arg) {}
public:
   typedef single_value_iterator<T> iterator;
   typedef typename iterator::const_iterator const_iterator;
   typedef iterator reverse_iterator;
   typedef const_iterator const_reverse_iterator;

   iterator begin() { return *this; }
   iterator end() { return iterator(); }
   iterator rbegin() { return *this; }
   iterator rend() { return iterator(); }

   const_iterator begin() const { return *this; }
   const_iterator end() const { return iterator(); }
   const_iterator rbegin() const { return *this; }
   const_iterator rend() const { return iterator(); }

   typename _super::reference front() { return _super::operator*(); }
   typename _super::reference back() { return front(); }
   typename _super::reference operator[] (int i) { assert(i==0); return front(); }
   typename _super::const_reference front() const { return _super::operator*(); }
   typename _super::const_reference back() const { return front(); }
   typename _super::const_reference operator[] (int i) const { assert(i==0); return front(); }

   bool empty() const { return false; }
   int size() const { return 1; }
   int max_size() const { return 1; }
};

template <typename T>
class single_value_container<T, true> : public single_value_container<T, false> {
   typedef single_value_container<T, false> _super;
   bool _empty;
public:
   single_value_container() : _empty(true) {}
   single_value_container(typename _super::arg_type arg) : _super(arg), _empty(false) {}
   single_value_container(typename _super::alt_arg_type arg) : _super(arg), _empty(false) {}

   typename _super::iterator begin()
   {
      if (_empty) return typename _super::iterator();
      return *this;
   }
   typename _super::const_iterator begin() const
   {
      if (_empty) return typename _super::const_iterator();
      return *this;
   }
   typename _super::iterator rbegin()
   {
      if (_empty) return typename _super::iterator();
      return *this;
   }
   typename _super::const_iterator rbegin() const
   {
      if (_empty) return typename _super::const_iterator();
      return *this;
   }

   bool empty() const { return _empty; }
   int size() const { return !_empty; }
   int dim() const { return 1; }
};

template <typename T, bool _sparse>
struct spec_object_traits< single_value_container<T,_sparse> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true, is_always_const=attrib<T>::is_const || !attrib<T>::is_reference;
};

template <typename E, bool _sparse>
struct check_container_feature<single_value_container<E,_sparse>, sparse> : bool2type<_sparse> {};

template <typename T>
struct check_container_feature<single_value_container<T,false>, provide_construction<rewindable,false> > : True {};

template <typename T>
class constant_value_container : public alias<T> {
   typedef alias<T> _super;
public:
   typedef typename if_else<identical_minus_const_ref<T, typename _super::arg_type>::value,
                            type2type<T>&, typename attrib<T>::plus_const_ref>::type
      alt_arg_type;
   constant_value_container(typename _super::arg_type arg) : _super(arg) {}
   constant_value_container(alt_arg_type arg) : _super(arg) {}
protected:
   constant_value_container(const _super& arg) : _super(arg) {}
public:
   typedef constant_value_iterator<T> iterator;
   typedef typename iterator::const_iterator const_iterator;
   typedef iterator reverse_iterator;
   typedef const_iterator const_reverse_iterator;

   typename _super::reference front() { return _super::operator*(); }
   typename _super::reference back() { return front(); }
   typename _super::reference operator[] (int) { return front(); }
   typename _super::const_reference front() const { return _super::operator*(); }
   typename _super::const_reference back() const { return front(); }
   typename _super::const_reference operator[] (int) const { return front(); }

   iterator begin() { return *this; }
   iterator end() { return iterator(); }
   iterator rbegin() { return *this; }
   iterator rend() { return iterator(); }

   const_iterator begin() const { return *this; }
   const_iterator end() const { return iterator(); }
   const_iterator rbegin() const { return *this; }
   const_iterator rend() const { return iterator(); }

   bool empty() const { return false; }
   int size() const { return std::numeric_limits<int>::max(); }
};

template <typename T>
class constant_masquerade_container {
protected:
   constant_masquerade_container();
   ~constant_masquerade_container();
public:
   typedef typename deref<T>::type value_type;
   typedef T& reference;
   typedef const T& const_reference;
   typedef constant_pointer_iterator<T> iterator;
   typedef constant_pointer_iterator<const T> const_iterator;
   typedef iterator reverse_iterator;
   typedef const_iterator const_reverse_iterator;

   iterator begin() { return front(); }
   iterator end() { return iterator(); }
   iterator rbegin() { return front(); }
   iterator rend() { return iterator(); }

   const_iterator begin() const { return front(); }
   const_iterator end() const { return const_iterator(); }
   const_iterator rbegin() const { return front(); }
   const_iterator rend() const { return const_iterator(); }

   reference front() { return reinterpret_cast<reference>(*this); }
   reference back() { return front(); }
   reference operator[] (int) { return front(); }

   const_reference front() const { return reinterpret_cast<const_reference>(*this); }
   const_reference back() const { return front(); }
   const_reference operator[] (int) const { return front(); }

   bool empty() const { return false; }
   int size() const { return std::numeric_limits<int>::max(); }
};

template <typename T>
struct spec_object_traits< constant_value_container<T> >
   : spec_object_traits<is_container> {
   static const bool
      is_temporary=true,
      is_always_const=attrib<T>::is_const || !attrib<T>::is_reference || object_traits<typename deref<T>::type>::is_always_const;
};

template <typename T>
struct spec_object_traits< constant_masquerade_container<T> >
   : spec_object_traits<is_container> {
   static const bool is_always_const=attrib<T>::is_const || object_traits<T>::is_always_const;
   typedef T masquerade_for;
};

template <typename T>
struct check_container_feature<constant_value_container<T>, provide_construction<rewindable,false> > : True {};

template <typename T>
struct check_container_feature<constant_masquerade_container<T>, provide_construction<rewindable,false> > : True {};

template <typename T> inline
single_value_container<T&>
item2container(T& x)
{
   return x;
}
template <typename T> inline
const single_value_container<const T&>
item2container(const T& x)
{
   return x;
}
template <typename T> inline
single_value_container<T>& item2container(alias<T>& x)
{
   return static_cast<single_value_container<T>&>(x);
}
template <typename T> inline
const single_value_container<T>& item2container(const alias<T>& x)
{
   return static_cast<const single_value_container<T>&>(x);
}

template <typename T> inline
constant_value_container<const typename Concrete<T>::type>
constant(const T& x)
{
   return concrete(x);
}

template <typename T> inline
const constant_value_container<T>&
constant(const alias<T>& x)
{
   return static_cast<const constant_value_container<T>&>(x);
}

template <typename E>
class single_elem_composite {
protected:
   ~single_elem_composite();
};

template <typename E>
struct spec_object_traits< single_elem_composite<E> >
   : spec_object_traits<is_composite> {
   typedef E elements;
   typedef E masquerade_for;

   template <typename Me, typename Visitor>
   static void visit_elements(Me& me, Visitor& v)
   {
      v << reinterpret_cast<typename inherit_const<E, Me>::type&>(me);
   }
};

template <typename E> inline
single_elem_composite<E>& item2composite(E& x)
{
   return reinterpret_cast<single_elem_composite<E>&>(x);
}

template <typename E> inline
const single_elem_composite<E>& item2composite(const E& x)
{
   return reinterpret_cast<const single_elem_composite<E>&>(x);
}

namespace object_classifier {
   enum { is_constant=is_manip+1 };

   namespace _impl {
      template <typename T>
      size_discriminant<is_constant>::type analyzer_f(const constant_value_container<T>*, bait*);

      template <typename T>
      size_discriminant<is_constant>::type analyzer_f(const constant_masquerade_container<T>*, bait*);
   }
}

template <typename T>
struct spec_object_traits< cons<T, int2type<object_classifier::is_constant> > >
   : spec_object_traits<is_container> {};

} // end namespace pm

namespace polymake {
   using pm::item2container;
   using pm::item2composite;
   using pm::constant;
}

#endif // POLYMAKE_INTERNAL_CONSTANT_CONTAINERS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
