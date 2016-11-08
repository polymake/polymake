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

#ifndef POLYMAKE_PERMUTATIONS_H
#define POLYMAKE_PERMUTATIONS_H

#include "polymake/GenericMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Map.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/Bitset.h"
#include "polymake/vector"
#include "polymake/list"
#include <algorithm>
#include <sstream>

namespace pm {

template <typename Permutation>
int permutation_sign(const Permutation& v)
{
   const int l=v.size();
   if (l<=1) return 1;

   std::vector<int> w(v.size());
   copy_range(v.begin(), entire(w));

   int sign=1;
   for (int i=0; i<l; ) {
      if (w[i]!=i) {
         int k=w[i];
         w[i]=w[k];
         w[k]=k;
         sign = -sign;
      } else {
         ++i;
      }
   }
   return sign;
}

template <typename Permutation>
int n_fixed_points(const Permutation& p)
{
   int i(0), n(0);
   for (typename Entire<Permutation>::const_iterator pit = entire(p); !pit.at_end(); ++pit, ++i)
      if (*pit == i)
         ++n;
   return n;
}

template <typename Permutation, typename InvPermutation> inline
void inverse_permutation(const Permutation& perm, InvPermutation& inv_perm)
{
   inv_perm.resize(perm.size());
   int pos=0;
   for (typename Entire<Permutation>::const_iterator i=entire(perm);  !i.at_end();  ++i, ++pos)
      inv_perm[*i]=pos;
}

template <typename Permutation>
class permutation_cycles_iterator {
public:
   typedef forward_iterator_tag iterator_category;
   typedef std::list<int> value_type;
   typedef const value_type& reference;
   typedef const value_type* pointer;
   typedef ptrdiff_t difference_type;
   typedef permutation_cycles_iterator iterator;
   typedef iterator const_iterator;

protected:
   typedef typename Permutation::const_iterator perm_iterator;
   int first, limit;
   Bitset visited;
   value_type value;
   perm_iterator cur;

   void valid_position()
   {
      for ( ;first<limit ;++first, ++cur)
         if (first!=*cur && !visited.contains(first)) {
            int v=first;
            do {
               visited+=v;
               value.push_back(v);
               const int prev=v;
               v=*cur;
               std::advance(cur, v-prev);
            } while (v!=first);
            break;
         }
   }

public:
   permutation_cycles_iterator() {}
   permutation_cycles_iterator(const Permutation& perm)
      : first(0), limit(perm.size()), visited(limit), cur(perm.begin()) { valid_position(); }
   permutation_cycles_iterator(const Permutation& perm, bool /* end_position */)
      : first(perm.size()), limit(first), cur(perm.end()) {}

   reference operator* () const { return value; }
   pointer operator-> () const { return &value; }

   iterator& operator++ ()
   {
      value.clear();
      ++first; ++cur;
      valid_position();
      return *this;
   }
   const iterator operator++ (int) { iterator copy=*this; operator++(); return copy; }

   bool operator== (const permutation_cycles_iterator& it) const { return cur==it.cur; }
   bool operator!= (const permutation_cycles_iterator& it) const { return !operator==(it); }

   bool at_end() const { return first>=limit; }
};

template <typename Permutation>
class PermutationCycles {
protected:
   PermutationCycles();
   ~PermutationCycles();

   const Permutation& hidden() const { return reinterpret_cast<const Permutation&>(*this); }
public:
   typedef permutation_cycles_iterator<Permutation> iterator;
   typedef iterator const_iterator;
   typedef typename iterator::value_type value_type;
   typedef typename iterator::reference reference;
   typedef reference const_reference;

   iterator begin() const { return hidden(); }
   iterator end() const { return iterator(hidden(), true); }
   value_type front() const { return value_type(hidden().begin(), 0, hidden().size()); }

