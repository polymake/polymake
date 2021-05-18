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

#include "polymake/vector"
#include "polymake/Set.h"
#include "polymake/SelectedSubset.h"
#include "polymake/Integer.h"

namespace pm {

template <typename ContainerRef>
class Subsets_of_1
   : public modified_container_impl< Subsets_of_1<ContainerRef>,
                                     mlist< ContainerRefTag< ContainerRef >,
                                            OperationTag< operations::construct_unary2<
                                                             SingleElementSetCmp,
                                                             typename generic_of_subsets< Subsets_of_1<ContainerRef>,
                                                                                          typename deref<ContainerRef>::type >::subset_element_comparator
                                                                       > > > >
   , public generic_of_subsets_t<Subsets_of_1<ContainerRef>, typename deref<ContainerRef>::type> {
protected:
   using alias_t = alias<add_const_t<ContainerRef>>;
   alias_t base;
public:
   template <typename Arg, typename=std::enable_if_t<std::is_constructible<alias_t, Arg>::value>>
   explicit Subsets_of_1(Arg&& base_arg)
      : base(std::forward<Arg>(base_arg)) {}

   decltype(auto) get_container() const { return *base; }
};

template <typename ContainerRef>
struct spec_object_traits< Subsets_of_1<ContainerRef> >
  : spec_object_traits<is_container> {
  static constexpr bool is_temporary = true, is_always_const = true;
};

template <typename Container>
auto all_subsets_of_1(Container&& c)
{
   return Subsets_of_1<add_const_t<Container>>(std::forward<Container>(c));
}

template <typename SkipIterator,
          bool is_std_rev=is_derived_from_instance_of<SkipIterator, std::reverse_iterator>::value>
class skip_predicate {
public:
   typedef void argument_type;
   typedef bool result_type;

   skip_predicate(const SkipIterator& skip_arg) : skip(skip_arg) {}

   template <typename Iterator>
   bool operator() (const Iterator& it) const { return it!=skip; }
private:
   SkipIterator skip;
};

template <typename SkipIterator>
class skip_predicate<SkipIterator, true> {
public:
   typedef typename SkipIterator::iterator_type BaseIterator;
   typedef void argument_type;
   typedef bool result_type;

   skip_predicate(const SkipIterator& skip_arg)
      : skip(skip_arg)
      , skip_base(skip.base())
   {
      --skip_base;
   }

   bool operator() (const SkipIterator& it) const { return it!=skip; }
   bool operator() (const BaseIterator& it) const { return it!=skip_base; }
private:
   SkipIterator skip;
   BaseIterator skip_base;
};

template <typename Container> class Subsets_less_1_iterator;

template <typename Container, bool is_bidir = is_derived_from<typename container_traits<Container>::category, bidirectional_iterator_tag>::value>
class Subset_less_1
   : public generic_of_subset_t<Subset_less_1<Container, is_derived_from<typename container_traits<Container>::category, bidirectional_iterator_tag>::value>, Container> {
   friend class Subsets_less_1_iterator<Container>;
protected:
   using skip_iterator_features = mlist<reversed, end_sensitive>;
   using skip_iterator = typename ensure_features<Container, skip_iterator_features>::const_iterator;

   const Container* base;
   skip_iterator skip;

public:
   using value_type = typename container_traits<Container>::value_type;
   using const_reference = typename container_traits<Container>::const_reference;
   using reference = const_reference;

   Subset_less_1()
      : base(nullptr) {}

   Subset_less_1(const Container& base_arg, bool at_begin)
      : base(&base_arg)
      , skip(at_begin ? ensure(base_arg, skip_iterator_features()).begin()
                      : ensure(base_arg, skip_iterator_features()).end()) {}

   using const_iterator = unary_predicate_selector<typename ensure_features<Container, end_sensitive>::const_iterator,
                                                   skip_predicate<skip_iterator> >;
   using iterator = const_iterator;

   iterator begin() const
   {
      return iterator(ensure(*base, end_sensitive()).begin(), skip);
   }
   iterator end() const
   {
      return iterator(ensure(*base, end_sensitive()).end(), skip);
   }
   reference front() const
   {
      auto b=base->begin();
      if (b==skip) ++b;
      return *b;
   }

   Int size() const { return base->size()-1; }
   bool empty() const { return base->empty() || base->size() <= 1; }

   decltype(auto) skipped_element() const { return *skip; }
};

template <typename Container>
class Subset_less_1<Container, true>
   : public Subset_less_1<Container, false> {
   friend class Subsets_less_1_iterator<Container>;
   using base_t = Subset_less_1<Container, false>;
public:
   using Subset_less_1<Container, false>::Subset_less_1;

   using const_reverse_iterator = unary_predicate_selector<typename ensure_features<Container, end_sensitive>::const_reverse_iterator,
                                                           skip_predicate<typename base_t::skip_iterator> >;
   using reverse_iterator = const_reverse_iterator;

   reverse_iterator rbegin() const
   {
      return reverse_iterator(ensure(*this->base, end_sensitive()).rbegin(), this->skip);
   }
   reverse_iterator rend() const
   {
      return reverse_iterator(ensure(*this->base, end_sensitive()).rend(), this->skip);
   }
};

template <typename Container>
class Subsets_less_1_iterator {
public:
   using iterator_category = forward_iterator_tag;
   using value_type = Subset_less_1<Container>;
   using reference = const value_type&;
   using pointer = const value_type*;
   using difference_type = ptrdiff_t;
   using iterator = Subsets_less_1_iterator;
   using const_iterator = Subsets_less_1_iterator;

