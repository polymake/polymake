/* Copyright (c) 1997-2019
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

#ifndef POLYMAKE_SINGULAR_CONSTANT_CONTAINERS_H
#define POLYMAKE_SINGULAR_CONSTANT_CONTAINERS_H

#include "polymake/internal/shared_object.h"

namespace pm {

template <typename TRef>
class same_value_iterator {
protected:
   // TODO: replace with a plain pointer when iterators stop outliving containers
   using alias_t = alias<TRef>;
   mutable alias_t value;

   template <typename> friend class same_value_iterator;
public:
   using iterator_category = random_access_iterator_tag;
   using value_type = typename alias_t::value_type;
   using reference = typename alias_t::reference;
   using pointer = typename alias_t::pointer;
   using difference_type = ptrdiff_t;

   using iterator = same_value_iterator<typename attrib<TRef>::minus_const>;
   using const_iterator = same_value_iterator<std::conditional_t<is_const<reference>::value, TRef, typename attrib<TRef>::plus_const>>;

   same_value_iterator() = default;

   template <typename AliasArg, typename=std::enable_if_t<std::is_constructible<alias_t, AliasArg>::value>>
   explicit same_value_iterator(AliasArg&& arg)
      : value(std::forward<AliasArg>(arg)) {}

   template <typename OtherRef, typename=std::enable_if_t<std::is_constructible<alias_t, const alias<OtherRef>&>::value>>
   explicit same_value_iterator(const same_value_iterator<OtherRef>& other)
      : value(other.value) {}

   template <typename OtherRef, typename=std::enable_if_t<std::is_constructible<alias_t, alias<OtherRef>&&>::value>>
   explicit same_value_iterator(same_value_iterator<OtherRef>&& other)
      : value(std::move(other.value)) {}

   same_value_iterator(const same_value_iterator&) = default;
   same_value_iterator(same_value_iterator&&) = default;
   same_value_iterator& operator= (const same_value_iterator&) = default;

   reference operator* () const { return *value; }
   pointer operator-> () const { return value.operator->(); }

   reference operator[] (int) const { return *value; }

   same_value_iterator& operator++ () { return *this; }
   const same_value_iterator& operator++ (int) { return *this; }
   same_value_iterator& operator-- () { return *this; }
   const same_value_iterator& operator-- (int) { return *this; }
   same_value_iterator& operator+= (int) { return *this; }
   same_value_iterator& operator-= (int) { return *this; }
   const same_value_iterator& operator+ (int) const { return *this; }
   const same_value_iterator& operator- (int) const { return *this; }
   friend const same_value_iterator& operator+ (int, const same_value_iterator& me) { return me; }

   difference_type operator- (const same_value_iterator& it) const { return this!=&it; }
   bool operator== (const same_value_iterator& it) const { return this==&it; }
   bool operator!= (const same_value_iterator& it) const { return !operator==(it); }
   bool operator< (const same_value_iterator& it) const { return !operator==(it); }
   bool operator> (const same_value_iterator& it) const { return !operator==(it); }
   bool operator<= (const same_value_iterator& it) const { return true; }
   bool operator>= (const same_value_iterator& it) const { return true; }

   void rewind() {}
   void contract(bool, int, int) {}
};


template <typename Iterator>
class single_position_iterator
   : public Iterator {
   using base_t = Iterator;
   template <typename> friend class single_position_iterator;
public:
   using iterator_category = bidirectional_iterator_tag;
   using iterator = single_position_iterator<typename iterator_traits<Iterator>::iterator>;
   using const_iterator = single_position_iterator<typename iterator_traits<Iterator>::const_iterator>;

   single_position_iterator()
      : single_pos(-1) {}

   template <typename SourceIterator, typename=typename suitable_arg_for_iterator<SourceIterator, Iterator>::type>
   single_position_iterator(SourceIterator&& it)
      : base_t(prepare_iterator_arg<Iterator>(std::forward<SourceIterator>(it)))
      , single_pos(it.at_end()) {}

   single_position_iterator(const iterator& it)
      : Iterator(static_cast<const typename iterator::base_t&>(it))
      , single_pos(it.single_pos) {}

   single_position_iterator& operator++ ()
   {
      if (POLYMAKE_DEBUG && single_pos != 0)
         throw std::runtime_error("single_position_iterator - increment out of range");
      ++single_pos;
      return *this;
   }

   single_position_iterator& operator-- ()
   {
      if (POLYMAKE_DEBUG && single_pos != 1)
         throw std::runtime_error("single_position_iterator - increment out of range");
      --single_pos;
      return *this;
   }

   const single_position_iterator operator++ (int) { single_position_iterator copy(*this);  operator++();  return copy; }
   const single_position_iterator operator-- (int) { single_position_iterator copy(*this);  operator--();  return copy; }

   bool at_end() const { return single_pos != 0; }

   bool operator== (const const_iterator& other) const { return single_pos == other.single_pos; }
   bool operator== (const iterator& other)       const { return single_pos == other.single_pos; }
   bool operator!= (const const_iterator& other) const { return !operator==(other); }
   bool operator!= (const iterator& other)       const { return !operator==(other); }

   void rewind() { single_pos &= ~1; }

protected:
   // 0: valid position, 1: after valid position, <0: no valid position at all (empty sequence)
   int single_pos;

private:
   // delete these if the original iterator is a random-access one
   void operator+=(int) = delete;
   void operator-=(int) = delete;
   void operator+ (int) = delete;
   void operator- (int) = delete;
   void operator[](int) = delete;
};

template <typename TRef>
struct check_iterator_feature<same_value_iterator<TRef>, unlimited> : std::true_type {};

template <typename TRef>
struct check_iterator_feature<same_value_iterator<TRef>, contractable> : std::true_type {};

template <typename TRef>
struct check_iterator_feature<same_value_iterator<TRef>, rewindable> : std::true_type {};

template <typename Iterator>
struct check_iterator_feature<single_position_iterator<Iterator>, end_sensitive> : std::true_type {};

template <typename Iterator>
struct check_iterator_feature<single_position_iterator<Iterator>, indexed> : check_iterator_feature<Iterator, indexed> {};

template <typename Iterator>
struct check_iterator_feature<single_position_iterator<Iterator>, rewindable> : std::true_type {};


template <typename TRef>
class same_value_container {
protected:
   using alias_t = alias<TRef>;
   alias_t value;
public:
   // TODO: revert to one argument when iterators stop outliving containers
   template <typename... Args, typename=std::enable_if_t<std::is_constructible<alias_t, Args...>::value>>
   explicit same_value_container(Args&&... args)
      : value(std::forward<Args>(args)...) {}

   using value_type = typename alias_t::value_type;
   using reference = typename alias_t::reference;
   using const_reference = typename alias_t::const_reference;

   using iterator = same_value_iterator<TRef>;
   using const_iterator = typename iterator::const_iterator;
   using reverse_iterator = iterator;
   using const_reverse_iterator = const_iterator;
   reference front() { return *value; }
   reference back() { return *value; }
   reference operator[] (int) { return *value; }
   const_reference front() const { return *value; }
   const_reference back() const { return *value; }
   const_reference operator[] (int) const { return *value; }

   iterator begin() & { return iterator(value); }
   iterator begin() && { return iterator(std::move(value)); }
   iterator end() { return iterator(); }
   iterator rbegin() & { return iterator(value); }
   iterator rbegin() && { return iterator(std::move(value)); }
   iterator rend() { return iterator(); }

   const_iterator begin() const & { return const_iterator(value); }
   const_iterator end() const { return const_iterator(); }
   const_iterator rbegin() const & { return const_iterator(value); }
   const_iterator rend() const { return const_iterator(); }

   bool empty() const { return false; }
   int size() const { return std::numeric_limits<int>::max(); }

   static same_value_container& cast_from(alias_t& a)
   {
      return *reverse_cast(&a, &same_value_container::value);
   }
   static const same_value_container& cast_from(const alias_t& a)
   {
      return *reverse_cast(&a, &same_value_container::value);
   }
};

template <typename TRef>
struct spec_object_traits< same_value_container<TRef> >
   : spec_object_traits<is_container> {
   static constexpr bool
      is_temporary = true,
      is_always_const = std::is_same<typename alias<TRef>::reference, typename alias<TRef>::const_reference>::value;
};

template <typename TRef>
struct check_container_feature<same_value_container<TRef>, provide_construction<rewindable,false>>
   : std::true_type {};


// to be removed
template <typename T>
class constant_pointer_iterator : public ptr_wrapper<T, false> {
   using base_t = ptr_wrapper<T, false>;
public:
   using iterator = constant_pointer_iterator;
   using const_iterator = constant_pointer_iterator;

   constant_pointer_iterator() {}
   constant_pointer_iterator(T& arg) : base_t(&arg) {}

   constant_pointer_iterator& operator++ () { return *this; }
   const constant_pointer_iterator& operator++ (int) { return *this; }
   constant_pointer_iterator& operator-- () { return *this; }
   const constant_pointer_iterator& operator-- (int) { return *this; }
   constant_pointer_iterator& operator+= (int) { return *this; }
   constant_pointer_iterator& operator-= (int) { return *this; }
   const constant_pointer_iterator& operator+ (int) { return *this; }
   const constant_pointer_iterator& operator- (int) { return *this; }
   friend const constant_pointer_iterator& operator+ (int, const constant_pointer_iterator& me) { return me; }

   ptrdiff_t operator- (const constant_pointer_iterator& it) const { return (this->cur != nullptr) != (it.cur != nullptr); }
   void rewind() {}
   void contract(bool, int, int) {}
};

template <typename T>
struct check_iterator_feature<constant_pointer_iterator<T>, unlimited> : std::true_type {};

template <typename T>
struct check_iterator_feature<constant_pointer_iterator<T>, contractable> : std::true_type {};

template <typename T>
struct check_iterator_feature<constant_pointer_iterator<T>, rewindable> : std::true_type {};

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
struct spec_object_traits< constant_masquerade_container<T> >
   : spec_object_traits<is_container> {
   static const bool is_always_const=attrib<T>::is_const || object_traits<T>::is_always_const;
   typedef T masquerade_for;
};

template <typename T>
struct check_container_feature<constant_masquerade_container<T>, provide_construction<rewindable,false>>
   : std::true_type {};


// TODO: some occurrences of this function in attach_operation() and friends
// can be replaced with lambdas once we get rid of operation objects
template <typename T>
auto same_value(T&& x)
{
   return same_value_container<T>(std::forward<T>(x));
}

template <typename T>
same_value_container<T>& as_same_value_container(alias<T>& x)
{
   return same_value_container<T>::cast_from(x);
}

template <typename T>
const same_value_container<T>& as_same_value_container(const alias<T>& x)
{
   return same_value_container<T>::cast_from(x);
}

namespace object_classifier {
   enum { is_constant=is_manip+1 };

   namespace _impl {
      template <typename TRef>
      size_discriminant<is_constant>::type analyzer_f(const same_value_container<TRef>*, bait*);

      template <typename T>
      size_discriminant<is_constant>::type analyzer_f(const constant_masquerade_container<T>*, bait*);
   }
}

template <typename T>
struct spec_object_traits< cons<T, int_constant<object_classifier::is_constant> > >
   : spec_object_traits<is_container> {};

} // end namespace pm

namespace polymake {
   using pm::same_value;
}

#endif // POLYMAKE_INTERNAL_CONSTANT_CONTAINERS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
