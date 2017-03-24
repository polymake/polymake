/* Copyright (c) 1997-2016
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
#include "polymake/Set.h"
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

   Bitset_iterator(mpz_srcptr bits_arg, int cur_arg)
      : bits(bits_arg)
      , cur(cur_arg) {}

public:
   Bitset_iterator()
      : bits(0)
      , cur(0) {}

   explicit Bitset_iterator(mpz_srcptr bits_arg)
      : bits(bits_arg)
   { rewind(); }

   reference operator* () const { return cur; }
   pointer operator-> () const { return &cur; }

   iterator& operator++ ()
   {
      ++cur;
      if (!at_end()) cur=mpz_scan1(bits, cur);
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
*/
class Bitset
   : public GenericSet<Bitset, int, operations::cmp> {
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

   /// @brief An empty set, with minimal preallocated storage.
   Bitset()
   {
      mpz_init_set_ui(rep, 0);
   }

   /** @brief An empty set with preallocated storage for elements 0..@a n-1.
   
       It can dynamically grow beyond this limit if needed; to avoid performance penalties, however,
       you should specify here the highest element you really expect to occur.
       
       @param n
       @param full %Set this to 1 if the set contains all elements from 0 to n-1, defaults to 0.
       */
   explicit Bitset(int n, bool full=false)
      : Bitset()
   {
      reserve(n);
      if (full && n>0) fill1s(n);
   }

   Bitset(const Bitset& s)
   {
      mpz_init_set(rep, s.rep);
   }

   Bitset(Bitset&& s) noexcept
   {
      rep[0]=s.rep[0];
      s.rep[0]._mp_d=nullptr;
      s.rep[0]._mp_size=0;
      s.rep[0]._mp_alloc=0;
   }

   /// Copy the value from a third party
   explicit Bitset(const mpz_t& b)
   {
      mpz_set(rep, b);
   }

   /// Steal the value from a third party
   /// The source must be re-initialized if it's going to be used afterwards
   explicit Bitset(mpz_t&& b) noexcept
   {
      *rep=b[0];
      b[0]._mp_alloc=0;
      b[0]._mp_size=1;
      b[0]._mp_d=nullptr;
   }

   ~Bitset() noexcept
   {
      mpz_clear(rep);
   }
      
   /// Copy of a disguised Bitset object.
   Bitset(const GenericSet<Bitset>& s)
   {
      mpz_init_set(rep, s.top().rep);
   }

   /// Copy of an abstract set of integers.
   template <typename TSet>
   explicit Bitset(const GenericSet<TSet, int>& s)
      : Bitset()
   {
      assign_from(s.top());
   }

   /// Copy of an abstract set with element conversion.
   template <typename TSet, typename E2, typename Comparator2,
             typename=typename std::enable_if<std::is_convertible<E2, int>::value>::type>
   explicit Bitset(const GenericSet<TSet, E2, Comparator2>& s)
      : Bitset()
   {
      assign_from(s.top());
   }

   template <typename TContainer,
             typename=typename std::enable_if<isomorphic_to_container_of<TContainer, int, is_set>::value>::type>
   Bitset(const TContainer& src)
      : Bitset()
   {
      assign_from(src);
   }

   template <typename Iterator,
             typename=typename std::enable_if<assess_iterator_value<Iterator, std::is_convertible, int>::value>::type>
   Bitset(Iterator&& src, Iterator&& src_end)
      : Bitset()
   {
      assign(ensure_private_mutable(std::forward<Iterator>(src)), src_end);
   }

   template <typename Iterator>
   explicit Bitset(Iterator&& src,
                   typename std::enable_if<(assess_iterator<Iterator, check_iterator_feature, end_sensitive>::value &&
                                            assess_iterator_value<Iterator, std::is_convertible, int>::value), void**>::type=nullptr)
      : Bitset()
   {
      assign(ensure_private_mutable(std::forward<Iterator>(src)));
   }

   template <typename E2,
             typename=typename std::enable_if<std::is_convertible<E2, int>::value>::type>
   Bitset(std::initializer_list<E2> l)
      : Bitset()
   {
      *this = l;
   }

   template <typename E2, typename Comparator2,
             typename=typename std::enable_if<std::is_convertible<E2, int>::value>::type>
   explicit Bitset(const SingleElementSetCmp<E2, Comparator2>& s)
      : Bitset()
   {
      mpz_setbit(rep, s.front());
   }

   explicit Bitset(const sequence& s)
      : Bitset()
   {
      fill1s(s);
   }

   /// Fill with a prescribed number of random bits
   Bitset(gmp_randstate_t rnd, unsigned long bits)
      : Bitset()
   {
      mpz_urandomb(rep, rnd, bits);
   }

   Bitset& operator= (const Bitset& s)
   {
      mpz_set(rep, s.rep);
      return *this;
   }

   Bitset& operator= (Bitset&& s) noexcept
   {
      swap(s);
      return *this;
   }

   template <typename E2,
             typename=typename std::enable_if<std::is_convertible<E2, int>::value>::type>
   Bitset& operator= (std::initializer_list<E2> l)
   {
      clear();
      if (l.size() > 0) {
         reserve(l[l.size()-1]+1);
         assign(l.begin(), l.end());
      }
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
      if (n > rep[0]._mp_alloc * iterator::bits_per_limb)
         mpz_realloc2(rep, n);
   }

   /// Make the set empty.
   void clear()
   {
      mpz_set_ui(rep, 0);
   }

   /// Assign elements from an abstract set of integers.
   template <typename TSet>
   Bitset& operator= (const GenericSet<TSet, int>& s)
   {
      clear();
      assign_from(s.top());
      return *this;
   }

   template <typename E2, typename Comparator2,
             typename=typename std::enable_if<std::is_convertible<E2, int>::value>::type>
   Bitset& operator= (const SingleElementSetCmp<E2, Comparator2>& s)
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

   void swap(Bitset& s) noexcept { mpz_swap(rep, s.rep); }

   // TODO: kill this
   friend void relocate(Bitset* from, Bitset* to)
   {
      to->rep[0] = from->rep[0];
   }

   bool empty() const noexcept
   {
      return !mpz_sgn(rep);
   }

   /** Count the elements.
       CAUTION: depending on the hardware, can take O(n) time! */
   int size() const noexcept
   {
      return mpz_popcount(rep);
   }

   bool contains(int i) const noexcept
   {
      return mpz_tstbit(rep, i);
   }

   bool exists(int i) const noexcept
   {
      return mpz_tstbit(rep, i);
   }

   int front() const noexcept
   {
      return mpz_scan1(rep, 0);
   }

   int back() const noexcept
   {
      int n=mpz_size(rep)-1;
      return n*iterator::bits_per_limb + log2_floor(mpz_getlimbn(rep, n));
   }

   /** Insert an element.
       This is the quickest way to manipulate single elements. */
   Bitset& operator+= (int i)
   {
      mpz_setbit(rep, i);
      return *this;
   }

   friend
   Bitset operator+ (const Bitset& s1, int i2)
   {
      Bitset result(s1);
      mpz_setbit(result.rep, i2);
      return result;
   }

   friend
   Bitset operator+ (int i1, const Bitset& s2)
   {
      return s2 + i1;
   }

   friend
   Bitset&& operator+ (Bitset&& s1, int i2)
   {
      return std::move(s1 += i2);
   }

   friend
   Bitset&& operator+ (int i1, Bitset&& s2)
   {
      return std::move(s2 += i1);
   }

   Bitset& operator+= (const Bitset& s)
   {
      mpz_ior(rep, rep, s.rep);
      return *this;
   }

   template <typename TSet>
   Bitset& operator+= (const GenericSet<TSet, int, element_comparator>& s)
   {
      for (auto e=entire(s.top()); !e.at_end(); ++e)
         *this += *e;
      return *this;
   }

   friend
   Bitset operator+ (const Bitset& s1, const Bitset& s2)
   {
      Bitset result;
      mpz_ior(result.rep, s1.rep, s2.rep);
      return result;
   }

   friend
   Bitset&& operator+ (Bitset&& s1, const Bitset& s2)
   {
      return std::move(s1 += s2);
   }

   friend
   Bitset&& operator+ (const Bitset& s1, Bitset&& s2)
   {
      return std::move(s2 += s1);
   }

   friend
   Bitset&& operator+ (Bitset&& s1, Bitset&& s2)
   {
      return std::move(s1 += s2);
   }

   template <typename TSet>
   friend
   Bitset&& operator+ (Bitset&& s1, const GenericSet<TSet, int, element_comparator>& s2)
   {
      return std::move(s1 += s2);
   }

   template <typename TSet>
   friend
   Bitset&& operator+ (const GenericSet<TSet, int, element_comparator>& s1, Bitset&& s2)
   {
      return std::move(s2 += s1);
   }

   /** Remove an element if it existed.
       This is the quickest way to manipulate single elements. */
   Bitset& operator-= (int i)
   {
      mpz_clrbit(rep, i);
      return *this;
   }

   friend
   Bitset operator- (const Bitset& s1, int i2)
   {
      Bitset result(s1);
      mpz_clrbit(result.rep, i2);
      return result;
   }

   friend
   Bitset&& operator- (Bitset&& s1, int i2)
   {
      return std::move(s1 -= i2);
   }

   /// difference
   Bitset& operator-= (const Bitset& s)
   {
      difference(rep, rep, s.rep);
      return *this;
   }

   friend
   Bitset operator- (const Bitset& s1, const Bitset& s2)
   {
      Bitset result;
      difference(result.rep, s1.rep, s2.rep);
      return result;
   }

   friend
   Bitset&& operator- (Bitset&& s1, const Bitset& s2)
   {
      return std::move(s1 -= s2);
   }

   template <typename TSet>
   Bitset& operator-= (const GenericSet<TSet, int, element_comparator>& s)
   {
      for (auto e=entire(s.top()); !e.at_end(); ++e)
         *this -= *e;
      return *this;
   }

   template <typename TSet>
   friend
   Bitset&& operator- (Bitset&& s1, const GenericSet<TSet, int, element_comparator>& s2)
   {
      return std::move(s1 -= s2);
   }

   /// intersection
   Bitset& operator*= (int i)
   {
      if (contains(i)) {
         clear();
         *this += i;
      } else {
         clear();
      }
      return *this;
   }

   friend
   Bitset operator* (const Bitset& s1, int i2)
   {
      Bitset result;
      if (s1.contains(i2)) result += i2;
      return result;
   }

   friend
   Bitset operator* (int i1, const Bitset& s2)
   {
      return s2 * i1;
   }

   friend
   Bitset&& operator* (Bitset&& s1, int i2)
   {
      return std::move(s1 *= i2);
   }

   friend
   Bitset&& operator* (int i1, Bitset&& s2)
   {
      return std::move(s2 *= i1);
   }

   Bitset& operator*= (const Bitset& s)
   {
      mpz_and(rep, rep, s.rep);
      return *this;
   }

   friend
   Bitset operator* (const Bitset& s1, const Bitset& s2)
   {
      Bitset result;
      mpz_and(result.rep, s1.rep, s2.rep);
      return result;
   }

   friend
   Bitset&& operator* (Bitset&& s1, const Bitset& s2)
   {
      return std::move(s1 *= s2);
   }

   friend
   Bitset&& operator* (const Bitset& s1, Bitset&& s2)
   {
      return std::move(s2 *= s1);
   }

   friend
   Bitset&& operator* (Bitset&& s1, Bitset&& s2)
   {
      return std::move(s1 *= s2);
   }

   template <typename TSet>
   Bitset& operator*= (const GenericSet<TSet, int, element_comparator>& s);

   template <typename TSet>
   friend
   Bitset&& operator* (Bitset&& s1, const GenericSet<TSet, int, element_comparator>& s2)
   {
      return std::move(s1 *= s2);
   }

   template <typename TSet>
   friend
   Bitset&& operator* (const GenericSet<TSet, int, element_comparator>& s1, Bitset&& s2)
   {
      return std::move(s2 *= s1);
   }

   Bitset& operator^= (int i)
   {
      if (mpz_tstbit(rep, i))
         mpz_clrbit(rep, i);
      else
         mpz_setbit(rep, i);
      return *this;
   }

   friend
   Bitset operator^ (const Bitset& s1, int i2)
   {
      Bitset result(s1);
      result ^= i2;
      return result;
   }

   friend
   Bitset operator^ (int i1, const Bitset& s2)
   {
      return s2 ^ i1;
   }

   friend
   Bitset&& operator^ (Bitset&& s1, int i2)
   {
      return std::move(s1 ^= i2);
   }

   friend
   Bitset&& operator^ (int i1, Bitset&& s2)
   {
      return std::move(s2 ^= i1);
   }

   Bitset& operator^= (const Bitset& s)
   {
      mpz_xor(rep, rep, s.rep);
      return *this;
   }

   friend
   Bitset operator^ (const Bitset& s1, const Bitset& s2)
   {
      Bitset result;
      mpz_xor(result.rep, s1.rep, s2.rep);
      return result;
   }

   friend
   Bitset&& operator^ (Bitset&& s1, const Bitset& s2)
   {
      return std::move(s1 ^= s2);
   }

   friend
   Bitset&& operator^ (const Bitset& s1, Bitset&& s2)
   {
      return std::move(s2 ^= s1);
   }

   template <typename TSet>
   Bitset& operator^= (const GenericSet<TSet, int, element_comparator>& s)
   {
      for (auto e=entire(s.top()); !e.at_end(); ++e)
         *this ^= *e;
      return *this;
   }

   template <typename TSet>
   friend
   Bitset&& operator^= (Bitset&& s1, const GenericSet<TSet, int, element_comparator>& s2)
   {
      return std::move(s1 ^= s2);
   }

   template <typename TSet>
   friend
   Bitset&& operator^= (const GenericSet<TSet, int, element_comparator>& s1, Bitset&& s2)
   {
      return std::move(s2 ^= s1);
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

   void erase(const iterator& where, const iterator& end) {
      iterator it(where);
      while (it!=end) *this -= *it;
   }

   void pop_front() { *this -= front(); }

   void pop_back() { *this -= back(); }

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

   mpz_srcptr get_rep() const noexcept { return rep; }

   operations::cmp get_comparator() const { return operations::cmp(); }

protected:
   template <typename, typename, typename> friend class GenericMutableSet;

   template <typename Iterator>
   void assign(Iterator&& src)
   {
      for (; !src.at_end(); ++src)
         mpz_setbit(rep, *src);
   }

   template <typename Iterator>
   void assign(Iterator&& src, Iterator&& src_end)
   {
      for (; src!=src_end; ++src)
         mpz_setbit(rep, *src);
   }

   void assign(const GenericSet<Bitset>& src)
   {
      mpz_set(rep,src.top().get_rep());
   }

   template <typename T>
   void assign(const GenericSet<T,int>& src)
   {
      assign_from(src.top());
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

   static void difference(mpz_ptr dst, mpz_srcptr src1, mpz_srcptr src2);

public:
   // consume data as if designated for Set<int>
   template <typename Input> friend
   Input& operator>> (GenericInput<Input>& in, Bitset& me)
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

/// See incl(GenericSet,GenericSet)
int incl(const Bitset& s1, const Bitset& s2) noexcept;

template <typename TSet>
Bitset& Bitset::operator*= (const GenericSet<TSet, int, element_comparator>& s)
{
   mp_limb_t *d=rep[0]._mp_d;
   mp_limb_t limb=0;
   auto e=entire(s.top());
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

template <typename Permutation> inline
Bitset permuted(const Bitset& s, const Permutation& perm)
{
   Bitset result;
   for (typename ensure_features<Permutation, cons<end_sensitive,indexed> >::const_iterator p=ensure(perm, (cons<end_sensitive,indexed>*)0).begin();  !p.at_end();  ++p)
      if (s.contains(*p)) result.insert(p.index());
   return result;

}

template <> struct check_iterator_feature<Bitset_iterator, end_sensitive> : std::true_type {};
template <> struct check_iterator_feature<Bitset_iterator, rewindable> : std::true_type {};

template <>
struct hash_func<Bitset, is_set> : hash_func<MP_INT> {
   size_t operator() (const Bitset& s) const
   {
      return impl(s.get_rep());
   }
};

} // end namespace pm

namespace polymake {
   using pm::Bitset;
}

namespace std {
   inline void swap(pm::Bitset& s1, pm::Bitset& s2) noexcept { s1.swap(s2); }
}

#endif // POLYMAKE_BITSET_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
