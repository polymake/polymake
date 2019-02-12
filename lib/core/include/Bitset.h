/* Copyright (c) 1997-2018
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

template <bool is_reversed>
class Bitset_iterator {
   friend class Bitset;
public:
   using iterator_category = bidirectional_iterator_tag;
   using value_type = int;
   using reference = int;
   using pointer = const mp_bitcnt_t*;
   using difference_type = ptrdiff_t;
   using iterator = Bitset_iterator;
   using const_iterator = Bitset_iterator;
protected:
   mpz_srcptr bits;
   mp_bitcnt_t cur;

   static constexpr int bits_per_limb
#if __GNU_MP_VERSION_MINOR > 0 || __GNU_MP_VERSION >=5
     =GMP_NUMB_BITS
#else
     =__GMP_BITS_PER_MP_LIMB
#endif
   ;

   static constexpr mp_bitcnt_t end_pos = std::numeric_limits<mp_bitcnt_t>::max();

   Bitset_iterator(mpz_srcptr bits_arg, mp_bitcnt_t cur_arg)
      : bits(bits_arg)
      , cur(cur_arg) {}

public:
   Bitset_iterator()
      : bits(nullptr)
      , cur(0) {}

   explicit Bitset_iterator(mpz_srcptr bits_arg)
      : bits(bits_arg)
      , cur(first_pos(bits)) {}

   reference operator* () const { return cur; }
   pointer operator-> () const { return &cur; }

   iterator& operator++ ()
   {
      is_reversed ? prev_pos() : next_pos();
      return *this;
   }

   iterator& operator-- ()
   {
      is_reversed ? next_pos() : prev_pos();
      return *this;
   }

   iterator operator++ (int) { iterator copy=*this; operator++(); return copy; }
   iterator operator-- (int) { iterator copy=*this; operator--(); return copy; }

   bool operator== (const iterator& it) const { return cur==it.cur; }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   bool at_end() const
   {
      return cur == end_pos;
   }

   void rewind() { cur = first_pos(bits); }
      
private:
   static mp_bitcnt_t first_pos(mpz_srcptr bits)
   {
      const int n = mpz_size(bits);
      if (n == 0) return end_pos;
      return is_reversed
             ? (n - 1) * bits_per_limb + log2_floor(mpz_getlimbn(bits, n - 1))
             : mpz_scan1(bits, 0);
   }

   void next_pos()
   {
      cur = mpz_scan1(bits, ++cur);
   }

   void prev_pos()
   {
      if (cur == end_pos) {
         cur = first_pos(bits);
      } else if (cur == 0) {
         cur = end_pos;
      } else {
         --cur;
         int n = cur / bits_per_limb;
         const int skip = bits_per_limb - 1 - cur % bits_per_limb;
         mp_limb_t limb = (mpz_getlimbn(bits, n) << skip) >> skip;
         for (;;) {
            if (limb != 0) {
               cur = n * bits_per_limb + log2_floor(limb);
               break;
            }
            if (--n < 0) {
               cur = end_pos;
               break;
            }
            limb = mpz_getlimbn(bits, n);
         }
      }
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
   template <typename Left, typename Right, typename Controller, typename = void>
   struct custom_op : std::false_type {};

   template <typename T>
   using is_Bitset = std::is_same<pure_type_t<T>, Bitset>;

   template <typename T>
   using is_element = std::is_same<pure_type_t<T>, int>;

   template <typename T>
   using propagate_rvalue = std::is_same<T, Bitset>;

   template <typename Left, typename Right>
   struct custom_op<Left, Right, set_union_zipper,
                    std::enable_if_t<is_Bitset<Left>::value && is_Bitset<Right>::value>> : std::true_type {
      using type = Bitset;
      static type make(Left&& l, Right&& r)
      {
         Bitset result;
         mpz_ior(result.rep, l.rep, r.rep);
         return result;
      }
   };

   template <typename Left, typename Right>
   struct custom_op<Left, Right, set_union_zipper,
                    std::enable_if_t<is_Bitset<Left>::value && is_element<Right>::value>> : std::true_type {
      using type = Bitset;
      static type make(Left&& l, Right&& r)
      {
         Bitset result(l);
         result += r;
         return result;
      }
   };

   template <typename Left, typename Right>
   struct custom_op<Left, Right, set_union_zipper,
                    std::enable_if_t<is_element<Left>::value && is_Bitset<Right>::value>> : std::true_type {
      using type = Bitset;
      static type make(Left&& l, Right&& r)
      {
         Bitset result(r);
         result += l;
         return result;
      }
   };

   template <typename Left, typename Right>
   struct custom_op<Left, Right, set_difference_zipper,
                    std::enable_if_t<is_Bitset<Left>::value && is_Bitset<Right>::value>> : std::true_type {
      using type = Bitset;
      static type make(Left&& l, Right&& r)
      {
         Bitset result;
         difference(result.rep, l.rep, r.rep);
         return result;
      }
   };

   template <typename Left, typename Right>
   struct custom_op<Left, Right, set_difference_zipper,
                    std::enable_if_t<is_Bitset<Left>::value && is_element<Right>::value>> : std::true_type {
      using type = Bitset;
      static type make(Left&& l, Right&& r)
      {
         Bitset result(l);
         result -= r;
         return result;
      }
   };

   template <typename Left, typename Right>
   struct custom_op<Left, Right, set_intersection_zipper,
                    std::enable_if_t<is_Bitset<Left>::value && is_Bitset<Right>::value>> : std::true_type {
      using type = Bitset;
      static type make(Left&& l, Right&& r)
      {
         Bitset result;
         mpz_and(result.rep, l.rep, r.rep);
         return result;
      }
   };

   template <typename Left, typename Right>
   struct custom_op<Left, Right, set_intersection_zipper,
                    std::enable_if_t<is_Bitset<Left>::value && is_element<Right>::value>> : std::true_type {
      using type = Bitset;
      static type make(Left&& l, Right&& r)
      {
         Bitset result(l);
         result *= r;
         return result;
      }
   };

   template <typename Left, typename Right>
   struct custom_op<Left, Right, set_intersection_zipper,
                    std::enable_if_t<is_element<Left>::value && is_Bitset<Right>::value>> : std::true_type {
      using type = Bitset;
      static type make(Left&& l, Right&& r)
      {
         Bitset result(r);
         result *= l;
         return result;
      }
   };

   template <typename Left, typename Right>
   struct custom_op<Left, Right, set_symdifference_zipper,
                    std::enable_if_t<is_Bitset<Left>::value && is_Bitset<Right>::value>> : std::true_type {
      using type = Bitset;
      static type make(Left&& l, Right&& r)
      {
         Bitset result;
         mpz_xor(result.rep, l.rep, r.rep);
         return result;
      }
   };

   template <typename Left, typename Right>
   struct custom_op<Left, Right, set_symdifference_zipper,
                    std::enable_if_t<is_Bitset<Left>::value && is_element<Right>::value>> : std::true_type {
      using type = Bitset;
      static type make(Left&& l, Right&& r)
      {
         Bitset result(l);
         result ^= r;
         return result;
      }
   };

   template <typename Left, typename Right>
   struct custom_op<Left, Right, set_symdifference_zipper,
                    std::enable_if_t<is_element<Left>::value && is_Bitset<Right>::value>> : std::true_type {
      using type = Bitset;
      static type make(Left&& l, Right&& r)
      {
         Bitset result(r);
         result ^= l;
         return result;
      }
   };

public:
   using value_type = int;
   using const_reference = const int;
   using reference = const_reference;
   using iterator = Bitset_iterator<false>;
   using const_iterator = iterator;
   using reverse_iterator = Bitset_iterator<true>;
   using const_reverse_iterator = reverse_iterator;

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
      : Bitset(std::move(s.rep)) {}

   /// Copy the value from a third party
   explicit Bitset(const mpz_t& b)
   {
      mpz_set(rep, b);
   }

   /// Steal the value from a third party
   /// The source must be re-initialized if it's going to be used afterwards
   explicit Bitset(mpz_t&& b) noexcept
   {
      Integer::set_finite(rep, b[0], Integer::initialized::no);
   }

   ~Bitset() noexcept
   {
      if (rep[0]._mp_d) mpz_clear(rep);
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
             typename=std::enable_if_t<std::is_convertible<E2, int>::value>>
   explicit Bitset(const GenericSet<TSet, E2, Comparator2>& s)
      : Bitset()
   {
      assign_from(s.top());
   }

   template <typename TContainer,
             typename=std::enable_if_t<isomorphic_to_container_of<TContainer, int, is_set>::value>>
   Bitset(const TContainer& src)
      : Bitset()
   {
      assign_from(src);
   }

   template <typename Iterator,
             typename=std::enable_if_t<assess_iterator_value<Iterator, std::is_convertible, int>::value>>
   Bitset(Iterator&& src, Iterator&& src_end)
      : Bitset()
   {
      assign(ensure_private_mutable(std::forward<Iterator>(src)), src_end);
   }

   template <typename Iterator>
   explicit Bitset(Iterator&& src,
                   std::enable_if_t<(assess_iterator<Iterator, check_iterator_feature, end_sensitive>::value &&
                                     assess_iterator_value<Iterator, std::is_convertible, int>::value), void**> =nullptr)
      : Bitset()
   {
      assign(ensure_private_mutable(std::forward<Iterator>(src)));
   }

   template <typename E2,
             typename=std::enable_if_t<std::is_convertible<E2, int>::value>>
   Bitset(std::initializer_list<E2> l)
      : Bitset()
   {
      *this = l;
   }

   template <typename E2, typename Comparator2,
             typename=std::enable_if_t<std::is_convertible<E2, int>::value>>
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

   Bitset& operator= (mpz_t&& s) noexcept
   {
      Integer::set_finite(rep, s[0], Integer::initialized::yes);
      return *this;
   }

   Bitset& operator= (Bitset&& s) noexcept
   {
      return operator=(std::move(s.rep));
   }

   template <typename E2,
             typename=std::enable_if_t<std::is_convertible<E2, int>::value>>
   Bitset& operator= (std::initializer_list<E2> l)
   {
      clear();
      if (l.size() > 0) {
         reserve(l.end()[-1] + 1);
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

   /// Synonym for reserve, for compatibility with other set classes+}
   void resize(int n)
   {
      reserve(n);
   }

   /// Make the set empty.
   void clear()
   {
      mpz_set_ui(rep, 0);
   }

   /// Assign elements from an abstract set of integers.
   template <typename Set2>
   std::enable_if_t<!std::is_same<Set2, Bitset>::value, Bitset&>
   operator= (const GenericSet<Set2, int, element_comparator>& s)
   {
      clear();
      assign_from(s.top());
      return *this;
   }

   template <typename E2, typename Comparator2,
             typename=std::enable_if_t<std::is_convertible<E2, int>::value>>
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
      return iterator::first_pos(rep);
   }

   int back() const noexcept
   {
      return reverse_iterator::first_pos(rep);
   }

   /** Insert an element.
       This is the quickest way to manipulate single elements. */
   Bitset& operator+= (int i)
   {
      mpz_setbit(rep, i);
      return *this;
   }

   Bitset& operator+= (const GenericSet<Bitset>& s)
   {
      mpz_ior(rep, rep, s.top().rep);
      return *this;
   }

   template <typename Set2>
   std::enable_if_t<!std::is_same<Set2, Bitset>::value, Bitset&>
   operator+= (const GenericSet<Set2, int, element_comparator>& s)
   {
      for (auto e=entire(s.top()); !e.at_end(); ++e)
         *this += *e;
      return *this;
   }

   /** Remove an element if it existed.
       This is the quickest way to manipulate single elements. */
   Bitset& operator-= (int i)
   {
      mpz_clrbit(rep, i);
      return *this;
   }

   Bitset& operator-= (const GenericSet<Bitset>& s)
   {
      difference(rep, rep, s.top().rep);
      return *this;
   }

   template <typename Set2>
   std::enable_if_t<!std::is_same<Set2, Bitset>::value, Bitset&>
   operator-= (const GenericSet<Set2, int, element_comparator>& s)
   {
      for (auto e=entire(s.top()); !e.at_end(); ++e)
         *this -= *e;
      return *this;
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

   Bitset& operator*= (const GenericSet<Bitset>& s)
   {
      mpz_and(rep, rep, s.top().rep);
      return *this;
   }

   template <typename Set2>
   std::enable_if_t<!std::is_same<Set2, Bitset>::value, Bitset&>
   operator*= (const GenericSet<Set2, int, element_comparator>& s);

   Bitset& operator^= (int i)
   {
      if (mpz_tstbit(rep, i))
         mpz_clrbit(rep, i);
      else
         mpz_setbit(rep, i);
      return *this;
   }

   Bitset& operator^= (const GenericSet<Bitset>& s)
   {
      mpz_xor(rep, rep, s.top().rep);
      return *this;
   }

   template <typename Set2>
   std::enable_if_t<!std::is_same<Set2, Bitset>::value, Bitset&>
   operator^= (const GenericSet<Set2, int, element_comparator>& s)
   {
      for (auto e = entire(s.top()); !e.at_end(); ++e)
         *this ^= *e;
      return *this;
   }

   iterator begin() const { return iterator(rep); }

   iterator end() const { return iterator(rep, iterator::end_pos); }

   reverse_iterator rbegin() const { return reverse_iterator(rep); }

   reverse_iterator rend() const { return reverse_iterator(rep, iterator::end_pos); }

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

   void erase(const iterator& where, const iterator& end)
   {
      iterator it(where);
      while (it != end) *this -= *it;
   }

   void pop_front() { *this -= front(); }

   void pop_back() { *this -= back(); }

   bool operator== (const GenericSet<Bitset>& s2) const
   {
      return !mpz_cmp(rep, s2.top().rep);
   }

   template <typename Set2>
   std::enable_if_t<!std::is_same<Set2, Bitset>::value, bool>
   operator== (const GenericSet<Set2, int, element_comparator>& s) const
   {
      return generic_type::operator==(s);
   }

   template <typename Set2>
   bool operator!= (const GenericSet<Set2, int, element_comparator>& s) const
   {
      return !operator==(s);
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
      assign(entire<reversed>(src));
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
std::enable_if_t<!std::is_same<TSet, Bitset>::value, Bitset&>
Bitset::operator*= (const GenericSet<TSet, int, element_comparator>& s)
{
   mp_limb_t* d = rep[0]._mp_d;
   mp_limb_t limb = 0;
   auto e = entire(s.top());
   for (int i = 0, size = mpz_size(rep), first = 0, last = iterator::bits_per_limb;
        i < size;
        ++i, first = last, last += iterator::bits_per_limb) {
      for (;;) {
         if (e.at_end()) {
            *d++ &= limb;
            rep[0]._mp_size = i + (limb != 0);
            return *this;
         }
         int elem = *e;  ++e;
         if (elem >= last) {
            *d++ &= limb;
            limb = mp_limb_t(1) << elem-last;   // prepare for the next outer loop run
            break;
         } else {
            limb |= mp_limb_t(1) << elem-first;
         }
      }
   }
   return *this;
}

template <typename Permutation>
Bitset permuted(const Bitset& s, const Permutation& perm)
{
   Bitset result;
   for (auto p=entire<indexed>(perm);  !p.at_end();  ++p)
      if (s.contains(*p)) result.insert(p.index());
   return result;

}

template <bool is_reversed>
struct check_iterator_feature<Bitset_iterator<is_reversed>, end_sensitive>
   : std::true_type {};

template <bool is_reversed>
struct check_iterator_feature<Bitset_iterator<is_reversed>, rewindable>
   : std::true_type {};

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
