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
--------------------------------------------------------------------------------
*/

#pragma once

#include "polymake/GenericMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Map.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/Bitset.h"
#include "polymake/hash_map"
#include "polymake/vector"
#include "polymake/list"
#include "polymake/optional"
#include <algorithm>
#include <sstream>

namespace pm {

template <typename Permutation>
int permutation_sign(const Permutation& v)
{
   const Int l = v.size();
   if (l <= 1) return 1;

   std::vector<Int> w(v.size());
   copy_range(v.begin(), entire(w));

   int sign = 1;
   for (Int i = 0; i < l; ) {
      if (w[i] != i) {
         Int k = w[i];
         w[i] = w[k];
         w[k] = k;
         sign = -sign;
      } else {
         ++i;
      }
   }
   return sign;
}

template <typename Permutation>
Int n_fixed_points(const Permutation& p)
{
   Int i(0), n(0);
   for (auto pit = entire(p); !pit.at_end(); ++pit, ++i)
      if (*pit == i)
         ++n;
   return n;
}

template <typename Permutation, typename InvPermutation>
void inverse_permutation(const Permutation& perm, InvPermutation& inv_perm)
{
   inv_perm.resize(perm.size());
   Int pos = 0;
   for (auto i = entire(perm);  !i.at_end();  ++i, ++pos)
      inv_perm[*i] = pos;
}

template <typename Permutation>
class permutation_cycles_iterator {
public:
   typedef forward_iterator_tag iterator_category;
   typedef std::list<Int> value_type;
   typedef const value_type& reference;
   typedef const value_type* pointer;
   typedef ptrdiff_t difference_type;
   typedef permutation_cycles_iterator iterator;
   typedef iterator const_iterator;

protected:
   typedef typename Permutation::const_iterator perm_iterator;
   Int first, limit;
   Bitset visited;
   value_type value;
   perm_iterator cur;

