/* Copyright (c) 1997-2022
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

#include "polymake/internal/comparators_basic_defs.h"
#include "polymake/ContainerUnion.h"
#include "polymake/internal/operations.h"

namespace pm {

enum {
   zipper_lt = 1, zipper_eq = 2, zipper_gt = 4,       // correspond to 1<<cmp_XX+1
   zipper_cmp = zipper_lt + zipper_eq + zipper_gt,    // mask for all of them
   zipper_first = (zipper_lt << 6),                   // first iterator is in a dereferenceable position
   zipper_second = (zipper_gt << 3),                  // second iterator is ...
   zipper_both = zipper_first + zipper_second         // both are ...
};

struct forward_zipper {
   static constexpr int state(int cmp) { return 1 << 1+cmp; }
};

struct set_union_zipper : forward_zipper {
   static constexpr int stable(int) { return 1; }
   static constexpr bool end1 = false, end2 = false;
   static constexpr bool contains(bool c1, bool c2) { return c1 || c2; }
};

struct set_difference_zipper : forward_zipper {
   static constexpr int stable(int state) { return state & zipper_lt; }
   static constexpr bool end1 = true, end2 = false;
   static constexpr bool contains(bool c1, bool c2) { return c1 && !c2; }
};

struct set_intersection_zipper : forward_zipper {
   static constexpr int stable(int state) { return state & zipper_eq; }
   static constexpr bool end1 = true, end2 = true;
   static constexpr bool contains(bool c1, bool c2) { return c1 && c2; }
};

struct set_symdifference_zipper : forward_zipper {
   static constexpr int stable(int state) { return state & zipper_lt+zipper_gt; }
   static constexpr bool end1 = false, end2 = false;
   static constexpr bool contains(bool c1, bool c2) { return c1 ^ c2; }
};

template <typename Zipper>
struct reverse_zipper : public Zipper {
   static constexpr int state(int cmp) { return 1 << 1-cmp; }
};

template <typename Iterator1, typename Iterator2, typename Comparator, typename Controller,
          bool use_index1 = false, bool use_index2 = false>
class iterator_zipper : public Iterator1 {
public:
   using first_type = Iterator1;
   using second_type = Iterator2;
   using iterator_category = typename least_derived_class<bidirectional_iterator_tag,
                                                          typename iterator_traits<Iterator1>::iterator_category,
                                                          typename iterator_traits<Iterator2>::iterator_category>::type;

   using iterator = iterator_zipper<typename iterator_traits<Iterator1>::iterator, typename iterator_traits<Iterator2>::iterator,
                                    Comparator, Controller, use_index1, use_index2>;
   using const_iterator = iterator_zipper<typename iterator_traits<Iterator1>::const_iterator, typename iterator_traits<Iterator2>::const_iterator,
                                          Comparator, Controller, use_index1, use_index2>;

   Iterator2 second;
protected:
   int state;
   Comparator cmp;
   Controller ctl;

   cmp_value compare(std::false_type, std::false_type) const { return cmp(**this, *second); }
   cmp_value compare(std::false_type, std::true_type) const { return cmp(**this, second.index()); }
   cmp_value compare(std::true_type, std::false_type) const { return cmp(first_type::index(), *second); }
   cmp_value compare(std::true_type, std::true_type) const { return cmp(first_type::index(), second.index()); }

   void compare()
   {
      state &= ~zipper_cmp;
      state += ctl.state(compare(bool_constant<use_index1>(), bool_constant<use_index2>()));
   }

   void incr()
   {
      const int cur_state = state;
      if (cur_state & int(zipper_lt) + int(zipper_eq)) {
         first_type::operator++();
         if (first_type::at_end()) {
            if (ctl.end1) {
               state = 0; return;
            } else {
               state >>= 3;
            }
         }
      }
      if (cur_state & int(zipper_gt) + int(zipper_eq)) {
         ++second;
         if (second.at_end()) {
            if (ctl.end2) {
               state = 0;
            } else {
               state >>= 6;
            }
         }
      }
   }

   void decr()
   {
      const int cur_state = state;
      state = zipper_both;
      if ((cur_state & int(zipper_lt)) == 0)
         first_type::operator--();
      if ((cur_state & int(zipper_gt)) == 0)
         --second;
   }

   void init()
   {
      state = zipper_both;
      if (first_type::at_end()) {
         if (ctl.end1) {
            state = 0; return;
         } else {
            state >>= 3;
         }
      }
      if (second.at_end()) {
         if (ctl.end2) {
            state = 0;
         } else {
            state >>= 6;
         }
         return;
      }
      while (state >= zipper_both) {
         compare();
         if (ctl.stable(state)) break;
         incr();
      }
   }

   template <typename, typename, typename, typename, bool, bool> friend class iterator_zipper;
   template <typename, typename, bool, bool> friend class iterator_product;
public:
   iterator_zipper() : state(0) {}
   iterator_zipper(const iterator& it)
      : first_type(static_cast<const typename iterator::first_type&>(it))
      , second(it.second)
      , state(it.state)
      , ctl(it.ctl) {}

   template <typename SourceIterator1, typename SourceIterator2,
             typename enable=typename std::enable_if<is_const_compatible_with<SourceIterator1, Iterator1>::value &&
                                                     is_const_compatible_with<SourceIterator2, Iterator2>::value>::type>
   iterator_zipper(const SourceIterator1& first_arg, const SourceIterator2& second_arg)
      : first_type(first_arg)
      , second(second_arg)
   {
      init();
   }

   iterator_zipper& operator++ ()
   {
      do {
         incr();
         if (state<zipper_both) break;
         compare();
      } while (!ctl.stable(state));
      return *this;
   }
   const iterator_zipper operator++ (int) { iterator_zipper copy=*this; operator++(); return copy; }

   iterator_zipper& operator-- ()
   {
      static_assert(iterator_pair_traits<Iterator1, Iterator2>::is_bidirectional, "iterator is not bidirectional");
      do {
         decr(); compare();
      } while (!ctl.stable(state));
      return *this;
   }
   const iterator_zipper operator-- (int) { iterator_zipper copy=*this; operator--(); return copy; }

   bool at_end() const { return state==0; }

   template <typename Other>
   std::enable_if_t<is_derived_from_any<Other, iterator, const_iterator>::value, bool>
   operator== (const Other& it) const
   {
      using other_iterator = typename is_derived_from_any<Other, iterator, const_iterator>::match;

      return at_end() ? it.at_end() :
             static_cast<const first_type&>(*this) == static_cast<const typename other_iterator::first_type&>(it)
             && second == static_cast<const other_iterator&>(it).second;
   }

   template <typename Other>
   std::enable_if_t<is_derived_from_any<Other, iterator, const_iterator>::value, bool>
   operator!= (const Other& it) const
   {
      return !operator==(it);
   }

   void rewind()
   {
      static_assert(check_iterator_feature<first_type, rewindable>::value && check_iterator_feature<second_type, rewindable>::value,
                    "iterator is not rewindable");
      first_type::rewind(); second.rewind();
      init();
   }
};

template <typename Iterator1, typename Iterator2, typename Comparator, typename Controller,
          bool use_index1, bool use_index2, typename Feature>
struct check_iterator_feature<iterator_zipper<Iterator1, Iterator2, Comparator, Controller, use_index1, use_index2>, Feature>
   : mlist_and< check_iterator_feature<Iterator1, Feature>,
                check_iterator_feature<Iterator2, Feature> > {};

template <typename Iterator1, typename Iterator2, typename Comparator, typename Controller,
          bool use_index1, bool use_index2>
struct has_partial_state< iterator_zipper<Iterator1, Iterator2, Comparator, Controller, use_index1, use_index2> > : std::true_type {};

template <typename Comparator, typename Controller, bool use_index1 = false, bool use_index2 = false>
struct zipping_coupler {
   template <typename Iterator1, typename Iterator2, typename... ExpectedFeatures>
   struct defs {
      using expected_features = typename mlist_wrap<ExpectedFeatures...>::type;

      using iterator = iterator_zipper<Iterator1, Iterator2, Comparator, Controller, use_index1, use_index2>;

      using needed_features1 = typename mix_features<expected_features,
                                                     typename mlist_prepend_if<use_index1, indexed, end_sensitive>::type >::type;
      using needed_features2 = typename mix_features<expected_features,
                                                     typename mlist_prepend_if<use_index2, indexed, end_sensitive>::type >::type;
   };
};

template <typename Comparator, typename Controller, bool use_index1, bool use_index2>
struct reverse_coupler< zipping_coupler<Comparator, Controller, use_index1, use_index2> > {
   using type = zipping_coupler< Comparator, reverse_zipper<Controller>, use_index1, use_index2>;
};

namespace operations {

template <typename OpRef1, typename OpRef2,
          bool are_equal=same_pure_type<OpRef1,OpRef2>::value,
          bool are_numeric=(std::numeric_limits<typename deref<OpRef1>::type>::is_specialized &&
                            std::numeric_limits<typename deref<OpRef2>::type>::is_specialized)>
struct zipper_helper
   : union_reference<OpRef1, OpRef2> {};

template <typename OpRef1, typename OpRef2>
struct zipper_helper<OpRef1, OpRef2, false, true>
   : add_result<typename deref<OpRef1>::type, typename deref<OpRef2>::type> {};

template <typename Iterator1Ref, typename Iterator2Ref>
struct zipper {
   typedef Iterator1Ref first_argument_type;
   typedef Iterator2Ref second_argument_type;
   typedef typename zipper_helper<typename iterator_traits<typename deref<Iterator1Ref>::type>::reference,
                                  typename iterator_traits<typename deref<Iterator2Ref>::type>::reference>::type
      result_type;

   result_type operator() (first_argument_type l, second_argument_type) const { return *l; }
   result_type operator() (partial_left, first_argument_type l, second_argument_type) const { return *l; }
   result_type operator() (partial_right, first_argument_type, second_argument_type r) const { return *r; }
};

template <typename Iterator1Ref, typename Iterator2Ref>
struct zipper_index {
   typedef Iterator1Ref first_argument_type;
   typedef Iterator2Ref second_argument_type;
   typedef Int result_type;

   result_type operator() (first_argument_type l, second_argument_type) const { return l.index(); }
   result_type operator() (partial_left, first_argument_type l, second_argument_type) const { return l.index(); }
   result_type operator() (partial_right, first_argument_type, second_argument_type r) const { return r.index(); }
};
} // end namespace operations

template <typename IteratorPair, typename Operation>
class binary_transform_eval<IteratorPair, Operation, true>
   : public transform_iterator_base<IteratorPair, Operation>::type {
   using base_t = typename transform_iterator_base<IteratorPair, Operation>::type;
public:
   typedef binary_helper<IteratorPair, Operation> helper;
   typedef typename helper::operation operation;
protected:
   operation op;

   typedef Operation op_arg_type;

   binary_transform_eval() {}

   template <typename Operation2>
   binary_transform_eval(const binary_transform_eval<typename iterator_traits<IteratorPair>::iterator, Operation2, true>& it)
      : base_t(static_cast<const typename std::remove_reference_t<decltype(it)>::base_t&>(it))
      , op(helper::create(it.op)) {}

   template <typename SourceIteratorPair>
   binary_transform_eval(const SourceIteratorPair& cur_arg, const op_arg_type& op_arg)
      : base_t(cur_arg)
      , op(helper::create(op_arg)) {}

   template <typename SourceIterator1, typename SourceIterator2>
   binary_transform_eval(const SourceIterator1& first_arg, const SourceIterator2& second_arg, const op_arg_type& op_arg)
      : base_t(first_arg, second_arg)
      , op(helper::create(op_arg)) {}

   template <typename, typename, bool> friend class binary_transform_eval;
public:
   typedef typename operation::result_type reference;

   reference operator* () const
   {
      if (this->state & zipper_lt)
         return op(operations::partial_left(), *helper::get1(*this), this->second);
      if (this->state & zipper_gt)
         return op(operations::partial_right(),
                   static_cast<const typename IteratorPair::first_type&>(*this), *helper::get2(this->second));
      // (this->state & zipper_eq)
      return op(*helper::get1(*this), *helper::get2(this->second));
   }
};

template <typename IteratorPair, typename Operation, typename IndexOperation>
class binary_transform_eval<IteratorPair, pair<Operation, IndexOperation>, true>
   : public binary_transform_eval<IteratorPair, Operation, true> {
   using base_t = binary_transform_eval<IteratorPair, Operation, true>;
protected:
   typedef binary_helper<IteratorPair,IndexOperation> ihelper;
   typename ihelper::operation iop;
   typedef pair<Operation, IndexOperation> op_arg_type;

   binary_transform_eval() {}

   template <typename Operation2, typename IndexOperation2>
   binary_transform_eval(const binary_transform_eval<typename iterator_traits<IteratorPair>::iterator, pair<Operation2, IndexOperation2>, true>& it)
      : base_t(it)
      , iop(ihelper::create(it.iop)) {}

   template <typename SourceIteratorPair>
   binary_transform_eval(const SourceIteratorPair& cur_arg, const op_arg_type& op_arg)
      : base_t(cur_arg, op_arg.first)
      , iop(ihelper::create(op_arg.second)) {}

   template <typename SourceIterator1, typename SourceIterator2>
   binary_transform_eval(const SourceIterator1& first_arg, const SourceIterator2& second_arg, const op_arg_type& op_arg)
      : base_t(first_arg, second_arg, op_arg.first)
      , iop(ihelper::create(op_arg.second)) {}

   template <typename, typename, bool> friend class binary_transform_eval;

protected:
   Int index_impl(std::false_type) const
   {
      return iop(*ihelper::get1(*this), *ihelper::get2(this->second));
   }

   Int index_impl(std::true_type) const
   {
      if (this->state & zipper_lt)
         return iop(operations::partial_left(), *ihelper::get1(*this), this->second);
      if (this->state & zipper_gt)
         return iop(operations::partial_right(),
                    static_cast<const typename IteratorPair::first_type&>(*this), *ihelper::get2(this->second));
      // (this->state & zipper_eq)
      return index_impl(std::false_type());
   }
public:
   Int index() const
   {
      return index_impl(bool_constant<operations::is_partially_defined<typename ihelper::operation>::value>());
   }
};

} // end namespace pm


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
