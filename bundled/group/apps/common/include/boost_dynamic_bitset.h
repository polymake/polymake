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

#ifndef POLYMAKE_DYNAMIC_BITSET_H
#define POLYMAKE_DYNAMIC_BITSET_H


#include "polymake/group/permlib.h"
#include "polymake/Set.h"
#include "polymake/internal/converters.h"
#include "polymake/Integer.h"

namespace pm { 

class boost_dynamic_bitset_iterator {
   friend class boost_dynamic_bitset;
public:
   typedef forward_iterator_tag iterator_category;
   typedef int value_type;
   typedef int reference;
   typedef const value_type* pointer;
   typedef ptrdiff_t difference_type;
   typedef boost_dynamic_bitset_iterator iterator;
   typedef boost_dynamic_bitset_iterator const_iterator;
protected:
   const permlib::dset* bits;
   size_t cur;

   boost_dynamic_bitset_iterator(const permlib::dset* bits_arg, size_t cur_arg) : bits(bits_arg), cur(cur_arg) {}

public:
   boost_dynamic_bitset_iterator() : bits(0), cur(0) {}

   explicit boost_dynamic_bitset_iterator(const permlib::dset* bits_arg) : bits(bits_arg) { rewind(); }

   int operator* () const { return int(cur); }

   iterator& operator++ ()
   {
      if (!at_end()) cur=bits->find_next(cur);
      return *this;
   }

   const iterator operator++ (int) { iterator copy=*this; operator++(); return copy; }

   bool operator== (const iterator& it) const { return cur==it.cur; }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   bool at_end() const
   {
      return cur == bits->npos;
   }

   void rewind()
   {
      cur = bits->find_first();
   }
};


class boost_dynamic_bitset : public GenericSet<boost_dynamic_bitset, int, operations::cmp> {
   typedef permlib::dset super;

protected:
   super rep;

public:
   typedef int value_type;
   typedef const value_type& const_reference;
   typedef const_reference reference;
   typedef boost_dynamic_bitset_iterator iterator;
   typedef iterator const_iterator;

   boost_dynamic_bitset() : rep(super()) {}
   explicit boost_dynamic_bitset(size_t start_cap) : rep(start_cap) {}
   explicit boost_dynamic_bitset(const super& other) : rep(other) {}
   boost_dynamic_bitset(const boost_dynamic_bitset& s) : rep(s.rep) {} 
   boost_dynamic_bitset(size_t n, const boost_dynamic_bitset& s) : rep(s.rep) {
      rep.resize(n);
   } 

   template <typename Iterator>
   boost_dynamic_bitset(Iterator first, Iterator last) : rep(first,last) {}

   template <typename Iterator>
   boost_dynamic_bitset(size_t n, Iterator first, Iterator last) : rep(first,last) {
      rep.resize(n);
   }

   boost_dynamic_bitset& operator=(super const& other) { rep = other; return *this; }

   ~boost_dynamic_bitset() { rep.clear(); }

   /// Copy of a disguised boost_dynamic_bitset object.
   explicit boost_dynamic_bitset(const GenericSet<boost_dynamic_bitset>& s) : rep(s.top().rep) {}
   explicit boost_dynamic_bitset(size_t n, const GenericSet<boost_dynamic_bitset>& s) : rep(s.top().rep) {
      rep.resize(n);
   }

protected:
   template <typename Iterator>
   void assign(Iterator src)
   {
      for (; !src.at_end(); ++src)
         rep.set(*src);
   }

   template <typename Iterator>
   void assign(Iterator src, Iterator src_end)
   {
      for (; src!=src_end; ++src)
         rep.set(*src);
   }

   template <typename Container>
   void assign_from(const Container& src, std::false_type)
   {
      assign(entire(src));
   }

   template <typename Container>
   void assign_from(const Container& src, std::true_type)
   {
      assign(entire(reversed(src)));
   }

   template <typename Container>
   void assign_from(const Container& src)
   {
      assign_from(src, bool_constant<container_traits<Container>::is_bidirectional>());
   }

public:

   /// Copy of an abstract set of integers.
   template <typename Set>
   explicit boost_dynamic_bitset(int n, const GenericSet<Set,int>& s)
      : rep(super())
   {
      rep.resize(n);
      assign_from(s.top());
   }

   /// Copy of an abstract set with element conversion.
   template <typename Set, typename E2, typename Comparator2>
   explicit boost_dynamic_bitset(int n, const GenericSet<Set,E2,Comparator2>& s)
      : rep(super())
   {
      rep.resize(n);
      assign_from(attach_converter<int>(s.top()));
   }

