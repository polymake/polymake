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

#ifndef POLYMAKE_INTERNAL_ITERATOR_ZIPPER_H
#define POLYMAKE_INTERNAL_ITERATOR_ZIPPER_H

#include "polymake/internal/comparators_basic_defs.h"
#include "polymake/ContainerUnion.h"
#include "polymake/internal/operations.h"

namespace pm {

enum {
   zipper_lt=1, zipper_eq=2, zipper_gt=4,       // correspond to 1<<cmp_XX+1
   zipper_cmp=zipper_lt+zipper_eq+zipper_gt,    // mask for all of them
   zipper_first= (zipper_lt<<6),                // first iterator is in a dereferenceable position
   zipper_second= (zipper_gt<<3),               // second iterator is ...
   zipper_both= zipper_first+zipper_second      // both are ...
};

struct forward_zipper {
   static int state(int cmp) { return 1 << 1+cmp; }
};

struct set_union_zipper : forward_zipper {
   static int stable(int) { return true; }
   static const bool end1=false, end2=false;
   static bool contains(bool c1, bool c2) { return c1 || c2; }
};

struct set_difference_zipper : forward_zipper {
   static int stable(int state) { return state & zipper_lt; }
   static const bool end1=true, end2=false;
   static bool contains(bool c1, bool c2) { return c1 && !c2; }
};

struct set_intersection_zipper : forward_zipper {
   static int stable(int state) { return state & zipper_eq; }
   static const bool end1=true, end2=true;
   static bool contains(bool c1, bool c2) { return c1 && c2; }
};

struct set_symdifference_zipper : forward_zipper {
   static int stable(int state) { return state & zipper_lt+zipper_gt; }
   static const bool end1=false, end2=false;
   static bool contains(bool c1, bool c2) { return c1 ^ c2; }
};

template <typename Zipper>
struct reverse_zipper : public Zipper {
   static int state(int cmp) { return 1 << 1-cmp; }
};

template <typename Iterator1, typename Iterator2, typename Comparator, typename Controller,
          bool _use_index1=false, bool _use_index2=false>
class iterator_zipper : public iterator_traits<Iterator1>::derivable_type {
public:
   typedef Iterator1 first_type;
   typedef Iterator2 second_type;
   typedef typename least_derived< cons<typename iterator_traits<Iterator1>::iterator_category,
                                   cons<typename iterator_traits<Iterator2>::iterator_category,
                                        bidirectional_iterator_tag> > >::type
      iterator_category;

   typedef iterator_zipper<typename iterator_traits<Iterator1>::iterator, typename iterator_traits<Iterator2>::iterator,
                           Comparator, Controller, _use_index1, _use_index2>
      iterator;
   typedef iterator_zipper<typename iterator_traits<Iterator1>::const_iterator, typename iterator_traits<Iterator2>::const_iterator,
                           Comparator, Controller, _use_index1, _use_index2>
      const_iterator;

   typedef typename iterator_traits<Iterator1>::derivable_type super;
   Iterator2 second;
protected:
   int state;
   Comparator cmp;
   Controller ctl;

   cmp_value _compare(False,False) const { return cmp(**this, *second); }
   cmp_value _compare(False,True) const { return cmp(**this, second.index()); }
   cmp_value _compare(True,False) const { return cmp(super::index(), *second); }
   cmp_value _compare(True,True) const { return cmp(super::index(), second.index()); }

   void compare()
   {
      state &= ~zipper_cmp;
      state += ctl.state(_compare(bool2type<_use_index1>(), bool2type<_use_index2>()));
   }

   void incr()
   {
      const int cur_state=state;
      if (cur_state & int(zipper_lt)+int(zipper_eq)) {
         super::operator++();
         if (super::at_end()) {
            if (ctl.end1) {
               state=0; return;
            } else {
               state>>=3;
            }
         }
      }
      if (cur_state & int(zipper_gt)+int(zipper_eq)) {
         ++second;
         if (second.at_end()) {
            if (ctl.end2) {
               state=0;
            } else {
               state>>=6;
            }
         }
      }
   }

