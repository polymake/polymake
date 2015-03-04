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

#ifndef POLYMAKE_BITSET_H
#define POLYMAKE_BITSET_H

#include "polymake/GenericSet.h"
#include "polymake/internal/converters.h"
#include "polymake/Integer.h"
#include "polymake/SelectedSubset.h"

namespace pm {

class Bitset;

class Bitset_iterator {
   friend class Bitset;
public:
   typedef forward_iterator_tag iterator_category;
   typedef int value_type;
   typedef int reference;
   typedef const int* pointer;
   typedef ptrdiff_t difference_type;
   typedef Bitset_iterator iterator;
   typedef Bitset_iterator const_iterator;
protected:
   mpz_srcptr bits;
   int cur;

   static const int bits_per_limb
#if __GNU_MP_VERSION_MINOR > 0 || __GNU_MP_VERSION >=5
     =GMP_NUMB_BITS
#else
     =__GMP_BITS_PER_MP_LIMB
#endif
   ;

   Bitset_iterator(mpz_srcptr bits_arg, int cur_arg) : bits(bits_arg), cur(cur_arg) {}

public:
   Bitset_iterator() : bits(0), cur(0) {}

   explicit Bitset_iterator(mpz_srcptr bits_arg) : bits(bits_arg) { rewind(); }

   reference operator* () const { return cur; }
   pointer operator-> () const { return &cur; }

   iterator& operator++ ()
   {
      ++cur;
      if (!at_end()) cur=mpz_scan1(bits,cur);
      return *this;
   }

   const iterator operator++ (int) { iterator copy=*this; operator++(); return copy; }

   bool operator== (const iterator& it) const { return cur==it.cur; }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   bool at_end() const
   {
      int n=cur/bits_per_limb,
          diff=n+1-mpz_size(bits);
      return diff>0 ||
             diff==0 && !(mpz_getlimbn(bits, n) & (mp_limb_t)(-1) << (cur%bits_per_limb));
   }

   void rewind()
   {
      cur=0;
      if (mpz_size(bits)) cur=mpz_scan1(bits,cur);
   }
};

/** \class Bitset
    \brief Container class for dense sets of integers.

    A special class optimized for representation of a constrained range of
    non-negative integer numbers.  Its implementation is based on the GMP
    (mpz_t), see http://www.swox.com/gmp/ You should consider to use it
    instead of the more general Set<int> if all these criteria hold:

    - the element range stays constant during the lifetime of the set

    - the element range is small (magnitude of tens), or the fill grade
    (number of elements divided through the element upper bound) is expected
    to be rather high (&gt;= 0.5)

    - the number of random access operations (testing/addition/removal
    of single elements) prevails significantly over the number of sequential
    visits via iterators

    Note that unlike @c std::bitset, the element range is <b>not</b> hard
    encoded in the Bitset object, but can be dynamically changed any time.

    Bitset is one of the few top-level classes in the Polymake Template
    Library that does not make use of reference counted @ref refcounting
    "smart pointers".  It's difficult to say why.  You should keep this in
    mind if you want to copy or return Bitset objects by value.
*/
class Bitset : public GenericSet<Bitset, int, operations::cmp> {
protected:
   mpz_t rep;

   void fill1s(size_t n);
   void fill1s(const sequence& s);
public:
   typedef int value_type;
   typedef const int const_reference;
   typedef const_reference reference;
   typedef Bitset_iterator iterator;
   typedef iterator const_iterator;

   /// @brief An empty set, without preallocated storage.
   Bitset() { mpz_init(rep); }

   /** @brief An empty set with preallocated storage for elements 0..@a n-1.
   
       It can dynamically grow beyond this limit if needed; to avoid performance penalties, however,
       you should specify here the highest element you really expect to occur.
       
       @param n
       @param full %Set this to 1 if the set contains all elements from 0 to n-1, defaults to 0.
       */
   explicit Bitset(int n, bool full=false)
   {
      mpz_init(rep);
      reserve(n);
      if (full && n>0) fill1s(n);
   }

