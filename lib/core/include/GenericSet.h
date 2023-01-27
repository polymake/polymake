/* Copyright (c) 1997-2023
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
/** @file GenericSet.h
    @brief Declaration of pm::GenericSet class

    Implementation of various kinds of pm::GenericSet types
 */
/** @defgroup genericSets Generic Sets
 *  Functions and operations for GenericSets
 *  @{
 */

#include "polymake/internal/SeriesRaw.h"
#include "polymake/GenericIO.h"
#include "polymake/internal/comparators.h"

namespace pm {

template <typename E, typename Comparator = operations::cmp> class Set;

template <typename SetRef1, typename SetRef2, typename Controller> class LazySet2;
template <typename Eref, typename Comparator> class SingleElementSetCmp;
template <typename SetRef> class Complement;

template <typename TSet, typename E=typename TSet::element_type, typename Comparator=typename TSet::element_comparator>
class GenericSet;

template <typename T, typename... E>
using is_generic_set = is_derived_from_instance_of<pure_type_t<T>, GenericSet, E...>;

template <typename T>
using is_integer_set = is_generic_set<T, Int, operations::cmp>;

/** @class GenericSet
    @brief @ref generic "Generic type" for \ref set_sec "ordered sets"
 */

template <typename Top, typename E, typename Comparator>
class GenericSet
   : public Generic<Top> {
   template <typename, typename, typename> friend class GenericSet;
protected:
   GenericSet() = default;
   GenericSet(const GenericSet&) = default;

public:
   /// element types
   using element_type = E;
   /// functor type for comparing elements
   using element_comparator = Comparator;

   static_assert(!std::is_same<Comparator, operations::cmp_unordered>::value, "comparator must define a total ordering");
   static_assert(!std::is_same<Comparator, operations::cmp>::value || is_ordered<E>::value, "elements must have a total ordering");
   static_assert(!is_among<E, bool, int>::value, "invalid Set element type");

   using persistent_type = Set<E, Comparator>;
   /// @ref generic "generic type"
   using generic_type = GenericSet;
   /// @ref generic "top type"
   using top_type = typename Generic<Top>::top_type;


   template <typename Left, typename Right, typename Controller>
   using custom_op = std::false_type;

   template <typename T>
   using propagate_rvalue = std::false_type;

protected:
   template <typename Left, typename Right, typename Controller, typename=void>
   struct lazy_op {};

   template <typename Left, typename Right, typename Controller>
   struct lazy_op<Left, Right, Controller,
                  std::enable_if_t<(is_derived_from<pure_type_t<Left>, GenericSet>::value &&
                                    !pure_unwary_t<Left>::template propagate_rvalue<Left>::value &&
                                    is_generic_set<Right, E, Comparator>::value &&
                                    (!pure_unwary_t<Right>::template propagate_rvalue<Right>::value || std::is_same<Controller, set_difference_zipper>::value) &&
                                    !(pure_unwary_t<Left>::template custom_op<Left, Right, Controller>::value &&
                                      pure_unwary_t<Right>::template custom_op<Left, Right, Controller>::value))>> {

      using type = LazySet2<add_const_t<unwary_t<Left>>, add_const_t<unwary_t<Right>>, Controller>;

      static type make(Left&& l, Right&& r)
      {
         return type(unwary(std::forward<Left>(l)), unwary(std::forward<Right>(r)));
      }
   };

   template <typename Left, typename Right, typename Controller>
   struct lazy_op<Left, Right, Controller,
                  std::enable_if_t<(is_derived_from<pure_type_t<Left>, GenericSet>::value &&
                                    !pure_unwary_t<Left>::template propagate_rvalue<Left>::value &&
                                    is_generic_set<Right, E, Comparator>::value &&
                                    (!pure_unwary_t<Right>::template propagate_rvalue<Right>::value || std::is_same<Controller, set_difference_zipper>::value) &&
                                    pure_unwary_t<Left>::template custom_op<Left, Right, Controller>::value &&
                                    pure_unwary_t<Right>::template custom_op<Left, Right, Controller>::value)>>
      : pure_unwary_t<Left>::template custom_op<Left, Right, Controller> {};

   template <typename Left, typename Right, typename Controller>
   struct lazy_op<Left, Right, Controller,
                  std::enable_if_t<(is_derived_from<pure_type_t<Left>, GenericSet>::value &&
                                    pure_unwary_t<Left>::template propagate_rvalue<Left>::value &&
                                    is_generic_set<Right, E, Comparator>::value)>> {
      using r_x_type = Left&&;
   };

   template <typename Left, typename Right, typename Controller>
   struct lazy_op<Left, Right, Controller,
                  std::enable_if_t<(is_generic_set<Left, E, Comparator>::value &&
                                    !pure_unwary_t<Left>::template propagate_rvalue<Left>::value &&
                                    is_generic_set<Right, E, Comparator>::value &&
                                    pure_unwary_t<Right>::template propagate_rvalue<Right>::value &&
                                    !std::is_same<Controller, set_difference_zipper>::value)>> {
      using x_r_type = Right&&;
   };

   template <typename Left, typename Right, typename Controller>
   struct lazy_op<Left, Right, Controller,
                  std::enable_if_t<(is_derived_from<pure_type_t<Left>, GenericSet>::value &&
                                    !pure_unwary_t<Left>::template propagate_rvalue<Left>::value &&
                                    std::is_same<E, prevent_int_element<pure_type_t<Right>>>::value &&
                                    !pure_unwary_t<Left>::template custom_op<Left, prevent_int_element<Right>, Controller>::value)>> {

      using e_set = SingleElementSetCmp<prevent_int_element<add_const_t<Right>>, Comparator>;
      using type = LazySet2<add_const_t<unwary_t<Left>>, e_set, Controller>;

      static type make(Left&& l, Right&& r)
      {
         return type(unwary(std::forward<Left>(l)), e_set(std::forward<Right>(r)));
      }
   };

   template <typename Left, typename Right, typename Controller>
   struct lazy_op<Left, Right, Controller,
                  std::enable_if_t<(is_derived_from<pure_type_t<Left>, GenericSet>::value &&
                                    !pure_unwary_t<Left>::template propagate_rvalue<Left>::value &&
                                    std::is_same<E, prevent_int_element<pure_type_t<Right>>>::value &&
                                    pure_unwary_t<Left>::template custom_op<Left, prevent_int_element<Right>, Controller>::value)>>
      : pure_unwary_t<Left>::template custom_op<Left, prevent_int_element<Right>, Controller> {};

   template <typename Left, typename Right, typename Controller>
   struct lazy_op<Left, Right, Controller,
                  std::enable_if_t<(is_derived_from<pure_type_t<Left>, GenericSet>::value &&
                                    pure_unwary_t<Left>::template propagate_rvalue<Left>::value &&
                                    std::is_same<E, prevent_int_element<pure_type_t<Right>>>::value)>> {
      using r_x_type = Left&&;
   };

   template <typename Left, typename Right, typename Controller>
   struct lazy_op<Left, Right, Controller,
                  std::enable_if_t<(std::is_same<E, prevent_int_element<pure_type_t<Left>>>::value &&
                                    is_derived_from<pure_type_t<Right>, GenericSet>::value &&
                                    (!pure_unwary_t<Right>::template propagate_rvalue<Right>::value || std::is_same<Controller, set_difference_zipper>::value) &&
                                    !pure_unwary_t<Right>::template custom_op<prevent_int_element<Left>, Right, Controller>::value)>> {
      using e_set = SingleElementSetCmp<prevent_int_element<add_const_t<Left>>, Comparator>;
      using type = LazySet2<e_set, add_const_t<unwary_t<Right>>, Controller>;

      static type make(Left&& l, Right&& r)
      {
         return type(e_set(std::forward<Left>(l)), unwary(std::forward<Right>(r)));
      }
   };

   template <typename Left, typename Right, typename Controller>
   struct lazy_op<Left, Right, Controller,
                  std::enable_if_t<(std::is_same<E, prevent_int_element<pure_type_t<Left>>>::value &&
                                    is_derived_from<pure_type_t<Right>, GenericSet>::value &&
                                    (!pure_unwary_t<Right>::template propagate_rvalue<Right>::value || std::is_same<Controller, set_difference_zipper>::value) &&
                                    pure_unwary_t<Right>::template custom_op<prevent_int_element<Left>, Right, Controller>::value)>>
      : pure_unwary_t<Right>::template custom_op<prevent_int_element<Left>, Right, Controller> {};

   template <typename Left, typename Right, typename Controller>
   struct lazy_op<Left, Right, Controller,
                  std::enable_if_t<(std::is_same<E, prevent_int_element<pure_type_t<Left>>>::value &&
                                    is_derived_from<pure_type_t<Right>, GenericSet>::value &&
                                    !std::is_same<Controller, set_difference_zipper>::value &&
                                    pure_unwary_t<Right>::template propagate_rvalue<Right>::value)>> {
      using x_r_type = Right&&;
   };

public:
   /// union of two sets
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, set_union_zipper>::type
   operator+ (Left&& l, Right&& r)
   {
      return lazy_op<Left, Right, set_union_zipper>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, set_union_zipper>::r_x_type
   operator+ (Left&& l, Right&& r)
   {
      return std::move(l += r);
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, set_union_zipper>::x_r_type
   operator+ (Left&& l, Right&& r)
   {
      return std::move(r += l);
   }

   /// difference of two sets
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, set_difference_zipper>::type
   operator- (Left&& l, Right&& r)
   {
      return lazy_op<Left, Right, set_difference_zipper>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, set_difference_zipper>::r_x_type
   operator- (Left&& l, Right&& r)
   {
      return std::move(l -= r);
   }

   /// intersection of two sets
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, set_intersection_zipper>::type
   operator* (Left&& l, Right&& r)
   {
      return lazy_op<Left, Right, set_intersection_zipper>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, set_intersection_zipper>::r_x_type
   operator* (Left&& l, Right&& r)
   {
      return std::move(l *= r);
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, set_intersection_zipper>::x_r_type
   operator* (Left&& l, Right&& r)
   {
      return std::move(r *= l);
   }

   /// symmetric difference of two sets
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, set_symdifference_zipper>::type
   operator^ (Left&& l, Right&& r)
   {
      return lazy_op<Left, Right, set_symdifference_zipper>::make(std::forward<Left>(l), std::forward<Right>(r));
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, set_symdifference_zipper>::r_x_type
   operator^ (Left&& l, Right&& r)
   {
      return std::move(l ^= r);
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, set_symdifference_zipper>::x_r_type
   operator^ (Left&& l, Right&& r)
   {
      return std::move(r ^= l);
   }

   auto operator~ () const &
   {
      return Complement<diligent_ref_t<unwary_t<const Top&>>>(diligent(unwary(*this)));
   }

   auto operator~ () &&
   {
      return Complement<diligent_ref_t<unwary_t<Top>>>(diligent(unwary(std::move(*this))));
   }

   /// comparison
   template <typename Set2>
   bool operator== (const GenericSet<Set2, E, Comparator>& s) const
   {
      return equal_ranges(entire_range(this->top()), entire_range(s.top()));
   }

   template <typename Set2>
   bool operator!= (const GenericSet<Set2, E, Comparator>& s) const
   {
      return !(*this == s);
   }

   /// lexicographical comparison
   template <typename Set2>
   bool operator< (const GenericSet<Set2, E, Comparator>& s) const
   {
      operations::lt<const Top&, const Set2&> cmp_op;
      return cmp_op(this->top(), s.top());
   }

   template <typename Set2>
   bool operator> (const GenericSet<Set2, E, Comparator>& s) const
   {
      return s < *this;
   }

   template <typename Set2>
   bool operator<= (const GenericSet<Set2, E, Comparator>& s) const
   {
      return !(s < *this);
   }

   template <typename Set2>
   bool operator>= (const GenericSet<Set2, E, Comparator>& s) const
   {
      return !(*this < s);
   }

   template <typename Result>
   struct rebind_generic {
      typedef GenericSet<Result, E, Comparator> type;
   };

   template <typename E1=E, typename E2=E1>
   struct rebind_comparator {
      typedef binary_op_builder<Comparator, const E1*, const E2*> builder;
      typedef typename builder::operation type;
   };

#if POLYMAKE_DEBUG
   void dump() const __attribute__((used)) { cerr << this->top() << endl; }
#endif
};

template <typename Set, typename E, typename Comparator>
struct spec_object_traits< GenericSet<Set,E,Comparator> >
   : spec_or_model_traits<Set,is_container> {
   typedef is_set generic_tag;
   static const IO_separator_kind IO_separator=IO_sep_inherit;
};

template <typename E, bool step_equal_1>
class Series
   : public SeriesRaw<E, step_equal_1>
   , public GenericSet<Series<E, step_equal_1>, E, operations::cmp> {
public:
   using SeriesRaw<E, step_equal_1>::SeriesRaw;
};

template <typename E, bool step_equal_1>
struct spec_object_traits< Series<E, step_equal_1> >
   : spec_object_traits<is_container> {
   static constexpr bool is_always_const=true;
};

// alias for an integer series
using series = Series<Int, false>;

// alias for an integer sequence
using sequence = Series<Int, true>;

// Create a sequence of all integral numbers between and including $start$ and $end$
inline sequence range(Int start, Int end)
{
   return sequence(start, end - start+1);
}

template <typename Subset, typename Source, typename = void>
struct generic_of_subset {
   using type = generic_none;
};

template <typename Subset, typename Source>
struct generic_of_subset<Subset, Source, std::enable_if_t<is_generic_set<Source>::value>> {
   using type = GenericSet<Subset, typename Source::element_type, typename Source::element_comparator>;
};

template <typename Subset, typename Source>
using generic_of_subset_t = typename generic_of_subset<Subset, Source>::type;

template <typename Subsets, typename Source, typename = void>
struct generic_of_subsets {
   using type = generic_none;
   using subset_element_comparator = operations::cmp;
};

template <typename Subsets, typename Source>
struct generic_of_subsets<Subsets, Source, std::enable_if_t<is_generic_set<Source>::value>> {
   using subset_element_comparator = typename Source::element_comparator;
   using type = GenericSet<Subsets, Set<typename Source::element_type, subset_element_comparator>, operations::cmp>;
};

template <typename Subset, typename Source>
using generic_of_subsets_t = typename generic_of_subsets<Subset, Source>::type;

/* ------------------
 *  SingleElementSet
 * ------------------ */

/** @ingroup genericSets
 *  @brief A set consisting of exactly one element.
 */
template <typename ElemRef, typename Comparator>
class SingleElementSetCmp
   : public repeated_value_container<ElemRef>
   , public GenericSet<SingleElementSetCmp<ElemRef, Comparator>, pure_type_t<ElemRef>, Comparator> {
   using base_t = repeated_value_container<ElemRef>;
public:
   using typename GenericSet<SingleElementSetCmp>::element_type;

   SingleElementSetCmp() = default;

   template <typename Arg, typename = std::enable_if_t<std::is_constructible<base_t, Arg, Int>::value>>
   explicit SingleElementSetCmp(Arg&& arg)
      : base_t(std::forward<Arg>(arg), 1) {}

   bool contains(const element_type& x) const
   {
      Comparator cmp;
      return cmp(x, this->front())==cmp_eq;
   }

   typename base_t::const_iterator find(const element_type& x) const
   {
      return contains(x) ? this->begin() : this->end();
   }

   Comparator get_comparator() const { return Comparator(); }
};

template <typename ElemRef>
using SingleElementSet = SingleElementSetCmp<ElemRef, operations::cmp>;

template <typename ElemRef, typename Comparator>
struct spec_object_traits< SingleElementSetCmp<ElemRef, Comparator> >
   : spec_object_traits< repeated_value_container<ElemRef> > {
   static const bool is_always_const=true;
};

/// construct an one-element set with explicitly specified comparator type
template <typename Comparator, typename E>
auto scalar2set(E&& x)
{
   return SingleElementSetCmp<prevent_int_element<E>, Comparator>(std::forward<E>(x));
}

/// construct an one-element set with standard (lexicographical) comparator type
template <typename E>
auto scalar2set(E&& x)
{
   return SingleElementSet<prevent_int_element<E>>(std::forward<E>(x));
}

/* ----------
 *  LazySet2
 * ---------- */

template <typename SetRef1, typename SetRef2, typename Controller>
class LazySet2
   : public container_pair_base<SetRef1, SetRef2>
   , public modified_container_pair_impl< LazySet2<SetRef1, SetRef2, Controller>,
                                          mlist< Container1RefTag< SetRef1 >,
                                                 Container2RefTag< SetRef2 >,
                                                 IteratorCouplerTag< zipping_coupler<typename deref<SetRef1>::type::element_comparator, Controller> >,
                                                 OperationTag< BuildBinaryIt<operations::zipper> >,
                                                 IteratorConstructorTag< binary_transform_constructor<BijectiveTag<std::false_type>> > > >
   , public GenericSet< LazySet2<SetRef1, SetRef2, Controller>,
                        typename deref<SetRef1>::type::element_type,
                        typename deref<SetRef1>::type::element_comparator > {
   using base_t = container_pair_base<SetRef1, SetRef2>;

protected:
   typename base_t::second_alias_t& get_alias2() & { return this->src2; }
   typename base_t::second_alias_t&& get_alias2() && { return this->src2; }
   template <typename> friend class Complement;

public:
   static_assert(std::is_same<typename LazySet2::element_type, typename deref<SetRef2>::type::element_type>::value,
                 "sets with different element types mixed in an expression");
   static_assert(std::is_same<typename LazySet2::element_comparator, typename deref<SetRef2>::type::element_comparator>::value,
                 "sets with different element comparators mixed in an expression");
   using cannot_enforce_features = indexed;
   using typename GenericSet<LazySet2>::element_type;

   LazySet2() = default;

   template <typename Arg1, typename Arg2,
             typename=std::enable_if_t<std::is_constructible<base_t, Arg1, Arg2>::value>>
   LazySet2(Arg1&& src1_arg, Arg2&& src2_arg, Controller=Controller())
      : base_t(std::forward<Arg1>(src1_arg), std::forward<Arg2>(src2_arg)) {}

   decltype(auto) get_comparator() const
   {
      return this->get_container1().get_comparator();
   }

   bool contains(const element_type& x) const
   {
      return Controller::contains(this->get_container1().contains(x), this->get_container2().contains(x));
   }
};

template <typename SetRef1, typename SetRef2, typename Controller>
struct spec_object_traits< LazySet2<SetRef1, SetRef2, Controller> >
   : spec_object_traits<is_container> {
   static constexpr bool is_lazy = true, is_temporary = true, is_always_const = true;
};

/* ------------
 *  Complement
 * ------------ */

/** @ingroup genericSets
 *  @brief %Complement as GenericSet.
 */
template <typename SetRef>
class Complement
   : public redirected_container< Complement<SetRef>,
                                  mlist< ContainerRefTag< LazySet2<sequence, SetRef, set_difference_zipper> > > >
   , public GenericSet< Complement<SetRef>, Int, operations::cmp> {

   using diff_t = LazySet2<sequence, SetRef, set_difference_zipper>;
   diff_t diff;
public:
   // TODO: delete both
   Complement() = default;
   Complement(const Complement&) = default;

   template <typename Arg,
             typename=std::enable_if_t<std::is_constructible<diff_t, sequence, Arg>::value>>
   explicit Complement(Arg&& arg)
      : diff(sequence(0, check_container_ref_feature<SetRef, sparse_compatible>::value ? get_dim(arg) : 0),
             std::forward<Arg>(arg)) {}

   Complement(const Complement& c, Int dim)
      : diff(sequence(0, dim), c.base())
   {
      if (POLYMAKE_DEBUG && c.dim() != 0 && c.dim() != dim)
         throw std::runtime_error("Complement - dimension mismatch");
   }

   Complement(Complement&& c, Int dim)
      : diff(sequence(0, dim), *c.diff.get_alias2())
   {
      if (POLYMAKE_DEBUG && c.dim() != 0 && c.dim() != dim)
         throw std::runtime_error("Complement - dimension mismatch");
   }

   Complement(Complement&&) = default;

   const diff_t& get_container() const
   {
      return diff;
   }
   decltype(auto) base() const
   {
      return diff.get_container2();
   }

   bool contains(Int k) const
   {
      return diff.contains(k);
   }
   Int size() const
   {
      return dim()==0 ? 0 : dim() - diff.get_container2().size();
   }
   bool empty() const
   {
      return size()==0;
   }
   Int dim() const
   {
      return diff.get_container1().size();
   }

   decltype(auto) operator~ () const
   {
      return base();
   }
};

template <typename SetRef>
struct spec_object_traits< Complement<SetRef> >
   : spec_object_traits<is_container> {
   // do not declare it lazy, complements are in most cases sequentially iterated
   static constexpr bool is_temporary = true, is_always_const = true;
};

template <typename SetRef>
Int get_dim(const Complement<SetRef>& c)
{
   return c.dim();
}

/* -----------------------------
 *  Set with assigned dimension
 * ----------------------------- */

/** @ingroup genericSets
 *  @brief %Set_with_dim as GenericSet.
 */
template <typename SetRef>
class Set_with_dim
   : public modified_container_impl< Set_with_dim<SetRef>,
                                     mlist< ContainerRefTag< SetRef >,
                                            OperationTag< pair<nothing, operations::identity<Int> > >,
                                            ExpectedFeaturesTag<end_sensitive> > >
   , public GenericSet< Set_with_dim<SetRef>, typename deref<SetRef>::type::element_type,
                        typename deref<SetRef>::type::element_comparator> {
   using base_t = modified_container_impl<Set_with_dim>;
protected:
   alias<SetRef> set;
   Int dim_ = 0;
public:
   using typename GenericSet<Set_with_dim>::element_type;

   Set_with_dim() = default;

   template <typename Arg, typename=std::enable_if_t<std::is_constructible<alias<SetRef>, Arg>::value>>
   Set_with_dim(Arg&& set_arg, Int dim_arg)
      : set(std::forward<Arg>(set_arg))
      , dim_(dim_arg) {}

   decltype(auto) get_container() const
   {
      return *set;
   }

   Int max_size() const
   {
      return dim_;
   }
   Int dim() const
   {
      return dim_;
   }

   bool contains(const element_type& x) const
   {
      return get_container().contains(x);
   }
   const auto& get_comparator() const
   {
      return get_container().get_comparator();
   }
   auto find(const element_type& x) const
   {
      return get_container().find(x);
   }
};

template <typename SetRef>
struct check_container_feature<Set_with_dim<SetRef>, sparse_compatible> : std::true_type {};

/** @ingroup genericSets
 *  @namespace operations
 *  @brief functors for %operations on GenericSet objects
 */

namespace operations {

template <typename LeftRef, typename RightRef>
struct add_impl<LeftRef, RightRef, cons<is_set, is_set> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef decltype(std::declval<LeftRef>() + std::declval<RightRef>()) result_type;

   template <typename L, typename R>
   result_type operator() (L&& l, R&& r) const
   {
      return std::forward<L>(l) + std::forward<R>(r);
   }

   template <typename L, typename R>
   void assign(L&& l, const R& r) const
   {
      l += r;
   }
};

template <typename LeftRef, typename RightRef>
struct add_impl<LeftRef, RightRef, cons<is_set, is_scalar> >
   : add_impl<LeftRef, RightRef, cons<is_set, is_set> > {};

template <typename LeftRef, typename RightRef>
struct sub_impl<LeftRef, RightRef, cons<is_set, is_set> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef decltype(std::declval<LeftRef>() - std::declval<RightRef>()) result_type;

   template <typename L, typename R>
   result_type operator() (L&& l, R&& r) const
   {
      return std::forward<L>(l) - std::forward<R>(r);
   }

   template <typename L, typename R>
   void assign(L&& l, const R& r) const
   {
      l -= r;
   }
};

template <typename LeftRef, typename RightRef>
struct sub_impl<LeftRef, RightRef, cons<is_set, is_scalar> >
   : sub_impl<LeftRef, RightRef, cons<is_set, is_set> > {};

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_set, is_set> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef decltype(std::declval<LeftRef>() * std::declval<RightRef>()) result_type;

   template <typename L, typename R>
   result_type operator() (L&& l, R&& r) const
   {
      return std::forward<L>(l) * std::forward<R>(r);
   }

   template <typename L, typename R>
   void assign(L&& l, const R& r) const
   {
      l *= r;
   }
};

template <typename LeftRef, typename RightRef>
struct bitwise_xor_impl<LeftRef, RightRef, cons<is_set, is_set> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef decltype(std::declval<LeftRef>() ^ std::declval<RightRef>()) result_type;

   template <typename L, typename R>
   result_type operator() (L&& l, R&& r) const
   {
      return std::forward<L>(l) * std::forward<R>(r);
   }

   template <typename L, typename R>
   void assign(L&& l, const R& r) const
   {
      l ^= r;
   }
};

template <typename LeftRef, typename RightRef>
struct includes {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef bool result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type s1,
                           typename function_argument<RightRef>::const_type s2) const
   {
      return incl(s2, s1) < 1;
   }
};

template <typename SetRef>
class element_of {
   alias<SetRef> set;
public:
   typedef typename deref<SetRef>::type set_type;
   typedef typename set_type::element_type argument_type;
   typedef bool result_type;

   element_of(typename alias<SetRef>::arg_type set_arg) : set(set_arg) {}

   result_type operator() (typename function_argument<argument_type>::type x) const
   {
      return set->contains(x);
   }
};

} // end namespace operations

template <typename Set1, typename Set2, bool both_have_size=
          (iterator_traits<typename Set1::iterator>::is_bidirectional &&
           iterator_traits<typename Set2::iterator>::is_bidirectional)>
struct size_estimator {
   /** Estimates how the insertion or removal of a sequence of elements could be made faster.
       Returns true if \\n2*log(n1) < n1+n2\\, which means that seeking for each element of set2
       in set1 would be faster than sequentially comparing element pairs from set1 and set2.
   */
   static bool seek_cheaper_than_sequential(const Set1& set1, const Set2& set2)
   {
      const Int n1 = set1.size();
      const Int n2 = set2.size();
      return n2 == 0 || set1.tree_form() && (n1/n2 >= 31 || (1L << (n1/n2)) > n1);
   }

   static Int compare(const Set1& set1, const Set2& set2)
   {
      return sign(set1.size() - set2.size());
   }
};

template <typename Set1, typename Set2>
struct size_estimator<Set1, Set2, false> {
   static bool seek_cheaper_than_sequential(const Set1&, const Set2&) { return true; }
   static Int compare(const Set1&, const Set2&) { return 0; }
};

/** Analyze the inclusion relation of two sets.
    @returnval  0 $s1$ and $s2$ are equal
    @returnval -1 $s1$ is included in $s2$
    @returnval  1 $s2$ is included in $s1$
    @returnval  2 $s1$ and $s2$ are independent
*/
template <typename Set1, typename Set2, typename E1, typename E2, class Comparator>
Int incl(const GenericSet<Set1, E1, Comparator>& s1, const GenericSet<Set2, E2, Comparator>& s2)
{
   auto e1 = entire(s1.top());
   auto e2 = entire(s2.top());
   Int result = size_estimator<Set1, Set2>::compare(s1.top(), s2.top());
   while (!e1.at_end() && !e2.at_end()) {
      switch (s1.top().get_comparator()(*e2,*e1)) {
      case cmp_eq: ++e1; ++e2; break;
      case cmp_lt:
         if (result > 0) return 2;
         result = -1;
         ++e2;
         break;
      case cmp_gt:
         if (result < 0) return 2;
         result = 1;
         ++e1;
         break;
      }
   }
   if ((!e1.at_end() && result < 0) || (!e2.at_end() && result > 0)) return 2;
   return result;
}

template <typename Container, typename Comparator=operations::cmp, typename ProvidedFeatures=mlist<>>
class OrderedContainer
   : public redirected_container< OrderedContainer<Container, Comparator, ProvidedFeatures>,
                                  mlist< HiddenTag< Container >,
                                         ExpectedFeaturesTag< ProvidedFeatures > > >
   , public GenericSet< OrderedContainer<Container, Comparator, ProvidedFeatures>,
                        typename object_traits<typename Container::value_type>::persistent_type,
                        Comparator > {};

template <typename Container, typename Comparator, typename ProvidedFeatures, typename Feature>
struct enforce_features<OrderedContainer<Container, Comparator, ProvidedFeatures>, Feature> {
   typedef OrderedContainer<Container, Comparator, typename mix_features<ProvidedFeatures, Feature>::type> container;
};

template <typename Container>
const OrderedContainer<Container>&
assure_ordered(const Container& c)
{
   return reinterpret_cast<const OrderedContainer<Container>&>(c);
}

template <typename Comparator, typename Container>
const OrderedContainer<Container, Comparator>&
assure_ordered(const Container& c)
{
   return reinterpret_cast<const OrderedContainer<Container, Comparator>&>(c);
}

template <typename ContainerRef>
class Indices
   : public modified_container_impl< Indices<ContainerRef>,
                                     mlist< ContainerRefTag< ContainerRef >,
                                            OperationTag< BuildUnaryIt<operations::index2element> >,
                                            ExpectedFeaturesTag< indexed > > >,
     public GenericSet< Indices<ContainerRef>, Int, operations::cmp> {
   using base_t = modified_container_impl<Indices>;
protected:
   using alias_t = alias<ContainerRef>;
   alias_t c;
public:
   using container_category = typename least_derived_class<bidirectional_iterator_tag, typename container_traits<ContainerRef>::category>::type;

   template <typename Arg, typename=std::enable_if_t<std::is_constructible<alias_t, Arg>::value>>
   explicit Indices(Arg&& c_arg)
      : c(std::forward<Arg>(c_arg)) {}

   decltype(auto) get_container() const { return *c; }

   bool contains(Int i) const { return !get_container().find(i).at_end(); }

   typename base_t::const_iterator find(Int i) const { return get_container().find(i); }
};

template <typename ContainerRef>
struct spec_object_traits< Indices<ContainerRef> >
   : spec_object_traits<is_container> {
   static constexpr bool is_temporary = true, is_always_const = true;
};

template <typename Container>
auto indices(Container&& c)
{
   return Indices<add_const_t<unwary_t<Container>>>(unwary(std::forward<Container>(c)));
}

template <typename Set>
struct hash_func<Set, is_set> {
   size_t operator() (const Set& s) const
   {
      hash_func<typename Set::element_type> element_hasher;
      size_t a=1, b=0;
      for (auto e=entire(s); !e.at_end(); ++e, ++b)
         a=a*element_hasher(*e)+b;
      return a;
   }
};

} // end namespace pm

namespace polymake {
   using pm::Series;
   using pm::series;
   using pm::sequence;
   using pm::range;
   using pm::GenericSet;
   using pm::scalar2set;
   using pm::assure_ordered;
   using pm::Indices;
   using pm::indices;

   namespace operations {
      typedef BuildBinary<pm::operations::includes> includes;

      template <typename Set>
      pm::operations::element_of<const Set&> element_of(const Set& s) { return s; }
   }
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
