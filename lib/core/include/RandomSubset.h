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

#ifndef POLYMAKE_RANDOM_SUBSET_H
#define POLYMAKE_RANDOM_SUBSET_H

#include "polymake/IndexedSubset.h"
#include "polymake/RandomGenerators.h"
#include "polymake/vector"
#include <stdexcept>

namespace pm {

template <typename Iterator>
class RandomSubset_iterator : public Iterator {
   using base_t = Iterator;
public:
   using iterator_category = forward_iterator_tag;
   using iterator =RandomSubset_iterator<typename iterator_traits<Iterator>::iterator>;
   using const_iterator = RandomSubset_iterator<typename iterator_traits<Iterator>::const_iterator>;

   RandomSubset_iterator() = default;

   RandomSubset_iterator(const iterator& it)
      : base_t(static_cast<const typename iterator::base_t&>(it))
      , rg(it.rg)
      , k(it.k) {}

   template <typename SourceIterator, typename=typename suitable_arg_for_iterator<SourceIterator, Iterator>::type>
   RandomSubset_iterator(const SourceIterator& cur_arg, int n_arg, int k_arg, const SharedRandomState& random_arg)
      : base_t(prepare_iterator_arg<Iterator>(cur_arg))
      , rg(n_arg, random_arg)
      , k(k_arg)
   {
      toss(typename iterator_traits<Iterator>::iterator_category(), 0);
   }

   template <typename SourceIterator, typename=std::enable_if_t<is_const_compatible_with<SourceIterator, Iterator>::value>>
   RandomSubset_iterator(const SourceIterator& end_arg, const SharedRandomState& random_arg)
      : base_t(end_arg)
      , rg(0,random_arg)
      , k(0) {}

   RandomSubset_iterator& operator++()
   {
      --k; --(rg.upper_limit());
      toss(typename iterator_traits<Iterator>::iterator_category(), 1);
      return *this;
   }

   RandomSubset_iterator operator++ (int) { RandomSubset_iterator copy=*this; operator++(); return copy; }

   bool at_end() const { return k==0; }

protected:
   void toss(input_iterator_tag, int incr)
   {
      if (incr) base_t::operator++();
      while (rg.upper_limit() > 0 && rg.get() >= k) {
         --(rg.upper_limit());
         base_t::operator++();
      }
   }

   void toss(random_access_iterator_tag, int incr)
   {
      while (rg.upper_limit() > 0 && rg.get() >= k) {
         --(rg.upper_limit()); ++incr;
      }
      base_t::operator+=(incr);
   }

   UniformlyRandomRanged<long> rg;
   long k;

   template <typename> friend class RandomSubset_iterator;
private:
   // shadow possible base_t:: operators
   void operator--() = delete;
   void operator+=(int) = delete;
   void operator-=(int) = delete;
   void operator+(int) = delete;
   void operator-(int) = delete;
   void operator[](int) = delete;
};

template <typename Iterator>
struct check_iterator_feature<RandomSubset_iterator<Iterator>, end_sensitive> : std::true_type {};

template <typename ContainerRef>
class RandomSubset
   : public generic_of_subset<RandomSubset<ContainerRef>, typename deref<ContainerRef>::type> {
public:
   using base_type = typename deref<ContainerRef>::minus_ref;
   using value_type = typename container_traits<ContainerRef>::value_type;
   using reference = typename container_traits<ContainerRef>::reference;
   using const_reference = typename container_traits<ContainerRef>::const_reference;

protected:
   using alias_t = alias<ContainerRef>;
   using random_t = UniformlyRandom<long>;

   alias_t base;
   random_t random_source;
   long k;

public:
   RandomSubset() {}

   template <typename BaseArg, typename SeedArg,
             typename=std::enable_if_t<std::is_constructible<alias_t, BaseArg>::value && std::is_constructible<random_t, SeedArg>::value>>
   RandomSubset(BaseArg&& base_arg, long k_arg, SeedArg&& seed=RandomSeed())
      : base(std::forward<BaseArg>(base_arg))
      , random_source(std::forward<SeedArg>(seed))
      , k(k_arg)
   {
      if (POLYMAKE_DEBUG) {
         if (k<0 || k>get_base().size())
            throw std::runtime_error("RandomSubset constructor - invalid size");
      }
   }

   template <typename BaseArg,
             typename=std::enable_if_t<std::is_constructible<alias_t, BaseArg>::value>>
   RandomSubset(BaseArg&& base_arg, long k_arg)
      : RandomSubset(std::forward<BaseArg>(base_arg), k_arg, RandomSeed()) {}

   decltype(auto) get_base() { return *base; }
   decltype(auto) get_base() const { return *base; }

   using iterator = RandomSubset_iterator<typename container_traits<ContainerRef>::iterator>;
   using const_iterator = RandomSubset_iterator<typename container_traits<ContainerRef>::const_iterator>;

   iterator begin()
   {
      return iterator(get_base().begin(), get_base().size(), k, random_source);
   }
   iterator end()
   {
      return iterator(get_base().end());
   }
   const_iterator begin() const
   {
      return const_iterator(get_base().begin(), get_base().size(), k, random_source);
   }
   const_iterator end() const
   {
      return const_iterator(get_base().end());
   }

   long size() const { return k; }
   bool empty() const { return k==0; }
};

template <typename ContainerRef>
struct spec_object_traits< RandomSubset<ContainerRef> >
   : spec_object_traits<is_container> {
   static constexpr bool is_temporary = true, is_always_const = is_effectively_const<ContainerRef>::value;
};

class RandomPermutation_iterator {
public:
   using iterator_category = forward_iterator_tag;
   using value_type = int;
   using reference = const int&;
   using pointer = const int*;
   using difference_type = ptrdiff_t;
   using iterator = RandomPermutation_iterator;
   using const_iterator = RandomPermutation_iterator;