   Subsets_less_1_iterator() = default;

   Subsets_less_1_iterator(const Container& c, bool at_begin)
      : value(c, at_begin) {}

   reference operator* () const { return value; }
   pointer operator-> () const { return &value; }

   iterator& operator++ () { ++value.skip; return *this; }
   const iterator operator++ (int) { iterator copy=*this; operator++(); return copy; }

   bool operator== (const iterator& it) const { return value.skip==it.value.skip; }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   bool at_end() const { return value.skip.at_end(); }
protected:
   value_type value;
};

template <typename ContainerRef>
class Subsets_less_1
   : public generic_of_subsets_t<Subsets_less_1<ContainerRef>, typename deref<ContainerRef>::type> {
protected:
   using alias_t = alias<add_const_t<ContainerRef>>;
   alias_t base;
public:
   using iterator = Subsets_less_1_iterator<typename deref<ContainerRef>::type>;
   using value_type = typename iterator::value_type;
   using reference = typename iterator::reference;
   using const_iterator = iterator;
   using const_reference = reference;

   template <typename Arg, typename=std::enable_if_t<std::is_constructible<alias_t, Arg>::value>>
   explicit Subsets_less_1(Arg&& base_arg)
      : base(std::forward<Arg>(base_arg)) {}

   Int size() const { return base->size(); }
   bool empty() const { return base->empty(); }

   iterator begin() const
   {
      return iterator(*base, true);
   }
   iterator end() const
   {
      return iterator(*base, false);
   }
};

template <typename Container>
struct check_iterator_feature<Subsets_less_1_iterator<Container>, end_sensitive> : std::true_type {};

template <typename ContainerRef>
struct spec_object_traits< Subsets_less_1<ContainerRef> >
  : spec_object_traits<is_container> {
  static constexpr bool is_temporary = true, is_always_const = true;
};

template <typename Container>
auto all_subsets_less_1(Container&& c)
{
   return Subsets_less_1<add_const_t<Container>>(std::forward<Container>(c));
}

template <typename ContainerRef> class Subsets_of_k_iterator;
template <typename ContainerRef> class AllSubsets_iterator;

template <typename BaseContainer>
class PointedSubset
   : public modified_container_impl< PointedSubset<BaseContainer>,
                                     mlist< ContainerTag< const std::vector<typename container_traits<BaseContainer>::const_iterator> >,
                                            OperationTag< BuildUnary<operations::dereference> > > >
   , public generic_of_subset_t<PointedSubset<BaseContainer>, BaseContainer> {
   typedef modified_container_impl<PointedSubset> base_t;

   template <typename> friend class Subsets_of_k_iterator;
   template <typename> friend class AllSubsets_iterator;
protected:
   using elem_iterator = typename container_traits<BaseContainer>::const_iterator;
   using it_vector = std::vector<elem_iterator>;
   shared_object<it_vector> itsp;
public:
   const it_vector& get_container() const { return *itsp; }

   PointedSubset() = default;

   explicit PointedSubset(Int k)
   {
      itsp->reserve(k);
   }

   PointedSubset(const BaseContainer& base, Int k)
   {
      it_vector& its = *itsp;
      its.reserve(k);
      for (elem_iterator e = base.begin();  k > 0;  --k, ++e)
         its.push_back(e);
   }

   decltype(auto) get_comparator() const { return operations::cmp(); }
};

template <typename Container>
class Subsets_of_k_iterator {
public:
   using iterator_category = forward_iterator_tag;
   using value_type = PointedSubset<Container>;
   using reference = value_type;
   using pointer = const value_type*;
   using difference_type = ptrdiff_t;
   using iterator = Subsets_of_k_iterator;
   using const_iterator = Subsets_of_k_iterator;
   using elem_iterator = typename value_type::elem_iterator;

