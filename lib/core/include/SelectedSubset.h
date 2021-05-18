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

#include "polymake/GenericSet.h"
#include "polymake/internal/iterator_filters.h"

namespace pm {

template <typename ContainerRef, typename Predicate>
class SelectedSubset
   : public modified_container_base<ContainerRef, Predicate>
   , public modified_container_impl< SelectedSubset<ContainerRef, Predicate>,
                                     mlist< ContainerRefTag< ContainerRef >,
                                            OperationTag< Predicate >,
                                            IteratorConstructorTag< unary_predicate_selector_constructor > > >
   , public generic_of_subset_t<SelectedSubset<ContainerRef, Predicate>, typename deref<ContainerRef>::type> {
   using base_t = modified_container_base<ContainerRef, Predicate>;
   using impl_t = modified_container_impl<SelectedSubset>;
public:
   using container_category = typename least_derived_class<bidirectional_iterator_tag, typename impl_t::container_category>::type;
   using modified_container_base<ContainerRef, Predicate>::modified_container_base;
   using base_t::get_operation;
};

template <typename ContainerRef, typename Predicate>
class TruncatedContainer
   : public modified_container_base<ContainerRef, Predicate>
   , public modified_container_impl< TruncatedContainer<ContainerRef, Predicate>,
                                     mlist< ContainerRefTag< ContainerRef >,
                                            OperationTag< Predicate >,
                                            IteratorConstructorTag< input_truncator_constructor > > >
   , public generic_of_subset_t<TruncatedContainer<ContainerRef, Predicate>, typename deref<ContainerRef>::type> {
   using base_t = modified_container_base<ContainerRef, Predicate>;
   using impl_t = modified_container_impl<TruncatedContainer>;
public:
   using container_category = forward_iterator_tag;
   using modified_container_base<ContainerRef, Predicate>::modified_container_base;
   using base_t::get_operation;

   decltype(auto) front()
   {
      return this->get_container().front();
   }
   decltype(auto) front() const
   {
      return this->get_container().front();
   }
};


template <typename ContainerRef, typename Predicate>
class ContractedRanges
   : public modified_container_base<ContainerRef, Predicate>
   , public modified_container_impl< ContractedRanges<ContainerRef, Predicate>,
                                     mlist< ContainerRefTag< ContainerRef >,
                                            OperationTag< Predicate >,
                                            IteratorConstructorTag< range_contractor_constructor > > > {
   using base_t = modified_container_base<ContainerRef, Predicate>;
public:
   using container_category = forward_iterator_tag;
   using modified_container_base<ContainerRef, Predicate>::modified_container_base;
   using base_t::get_operation;
};


template <typename ContainerRef, typename FoldingOperation>
class FoldedRanges
   : public modified_container_base<ContainerRef, FoldingOperation>
   , public modified_container_impl< FoldedRanges<ContainerRef, FoldingOperation>,
                                     mlist< ContainerRefTag< ContainerRef >,
                                            OperationTag< FoldingOperation >,
                                            IteratorConstructorTag< range_folder_constructor > > > {
   using base_t = modified_container_base<ContainerRef, FoldingOperation>;
public:
   using container_category = forward_iterator_tag;
   using modified_container_base<ContainerRef, FoldingOperation>::modified_container_base;
   using base_t::get_operation;
};


template <typename ContainerRef, typename Predicate>
struct spec_object_traits< SelectedSubset<ContainerRef, Predicate> > :
   spec_object_traits<is_container> {
   static constexpr bool
      is_temporary = true,
      is_always_const = is_effectively_const<ContainerRef>::value;
};

template <typename ContainerRef, typename Predicate>
struct spec_object_traits< TruncatedContainer<ContainerRef, Predicate> > :
   spec_object_traits<is_container> {
   static constexpr bool
      is_temporary = true,
      is_always_const = is_effectively_const<ContainerRef>::value;
};

template <typename ContainerRef, typename Predicate>
struct spec_object_traits< ContractedRanges<ContainerRef, Predicate> > :
   spec_object_traits<is_container> {
   static constexpr bool is_temporary = true, is_always_const = true;
};

template <typename ContainerRef, typename FoldingOperation>
struct spec_object_traits< FoldedRanges<ContainerRef, FoldingOperation> > :
   spec_object_traits<is_container> {
   static constexpr bool is_temporary = true, is_always_const = true;
};


class index_truncator {
protected:
   Int last;
public:
   index_truncator(Int l = -1) : last(l) {}