   void decr()
   {
      const int cur_state=state;
      state=zipper_both;
      if (!(cur_state & int(zipper_lt)))
         super::operator--();
      if (!(cur_state & int(zipper_gt)))
         --second;
   }

   void init()
   {
      state=zipper_both;
      if (super::at_end()) {
         if (ctl.end1) {
            state=0; return;
         } else {
            state>>=3;
         }
      }
      if (second.at_end()) {
         if (ctl.end2) {
            state=0;
         } else {
            state>>=6;
         }
         return;
      }
      while (state>=zipper_both) {
         compare();
         if (ctl.stable(state)) break;
         incr();
      }
   }

   template <typename,typename,typename,typename,bool,bool> friend class iterator_zipper;
   template <typename,typename,bool,bool> friend class iterator_product;
public:
   iterator_zipper() : state(0) {}
   iterator_zipper(const iterator& it)
      : super(static_cast<const typename iterator::super&>(it)), second(it.second), state(it.state), ctl(it.ctl) {}

   iterator_zipper(const Iterator1& first_arg,
                   const Iterator2& second_arg)
      : super(first_arg), second(second_arg) { init(); }
   iterator_zipper(const Iterator1& first_arg,
                   typename alt_constructor<Iterator2>::arg_type& second_arg)
      : super(first_arg), second(second_arg) { init(); }
   iterator_zipper(typename alt_constructor<Iterator1>::arg_type& first_arg,
                   const Iterator2& second_arg)
      : super(first_arg), second(second_arg) { init(); }
   iterator_zipper(typename alt_constructor<Iterator1>::arg_type& first_arg,
                   typename alt_constructor<Iterator2>::arg_type& second_arg)
      : super(first_arg), second(second_arg) { init(); }

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
      typedef typename enable_if<super, iterator_pair_traits<Iterator1,Iterator2>::is_bidirectional>::type
         error_if_unimplemented __attribute__((unused));
      do {
         decr(); compare();
      } while (!ctl.stable(state));
      return *this;
   }
   const iterator_zipper operator-- (int) { iterator_zipper copy=*this; operator--(); return copy; }

   bool at_end() const { return state==0; }

   template <typename Iterator>
   typename enable_if<bool, comparable_iterator<iterator_zipper, Iterator>::value>::type
   operator== (const Iterator& it) const
   {
      return static_cast<const super&>(*this)==it && second==it.second;
   }
   template <typename Iterator>
   typename enable_if<bool, comparable_iterator<iterator_zipper, Iterator>::value>::type
   operator!= (const Iterator& it) const
   {
      return !operator==(it);
   }

   void rewind()
   {
      typedef typename enable_if<super, check_iterator_feature<Iterator1,rewindable>::value &&
                                        check_iterator_feature<Iterator2,rewindable>::value>::type
         error_if_unimplemented __attribute__((unused));
      super::rewind(); second.rewind();
      init();
   }
};

template <typename Iterator1, typename Iterator2, typename Comparator, typename Controller,
          bool _use_index1, bool _use_index2, typename Feature>
struct check_iterator_feature<iterator_zipper<Iterator1, Iterator2, Comparator, Controller, _use_index1, _use_index2>,
                              Feature> {
   static const bool value=check_iterator_feature<Iterator1, Feature>::value &&
                           check_iterator_feature<Iterator2, Feature>::value;
};

template <typename Iterator1, typename Iterator2, typename Comparator, typename Controller,
          bool _use_index1, bool _use_index2>
struct has_partial_state< iterator_zipper<Iterator1, Iterator2, Comparator, Controller, _use_index1, _use_index2> > : True {};