   bool empty() const { return begin().at_end(); }
   int size() const { return count_it(begin()); }
};

template <typename Permutation> inline
const PermutationCycles<Permutation>& permutation_cycles(const Permutation& p)
{
   return reinterpret_cast<const PermutationCycles<Permutation>&>(p);
}

template <typename Permutation>
struct check_iterator_feature<permutation_cycles_iterator<Permutation>, end_sensitive> : std::true_type {};

template <typename Permutation>
struct spec_object_traits< PermutationCycles<Permutation> >
   : spec_object_traits<is_container> {
   static const bool is_lazy=true, is_always_const=true;
   typedef Permutation masquerade_for;
   static const IO_separator_kind IO_separator=IO_sep_inherit;
};

template <typename Input1, typename Input2, typename Output, typename Comparator>
void find_permutation(Input1 src1, Input2 src2, Output dst, const Comparator& comparator)
{
   typedef Map<typename iterator_traits<Input1>::value_type, int, Comparator> map_type;
   map_type index_map(comparator);

   for (int i=0; !src1.at_end(); ++src1, ++i)
      index_map[*src1]=i;

   for (; !src2.at_end(); ++src2, ++dst) {
      typename map_type::iterator where=index_map.find(*src2);
      if (where.at_end()) {
         std::string reason;
         if (index_map.empty()) {
            reason="not a permutation: first sequence is shorter";
         } else {
            std::ostringstream os;
            wrap(os) << "not a permutation: no match for <" << *src2 << ">";
            reason=os.str();
         }
         throw no_match(reason);
      }
      *dst=where->second;
      index_map.erase(where);
   }

   if (!index_map.empty())
      throw no_match("not a permutation: second sequence is shorter");
}

template <typename Container1, typename Container2, typename Comparator> inline
Array<int> find_permutation(const Container1& c1, const Container2& c2, const Comparator& comparator)
{
   Array<int> perm(c1.size());
   find_permutation(entire(c1), entire(c2), perm.begin(), comparator);
   return perm;
}

template <typename Container1, typename Container2> inline
Array<int> find_permutation(const Container1& c1, const Container2& c2)
{
   return find_permutation(c1,c2,polymake::operations::cmp());
}

template <typename Container1, typename Container2, typename Comparator> inline
bool are_permuted(const Container1& c1, const Container2& c2, const Comparator& comparator)
{
   Array<int> perm(c1.size());
   try {
      find_permutation(entire(c1), entire(c2), perm.begin(), comparator);
      return true;
   } catch (no_match) {
      return false;
   }
   return false;
}

template <typename Container1, typename Container2> inline
bool are_permuted(const Container1& c1, const Container2& c2)
{
   return are_permuted(c1,c2,polymake::operations::cmp());
}

template <typename Container, typename Permutation> inline
typename std::enable_if<std::is_same<typename object_traits<Container>::generic_tag, is_container>::value,
                        typename object_traits<Container>::persistent_type>::type
permuted(const Container& c, const Permutation& perm)
{
   if (POLYMAKE_DEBUG) {
      if (c.size() != perm.size())
         throw std::runtime_error("permuted - dimension mismatch");
   }
   typename object_traits<Container>::persistent_type result(c.size());
   copy_range(entire(select(c, perm)), result.begin());
   return result;
}

template <typename Container, typename Permutation> inline
typename std::enable_if<std::is_same<typename object_traits<Container>::generic_tag, is_container>::value,
                        typename object_traits<Container>::persistent_type>::type
permuted_inv(const Container& c, const Permutation& perm)
{
   if (POLYMAKE_DEBUG) {
      if (c.size() != perm.size())
         throw std::runtime_error("permuted_inv - dimension mismatch");
   }
   typename object_traits<Container>::persistent_type result(c.size());
   copy_range(entire(c), select(result,perm).begin());
   return result;
}


enum permutation_sequence {
   permutations_heap,
   permutations_adjacent,
   permutations_lex
};

template <permutation_sequence kind=permutations_heap>
class AllPermutations;

template <permutation_sequence kind>
class permutation_iterator;

class permutation_iterator_base {
public:
   typedef forward_iterator_tag iterator_category;
   typedef std::vector<int> value_type;
   typedef const value_type* pointer;
   typedef const value_type& reference;
   typedef ptrdiff_t difference_type;
protected:
   value_type perm;