   Bitset(const Bitset& s) { mpz_init_set(rep, s.rep); }

   ~Bitset() { mpz_clear(rep); }
      
   /// Copy of a disguised Bitset object.
   Bitset(const GenericSet<Bitset>& s) { mpz_init_set(rep, s.top().rep); }

protected:
   template <typename Iterator>
   void _assign(Iterator src)
   {
      for (; !src.at_end(); ++src)
         mpz_setbit(rep, *src);
   }

   template <typename Iterator>
   void _assign(Iterator src, Iterator src_end)
   {
      for (; src!=src_end; ++src)
         mpz_setbit(rep, *src);
   }

   template <typename Container>
   void _assign_from(const Container& src, False)
   {
      _assign(entire(src));
   }

   template <typename Container>
   void _assign_from(const Container& src, True)
   {
      _assign(entire(reversed(src)));
   }

   template <typename Container>
   void _assign_from(const Container& src)
   {
      _assign_from(src, bool2type<container_traits<Container>::is_bidirectional>());
   }
public:
   Bitset(void (*f)(mpz_ptr,unsigned long), mpz_srcptr a, unsigned long k)
   {
      mpz_init_set(rep,a);
      f(rep,k);
   }

   template <typename Arg>
   Bitset(void (*f)(mpz_ptr,Arg), Arg a)
   {
      mpz_init(rep);
      f(rep,a);
   }

   template <typename Arg1, typename Arg2>
   Bitset(void (*f)(mpz_ptr,Arg1,Arg2), Arg1 a, Arg2 b)
   {
      mpz_init(rep);
      f(rep,a,b);
   }

   /// Copy of an abstract set of integers.
   template <typename Set>
   explicit Bitset(const GenericSet<Set,int>& s)
   {
      mpz_init(rep);
      _assign_from(s.top());
   }

   /// Copy of an abstract set with element conversion.
   template <typename Set, typename E2, typename Comparator2>
   explicit Bitset(const GenericSet<Set,E2,Comparator2>& s)
   {
      mpz_init(rep);
      _assign_from(attach_converter<int>(s.top()));
   }

   template <typename Iterator>
   Bitset(Iterator src, Iterator src_end)
   {
      mpz_init(rep);
      _assign(src,src_end);
   }

   template <typename Iterator>
   explicit Bitset(Iterator src, typename enable_if_iterator<Iterator,end_sensitive>::type=0)
   {
      mpz_init(rep);
      _assign(src);
   }

   template <size_t n>
   explicit Bitset(const int (&a)[n])
   {
      mpz_init(rep);
      _assign_from(array2container(a));
   }

   template <typename E2, typename Comparator2>
   explicit Bitset(const SingleElementSetCmp<E2,Comparator2>& s)
   {
      mpz_init(rep);
      mpz_setbit(rep, s.front());
   }

   explicit Bitset(const sequence& s)
   {
      mpz_init(rep);
      fill1s(s);
   }

   Bitset& operator= (const Bitset& s)
   {
      mpz_set(rep, s.rep);
      return *this;
   }

   /// Assign elements from a disguised Bitset object.
   Bitset& operator= (const GenericSet<Bitset>& s)
   {
      return *this=s.top();
   }

   /// Reserve storage for n elements
   void reserve(int n)
   {
      if (n > rep[0]._mp_alloc*iterator::bits_per_limb)
         mpz_realloc2(rep,n);
   }

   /// Make the set empty.
   void clear()
   {
      mpz_set_ui(rep, 0);
   }

   /// Assign elements from an abstract set of integers.
   template <typename Set>
   Bitset& operator= (const GenericSet<Set,int>& s)
   {
      clear();
      _assign_from(s.top());
      return *this;
   }

   template <typename E2, typename Comparator2>
   Bitset& operator= (const SingleElementSetCmp<E2,Comparator2>& s)
   {
      clear();
      mpz_setbit(rep, s.front());
      return *this;
   }