template <typename Comparator, typename Controller, bool _use_index1=false, bool _use_index2=false>
struct zipping_coupler {
   template <typename Iterator1, typename Iterator2, typename ExpectedFeatures>
   struct defs {
      typedef iterator_zipper<Iterator1,Iterator2,Comparator,Controller,_use_index1,_use_index2> iterator;

      typedef typename mix_features<ExpectedFeatures,
                                    typename concat_if<_use_index1, indexed, end_sensitive>::type >::type
         needed_features1;
      typedef typename mix_features<ExpectedFeatures,
                                    typename concat_if<_use_index2, indexed, end_sensitive>::type >::type
         needed_features2;
   };
};

template <typename Comparator, typename Controller, bool _use_index1, bool _use_index2>
struct reverse_coupler< zipping_coupler<Comparator, Controller, _use_index1, _use_index2> > {
   typedef zipping_coupler< Comparator, reverse_zipper<Controller>, _use_index1, _use_index2> type;
};

namespace operations {

template <typename OpRef1, typename OpRef2,
          bool _equal=identical_minus_const_ref<OpRef1,OpRef2>::value,
          bool _numeric=(std::numeric_limits<typename deref<OpRef1>::type>::is_specialized &&
                         std::numeric_limits<typename deref<OpRef2>::type>::is_specialized)>
struct zipper_helper
   : union_reference<OpRef1,OpRef2> {};

template <typename OpRef1, typename OpRef2>
struct zipper_helper<OpRef1,OpRef2,false,true>
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
   typedef const int result_type;

   result_type operator() (first_argument_type l, second_argument_type) const { return l.index(); }
   result_type operator() (partial_left, first_argument_type l, second_argument_type) const { return l.index(); }
   result_type operator() (partial_right, first_argument_type, second_argument_type r) const { return r.index(); }
};
} // end namespace operations

template <typename IteratorPair, typename Operation>
class binary_transform_eval<IteratorPair, Operation, true>
   : public transform_iterator_base<IteratorPair,Operation>::type {
public:
   typedef typename transform_iterator_base<IteratorPair,Operation>::type super;
   typedef binary_helper<IteratorPair,Operation> helper;
   typedef typename helper::operation operation;
protected:
   operation op;

   typedef Operation op_arg_type;

   binary_transform_eval() {}

   template <typename Operation2>
   binary_transform_eval(const binary_transform_eval<typename iterator_traits<super>::iterator, Operation2, true>& it)
      : super(it), op(helper::create(it.op)) {}

   binary_transform_eval(const IteratorPair& cur_arg,
                         const op_arg_type& op_arg)
      : super(cur_arg), op(helper::create(op_arg)) {}
   binary_transform_eval(typename alt_constructor<IteratorPair>::arg_type& cur_arg,
                         const op_arg_type& op_arg)
      : super(cur_arg), op(helper::create(op_arg)) {}

   binary_transform_eval(const typename IteratorPair::first_type& first_arg,
                         const typename IteratorPair::second_type& second_arg,
                         const op_arg_type& op_arg)
      : super(first_arg,second_arg), op(helper::create(op_arg)) {}
   binary_transform_eval(const typename IteratorPair::first_type& first_arg,
                         typename alt_constructor<typename IteratorPair::second_type>::arg_type& second_arg,
                         const op_arg_type& op_arg)
      : super(first_arg,second_arg), op(helper::create(op_arg)) {}
   binary_transform_eval(typename alt_constructor<typename IteratorPair::first_type>::arg_type& first_arg,
                         const typename IteratorPair::second_type& second_arg,
                         const op_arg_type& op_arg)
      : super(first_arg,second_arg), op(helper::create(op_arg)) {}
   binary_transform_eval(typename alt_constructor<typename IteratorPair::first_type>::arg_type& first_arg,
                         typename alt_constructor<typename IteratorPair::second_type>::arg_type& second_arg,
                         const op_arg_type& op_arg)
      : super(first_arg,second_arg), op(helper::create(op_arg)) {}

   template <typename,typename,bool> friend class binary_transform_eval;
public:
   typedef typename operation::result_type reference;

   reference operator* () const
   {
      if (this->state & zipper_lt)
         return op(operations::partial_left(), *helper::get1(*this), this->second);
      if (this->state & zipper_gt)
         return op(operations::partial_right(),
                   static_cast<const typename IteratorPair::super&>(*this), *helper::get2(this->second));
      // (this->state & zipper_eq)
      return op(*helper::get1(*this), *helper::get2(this->second));
   }
};