   typedef void argument_type;
   typedef bool result_type;

   template <typename Iterator>
   bool operator() (const Iterator& it) const
   {
      return it.index() <= last;
   }
};


class equal_index_folder {
public:
   typedef void argument_type;
   typedef Int value_type;
   typedef const Int& reference;

   template <typename Iterator>
   void reset(const Iterator& it)
   {
      cnt = 1;
      index = it.index();
   }

   template <typename Iterator>
   bool operator() (const Iterator& it)
   {
      if (it.index() == index) {
         ++cnt;
         return true;
      } else {
         return false;
      }
   }

   const Int& get() const { return cnt; }
   Int get_index() const { return index; }

private:
   Int index;
   Int cnt;
};


template <typename Container, typename Predicate>
auto attach_selector(Container&& c, const Predicate& pred)
{
   return SelectedSubset<Container, Predicate>(std::forward<Container>(c), pred);
}

template <typename Container, typename Predicate>
auto attach_truncator(Container&& c, const Predicate& pred)
{
   return TruncatedContainer<Container, Predicate>(std::forward<Container>(c), pred);
}

template <typename Container, typename Predicate>
auto contract_ranges(Container&& c, const Predicate& pred)
{
   return ContractedRanges<add_const_t<Container>, Predicate>(std::forward<Container>(c), pred);
}

template <typename Container>
auto contract_equal_ranges(Container&& c)
{
   return ContractedRanges<add_const_t<Container>, BuildBinary<operations::eq> >(std::forward<Container>(c));
}

template <typename Container, typename Operation>
auto fold_ranges(Container&& c, const Operation& op)
{
   return FoldedRanges<add_const_t<Container>, Operation>(std::forward<Container>(c), op);
}


template <typename ContainerRef1, typename ContainerRef2, typename Predicate>
class SelectedContainerPairSubset
   : public modified_container_pair_base<ContainerRef1, ContainerRef2, Predicate>
   , public modified_container_pair_impl< SelectedContainerPairSubset<ContainerRef1, ContainerRef2, Predicate>,
                                          mlist< Container1RefTag< ContainerRef1 >,
                                                 Container2RefTag< ContainerRef2 >,
                                                 IteratorConstructorTag< binary_predicate_selector_constructor >,
                                                 OperationTag< Predicate > > >
   , public generic_of_subset_t<SelectedContainerPairSubset<ContainerRef1, ContainerRef2, Predicate>, typename deref<ContainerRef1>::type > {
   using base_t = modified_container_pair_base<ContainerRef1, ContainerRef2, Predicate>;
   using impl_t = modified_container_pair_impl<SelectedContainerPairSubset>;
public:
   using container_category = typename least_derived_class<bidirectional_iterator_tag, typename impl_t::container_category>::type;
   using modified_container_pair_base<ContainerRef1, ContainerRef2, Predicate>::modified_container_pair_base;
   using base_t::get_operation;
};

template <typename ContainerRef1, typename ContainerRef2, typename Predicate>
struct spec_object_traits< SelectedContainerPairSubset<ContainerRef1, ContainerRef2, Predicate> >
   : spec_object_traits<is_container> {
   static constexpr bool
      is_temporary = true,
      is_always_const = is_effectively_const<ContainerRef1>::value;
};

template <typename Container1, typename Container2, typename Predicate>
auto attach_selector(Container1&& c1, Container2&& c2, const Predicate& pred)
{
   return SelectedContainerPairSubset<Container1, Container2, Predicate>
          (std::forward<Container1>(c1), std::forward<Container2>(c2), pred);
}

template <typename Container1, typename Container2>
auto attach_mask(Container1&& data, Container2&& boolean)
{
   return SelectedContainerPairSubset<Container1, add_const_t<Container2>, operations::apply2< BuildUnaryIt<operations::dereference> > >
          (std::forward<Container1>(data), std::forward<Container2>(boolean));
}

} // end namespace pm

namespace polymake {

using pm::SelectedSubset;
using pm::TruncatedContainer;
using pm::ContractedRanges;
using pm::FoldedRanges;
using pm::SelectedContainerPairSubset;
using pm::attach_selector;
using pm::attach_mask;
using pm::contract_ranges;
using pm::contract_equal_ranges;
using pm::fold_ranges;

} // end namespace polymake


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