   template <typename Iterator>
   boost_dynamic_bitset(int n,  Iterator&& src, Iterator&& src_end)
      : rep(super())
   {
      rep.resize(n);
      assign(src, src_end);
   }

   template <typename Iterator>
   explicit boost_dynamic_bitset(int n, Iterator src,
                                 typename std::enable_if<assess_iterator<Iterator, check_iterator_feature, end_sensitive>::value, void**>::type=nullptr)
      : rep(super())
   {
      rep.resize(n);
      assign(src);
   }

   template <size_t n>
   explicit boost_dynamic_bitset(const int (&a)[n]) : rep(super())
   {
      rep.resize(n);
      assign_from(array2container(a));
   }

   template <typename E2, typename Comparator2>
   explicit boost_dynamic_bitset(int n, const SingleElementSetCmp<E2,Comparator2>& s) : rep(super())
   {
      rep.resize(n);
      rep.set(s.front());
   }

   explicit boost_dynamic_bitset(const Set<int> &s) : rep(super())
   {
#if POLYMAKE_DEBUG
      cerr << "enter boost_dynamic_bitset(const Set<int>&). "
           << "argument=" << s << ", of size=" << s.size() << endl;
#endif
      if (s.size() == 0) {
         rep.resize(1);
      } else {
#if POLYMAKE_DEBUG
         cerr << "s.front(): " << s.front() 
              << " s.back(): " << s.back()
              << " s.size(): " << s.size() << endl;
#endif
         rep.resize(s.back());
         for (Entire<Set<int> >::const_iterator sit  = entire(s); !sit.at_end(); ++sit) 
            *this += *sit;
      }
#if POLYMAKE_DEBUG
     cerr << "exit boost_dynamic_bitset(const Set<int>&). "
          << "rep.size()=" << rep.size() << endl;
#endif
   }

   boost_dynamic_bitset& operator= (const boost_dynamic_bitset& s)
   {
      rep = s.rep;
      return *this;
   }

   /// Assign elements from a disguised Bitset object.
   boost_dynamic_bitset& operator= (const GenericSet<boost_dynamic_bitset>& s)
   {
      return *this=s.top();
   }

   void swap(boost_dynamic_bitset& s) { rep.swap(s.rep); }

   const super& dset() const { return rep; }

   unsigned long to_ulong() const { return rep.to_ulong(); }

   // conversion to and from GMP Integers
   Integer toInteger() const {
      mpz_t out;
      mpz_init(out);
      size_t i = rep.find_first();
      while (i != super::npos) {
         mpz_setbit(out, i);
         i = rep.find_next(i);
      }
      return Integer(std::move(out));
   }

   boost_dynamic_bitset& fromInteger(const size_t capacity, const Integer& in) {
      rep.resize(capacity);
      mpz_srcptr other_rep = in.get_rep();
#if __GNU_MP_VERSION < 5
      typedef unsigned long mp_bitcnt_t;
#endif
      mp_bitcnt_t bit = mpz_scan1(other_rep, 0);
      while (bit != std::numeric_limits<mp_bitcnt_t>::max()) {
         rep.set(bit);
         bit = mpz_scan1(other_rep, bit);
      }
      return *this;
   }


   // let's make it at least partially compatible with Set

   boost_dynamic_bitset& operator+= (const size_t k)
   {
      if (k >= rep.size()) rep.resize(k+1);
      rep.set(k);
      return *this;
   }

   void insert_unchecked(const size_t k) {
      rep.set(k);
   }
   
   boost_dynamic_bitset& operator-= (const size_t k)
   {
      rep.set(k, false);
      return *this;
   }

   void erase(size_t i) { *this -= i; }

   void erase(const iterator& where) { *this -= *where; }   

   void erase(const iterator& where, const iterator& end) {
      iterator it(where);
      while (it!=end) *this -= *it; 
   }



   boost_dynamic_bitset& operator^= (const size_t k)
   {
      if (rep.test(k)) rep.set(k, false);
      return *this;
   }

   boost_dynamic_bitset& operator+= (const boost_dynamic_bitset& s)
   {
      rep |= s.rep;
      return *this;
   }

   template <typename Set>
   boost_dynamic_bitset& operator+= (const GenericSet<Set, int, element_comparator>& s)
   {
      for (typename Entire<Set>::const_iterator e=entire(s.top()); !e.at_end(); ++e)
         *this += *e;
      return *this;
   }