   permutation_iterator_base() {}
   permutation_iterator_base(int n) : perm(n) { reset(n); }
   void reset(int n) { copy_range(entire(sequence(0, n)), perm.begin()); }

public:
   reference operator* () const { return perm; }
   pointer operator-> () const { return &perm; }
};

/// Implementation of the Heap's algorithm by R. Sedgewick
template <>
class permutation_iterator<permutations_heap>
   : public permutation_iterator_base {
   friend class AllPermutations<permutations_heap>;
public:
   typedef permutation_iterator iterator;
   typedef iterator const_iterator;

protected:
   std::vector<int> cnt;
   int n, pos;

   permutation_iterator(int n_arg, bool)
      : permutation_iterator_base(n_arg)
      , cnt(n_arg, 0)
      , n(n_arg)
      , pos(n_arg) {}

public:
   permutation_iterator()
      : n(0)
      , pos(0) {}

   explicit permutation_iterator(int n_arg)
      : permutation_iterator_base(n_arg)
      , cnt(size_t(n_arg), 0)
      , n(n_arg)
      , pos(n_arg>1) {}

   iterator& operator++ ()
   {
      do {
         if (cnt[pos]<pos) {
            std::swap(perm[pos], perm[pos%2 * cnt[pos]]);
            ++cnt[pos];
            pos=1;
            break;
         }
         cnt[pos]=0;
      } while (++pos < n);
      return *this;
   }
   const iterator operator++ (int) { iterator copy(*this); operator++(); return copy; }

   bool at_end() const { return pos>=n; }

   bool operator== (const iterator& it) const
   {
      return n==it.n && pos==it.pos && cnt==it.cnt;
   }
   bool operator!= (const iterator& it) const
   {
      return !operator==(it);
   }

   void rewind()
   {
      reset(n);
      pos=1;
      fill_range(entire(cnt), 0);
   }
};

template <>
class permutation_iterator<permutations_adjacent>
   : public permutation_iterator_base {
   friend class AllPermutations<permutations_adjacent>;
public:
   typedef permutation_iterator iterator;
   typedef iterator const_iterator;

protected:
   std::vector<int> move;
   int n, next;

   void incr()
   {
      if (next==0) {
         next=-1; return;
      }
      int lim=n, offset=0;
      std::vector<int>::iterator pos=move.begin();
      for (;;) {
         int m=*pos;
         if (m>0) {
            if (m<lim) {
               next=m+offset;
               ++*pos;
               break;
            }
         } else {
            if (m<-1) {
               next=offset-m-1;
               ++*pos;
               break;
            }
            ++offset;
         }
         if (--lim > 1) {
            *pos=-m;
            ++pos;
         } else {
            next=0;
            break;
         }
      }
   }

public:
   permutation_iterator() : n(0) {}

   explicit permutation_iterator(int n_arg)
      : permutation_iterator_base(n_arg)
      , move(size_t(n_arg), 1)
      , n(n_arg)
      , next(n-1)
   {
      if (next>0) {
         next=1; move[0]=2;
      }
   }

   iterator& operator++ ()
   {
      std::swap(perm[next-1], perm[next]);
      incr();
      return *this;
   }
   const iterator operator++ (int) { iterator copy(*this); operator++(); return copy; }

   int next_swap() const { return next; }

   bool at_end() const { return next<0; }

   bool operator== (const iterator& it) const
   {
      return next==it.next && n==it.n && (!next || move==it.move);
   }
   bool operator!= (const iterator& it) const
   {
      return !operator==(it);
   }

   void rewind()
   {
      reset(n);
      fill_range(entire(move), 1);
      next=n-1;
      if (next>1) {
         next=1; move[0]=2;
      }
   }
};

// permutations in lexicographic order via STL
template <>
class permutation_iterator<permutations_lex>
   : public permutation_iterator_base {
   friend class AllPermutations<permutations_lex>;
public:
   typedef permutation_iterator iterator;
   typedef iterator const_iterator;

protected:
   bool _at_end;

public:
   permutation_iterator() {}

   explicit permutation_iterator(int n, bool end_arg=false)
      : permutation_iterator_base(n), _at_end(end_arg || n==0) {}

   iterator& operator++ ()
   {
      _at_end=!std::next_permutation(perm.begin(), perm.end());
      return *this;
   }
   const iterator operator++ (int) { iterator copy(*this); operator++(); return copy; }

   bool at_end() const { return _at_end; }

   bool operator== (const iterator& it) const
   {
      return _at_end ? it._at_end : !it._at_end && perm==it.perm;
   }
   bool operator!= (const iterator& it) const
   {
      return !operator==(it);
   }

   void rewind()
   {
      reset(perm.size());
      _at_end=perm.empty();
   }
};

template <permutation_sequence kind>
struct check_iterator_feature<permutation_iterator<kind>, end_sensitive> : std::true_type {};
template <permutation_sequence kind>
struct check_iterator_feature<permutation_iterator<kind>, rewindable> : std::true_type {};

template <permutation_sequence kind>
class AllPermutations {
protected:
   int n;
public:
   explicit AllPermutations(int n_arg=0) : n(n_arg) {}