   Subsets_of_k_iterator()
      : at_end_(true) {}

   Subsets_of_k_iterator(const Container& base, Int k)
      : value(base, k)
      , e_end(base.end())
      , at_end_(false) {}

   Subsets_of_k_iterator(const Subsets_of_k_iterator&) = default;
   Subsets_of_k_iterator(Subsets_of_k_iterator&&) = default;

   reference operator* () const { return value; }
   pointer operator-> () const { return &value; }

   iterator& operator++()
   {
      elem_iterator stop = e_end;
      for (auto it_first = value.itsp->begin(), it_last = value.itsp->end(), it_i = it_last;
           it_i != it_first; --it_i) {
         elem_iterator new_stop = (it_i[-1])++;
         if (it_i[-1] != stop) {
            for (; it_i != it_last; ++it_i)
               ++(*it_i = it_i[-1]);
            return *this;
         }
         stop = new_stop;
      }
      at_end_ = true;
      return *this;
   }

   const iterator operator++ (int) { iterator copy=*this; operator++(); return copy; }

   bool operator== (const iterator& other) const
   {
      return at_end_ == other.at_end_ && (at_end_ || *value.itsp == *other.value.itsp);
   }
   bool operator!= (const iterator& other) const { return !operator==(other); }

   bool at_end() const { return at_end_; }
protected:
   value_type value;
   elem_iterator e_end;
   bool at_end_;
};

template <typename ContainerRef>
class Subsets_of_k
   : public generic_of_subsets_t<Subsets_of_k<ContainerRef>, typename deref<ContainerRef>::type> {
public:
   using iterator = Subsets_of_k_iterator<typename deref<ContainerRef>::type>;
   using value_type = typename iterator::value_type;
   using reference = typename iterator::reference;
   using const_iterator = iterator;
   using const_reference = reference;

   using alias_t = alias<add_const_t<ContainerRef>>;

   template <typename Arg, typename=std::enable_if_t<std::is_constructible<alias_t, Arg>::value>>
   Subsets_of_k(Arg&& base_arg, Int k_arg)
      : base(std::forward<Arg>(base_arg))
      , k(k_arg)
   {
      if (POLYMAKE_DEBUG) {
         if (k < 0 || k > base->size())
            throw std::runtime_error("Subsets_of_k - invalid size");
      }
   }

   Int size() const
   {
      return static_cast<Int>(Integer::binom(base->size(), k));
   }

   bool empty() const { return false; }

   iterator begin() const { return iterator(*base, k); }
   iterator end() const { return iterator(); }
   value_type front() const { return value_type(*base, k); }
protected:
   alias_t base;
   Int k;
};

template <typename Container>
class AllSubsets_iterator {
public:
   using iterator_category = forward_iterator_tag;
   using value_type = PointedSubset<Container>;
   using reference = value_type;
   using pointer = const value_type*;
   using difference_type = ptrdiff_t;
   using iterator = AllSubsets_iterator;
   using const_iterator = AllSubsets_iterator;

   AllSubsets_iterator()
      : at_end_(true) {}

   AllSubsets_iterator(const Container& base)
      : value(base.size())
      , e_next(base.begin())
      , e_end(base.end())
      , at_end_(false) {}

   AllSubsets_iterator(const AllSubsets_iterator&) = default;
   AllSubsets_iterator(AllSubsets_iterator&&) = default;

   reference operator* () const { return value; }
   pointer operator-> () const { return &value; }

   iterator& operator++()
   {
      auto& its = *value.itsp;
      if (e_next != e_end) {
         its.push_back(e_next);
         ++e_next;
      } else {
         if (!its.empty())
            its.pop_back();
         if (its.empty()) {
            at_end_=true;
         } else {
            e_next = ++its.back();
            ++e_next;
         }
      }
      return *this;
   }

   const iterator operator++ (int) { iterator copy=*this; operator++(); return copy; }

   bool operator== (const iterator& other) const
   {
      return at_end_ == other.at_end_ && (at_end_ || *value.itsp == *other.value.itsp);
   }
   bool operator!= (const iterator& other) const { return !operator==(other); }