   Bitset& operator= (const sequence& s)
   {
      clear();
      fill1s(s);
      return *this;
   }

   void swap(Bitset& s) { mpz_swap(rep, s.rep); }

   friend void relocate(Bitset* from, Bitset* to)
   {
      to->rep[0] = from->rep[0];
   }

   bool empty() const
   {
      return !mpz_sgn(rep);
   }

   /** Count the elements.
       CAUTION: depending on the hardware, can take O(n) time! */
   int size() const
   {
      return mpz_popcount(rep);
   }

   bool contains(int i) const
   {
      return mpz_tstbit(rep, i);
   }

   int front() const
   {
      return mpz_scan1(rep,0);
   }

   int back() const
   {
      int n=mpz_size(rep)-1;
      return n*iterator::bits_per_limb + log2_floor(mpz_getlimbn(rep,n));
   }

   /** Insert an element.
       This is the quickest way to manipulate single elements. */
   Bitset& operator+= (int i)
   {
      mpz_setbit(rep, i);
      return *this;
   }

   /** Remove an element if it existed.
       This is the quickest way to manipulate single elements. */
   Bitset& operator-= (int i)
   {
      mpz_clrbit(rep, i);
      return *this;
   }

   Bitset& operator+= (const Bitset& s)
   {
      mpz_ior(rep, rep, s.rep);
      return *this;
   }

   template <typename Set>
   Bitset& operator+= (const GenericSet<Set, int, element_comparator>& s)
   {
      for (typename Entire<Set>::const_iterator e=entire(s.top()); !e.at_end(); ++e)
         *this += *e;
      return *this;
   }

private:
   static void difference(mpz_ptr dst, mpz_srcptr src1, mpz_srcptr src2);
public:
   /// difference
   Bitset& operator-= (const Bitset& s)
   {
      difference(rep, rep, s.rep);
      return *this;
   }

   template <typename Set>
   Bitset& operator-= (const GenericSet<Set, int, element_comparator>& s)
   {
      for (typename Entire<Set>::const_iterator e=entire(s.top()); !e.at_end(); ++e)
         *this -= *e;
      return *this;
   }

   /// intersection
   Bitset& operator*= (const Bitset& s)
   {
      mpz_and(rep, rep, s.rep);
      return *this;
   }

   template <typename Set>
   Bitset& operator*= (const GenericSet<Set, int, element_comparator>& s);

   Bitset& operator^= (int i)
   {
      if (mpz_tstbit(rep, i))
         mpz_clrbit(rep, i);
      else
         mpz_setbit(rep, i);
      return *this;
   }

   Bitset& operator^= (const Bitset& s)
   {
      mpz_xor(rep, rep, s.rep);
      return *this;
   }

   template <typename Set>
   Bitset& operator^= (const GenericSet<Set, int, element_comparator>& s)
   {
      for (typename Entire<Set>::const_iterator e=entire(s.top()); !e.at_end(); ++e)
         *this ^= *e;
      return *this;
   }

   iterator begin() const { return iterator(rep); }
   iterator end() const { return iterator(rep, empty() ? 0 : back()+1); }

   iterator insert(int i)
   {
      *this += i;
      return iterator(rep, i);
   }

   void push_back(int i) { *this += i; }
   void push_front(int i) { *this += i; }
   void erase(int i) { *this -= i; }

   iterator insert(const iterator&, int i) { return insert(i); }

   void erase(const iterator& where) { *this -= *where; }

   void pop_front() { *this -= front(); }

   void pop_back() { *this -= back(); }

   friend
   Bitset operator+ (const Bitset& s1, const Bitset& s2)
   {
      return Bitset(mpz_ior, s1.rep, s2.rep);
   }

   /// alias for operator+
   friend
   Bitset operator| (const Bitset& s1, const Bitset& s2)
   {
      return Bitset(mpz_ior, s1.rep, s2.rep);
   }