   typedef permutation_iterator<kind> iterator;
   typedef iterator const_iterator;
   typedef typename iterator::value_type value_type;
   typedef typename iterator::reference reference;
   typedef reference const_reference;

   iterator begin() const { return iterator(n); }
   iterator end() const { return iterator(n,true); }

   value_type front() const
   {
      return value_type(sequence(0,n).begin(), sequence(0,n).end());
   }
   //FIXME: ticket 727 changed from Integer to int to get perl acces 
   long size() const { return n ? long(Integer::fac(n)) : 0; } 
   bool empty() const { return n==0; }
};

template <permutation_sequence kind>
struct spec_object_traits< AllPermutations<kind> >
   : spec_object_traits<is_container> {
   static const bool is_always_const=true;
};

inline
AllPermutations<> all_permutations(int n)
{
   return AllPermutations<>(n);
}

template <permutation_sequence kind>
AllPermutations<kind> all_permutations(int n)
{
   return AllPermutations<kind>(n);
}


template <typename PermutationRef, typename Element=int>
class PermutationMatrix
   : public GenericMatrix< PermutationMatrix<PermutationRef,Element>, Element > {
protected:
   alias<PermutationRef> perm;
   mutable std::vector<int> inv_perm;
public:
   typedef Element value_type;
   typedef const Element& reference;
   typedef reference const_reference;

   typedef typename alias<PermutationRef>::arg_type arg_type;
   PermutationMatrix(arg_type perm_arg) : perm(perm_arg) {}

   typename alias<PermutationRef>::const_reference get_perm() const { return *perm; }

   const std::vector<int>& get_inv_perm() const
   {
      if (inv_perm.empty() && !get_perm().empty())
         inverse_permutation(get_perm(), inv_perm);
      return inv_perm;
   }
};

template <typename PermutationRef, typename Element>
struct spec_object_traits< PermutationMatrix<PermutationRef,Element> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true, is_always_const=true;
};

template <typename PermutationRef, typename Element>
struct check_container_feature< PermutationMatrix<PermutationRef,Element>, pure_sparse> : std::true_type {};

template <typename PermutationRef, typename Element>
class matrix_random_access_methods< PermutationMatrix<PermutationRef,Element> > {
   typedef PermutationMatrix<PermutationRef,Element> master;
public:
   const Element& operator() (int i, int j) const
   {
      const master& me=*static_cast<const master*>(this);
      return me.get_perm()[i]==j ? one_value<Element>() : zero_value<Element>();
   }
};