   bool at_end() const { return at_end_; }
protected:
   value_type value;
   typename value_type::elem_iterator e_next, e_end;
   bool at_end_;
};

template <typename ContainerRef>
class AllSubsets
   : public generic_of_subsets_t<AllSubsets<ContainerRef>, typename deref<ContainerRef>::type> {
public:
   using iterator = AllSubsets_iterator<typename deref<ContainerRef>::type>;
   using value_type = typename iterator::value_type;
   using reference = typename iterator::reference;
   using const_iterator = iterator;
   using const_reference = reference;

   using alias_t = alias<add_const_t<ContainerRef>>;

   template <typename Arg, typename=std::enable_if_t<std::is_constructible<alias_t, Arg>::value>>
   explicit AllSubsets(Arg&& base_arg)
      : base(std::forward<Arg>(base_arg)) {}

   Int size() const { return Int(1) << base->size(); }
   bool empty() const { return false; }

   iterator begin() const { return iterator(*base); }
   iterator end() const { return iterator(); }
   value_type front() const { return value_type(); }
protected:
   alias_t base;
};

template <typename Container>
struct check_iterator_feature<Subsets_of_k_iterator<Container>, end_sensitive> : std::true_type {};

template <typename Container>
struct check_iterator_feature<AllSubsets_iterator<Container>, end_sensitive> : std::true_type {};

template <typename ContainerRef>
struct spec_object_traits< Subsets_of_k<ContainerRef> >
  : spec_object_traits<is_container> {
  static constexpr bool is_temporary = true, is_always_const = true;
};

template <typename ContainerRef>
struct spec_object_traits< AllSubsets<ContainerRef> >
  : spec_object_traits<is_container> {
  static constexpr bool is_temporary = true, is_always_const = true;
};

template <typename ContainerRef>
struct spec_object_traits< PointedSubset<ContainerRef> >
  : spec_object_traits<is_container> {
  static constexpr bool is_temporary = true, is_always_const = true;
};


template <typename Container>
auto all_subsets_of_k(Container&& c, Int k)
{
   return Subsets_of_k<add_const_t<Container>>(std::forward<Container>(c), k);
}

template <typename Container>
auto all_subsets(Container&& c)
{
   return AllSubsets<add_const_t<Container>>(std::forward<Container>(c));
}

template <typename ElementSet, typename Top, typename E, typename Comparator>
std::enable_if_t<is_generic_set<ElementSet, E, Comparator>::value, Int>
insertMax(Set<ElementSet>& power_set, const GenericSet<Top, E, Comparator>& element_set_arg)
{
   const auto& element_set = diligent(element_set_arg);
   if (element_set.empty())
      return -1;
   for (auto e = entire(power_set); !e.at_end(); ) {
      const Int inc = incl(element_set, *e);
      // found a subset containing or equal to set
      if (inc <= 0)
         return inc;
      // found a subset being contained in set
      if (inc == 1)
         power_set.erase(e++);
      else
         ++e;
   }
   power_set.insert(element_set);
   return 1;
}

template <typename ElementSet, typename Top, typename E, typename Comparator>
std::enable_if_t<is_generic_set<ElementSet, E, Comparator>::value, Int>
insertMin(Set<ElementSet>& power_set, const GenericSet<Top, E, Comparator>& element_set_arg)
{
   const auto& element_set = diligent(element_set_arg);
   if (element_set.empty())
      return -1;
   for (auto e = entire(power_set); !e.at_end(); ) {
      const Int inc = incl(*e, element_set);
      // found a subset containing or equal to set
      if (inc <= 0)
         return inc;
      // found a subset being contained in set
      if (inc == 1)
         power_set.erase(e++);
      else
         ++e;
   }
   power_set.insert(element_set);
   return 1;
}

template <typename E, typename Comparator = operations::cmp>
using PowerSet = Set<Set<E, Comparator>>;


/// Gather all independent intersections of subsets from the given PowerSet.
template <typename Top, typename ElementSet, typename = std::enable_if_t<is_generic_set<ElementSet>::value>>
PowerSet<typename ElementSet::element_type, typename ElementSet::element_comparator>
ridges(const GenericSet<Top, ElementSet>& power_set)
{
   PowerSet<typename ElementSet::element_type, typename ElementSet::element_comparator> R;
   for (auto it = power_set.top().begin(), end = power_set.top().end();  it != end; ++it) {
      auto it2 = it;
      for (++it2; it2 != end; ++it2)
         insertMax(R, (*it) * (*it2));
   }
   return R;
}

} // end namespace pm

namespace polymake {
   using pm::PowerSet;
   using pm::ridges;
   using pm::Subsets_of_1;
   using pm::all_subsets_of_1;
   using pm::Subsets_less_1;
   using pm::all_subsets_less_1;
   using pm::Subsets_of_k;
   using pm::all_subsets_of_k;
   using pm::AllSubsets;
   using pm::all_subsets;
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