   friend
   Bitset operator+ (const Bitset& s1, int i2)
   {
      return Bitset(mpz_setbit, s1.rep, (unsigned long)i2);
   }

   friend
   Bitset operator+ (int i1, const Bitset& s2)
   {
      return Bitset(mpz_setbit, s2.rep, (unsigned long)i1);
   }

   friend
   Bitset operator- (const Bitset& s1, const Bitset& s2)
   {
      return Bitset(difference, s1.rep, s2.rep);
   }

   friend
   Bitset operator- (const Bitset& s1, int i2)
   {
      return Bitset(mpz_clrbit, s1.rep, (unsigned long)i2);
   }

   friend
   Bitset operator* (const Bitset& s1, const Bitset& s2)
   {
      return Bitset(mpz_and, s1.rep, s2.rep);
   }

   /// alias for operator*
   friend
   Bitset operator& (const Bitset& s1, const Bitset& s2)
   {
      return Bitset(mpz_and, s1.rep, s2.rep);
   }

   friend
   Bitset operator* (const Bitset& s1, int i2)
   {
      return s1.contains(i2) ? Bitset(scalar2set(i2)) : Bitset();
   }

   friend
   Bitset operator* (int i1, const Bitset& s2)
   {
      return s2.contains(i1) ? Bitset(scalar2set(i1)) : Bitset();
   }

   friend
   Bitset operator^ (const Bitset& s1, const Bitset& s2)
   {
      return Bitset(mpz_xor, s1.rep, s2.rep);
   }

   friend
   Bitset operator^ (const Bitset& s1, int i2)
   {
      return Bitset(s1.contains(i2) ? &mpz_clrbit : &mpz_setbit, s1.rep, (unsigned long)i2);
   }

   friend
   Bitset operator^ (int i1, const Bitset& s2)
   {
      return Bitset(s2.contains(i1) ? &mpz_clrbit : &mpz_setbit, s2.rep, (unsigned long)i1);
   }

   friend
   bool operator== (const Bitset& s1, const Bitset& s2)
   {
      return !mpz_cmp(s1.rep, s2.rep);
   }

   friend
   bool operator!= (const Bitset& s1, const Bitset& s2)
   {
      return mpz_cmp(s1.rep, s2.rep);
   }

   mpz_srcptr get_rep() const { return rep; }
};

/// See incl(GenericSet,GenericSet)
int incl(const Bitset& s1, const Bitset& s2);

template <typename Set>
Bitset& Bitset::operator*= (const GenericSet<Set, int, element_comparator>& s)
{
   mp_limb_t *d=rep[0]._mp_d;
   mp_limb_t limb=0;
   typename Entire<Set>::const_iterator e=entire(s.top());
   for (int i=0, size=mpz_size(rep), first=0, last=iterator::bits_per_limb;
        i<size;
        ++i, first=last, last+=iterator::bits_per_limb) {
      for (;;) {
         if (e.at_end()) {
            *d++ &= limb;
            rep[0]._mp_size=i+(limb!=0);
            return *this;
         }
         int elem=*e;  ++e;
         if (elem>=last) {
            *d++ &= limb;
            limb = ((mp_limb_t)1)<<elem-last;   // prepare for the next outer loop run
            break;
         } else {
            limb |= ((mp_limb_t)1)<<elem-first;
         }
      }
   }
   return *this;
}

template <> struct check_iterator_feature<Bitset_iterator, end_sensitive> : True {};
template <> struct check_iterator_feature<Bitset_iterator, rewindable> : True {};

template <>
struct hash_func<Bitset, is_set> : hash_func<MP_INT> {
   size_t operator() (const Bitset& s) const
   {
      return _do(s.get_rep());
   }
};

} // end namespace pm

namespace polymake {
   using pm::Bitset;
}

namespace std {
   inline void swap(pm::Bitset& s1, pm::Bitset& s2) { s1.swap(s2); }
}

#endif // POLYMAKE_BITSET_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