template <typename IteratorPair, typename Operation, typename IndexOperation>
class binary_transform_eval<IteratorPair, pair<Operation, IndexOperation>, true>
   : public binary_transform_eval<IteratorPair, Operation, true> {
   typedef binary_transform_eval<IteratorPair, Operation, true> _super;
protected:
   typedef binary_helper<IteratorPair,IndexOperation> ihelper;
   typename ihelper::operation iop;

   typedef pair<Operation, IndexOperation> op_arg_type;

   binary_transform_eval() {}

   template <typename Operation2, typename IndexOperation2>
   binary_transform_eval(const binary_transform_eval<typename iterator_traits<IteratorPair>::iterator,
                                                     pair<Operation2, IndexOperation2>, true>& it)
      : _super(it), iop(ihelper::create(it.iop)) {}

   binary_transform_eval(const IteratorPair& cur_arg,
                         const op_arg_type& op_arg)
      : _super(cur_arg,op_arg.first), iop(ihelper::create(op_arg.second)) {}
   binary_transform_eval(typename alt_constructor<IteratorPair>::arg_type& cur_arg,
                         const op_arg_type& op_arg)
      : _super(cur_arg,op_arg.first), iop(ihelper::create(op_arg.second)) {}

   binary_transform_eval(const typename IteratorPair::first_type& first_arg,
                         const typename IteratorPair::second_type& second_arg,
                         const op_arg_type& op_arg)
      : _super(first_arg,second_arg,op_arg.first), iop(ihelper::create(op_arg.second)) {}
   binary_transform_eval(const typename IteratorPair::first_type& first_arg,
                         typename alt_constructor<typename IteratorPair::second_type>::arg_type& second_arg,
                         const op_arg_type& op_arg)
      : _super(first_arg,second_arg,op_arg.first), iop(ihelper::create(op_arg.second)) {}
   binary_transform_eval(typename alt_constructor<typename IteratorPair::first_type>::arg_type& first_arg,
                         const typename IteratorPair::second_type& second_arg,
                         const op_arg_type& op_arg)
      : _super(first_arg,second_arg,op_arg.first), iop(ihelper::create(op_arg.second)) {}
   binary_transform_eval(typename alt_constructor<typename IteratorPair::first_type>::arg_type& first_arg,
                         typename alt_constructor<typename IteratorPair::second_type>::arg_type& second_arg,
                         const op_arg_type& op_arg)
      : _super(first_arg,second_arg,op_arg.first), iop(ihelper::create(op_arg.second)) {}

   template <typename,typename,bool> friend class binary_transform_eval;

protected:
   int _index(False) const
   {
      return iop(*ihelper::get1(*this), *ihelper::get2(this->second));
   }

   int _index(True) const
   {
      if (this->state & zipper_lt)
         return iop(operations::partial_left(), *ihelper::get1(*this), this->second);
      if (this->state & zipper_gt)
         return iop(operations::partial_right(),
                    static_cast<const typename IteratorPair::super&>(*this), *ihelper::get2(this->second));
      // (this->state & zipper_eq)
      return _index(False());
   }
public:
   int index() const
   {
      return _index(bool2type<operations::is_partially_defined<typename ihelper::operation>::value>());
   }
};

} // end namespace pm

#endif // POLYMAKE_INTERNAL_ITERATOR_ZIPPER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