   RandomPermutation_iterator()
      : rg(0) {}

   RandomPermutation_iterator(const sequence& start_arg, const SharedRandomState& random_arg)
      : perm_index(start_arg.begin(), start_arg.end()), rg(start_arg.size(), random_arg)
   {
      if (!at_end()) toss();
   }

   explicit RandomPermutation_iterator(const SharedRandomState& random_arg)
      : rg(0, random_arg) {}

   reference operator* () const { return perm_index.back(); }
   pointer operator-> () const { return &operator*(); }

   iterator& operator++()
   {
      perm_index.pop_back();
      --(rg.upper_limit());
      if (!at_end()) toss();
      return *this;
   }

   const iterator operator++ (int) { iterator copy=*this; operator++(); return copy; }

   bool at_end() const { return perm_index.empty(); }

   bool operator== (const iterator& it) const
   {
      return perm_index.size() == it.perm_index.size()  &&
             std::equal(perm_index.begin(), perm_index.end(), it.perm_index.begin());
   }

   bool operator!= (const iterator& it) const { return !operator==(it); }
protected:
   void toss()
   {
      const int i=rg.get();
      std::swap(perm_index[i], perm_index.back());
   }

   std::vector<int> perm_index;
   UniformlyRandomRanged<long> rg;
};

template <>
struct check_iterator_feature<RandomPermutation_iterator, end_sensitive> : std::true_type {};

template <typename ContainerRef=sequence,
          bool is_direct=same_pure_type<ContainerRef, sequence>::value>
class RandomPermutation {
public:
   using value_type = int;
   using reference = const int&;
   using const_reference = reference;

   explicit RandomPermutation(const sequence& base_arg, const RandomSeed& seed=RandomSeed())
      : base(base_arg), random_source(seed) {}

   RandomPermutation(const sequence& base_arg, const SharedRandomState& s)
      : base(base_arg), random_source(s) {}

   explicit RandomPermutation(int n, const RandomSeed& seed=RandomSeed())
      : base(0,n), random_source(seed) {}

   RandomPermutation(int n, const SharedRandomState& random_arg)
      : base(0,n), random_source(random_arg) {}

   using iterator = RandomPermutation_iterator;
   using const_iterator = iterator;

   iterator begin() const
   {
      return iterator(base, random_source);
   }
   iterator end() const
   {
      return iterator(random_source);
   }

   int size() const { return base.size(); }
   bool empty() const { return base.empty(); }
protected:
   sequence base;
   UniformlyRandom<long> random_source;
};

template <typename ContainerRef>
class RandomPermutation<ContainerRef, false>
   : public indexed_subset_impl< RandomPermutation<ContainerRef, false>,
                                 mlist< Container1RefTag< ContainerRef >,
                                        Container2Tag< RandomPermutation<> > > > {
   using impl_t = indexed_subset_impl<RandomPermutation>;
protected:
   using alias_t = alias<ContainerRef>;
   alias_t base;
   RandomPermutation<> perm;
public:
   template <typename BaseArg, typename SeedArg,
             typename=std::enable_if_t<std::is_constructible<alias_t, BaseArg>::value &&
                                       std::is_constructible<RandomPermutation<>, int, SeedArg>::value>>
   RandomPermutation(BaseArg&& base_arg, SeedArg&& seed)
      : base(std::forward<BaseArg>(base_arg))
      , perm(base->size(), std::forward<SeedArg>(seed)) {}

   template <typename BaseArg,
             typename=std::enable_if_t<std::is_constructible<alias_t, BaseArg>::value>>
   explicit RandomPermutation(BaseArg&& base_arg)
      : RandomPermutation(std::forward<BaseArg>(base_arg), RandomSeed()) {}

   decltype(auto) get_container1() { return *base; }
   decltype(auto) get_container1() const { return *base; }
   const RandomPermutation<>& get_container2() const { return perm; }
};

template <typename ContainerRef>
struct spec_object_traits< RandomPermutation<ContainerRef> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true;
};

template <typename Container>
auto select_random_subset(Container&& c, int k, const RandomSeed& seed=RandomSeed())
{
   return RandomSubset<Container>(std::forward<Container>(c), k, seed);
}

template <typename Container>
auto select_random_subset(Container&& c, int k, const SharedRandomState& s)
{
   return RandomSubset<Container>(std::forward<Container>(c), k, s);
}

template <typename Container>
auto random_permutation(Container&& c, const RandomSeed& seed=RandomSeed())
{
   return RandomPermutation<Container>(std::forward<Container>(c), seed);
}

template <typename Container>
auto random_permutation(Container&& c, const SharedRandomState& s)
{
   return RandomPermutation<Container>(std::forward<Container>(c), s);
}

inline
RandomPermutation<>
random_permutation(int n, const RandomSeed& seed=RandomSeed())
{
   return RandomPermutation<>(n, seed);
}

inline
RandomPermutation<>
random_permutation(int n, const SharedRandomState& s)
{
   return RandomPermutation<>(n, s);
}

} // end namespace pm

namespace polymake {
   using pm::RandomSubset;
   using pm::select_random_subset;
   using pm::RandomPermutation;
   using pm::random_permutation;
}

#endif // POLYMAKE_RANDOM_SUBSET_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