template <typename PermutationRef, typename Element>
class Rows< PermutationMatrix<PermutationRef, Element> >
   : public modified_container_pair_impl< Rows< PermutationMatrix<PermutationRef, Element> >,
                                          mlist< Container1Tag< PermutationRef >,
                                                 Container2Tag< constant_value_container<const Element&> >,
                                                 OperationTag< SameElementSparseVector_factory<2> >,
                                                 MasqueradedTop > > {
   typedef modified_container_pair_impl<Rows> base_t;
protected:
   ~Rows();
public:
   const typename base_t::container1& get_container1() const
   {
      return this->hidden().get_perm();
   }
   typename base_t::container2 get_container2() const
   {
      return one_value<Element>();
   }
   typename base_t::operation get_operation() const
   {
      return this->size();
   }
};

template <typename PermutationRef, typename Element>
class Cols< PermutationMatrix<PermutationRef, Element> >
   : public modified_container_pair_impl< Cols< PermutationMatrix<PermutationRef, Element> >,
                                          mlist< Container1Tag< std::vector<int> >,
                                                 Container2Tag< constant_value_container<const Element&> >,
                                                 OperationTag< SameElementSparseVector_factory<2> >,
                                                 MasqueradedTop > > {
   typedef modified_container_pair_impl<Cols> base_t;
protected:
   ~Cols();
public:
   const typename base_t::container1& get_container1() const
   {
      return this->hidden().get_inv_perm();
   }
   typename base_t::container2 get_container2() const
   {
      return one_value<Element>();
   }
   typename base_t::operation get_operation() const
   {
      return this->size();
   }
};

template <typename Element, typename Permutation> inline
PermutationMatrix<const Permutation&, Element>
permutation_matrix(const Permutation& perm)
{
   return perm;
}

template<typename Permutation> inline
bool is_permutation(const Permutation& perm){
   // check that perm really is a permutation
   Set<typename Permutation::value_type> values;
   
   for (typename Entire<Permutation>::const_iterator i=entire(perm);  !i.at_end();  ++i){
      if(*i >= perm.size() or *i < 0){
         return 0;
      }
      values.insert(*i);
   }
   if(values.size() != perm.size() ){
      return 0;
   }
   return 1;
}

template <typename Scalar>
Array<Array<int> > rows_induced_from_cols(const Matrix<Scalar>& M, const Array<Array<int> > G)
{
   Map<Vector<Scalar>,int> RevPerm;
   int index = 0;
   for (typename Entire<Rows<Matrix<Scalar > > >::const_iterator v = entire(rows(M)); !v.at_end(); ++v, ++index){
      RevPerm[*v] = index;
   }
   Array<Array<int> > RowPerm(G.size());
   Entire<Array<Array<int > > >::const_iterator old_perm = entire(G);
       
       
   for (Entire<Array<Array<int > > >::iterator new_perm = entire(RowPerm); !new_perm.at_end(); ++new_perm, ++old_perm){
      new_perm->resize(M.rows());
      Entire<Array<int > >::iterator new_perm_entry = entire(*new_perm);
      for (typename Entire<Rows<Matrix<Scalar > > >::const_iterator v = entire(rows(M)); !v.at_end(); ++v, ++new_perm_entry){
         *new_perm_entry = RevPerm[permuted(*v,*old_perm)];
      }
   }
   return RowPerm;
}

} // end namespace pm

namespace polymake {
   using pm::permutation_sign;
   using pm::inverse_permutation;
   using pm::permutation_cycles;
   using pm::find_permutation;
   using pm::permuted;
   using pm::permuted_inv;
   using pm::permutation_sequence;
   using pm::permutations_heap;
   using pm::permutations_adjacent;
   using pm::permutations_lex;
   using pm::AllPermutations;
   using pm::all_permutations;
   using pm::PermutationMatrix;
   using pm::permutation_matrix;
}

#endif // POLYMAKE_PERMUTATIONS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