   bool exists(const size_t k) const
   {
      return rep.test(k);
   }

   bool contains(size_t i) const {
      return rep.test(i);
   }


   void clear() {
      rep.clear();
   }

   bool empty() const {
      return rep.empty();
   } 

   void reset() {
      rep.reset();
   }

   size_t capacity() const {
      return rep.size();
   }

   void capacity(const size_t new_size) {
      rep.resize(new_size);
   }

   void resize(const size_t new_size) {
      rep.resize(new_size);
   }

   size_t size() const {
      return rep.count();
   }

   int front() const {
      return static_cast<int>(rep.find_first());
   }

   // ATTENTION: The runtime is linear in the size of the set.
   // If this becomes a problem (demonstratable by profiling),
   // add a member "size_t last" to the class and update it upon each insert and delete. 
   int back() const {
      size_t result(rep.find_first()), tmp(result);
      while ((tmp = rep.find_next(result)) != super::npos)
         result = tmp;
      return static_cast<int>(result);
   }

   bool operator== (const boost_dynamic_bitset& other) const
   {
      const_iterator it1(this->begin()), it2(other.begin());
      while (!it1.at_end() && !it2.at_end()) {
         if (*it1 != *it2) return false;
         ++it1; ++it2;
      }
      return it1.at_end() && it2.at_end();
   }

   operations::cmp get_comparator() const { return operations::cmp(); }

   const_iterator begin() const { return iterator(&rep); }
   iterator begin() { return iterator(&rep); }
   const_iterator end() const { return iterator(&rep, super::npos); }

   Set<int> to_polymake_set() const {
      Set<int> out;
      size_t pos = rep.find_first();
      while (pos != super::npos) {
         out += static_cast<int>(pos);
         pos = rep.find_next(pos);
      }
      return out;
   }

   iterator insert(int i)
   {
      *this += i;
      return iterator(&rep, i);
   }

   iterator insert(const iterator&, int i) { return insert(i); }

   // consume data as if designated for Set<int>
   template <typename Input> friend
   Input& operator>> (GenericInput<Input>& in, boost_dynamic_bitset& me)
   {
      me.clear();
      for (typename Input::template list_cursor< Set<int> >::type c=in.top().begin_list((Set<int>*)0);
           !c.at_end(); ) {
         int elem=-1;
         c >> elem;
         me += elem;
      }
      return in.top();
   }
};


template <>
struct spec_object_traits< Serialized< boost_dynamic_bitset > > :
   spec_object_traits<is_composite> {

   typedef boost_dynamic_bitset masquerade_for;

   typedef cons<int, Integer> elements;

   template <typename Visitor>
   static void visit_elements(const Serialized< boost_dynamic_bitset >& me, Visitor& v)
   {
      v << me.capacity() << me.toInteger();
   }

   template <typename Visitor>
   static void visit_elements(Serialized< boost_dynamic_bitset >& me, Visitor& v)
   {
      int cap;
      Integer stored;
      v << cap << stored;
      me.fromInteger(cap, stored);
   }
};


template <> struct check_iterator_feature<boost_dynamic_bitset_iterator, end_sensitive> : std::true_type {};
template <> struct check_iterator_feature<boost_dynamic_bitset_iterator, rewindable> : std::true_type {};
template <> struct check_container_feature< boost_dynamic_bitset_iterator, sparse_compatible > : std::true_type {};

template <typename Permutation> inline
boost_dynamic_bitset
permuted(const boost_dynamic_bitset& c, const Permutation& perm)
{
   boost_dynamic_bitset p(c.capacity());
   for (const auto& i : c)
      p += perm[i];
   return p;
}

template<>
struct hash_func<boost_dynamic_bitset, is_set> {
   size_t operator() (const boost_dynamic_bitset& s) const
   {
      return s.to_ulong();
   }
};

} // end namespace pm

namespace polymake { namespace common {
   using pm::boost_dynamic_bitset;
} }

namespace polymake { namespace polytope {
   using pm::boost_dynamic_bitset;
} }

namespace polymake { namespace group {

template <typename Iterator, typename Permutation> inline
void permute_to(Iterator in_it,          // deliberately no reference, so we can increment it inside the function
                const Permutation& perm,
                common::boost_dynamic_bitset& out)
{
   out.reset();
   while (!in_it.at_end()) {
      out.insert_unchecked(perm[*in_it]);
      ++in_it;
   }
}

} }

#endif // POLYMAKE_DYNAMIC_BITSET_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