   void valid_position()
   {
      for (; first < limit; ++first, ++cur)
         if (first != *cur && !visited.contains(first)) {
            Int v = first;
            do {
               visited += v;
               value.push_back(v);
               const Int prev = v;
               v = *cur;
               std::advance(cur, v-prev);
            } while (v != first);
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
   Int size() const { return count_it(begin()); }
};

template <typename Permutation>
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

template <typename Value, typename Comparator, typename ExpectDuplicates, typename = void>
struct permutation_map {
   using type = Map<Value, Int, ComparatorTag<Comparator>, MultiTag<ExpectDuplicates>>;
};

template <typename Value>
struct permutation_map<Value, operations::cmp, std::false_type, std::enable_if_t<!std::numeric_limits<Value>::is_specialized && is_ordered<Value>::value>> {
   using type = Map<Value, Int>;
};

template <typename Value>
struct permutation_map<Value, operations::cmp, std::false_type, std::enable_if_t<std::numeric_limits<Value>::is_specialized>> {
   using type = hash_map<Value, Int>;
};

template <typename Value>
struct permutation_map<Value, operations::cmp, std::true_type, std::enable_if_t<!std::numeric_limits<Value>::is_specialized && is_ordered<Value>::value>> {
   using type = Map<Value, Int, MultiTag<std::true_type>>;
};

template <typename Value>
struct permutation_map<Value, operations::cmp, std::true_type, std::enable_if_t<std::numeric_limits<Value>::is_specialized>> {
   using type = hash_map<Value, Int, MultiTag<std::true_type>>;
};

template <typename Input1, typename Input2, typename Output, typename Comparator, typename ExpectDuplicates>
bool find_permutation_impl(Input1&& src1, Input2&& src2, Output&& dst, const Comparator& comparator, ExpectDuplicates)
{
   typename permutation_map<typename iterator_traits<Input1>::value_type, Comparator, ExpectDuplicates>::type index_map;

   for (Int i = 0; !src1.at_end(); ++src1, ++i)
      index_map.emplace(*src1, i);

   for (; !src2.at_end(); ++src2, ++dst) {
      const auto where = index_map.find(*src2);
      if (where == index_map.end())
         return false;
      *dst = where->second;
      index_map.erase(where);
   }

   return index_map.empty();
}

template <typename Container1, typename Container2, typename Comparator = polymake::operations::cmp>
optional<Array<Int>>
find_permutation(const Container1& c1, const Container2& c2, const Comparator& comparator = Comparator())
{
   Array<Int> perm(c1.size());
   if (find_permutation_impl(entire(c1), entire(c2), perm.begin(), comparator, std::false_type()))
      return make_optional(std::move(perm));
   else
      return nullopt;
}

template <typename Container1, typename Container2, typename Comparator = polymake::operations::cmp>
optional<Array<Int>>
find_permutation_with_duplicates(const Container1& c1, const Container2& c2, const Comparator& comparator = Comparator())
{
   Array<Int> perm(c1.size());
   if (find_permutation_impl(entire(c1), entire(c2), perm.begin(), comparator, std::true_type()))
      return make_optional(std::move(perm));
   else
      return nullopt;
}

template <typename Container1, typename Container2, typename Comparator = polymake::operations::cmp>
bool are_permuted(const Container1& c1, const Container2& c2, const Comparator& comparator = Comparator())
{
   return !!find_permutation(c1, c2, comparator);
}

template <typename Container1, typename Container2, typename Comparator=polymake::operations::cmp>
bool are_permuted_with_duplicates(const Container1& c1, const Container2& c2, const Comparator& comparator = Comparator())
{
   return !!find_permutation_with_duplicates(c1, c2, comparator);
}

template <typename Container, typename Permutation>
std::enable_if_t<std::is_same<typename object_traits<Container>::generic_tag, is_container>::value,
                 typename object_traits<Container>::persistent_type>
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

template <typename Container, typename Permutation>
std::enable_if_t<std::is_same<typename object_traits<Container>::generic_tag, is_container>::value,
                 typename object_traits<Container>::persistent_type>
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

namespace operations {

template <typename Container, typename Permutation>
struct permute {
   using first_argument_type = const Container&;
   using second_argument_type = const Permutation&;
   using result_type = decltype(pm::permuted(std::declval<first_argument_type>(), std::declval<second_argument_type>()));

   result_type operator() (first_argument_type c, second_argument_type perm) const
   {
      return pm::permuted(c, perm);
   }
};

template <typename Container, typename Permutation>
struct permute_inv {
   using first_argument_type = const Container&;
   using second_argument_type = const Permutation&;
   using result_type = decltype(pm::permuted_inv(std::declval<first_argument_type>(), std::declval<second_argument_type>()));

   result_type operator() (first_argument_type c, second_argument_type perm) const
   {
      return pm::permuted_inv(c, perm);
   }
};

}

template <typename Container, typename Permutation>
std::enable_if_t<std::is_same<typename object_traits<Container>::model, is_container>::value,
                 Array<Container>>
permuted_elements(const Array<Container>& arr, const Permutation& perm)
{
   return Array<Container>(arr.size(), attach_operation(arr, same_value(perm), operations::permute<Container, Permutation>()).begin());
}

template <typename Container, typename Permutation>
std::enable_if_t<std::is_same<typename object_traits<Container>::model, is_container>::value,
                 Array<Container>>
permuted_elements_inv(const Array<Container>& arr, const Permutation& perm)
{
   return Array<Container>(arr.size(), attach_operation(arr, same_value(perm), operations::permute_inv<Container, Permutation>()).begin());
}

template <typename Container, typename Comparator, typename Permutation>
std::enable_if_t<std::is_same<typename object_traits<Container>::model, is_container>::value,
                 Set<Container, Comparator>>
permuted_elements(const Set<Container, Comparator>& set, const Permutation& perm)
{
   return Set<Container, Comparator>(entire(attach_operation(set, same_value(perm), operations::permute<Container, Permutation>())));
}

template <typename Container, typename Comparator, typename Permutation>
std::enable_if_t<std::is_same<typename object_traits<Container>::model, is_container>::value,
                 Set<Container, Comparator>>
permuted_elements_inv(const Set<Container, Comparator>& set, const Permutation& perm)
{
   return Set<Container, Comparator>(entire(attach_operation(set, same_value(perm), operations::permute_inv<Container, Permutation>())));
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
   using iterator_category = forward_iterator_tag;
   using value_type = Array<Int>;
   using reference = value_type;
   using pointer = const value_type*;
   using difference_type = ptrdiff_t;
protected:
   value_type perm;

   permutation_iterator_base() {}
   permutation_iterator_base(Int n) : perm(n) { reset(n); }
   void reset(Int n) { copy_range(entire(sequence(0, n)), perm.begin()); }

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
   using iterator = permutation_iterator;
   using const_iterator = iterator;

protected:
   std::vector<Int> cnt;
   Int n, pos;

   permutation_iterator(Int n_arg, bool)
      : permutation_iterator_base(n_arg)
      , cnt(n_arg, 0)
      , n(n_arg)
      , pos(n_arg) {}

public:
   permutation_iterator()
      : n(0)
      , pos(0) {}

   explicit permutation_iterator(Int n_arg)
      : permutation_iterator_base(n_arg)
      , cnt(size_t(n_arg), 0)
      , n(n_arg)
      , pos(n_arg > 1) {}

   iterator& operator++ ()
   {
      do {
         if (cnt[pos] < pos) {
            std::swap(perm[pos], perm[pos%2 * cnt[pos]]);
            ++cnt[pos];
            pos = 1;
            break;
         }
         cnt[pos] = 0;
      } while (++pos < n);
      return *this;
   }
   const iterator operator++ (int) { iterator copy(*this); operator++(); return copy; }

   bool at_end() const { return pos>=n; }

   bool operator== (const iterator& it) const
   {
      return n == it.n && pos == it.pos && cnt == it.cnt;
   }
   bool operator!= (const iterator& it) const
   {
      return !operator==(it);
   }

   void rewind()
   {
      reset(n);
      pos = 1;
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
   std::vector<Int> move;
   Int n, next;

   void incr()
   {
      if (next == 0) {
         next = -1; return;
      }
      Int lim = n, offset = 0;
      auto pos = move.begin();
      for (;;) {
         const Int m = *pos;
         if (m > 0) {
            if (m < lim) {
               next = m+offset;
               ++*pos;
               break;
            }
         } else {
            if (m < -1) {
               next = offset-m-1;
               ++*pos;
               break;
            }
            ++offset;
         }
         if (--lim > 1) {
            *pos = -m;
            ++pos;
         } else {
            next = 0;
            break;
         }
      }
   }

public:
   permutation_iterator() : n(0) {}

   explicit permutation_iterator(Int n_arg)
      : permutation_iterator_base(n_arg)
      , move(size_t(n_arg), 1)
      , n(n_arg)
      , next(n-1)
   {
      if (next > 0) {
         next = 1; move[0] = 2;
      }
   }

   iterator& operator++ ()
   {
      std::swap(perm[next-1], perm[next]);
      incr();
      return *this;
   }
   const iterator operator++ (int) { iterator copy(*this); operator++(); return copy; }

   Int next_swap() const { return next; }

   bool at_end() const { return next < 0; }

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
      next = n-1;
      if (next > 1) {
         next = 1; move[0] = 2;
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
   bool at_end_;

public:
   permutation_iterator() {}

   explicit permutation_iterator(Int n, bool end_arg = false)
      : permutation_iterator_base(n), at_end_(end_arg || n == 0) {}

   iterator& operator++ ()
   {
      at_end_ = !std::next_permutation(perm.begin(), perm.end());
      return *this;
   }
   const iterator operator++ (int) { iterator copy(*this); operator++(); return copy; }

   bool at_end() const { return at_end_; }

   bool operator== (const iterator& it) const
   {
      return at_end_ ? it.at_end_ : !it.at_end_ && perm == it.perm;
   }
   bool operator!= (const iterator& it) const
   {
      return !operator==(it);
   }

   void rewind()
   {
      reset(perm.size());
      at_end_ = perm.empty();
   }
};

template <permutation_sequence kind>
struct check_iterator_feature<permutation_iterator<kind>, end_sensitive> : std::true_type {};
template <permutation_sequence kind>
struct check_iterator_feature<permutation_iterator<kind>, rewindable> : std::true_type {};

template <permutation_sequence kind>
class AllPermutations {
protected:
   Int n;
public:
   explicit AllPermutations(Int n_arg = 0) : n(n_arg) {}

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

   Int size() const { return n != 0 ? static_cast<Int>(Integer::fac(n)) : 0; } 
   bool empty() const { return n == 0; }
};

template <permutation_sequence kind>
struct spec_object_traits< AllPermutations<kind> >
   : spec_object_traits<is_container> {
   static const bool is_always_const=true;
};

inline
AllPermutations<> all_permutations(Int n)
{
   return AllPermutations<>(n);
}

template <permutation_sequence kind>
AllPermutations<kind> all_permutations(Int n)
{
   return AllPermutations<kind>(n);
}


template <typename PermutationRef, typename Element = Int>
class PermutationMatrix
   : public GenericMatrix< PermutationMatrix<PermutationRef, Element>, Element > {
protected:
   using alias_t = alias<PermutationRef>;
   alias_t perm;
   mutable std::vector<Int> inv_perm;
public:
   using value_type = Element;
   using reference = const Element&;
   using const_reference = reference;

   template <typename Arg, typename=std::enable_if_t<std::is_constructible<alias_t, Arg>::value>>
   explicit PermutationMatrix(Arg&& perm_arg)
      : perm(std::forward<Arg>(perm_arg)) {}

   decltype(auto) get_perm() const { return *perm; }

   const std::vector<Int>& get_inv_perm() const
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
   const Element& operator() (Int i, Int j) const
   {
      const master& me=*static_cast<const master*>(this);
      return me.get_perm()[i]==j ? one_value<Element>() : zero_value<Element>();
   }
};

template <typename PermutationRef, typename Element>
class Rows< PermutationMatrix<PermutationRef, Element> >
   : public modified_container_pair_impl< Rows< PermutationMatrix<PermutationRef, Element> >,
                                          mlist< Container1RefTag< PermutationRef >,
                                                 Container2RefTag< same_value_container<const Element&> >,
                                                 OperationTag< SameElementSparseVector_factory<2> >,
                                                 MasqueradedTop > > {
   typedef modified_container_pair_impl<Rows> base_t;
protected:
   ~Rows();
public:
   decltype(auto) get_container1() const
   {
      return this->hidden().get_perm();
   }
   auto get_container2() const
   {
      return same_value_container<const Element&>(one_value<Element>());
   }
   typename base_t::operation get_operation() const
   {
      return this->size();
   }
};

template <typename PermutationRef, typename Element>
class Cols< PermutationMatrix<PermutationRef, Element> >
   : public modified_container_pair_impl< Cols< PermutationMatrix<PermutationRef, Element> >,
                                          mlist< Container1Tag< std::vector<Int> >,
                                                 Container2RefTag< same_value_container<const Element&> >,
                                                 OperationTag< SameElementSparseVector_factory<2> >,
                                                 MasqueradedTop > > {
   typedef modified_container_pair_impl<Cols> base_t;
protected:
   ~Cols();
public:
   decltype(auto) get_container1() const
   {
      return this->hidden().get_inv_perm();
   }
   auto get_container2() const
   {
      return same_value_container<const Element&>(one_value<Element>());
   }
   typename base_t::operation get_operation() const
   {
      return this->size();
   }
};

template <typename Element, typename Permutation>
auto permutation_matrix(Permutation&& perm)
{
   return PermutationMatrix<Permutation, Element>(std::forward<Permutation>(perm));
}

template <typename Permutation>
bool is_permutation(const Permutation& perm)
{
   // check that perm really is a permutation
   Set<typename Permutation::value_type> values;
   
   for (auto i=entire(perm); !i.at_end(); ++i) {
      if (*i >= perm.size() or *i < 0)
         return false;
      values.insert(*i);
   }
   return values.size() == perm.size();
}

template <typename Scalar>
Array<Array<Int>> rows_induced_from_cols(const Matrix<Scalar>& M, const Array<Array<Int>> G)
{
   Map<Vector<Scalar>, Int> RevPerm;
   Int index = 0;
   for (auto v = entire(rows(M)); !v.at_end(); ++v, ++index){
      RevPerm[*v] = index;
   }
   Array<Array<Int>> RowPerm(G.size());
   auto old_perm = entire(G);
       
   for (auto new_perm = entire(RowPerm); !new_perm.at_end(); ++new_perm, ++old_perm){
      new_perm->resize(M.rows());
      auto new_perm_entry = entire(*new_perm);
      for (auto v = entire(rows(M)); !v.at_end(); ++v, ++new_perm_entry){
         *new_perm_entry = RevPerm[permuted(*v,*old_perm)];
      }
   }
   return RowPerm;
}

template<typename Permutation, typename SubdomainType>
Array<Permutation>
permutation_subgroup_generators(const Array<Permutation>& gens,
                                const SubdomainType& subdomain)
{
   Map<Int, Int> index_of;
   Int index = 0;
   for (const auto s: subdomain)
      index_of[s] = index++;
   
   Set<Array<Int>> subgens;
   const Array<Int> id(sequence(0, subdomain.size()));
   
   for (const auto& g: gens) {
      Array<Int> candidate_gen(subdomain.size());
      bool candidate_ok(true);
      for (const auto i: subdomain) {
         if (!index_of.exists(g[i])) {
            candidate_ok = false;
            break;
         }
         candidate_gen[index_of[i]] = index_of[g[i]];
      }
      if (candidate_ok && candidate_gen != id)
         subgens += candidate_gen;
   }
   return Array<Permutation>(subgens.size(), subgens.begin());
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
   using pm::permutation_subgroup_generators;
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
