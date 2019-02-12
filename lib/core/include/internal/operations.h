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

#ifndef POLYMAKE_INTERNAL_OPERATIONS_H
#define POLYMAKE_INTERNAL_OPERATIONS_H

#include "polymake/internal/operations_basic_defs.h"
#include "polymake/internal/iterators.h"

namespace pm {
namespace operations {

template <typename Operation>
class is_partially_defined {
   typedef typename Operation::first_argument_type first_orig_argument_type;
   typedef typename Operation::second_argument_type second_orig_argument_type;
   typedef typename std::conditional<std::is_same<first_orig_argument_type, void>::value, void*, first_orig_argument_type>::type
      first_argument_type;
   typedef typename std::conditional<std::is_same<second_orig_argument_type, void>::value, void*, second_orig_argument_type>::type
      second_argument_type;
   struct helper {
      template <typename Result>
      static std::true_type Test(const Result&);
      static std::false_type Test(helper*);
   };
   struct catch_first {
      catch_first(typename function_argument<first_argument_type>::const_type) {}
   };
   struct catch_second {
      catch_second(typename function_argument<second_argument_type>::const_type) {}
   };
   struct mix_in : public Operation {
      using Operation::operator();
      helper* operator() (partial_left, catch_first, catch_second) const;
      helper* operator() (partial_right, catch_first, catch_second) const;
   };
public:
   using type = typename mlist_or< decltype(helper::Test( std::declval<mix_in&>()(partial_left(), std::declval<first_argument_type&>(), std::declval<second_argument_type&>()))),
                                   decltype(helper::Test( std::declval<mix_in&>()(partial_right(), std::declval<first_argument_type&>(), std::declval<second_argument_type&>())))
                >::type;
   static constexpr bool value = type::value;
};

template <typename Operation, typename Container1, typename Container2>
struct is_partially_defined_for
   : is_partially_defined<typename binary_op_builder<Operation, typename Container1::const_iterator,
                                                                typename Container2::const_iterator>::operation> {};

template <typename OpRef,
          typename Discr=typename object_traits<typename deref<OpRef>::type>::generic_tag>
struct neg_impl;

template <typename LeftRef, class RightRef,
          typename Discr=typename isomorphic_types<typename deref<LeftRef>::type, typename deref<RightRef>::type>::discriminant>
struct add_impl;

template <typename LeftRef, typename RightRef,
          typename Discr=typename isomorphic_types<typename deref<LeftRef>::type, typename deref<RightRef>::type>::discriminant>
struct sub_impl;

template <typename LeftRef, typename RightRef,
          typename Discr=typename isomorphic_types<typename deref<LeftRef>::type, typename deref<RightRef>::type>::discriminant>
struct mul_impl;

template <typename LeftRef, typename RightRef,
          typename Discr=typename isomorphic_types<typename deref<LeftRef>::type, typename deref<RightRef>::type>::discriminant>
struct div_impl;

template <typename LeftRef, typename RightRef,
          typename Discr=typename isomorphic_types<typename deref<LeftRef>::type, typename deref<RightRef>::type>::discriminant>
struct divexact_impl;

template <typename LeftRef, typename RightRef,
          typename Discr=typename isomorphic_types<typename deref<LeftRef>::type, typename deref<RightRef>::type>::discriminant>
struct tensor_impl;

template <typename OpRef>
struct neg_impl<OpRef, is_scalar>
   : neg_scalar<typename deref<OpRef>::type, typename deref<OpRef>::type> {};

template <typename LeftRef, typename RightRef>
struct add_impl<LeftRef, RightRef, cons<is_scalar, is_scalar> >
   : add_scalar<typename deref<LeftRef>::type, typename deref<RightRef>::type,
                typename add_result<typename deref<LeftRef>::type, typename deref<RightRef>::type>::type> {};

template <typename LeftRef, typename RightRef>
struct sub_impl<LeftRef, RightRef, cons<is_scalar, is_scalar> >
   : sub_scalar<typename deref<LeftRef>::type, typename deref<RightRef>::type,
                typename sub_result<typename deref<LeftRef>::type, typename deref<RightRef>::type>::type> {};

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_scalar, is_scalar> >
   : mul_scalar<typename deref<LeftRef>::type, typename deref<RightRef>::type,
                typename mul_result<typename deref<LeftRef>::type, typename deref<RightRef>::type>::type> {};

template <typename LeftRef, typename RightRef>
struct div_impl<LeftRef, RightRef, cons<is_scalar, is_scalar> >
   : div_scalar<typename deref<LeftRef>::type, typename deref<RightRef>::type,
                typename div_result<typename deref<LeftRef>::type, typename deref<RightRef>::type>::type> {};

template <typename LeftRef, typename RightRef>
struct divexact_impl<LeftRef, RightRef, cons<is_scalar, is_scalar> >
   : divexact_scalar<typename deref<LeftRef>::type, typename deref<RightRef>::type,
                     typename div_result<typename deref<LeftRef>::type, typename deref<RightRef>::type>::type> {};

template <typename OpRef,
          typename Discr=typename object_traits<typename deref<OpRef>::type>::generic_tag>
struct square_impl {
   typedef OpRef argument_type;
   typedef decltype(sqr(std::declval<OpRef>())) result_type;

   result_type operator() (argument_type a) const
   {
      return sqr(a);
   }
};

template <typename Char, typename Traits, typename Alloc>
struct add_impl<const std::basic_string<Char, Traits, Alloc>&, const std::basic_string<Char, Traits, Alloc>&, cons<is_opaque, is_opaque> > :
   add_scalar<std::basic_string<Char, Traits, Alloc>, std::basic_string<Char, Traits, Alloc>, std::basic_string<Char, Traits, Alloc> > {};

template <typename Char, typename Traits, typename Alloc>
struct add_impl<std::basic_string<Char, Traits, Alloc>&, const std::basic_string<Char, Traits, Alloc>&, cons<is_opaque, is_opaque> > :
   add_scalar<std::basic_string<Char, Traits, Alloc>, std::basic_string<Char, Traits, Alloc>, std::basic_string<Char, Traits, Alloc> > {};

template <typename Char, typename Traits, typename Alloc>
struct add_impl<const std::basic_string<Char, Traits, Alloc>&, std::basic_string<Char, Traits, Alloc>&, cons<is_opaque, is_opaque> > :
   add_scalar<std::basic_string<Char, Traits, Alloc>, std::basic_string<Char, Traits, Alloc>, std::basic_string<Char, Traits, Alloc> > {};

template <typename Char, typename Traits, typename Alloc>
struct add_impl<std::basic_string<Char, Traits, Alloc>&, std::basic_string<Char, Traits, Alloc>&, cons<is_opaque, is_opaque> > :
   add_scalar<std::basic_string<Char, Traits, Alloc>, std::basic_string<Char, Traits, Alloc>, std::basic_string<Char, Traits, Alloc> > {};

template <typename Char, typename Traits, typename Alloc>
struct add_impl<const std::basic_string<Char, Traits, Alloc>&, const Char*, cons<is_opaque, is_not_object> > :
   add_scalar<std::basic_string<Char, Traits, Alloc>, const Char*, std::basic_string<Char, Traits, Alloc> > {};

template <typename Char, typename Traits, typename Alloc>
struct add_impl<std::basic_string<Char, Traits, Alloc>&, const Char*, cons<is_opaque, is_not_object> > :
   add_scalar<std::basic_string<Char, Traits, Alloc>, const Char*, std::basic_string<Char, Traits, Alloc> > {};

template <typename OpRef>
struct neg : neg_impl<OpRef> {};
template <typename LeftRef, typename RightRef>
struct add : add_impl<LeftRef,RightRef> {};
template <typename LeftRef, typename RightRef>
struct sub : sub_impl<LeftRef,RightRef> {};
template <typename LeftRef, typename RightRef>
struct mul : mul_impl<LeftRef,RightRef> {};
template <typename LeftRef, typename RightRef>
struct div : div_impl<LeftRef,RightRef> {};
template <typename LeftRef, typename RightRef>
struct divexact : divexact_impl<LeftRef,RightRef> {};
template <typename OpRef>
struct square : square_impl<OpRef> {};
template <typename LeftRef, typename RightRef>
struct tensor : tensor_impl<LeftRef,RightRef> {};

// TODO: replace with a generic swap_operands
template <typename LeftRef, typename RightRef>
struct mul_from_left : mul_impl<RightRef, LeftRef> {
   typedef RightRef first_argument_type;
   typedef LeftRef second_argument_type;

   template <typename L, typename R>
   decltype(auto) operator() (L&& l, R&& r) const
   {
      return r * l;
   }

   template <typename L, typename R>
   void assign(L&& l, R&& r) const
   {
      l = r * l;
   }
};

template <typename LeftRef, typename RightRef,
          typename Discr=typename isomorphic_types<typename deref<LeftRef>::type, typename deref<RightRef>::type>::discriminant>
struct bitwise_xor_impl;

template <typename LeftRef, typename RightRef>
struct bitwise_xor : bitwise_xor_impl<LeftRef,RightRef> {};

struct unary_noop {
   typedef void argument_type;
   typedef void result_type;
   template <typename Any>
   result_type operator() (const Any&) const {}
};

struct binary_noop {
   typedef void first_argument_type;
   typedef void second_argument_type;
   typedef void result_type;
   template <typename Any1, typename Any2>
   result_type operator() (const Any1&, const Any2&) const {}
};

template <typename T> class composite_clear;

template <typename OpRef>
class clear {
public:
   typedef OpRef argument_type;
   typedef typename deref<OpRef>::type value_type;
   typedef typename std::conditional<object_traits<value_type>::allow_static, typename attrib<OpRef>::plus_const_ref, value_type>::type result_type;

   result_type operator() () const
   {
      return default_instance(bool_constant<object_traits<value_type>::allow_static>());
   }

   void operator() (typename lvalue_arg<OpRef>::type x) const
   {
      do_clear(x, typename object_traits<value_type>::model());
   }

private:
   static
   result_type default_instance(std::true_type)
   {
      static const value_type dflt = value_type();
      return dflt;
   }

   static
   result_type default_instance(std::false_type)
   {
      return value_type();
   }

   template <typename Model>
   void do_clear(typename lvalue_arg<OpRef>::type x, Model) const
   {
      x=operator()();
   }

   void do_clear(typename lvalue_arg<OpRef>::type x, is_scalar) const
   {
      x=object_traits<value_type>::zero();
   }

   void do_clear(typename lvalue_arg<OpRef>::type, nothing) const {}

   void do_clear(typename lvalue_arg<OpRef>::type x, is_container) const
   {
      x.clear();
   }

   void do_clear(typename lvalue_arg<OpRef>::type x, is_composite) const
   {
      composite_clear<typename object_traits<value_type>::elements> cc;
      object_traits<value_type>::visit_elements(x, cc);
   }
};

template <> class composite_clear<void> {};

template <typename T>
class composite_clear : public composite_clear<typename list_tail<T>::type> {
public:
   typedef typename list_head<T>::type element_type;
   typedef typename deref<element_type>::type value_type;

   composite_clear<typename list_tail<T>::type>&
   operator<< (typename attrib<element_type>::plus_ref elem)
   {
      operations::clear<value_type> clr;
      clr(elem);
      return *this;
   }
};

template <typename Cref, typename Operation>
struct fix1 : Operation {
   typedef Operation super;
   typedef Cref stored_type;
   stored_type c;

   typedef typename Operation::second_argument_type argument_type;
   typedef typename Operation::result_type result_type;

   fix1() {}

   fix1(typename function_argument<Cref>::type c_arg, const Operation& op_arg=Operation())
      : super(op_arg), c(c_arg) {}

   result_type operator() (typename function_argument<argument_type>::type b) const
   {
      return super::operator()(c,b);
   }
};

template <typename Cref, typename Operation>
struct fix2 : Operation {
   typedef Operation super;
   typedef Cref stored_type;
   stored_type c;

   typedef typename Operation::first_argument_type argument_type;
   typedef typename Operation::result_type result_type;

   fix2() {}

   fix2(typename function_argument<Cref>::type c_arg, const Operation& op_arg=Operation())
      : super(op_arg), c(c_arg) {}

   result_type operator() (typename function_argument<argument_type>::type a) const
   {
      return super::operator()(a,c);
   }

   void assign(typename lvalue_arg<argument_type>::type a) const
   {
      super::assign(a,c);
   }
};

// Compositions of two operations

// OuterUnary(InnerUnary(x))
template <typename InnerOperation, typename OuterOperation>
struct composed11 {
   InnerOperation inner;
   OuterOperation outer;

   typedef typename InnerOperation::argument_type argument_type;
   typedef typename OuterOperation::result_type result_type;

   composed11(const InnerOperation& inner_arg=InnerOperation(), const OuterOperation& outer_arg=OuterOperation())
      : inner(inner_arg), outer(outer_arg) {}
   composed11(const OuterOperation& outer_arg)
      : outer(outer_arg) {}

   result_type operator() (typename function_argument<argument_type>::type a) const
   {
      return outer(inner(a));
   }

   void assign(typename lvalue_arg<argument_type>::type a) const
   {
      inner.assign(a); outer.assign(a);
   }
};

// OuterUnary(InnerBinary(x,y))
template <typename InnerOperation, typename OuterOperation, bool is_partial=is_partially_defined<InnerOperation>::value>
struct composed21 {
   InnerOperation inner;
   OuterOperation outer;

   typedef typename InnerOperation::first_argument_type first_argument_type;
   typedef typename InnerOperation::second_argument_type second_argument_type;
   typedef typename OuterOperation::result_type result_type;

   composed21(const InnerOperation& inner_arg=InnerOperation(), const OuterOperation& outer_arg=OuterOperation())
      : inner(inner_arg), outer(outer_arg) {}
   composed21(const OuterOperation& outer_arg)
      : outer(outer_arg) {}

   result_type operator() (typename function_argument<first_argument_type>::type a,
                           typename function_argument<second_argument_type>::type b) const
   {
      return outer(inner(a,b));
   }

   void assign(typename lvalue_arg<first_argument_type>::type a,
               typename function_argument<second_argument_type>::type b) const
   {
      inner.assign(a,b); outer.assign(a);
   }
};

template <typename InnerOperation, typename OuterOperation>
struct composed21<InnerOperation, OuterOperation, true>
   : public composed21<InnerOperation, OuterOperation, false> {
   typedef composed21<InnerOperation, OuterOperation, false> _super;
public:
   composed21(const InnerOperation& inner_arg=InnerOperation(), const OuterOperation& outer_arg=OuterOperation())
      : _super(inner_arg,outer_arg) {}
   composed21(const OuterOperation& outer_arg)
      : _super(outer_arg) {}

   template <typename Iterator2>
   typename _super::result_type
   operator() (partial_left, typename function_argument<typename _super::first_argument_type>::type a,
               const Iterator2& it2) const
   {
      return outer(inner(partial_left(), a, it2));
   }
   template <typename Iterator1>
   typename _super::result_type
   operator() (partial_right, const Iterator1& it1,
               typename function_argument<typename _super::second_argument_type>::type b) const
   {
      return outer(inner(partial_right(), it1, b));
   }
   using _super::operator();
};

template <typename InnerOperation2, typename OuterOperation>
struct composed12_is_partially_defined : is_partially_defined<OuterOperation> {};

template <typename OuterOperation>
struct composed12_is_partially_defined<OuterOperation, void> : is_partially_defined<OuterOperation> {};

// OuterBinary(InnerUnary1(x), InnerUnary2(y))
template <typename InnerOperation1, typename InnerOperation2, typename OuterOperation,
          bool is_partial=composed12_is_partially_defined<InnerOperation2,OuterOperation>::value>
struct composed12 {
   InnerOperation1 inner1;
   InnerOperation2 inner2;
   OuterOperation outer;

   typedef typename InnerOperation1::argument_type first_argument_type;
   typedef typename InnerOperation2::argument_type second_argument_type;
   typedef typename OuterOperation::result_type result_type;

   composed12(const InnerOperation1& inner_arg1=InnerOperation1(),
              const InnerOperation2& inner_arg2=InnerOperation2(),
              const OuterOperation& outer_arg=OuterOperation())
      : inner1(inner_arg1), inner2(inner_arg2), outer(outer_arg) {}
   composed12(const InnerOperation1& inner_arg1, const OuterOperation& outer_arg)
      : inner1(inner_arg1), outer(outer_arg) {}
   composed12(const InnerOperation2& inner_arg2, const OuterOperation& outer_arg)
      : inner2(inner_arg2), outer(outer_arg) {}
   composed12(const OuterOperation& outer_arg)
      : outer(outer_arg) {}

   result_type operator() (typename function_argument<first_argument_type>::type a,
                           typename function_argument<second_argument_type>::type b) const
   {
      return outer(inner1(a), inner2(b));
   }

   void assign(typename lvalue_arg<first_argument_type>::type a,
               typename function_argument<second_argument_type>::type b) const
   {
      inner1.assign(a); outer.assign(a, inner2(b));
   }
};

template <typename InnerOperation1, typename InnerOperation2, typename OuterOperation>
struct composed12<InnerOperation1, InnerOperation2, OuterOperation, true>
   : public composed12<InnerOperation1, InnerOperation2, OuterOperation, false> {
   typedef composed12<InnerOperation1, InnerOperation2, OuterOperation, false> _super;
public:
   composed12(const InnerOperation1& inner_arg1=InnerOperation1(),
              const InnerOperation2& inner_arg2=InnerOperation2(),
              const OuterOperation& outer_arg=OuterOperation())
      : _super(inner_arg1,inner_arg2,outer_arg) {}
   composed12(const InnerOperation1& inner_arg1, const OuterOperation& outer_arg)
      : _super(inner_arg1,outer_arg) {}
   composed12(const InnerOperation2& inner_arg2, const OuterOperation& outer_arg)
      : _super(inner_arg2,outer_arg) {}
   composed12(const OuterOperation& outer_arg)
      : _super(outer_arg) {}

   template <typename Iterator2>
   typename _super::result_type
   operator() (partial_left, typename function_argument<typename _super::first_argument_type>::type a,
               const Iterator2& it2) const
   {
      return outer(partial_left(), inner1(a), it2);
   }
   template <typename Iterator1>
   typename _super::result_type
   operator() (partial_right, const Iterator1& it1,
               typename function_argument<typename _super::second_argument_type>::type b) const
   {
      return outer(partial_right(), it1, inner2(b));
   }
   using _super::operator();
};

// OuterBinary(InnerUnary(x), y)
template <typename InnerOperation1, typename OuterOperation>
struct composed12<InnerOperation1, void, OuterOperation, false> {
   InnerOperation1 inner1;
   OuterOperation outer;

   typedef typename InnerOperation1::argument_type first_argument_type;
   typedef typename OuterOperation::second_argument_type second_argument_type;
   typedef typename OuterOperation::result_type result_type;

   composed12(const InnerOperation1& inner_arg1=InnerOperation1(),
              const OuterOperation& outer_arg=OuterOperation())
      : inner1(inner_arg1), outer(outer_arg) {}
   composed12(const OuterOperation& outer_arg)
      : outer(outer_arg) {}

   result_type operator() (typename function_argument<first_argument_type>::type a,
                           typename function_argument<second_argument_type>::type b) const
   {
      return outer(inner1(a), b);
   }

   void assign(typename lvalue_arg<first_argument_type>::type a,
               typename function_argument<second_argument_type>::type b) const
   {
      inner1.assign(a); outer.assign(a, b);
   }
};

template <typename InnerOperation1, typename OuterOperation>
struct composed12<InnerOperation1, void, OuterOperation, true>
   : public composed12<InnerOperation1, void, OuterOperation, false> {
   typedef composed12<InnerOperation1, void, OuterOperation, false> _super;
public:
   composed12(const InnerOperation1& inner_arg1=InnerOperation1(),
              const OuterOperation& outer_arg=OuterOperation())
      : _super(inner_arg1,outer_arg) {}
   composed12(const OuterOperation& outer_arg)
      : _super(outer_arg) {}

   template <typename Iterator2>
   typename _super::result_type
   operator() (partial_left, typename function_argument<typename _super::first_argument_type>::type a,
               const Iterator2& it2) const
   {
      return outer(partial_left(), inner1(a), it2);
   }
   template <typename Iterator1>
   typename _super::result_type
   operator() (partial_right, const Iterator1& it1,
               typename function_argument<typename _super::second_argument_type>::type b) const
   {
      return outer(partial_right(), it1, b);
   }
   using _super::operator();
};

// OuterBinary(x, InnerUnary(y))
template <typename InnerOperation2, typename OuterOperation>
struct composed12<void, InnerOperation2, OuterOperation, false> {
   InnerOperation2 inner2;
   OuterOperation outer;

   typedef typename OuterOperation::first_argument_type first_argument_type;
   typedef typename InnerOperation2::argument_type second_argument_type;
   typedef typename OuterOperation::result_type result_type;

   composed12(const InnerOperation2& inner_arg2=InnerOperation2(),
              const OuterOperation& outer_arg=OuterOperation())
      : inner2(inner_arg2), outer(outer_arg) {}
   composed12(const OuterOperation& outer_arg)
      : outer(outer_arg) {}

   result_type operator() (typename function_argument<first_argument_type>::type a,
                           typename function_argument<second_argument_type>::type b) const
   {
      return outer(a, inner2(b));
   }

   void assign(typename lvalue_arg<first_argument_type>::type a,
               typename function_argument<second_argument_type>::type b) const
   {
      outer.assign(a, inner2(b));
   }
};

template <typename InnerOperation2, typename OuterOperation>
struct composed12<void, InnerOperation2, OuterOperation, true>
   : public composed12<void, InnerOperation2, OuterOperation, false> {
   typedef composed12<void, InnerOperation2, OuterOperation, false> _super;
public:
   composed12(const InnerOperation2& inner_arg2=InnerOperation2(),
              const OuterOperation& outer_arg=OuterOperation())
      : _super(inner_arg2,outer_arg) {}
   composed12(const OuterOperation& outer_arg)
      : _super(outer_arg) {}

   template <typename Iterator2>
   typename _super::result_type
   operator() (partial_left, typename function_argument<typename _super::first_argument_type>::type a,
               const Iterator2& it2) const
   {
      return outer(partial_left(), a, it2);
   }

   template <typename Iterator1>
   typename _super::result_type
   operator() (partial_right, const Iterator1& it1,
               typename function_argument<typename _super::second_argument_type>::type b) const
   {
      return outer(partial_right(), it1, inner2(b));
   }
   using _super::operator();
};

// OuterBinary(InnerUnary(x), InnerUnary(y))
template <typename InnerOperation, typename OuterOperation>
struct composed12<InnerOperation, OuterOperation, void, false> {
   InnerOperation inner;
   OuterOperation outer;

   typedef typename InnerOperation::argument_type first_argument_type;
   typedef typename InnerOperation::argument_type second_argument_type;
   typedef typename OuterOperation::result_type result_type;

   composed12(const InnerOperation& inner_arg=InnerOperation(),
              const OuterOperation& outer_arg=OuterOperation())
      : inner(inner_arg), outer(outer_arg) {}
   composed12(const OuterOperation& outer_arg)
      : outer(outer_arg) {}

   result_type operator() (typename function_argument<first_argument_type>::type a,
                           typename function_argument<second_argument_type>::type b) const
   {
      return outer(inner(a), inner(b));
   }

   void assign(typename lvalue_arg<first_argument_type>::type a,
               typename function_argument<second_argument_type>::type b) const
   {
      inner.assign(a); outer.assign(a, inner(b));
   }
};

template <typename InnerOperation, typename OuterOperation>
struct composed12<InnerOperation, OuterOperation, void, true>
   : public composed12<InnerOperation, OuterOperation, void, false> {
   typedef composed12<InnerOperation, OuterOperation, void, false> _super;
public:
   composed12(const InnerOperation& inner_arg=InnerOperation(),
              const OuterOperation& outer_arg=OuterOperation())
      : _super(inner_arg,outer_arg) {}
   composed12(const OuterOperation& outer_arg)
      : _super(outer_arg) {}

   template <typename Iterator2>
   typename _super::result_type
   operator() (partial_left, typename function_argument<typename _super::first_argument_type>::type a,
               const Iterator2& it2) const
   {
      return outer(partial_left(), inner(a), it2);
   }

   template <typename Iterator1>
   typename _super::result_type
   operator() (partial_right, const Iterator1& it1,
               typename function_argument<typename _super::second_argument_type>::type b) const
   {
      return outer(partial_right(), it1, inner(b));
   }
   using _super::operator();
};

template <typename UnaryOperation, typename Right=void>
struct apply1 : UnaryOperation {
   typedef typename UnaryOperation::argument_type first_argument_type;
   typedef Right second_argument_type;

   apply1(const UnaryOperation& op_arg=UnaryOperation()) : UnaryOperation(op_arg) {}

   typename UnaryOperation::result_type
   operator() (typename function_argument<first_argument_type>::type x, typename attrib<Right>::plus_const_ref) const
   {
      return UnaryOperation::operator()(x);
   }
};

template <typename UnaryOperation>
struct apply1<UnaryOperation,void> : incomplete {};

template <typename UnaryOperation, typename Left=void>
struct apply2 : UnaryOperation {
   typedef Left first_argument_type;
   typedef typename UnaryOperation::argument_type second_argument_type;

   apply2(const UnaryOperation& op_arg=UnaryOperation()) : UnaryOperation(op_arg) {}

   typename UnaryOperation::result_type
   operator() (typename attrib<Left>::plus_const_ref, typename function_argument<second_argument_type>::type x) const
   {
      return UnaryOperation::operator()(x);
   }
};

template <typename UnaryOperation>
struct apply2<UnaryOperation,void> : incomplete {};

template <typename Iterator>
class random_access {
protected:
   Iterator start;
public:
   typedef typename iterator_traits<Iterator>::difference_type argument_type;
   typedef typename iterator_traits<Iterator>::reference result_type;

   random_access(const Iterator& start_arg=Iterator())
      : start(start_arg) {}

   result_type operator() (argument_type i) const
   {
      return start[i];
   }
};

template <typename Operation, typename Iterator>
class unary_indirect : public Operation {
protected:
   Iterator start;
public:
   typedef typename iterator_traits<Iterator>::difference_type argument_type;

   unary_indirect(const Iterator& start_arg=Iterator())
      : start(start_arg) {}

   typename Operation::result_type operator() (argument_type i) const
   {
      return Operation::operator()(start[i]);
   }
};

template <typename Operation, typename Iterator1, typename Iterator2=Iterator1>
class binary_indirect : public Operation {
protected:
   Iterator1 start1;
   Iterator2 start2;
public:
   typedef typename iterator_traits<Iterator1>::difference_type first_argument_type;
   typedef typename iterator_traits<Iterator2>::difference_type second_argument_type;

   binary_indirect(const Iterator1& start1_arg)
      : start1(start1_arg), start2(start1_arg) {}
   binary_indirect(const Iterator1& start1_arg, const Iterator2& start2_arg)
      : start1(start1_arg), start2(start2_arg) {}

   typename Operation::result_type operator() (first_argument_type i1, second_argument_type i2) const
   {
      return Operation::operator()(start1[i1], start2[i2]);
   }
   // for equal_range & Co.
   typename Operation::result_type operator() (first_argument_type i1,
                                               typename function_argument<typename Operation::second_argument_type>::type x2) const
   {
      return Operation::operator()(start1[i1], x2);
   }
   typename Operation::result_type operator() (typename function_argument<typename Operation::first_argument_type>::type x1,
                                               second_argument_type i2) const
   {
      return Operation::operator()(x1, start2[i2]);
   }
};

template <typename Class, typename Member, Member Class::*Ptr, typename ObjRef=void>
class member {
public:
   typedef ObjRef argument_type;
   typedef typename inherit_ref<Member,ObjRef>::type result_type;
   result_type operator() (argument_type obj) const { return obj.*Ptr; }
};

template <typename Class, typename Member, Member Class::*Ptr>
class member<Class, Member, Ptr, void> : incomplete {};

template <typename Cref, typename M>
class var_member {
public:
   typedef Cref argument_type;
   typedef typename deref<Cref>::type C;
   typedef typename inherit_ref<M,Cref>::type result_type;

   var_member(M C::* ptr_arg) : ptr(ptr_arg) {}

   result_type operator() (typename function_argument<Cref>::type c) const { return c.*ptr; }
protected:
   M C::* ptr;
};

template <typename Left, typename Right>
struct swap_op {
   typedef Left first_argument_type;
   typedef Right second_argument_type;
   typedef const swap_op& result_type;

   result_type operator() (typename function_argument<Left>::type a, typename function_argument<Right>::type b) const
   {
      std::swap(a,b);
      return *this;
   }
};

template <typename TRef>
struct move {
   typedef TRef argument_type;
   typedef std::add_lvalue_reference_t<pure_type_t<TRef>> unconst_type;
   typedef std::add_rvalue_reference_t<pure_type_t<TRef>> result_type;

   result_type operator() (argument_type x) const { return static_cast<result_type>(const_cast<unconst_type>(x)); }
};

}

template <typename Iterator>
struct operation_cross_const_helper< operations::random_access<Iterator> > {
   typedef operations::random_access<typename iterator_traits<Iterator>::iterator> operation;
   typedef operations::random_access<typename iterator_traits<Iterator>::const_iterator> const_operation;
};

// Automatic construction of composed operations

template <typename InnerOperation, typename OuterOperation, typename Iterator, typename Reference>
struct unary_op_builder< operations::composed11<InnerOperation, OuterOperation>,
                         Iterator, Reference> {
   typedef operations::composed11<InnerOperation, OuterOperation> Operation;
   typedef unary_op_builder<InnerOperation, Iterator, Reference> inner_builder;
   typedef typename inner_builder::operation inner_operation;
   typedef typename inner_operation::result_type inner_result;
   typedef unary_op_builder<OuterOperation, typename deref<inner_result>::type, inner_result> outer_builder;
   typedef typename outer_builder::operation outer_operation;
   typedef operations::composed11<inner_operation, outer_operation> operation;

   static const operation& create(const operation& op) { return op; }

   template <typename AltInnerOperation, typename AltOuterOperation>
   static operation create(const operations::composed11<AltInnerOperation, AltOuterOperation>& op)
   {
      return operation(inner_builder::create(op.inner), outer_builder::create(op.outer));
   }
};

template <typename InnerOperation, typename OuterOperation, bool is_partial,
          typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder< operations::composed21<InnerOperation, OuterOperation, is_partial>,
                          Iterator1, Iterator2, Reference1, Reference2> {
   typedef operations::composed21<InnerOperation, OuterOperation, is_partial> Operation;
   typedef binary_op_builder<InnerOperation, Iterator1, Iterator2, Reference1, Reference2> inner_builder;
   typedef typename inner_builder::operation inner_operation;
   typedef typename inner_operation::result_type inner_result;
   typedef unary_op_builder<OuterOperation, typename deref<inner_result>::type, inner_result> outer_builder;
   typedef typename outer_builder::operation outer_operation;
   typedef operations::composed21<inner_operation, outer_operation> operation;

   static const operation& create(const operation& op) { return op; }

   template <typename AltInnerOperation, typename AltOuterOperation>
   static operation create(const operations::composed21<AltInnerOperation, AltOuterOperation, is_partial>& op)
   {
      return operation(inner_builder::create(op.inner), outer_builder::create(op.outer));
   }
};
                         
template <typename InnerOperation1, typename InnerOperation2, typename OuterOperation, bool is_partial,
          typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder< operations::composed12<InnerOperation1, InnerOperation2, OuterOperation, is_partial>,
                          Iterator1, Iterator2, Reference1, Reference2> {
   typedef operations::composed12<InnerOperation1, InnerOperation2, OuterOperation, is_partial> Operation;
   typedef unary_op_builder<InnerOperation1, Iterator1, Reference1> inner_builder1;
   typedef unary_op_builder<InnerOperation2, Iterator2, Reference2> inner_builder2;
   typedef typename inner_builder1::operation inner_operation1;
   typedef typename inner_builder2::operation inner_operation2;
   typedef typename inner_operation1::result_type inner_result1;
   typedef typename inner_operation2::result_type inner_result2;
   typedef binary_op_builder<OuterOperation, typename deref<inner_result1>::type, typename deref<inner_result2>::type,
                             inner_result1, inner_result2>
      outer_builder;
   typedef typename outer_builder::operation outer_operation;
   typedef operations::composed12<inner_operation1, inner_operation2, outer_operation> operation;

   static const operation& create(const operation& op) { return op; }

   template <typename AltInnerOperation1, typename AltInnerOperation2, typename AltOuterOperation>
   static operation create(const operations::composed12<AltInnerOperation1, AltInnerOperation2, AltOuterOperation, is_partial>& op)
   {
      return operation(inner_builder1::create(op.inner1), inner_builder2::create(op.inner2), outer_builder::create(op.outer));
   }
};

template <typename InnerOperation1, typename OuterOperation, bool is_partial,
          typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder< operations::composed12<InnerOperation1, void, OuterOperation, is_partial>,
                          Iterator1, Iterator2, Reference1, Reference2> {
   typedef operations::composed12<InnerOperation1, void, OuterOperation, is_partial> Operation;
   typedef unary_op_builder<InnerOperation1, Iterator1, Reference1> inner_builder1;
   typedef typename inner_builder1::operation inner_operation1;
   typedef typename inner_operation1::result_type inner_result1;
   typedef binary_op_builder<OuterOperation, typename deref<inner_result1>::type, Iterator2,
                             inner_result1, Reference2>
      outer_builder;
   typedef typename outer_builder::operation outer_operation;
   typedef operations::composed12<inner_operation1, void, outer_operation> operation;

   static const operation& create(const operation& op) { return op; }

   template <typename AltInnerOperation1, typename AltOuterOperation>
   static operation create(const operations::composed12<AltInnerOperation1, void, AltOuterOperation, is_partial>& op)
   {
      return operation(inner_builder1::create(op.inner1), outer_builder::create(op.outer));
   }
};
                         
template <typename InnerOperation2, typename OuterOperation, bool is_partial,
          typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder< operations::composed12<void, InnerOperation2, OuterOperation, is_partial>,
                          Iterator1, Iterator2, Reference1, Reference2> {
   typedef operations::composed12<void, InnerOperation2, OuterOperation, is_partial> Operation;
   typedef unary_op_builder<InnerOperation2, Iterator2, Reference2> inner_builder2;
   typedef typename inner_builder2::operation inner_operation2;
   typedef typename inner_operation2::result_type inner_result2;
   typedef binary_op_builder<OuterOperation, Iterator1, typename deref<inner_result2>::type,
                             Reference1, inner_result2>
      outer_builder;
   typedef typename outer_builder::operation outer_operation;
   typedef operations::composed12<void, inner_operation2, outer_operation> operation;

   static const operation& create(const operation& op) { return op; }

   template <typename AltInnerOperation2, typename AltOuterOperation>
   static operation create(const operations::composed12<void, AltInnerOperation2, AltOuterOperation, is_partial>& op)
   {
      return operation(inner_builder2::create(op.inner2), outer_builder::create(op.outer));
   }
};

template <typename InnerOperation, typename OuterOperation, bool is_partial,
          typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder< operations::composed12<InnerOperation, OuterOperation, void, is_partial>,
                          Iterator1, Iterator2, Reference1, Reference2> {
   typedef operations::composed12<InnerOperation, OuterOperation, void, is_partial> Operation;
   typedef unary_op_builder<InnerOperation, Iterator1, Reference1> inner_builder1;
   typedef unary_op_builder<InnerOperation, Iterator2, Reference2> inner_builder2;
   typedef typename inner_builder1::operation inner_operation1;
   typedef typename inner_builder2::operation inner_operation2;
   typedef typename inner_operation1::result_type inner_result1;
   typedef typename inner_operation2::result_type inner_result2;
   typedef binary_op_builder<OuterOperation, typename deref<inner_result1>::type, typename deref<inner_result2>::type,
                             inner_result1, inner_result2>
      outer_builder;
   typedef typename outer_builder::operation outer_operation;

   static const bool ident12=std::is_same<inner_operation1, inner_operation2>::value;
   typedef typename std::conditional< ident12,
                                      operations::composed12<inner_operation1, outer_operation, void>,
                                      operations::composed12<inner_operation1, inner_operation2, outer_operation> >::type
      operation;

   static const operation& create(const operation& op) { return op; }

   template <typename AltInnerOperation, typename AltOuterOperation>
   static operation create_impl(const operations::composed12<AltInnerOperation, AltOuterOperation, void, is_partial>& op, std::false_type)
   {
      return operation(inner_builder1::create(op.inner), inner_builder2::create(op.inner), outer_builder::create(op.outer));
   }

   template <typename AltInnerOperation, typename AltOuterOperation>
   static operation create_impl(const operations::composed12<AltInnerOperation, AltOuterOperation, void, is_partial>& op, std::true_type)
   {
      return operation(inner_builder1::create(op.inner), outer_builder::create(op.outer));
   }

   template <typename AltInnerOperation, typename AltOuterOperation>
   static operation create(const operations::composed12<AltInnerOperation, AltOuterOperation, void, is_partial>& op)
   {
      return create_impl(op, bool_constant<ident12>());
   }
};

template <typename Cref, typename InnerOperation, typename Iterator, typename Reference>
struct unary_op_builder< operations::fix1<Cref, InnerOperation>, Iterator, Reference> {
   typedef operations::fix1<Cref, InnerOperation> Operation;
   typedef binary_op_builder<InnerOperation, void, Iterator, typename attrib<typename Operation::stored_type>::plus_const_ref, Reference> inner_builder;
   typedef typename inner_builder::operation inner_operation;
   typedef operations::fix1<Cref, inner_operation> operation;

   static const operation& create(const operation& op) { return op; }

   template <typename AltInnerOperation>
   static operation create(const operations::fix1<Cref, AltInnerOperation>& op)
   {
      return operation(op.c, inner_builder::create(op));
   }
};

template <typename Cref, typename InnerOperation, typename Iterator, typename Reference>
struct unary_op_builder< operations::fix2<Cref, InnerOperation>, Iterator, Reference> {
   typedef operations::fix2<Cref, InnerOperation> Operation;
   typedef binary_op_builder<InnerOperation, Iterator, void, Reference, typename attrib<typename Operation::stored_type>::plus_const_ref> inner_builder;
   typedef typename inner_builder::operation inner_operation;
   typedef operations::fix2<Cref, inner_operation> operation;

   static const operation& create(const operation& op) { return op; }

   template <typename AltInnerOperation>
   static operation create(const operations::fix2<Cref, AltInnerOperation>& op)
   {
      return operation(op.c, inner_builder::create(op));
   }
};

template <typename Class, typename Member, Member Class::* Ptr, typename Iterator, typename Reference>
struct unary_op_builder< operations::member<Class,Member,Ptr,void>, Iterator, Reference>
   : empty_op_builder< operations::member<Class,Member,Ptr,Reference> > {};

template <typename UnaryOperation, typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder<operations::apply1<UnaryOperation>, Iterator1, Iterator2, Reference1, Reference2>
   : empty_op_builder< operations::apply1<typename unary_op_builder<UnaryOperation,Iterator1,Reference1>::operation, Iterator2> > {};

template <typename UnaryOperation, typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder<operations::apply2<UnaryOperation>, Iterator1, Iterator2, Reference1, Reference2>
   : empty_op_builder< operations::apply2<typename unary_op_builder<UnaryOperation,Iterator2,Reference2>::operation, Iterator1> > {};

template <template <typename> class Operation, typename Iterator> inline
operations::unary_indirect< Operation<typename iterator_traits<Iterator>::reference>, Iterator >
construct_indirect_operation(const Iterator& src)
{
   return src;
}

template <typename Iterator, template <typename> class Operation> inline
operations::unary_indirect< Operation<typename iterator_traits<Iterator>::reference>, Iterator >
construct_indirect_operation(const Iterator& src, const BuildUnary<Operation>&)
{
   return src;
}

template <template <typename,typename> class Operation, typename Iterator> inline
operations::binary_indirect< Operation<typename iterator_traits<Iterator>::reference,
                                       typename iterator_traits<Iterator>::reference>, Iterator >
construct_indirect_operation(const Iterator& src)
{
   return src;
}

template <typename Iterator, template <typename,typename> class Operation> inline
operations::binary_indirect< Operation<typename iterator_traits<Iterator>::reference,
                                       typename iterator_traits<Iterator>::reference>, Iterator >
construct_indirect_operation(const Iterator& src, const BuildBinary<Operation>&)
{
   return src;
}

template <template <typename,typename> class Operation, typename Iterator1, typename Iterator2> inline
operations::binary_indirect< Operation<typename iterator_traits<Iterator1>::reference,
                                       typename iterator_traits<Iterator2>::reference>, Iterator1, Iterator2 >
construct_indirect_operation(const Iterator1& src1, const Iterator2& src2)
{
   return operations::binary_indirect< Operation<typename iterator_traits<Iterator1>::reference,
                                                 typename iterator_traits<Iterator2>::reference>, Iterator1, Iterator2 >
                                     (src1,src2);
}

template <typename Iterator1, typename Iterator2, template <typename,typename> class Operation>
operations::binary_indirect< Operation<typename iterator_traits<Iterator1>::reference,
                                       typename iterator_traits<Iterator2>::reference>, Iterator1, Iterator2 >
construct_indirect_operation(const Iterator1& src1, const Iterator2& src2, const BuildBinary<Operation>&)
{
   return operations::binary_indirect< Operation<typename iterator_traits<Iterator1>::reference,
                                                 typename iterator_traits<Iterator2>::reference>, Iterator1, Iterator2 >
                                     (src1,src2);
}

template <typename Iterator, typename Operation>
void perform_assign(Iterator&& dst, const Operation& op_arg)
{
   typedef unary_op_builder<Operation, pure_type_t<Iterator>> opb;
   const typename opb::operation& op=opb::create(op_arg);
   for (; !dst.at_end(); ++dst)
      op.assign(*dst);
}

template <typename Iterator1, typename Iterator2, typename Operation>
void perform_assign(Iterator1&& dst, Iterator2&& src2, const Operation& op_arg,
                    std::enable_if_t<check_iterator_feature<pure_type_t<Iterator1>, end_sensitive>::value, void**> =nullptr)
{
   typedef binary_op_builder<Operation, pure_type_t<Iterator1>, pure_type_t<Iterator2>> opb;
   const typename opb::operation& op=opb::create(op_arg);
   for (; !dst.at_end(); ++dst, ++src2)
      op.assign(*dst, *src2);
}

template <typename Iterator1, typename Iterator2, typename Operation>
void perform_assign(Iterator1&& dst, Iterator2&& src2, const Operation& op_arg,
                    std::enable_if_t<!check_iterator_feature<pure_type_t<Iterator1>, end_sensitive>::value &&
                                     check_iterator_feature<pure_type_t<Iterator2>, end_sensitive>::value, void**> =nullptr)
{
   typedef binary_op_builder<Operation, pure_type_t<Iterator1>, pure_type_t<Iterator2>> opb;
   const typename opb::operation& op=opb::create(op_arg);
   for (; !src2.at_end(); ++dst, ++src2)
      op.assign(*dst, *src2);
}

template <typename Iterator, typename Operation, typename Object,
          typename=std::enable_if_t<!is_effectively_const<Object>::value>>
void accumulate_in(Iterator&& src, const Operation& op_arg, Object&& x)
{
   typedef binary_op_builder<Operation, const pure_type_t<Object>*, pure_type_t<Iterator>> opb;
   const typename opb::operation& op=opb::create(op_arg);
   for (; !src.at_end(); ++src) op.assign(x, *src);
}

template <typename Container, typename Operation>
typename object_traits<typename Container::value_type>::persistent_type
accumulate(const Container& c, const Operation& op_arg)
{
   typedef typename object_traits<typename Container::value_type>::persistent_type Object;
   if (c.empty()) return Object();
   auto src=entire_range(c);
   Object x=*src;
   accumulate_in(++src, op_arg, x);
   return x;
}

template <typename Container>
typename object_traits<typename Container::value_type>::persistent_type
average(const Container& c)
{
   return accumulate(c, BuildBinary<operations::add>()) / c.size();
}

namespace operations {

template <template <typename> class Result, typename ArgRef=void>
struct construct_unary {
   typedef ArgRef argument_type;
   typedef Result<ArgRef> result_type;

   template <typename X>
   result_type operator() (X&& x) const
   {
      return result_type(std::forward<X>(x));
   }
};

template <template <typename> class Result>
struct construct_unary<Result, void> : incomplete {};

template <template <typename,typename> class Result, typename Second, typename ArgRef=void>
struct construct_unary2 {
   typedef ArgRef argument_type;
   typedef Result<ArgRef, Second> result_type;

   template <typename X>
   result_type operator() (X&& x) const
   {
      return result_type(std::forward<X>(x));
   }
};

template <template <typename,typename> class Result, typename Second>
struct construct_unary2<Result, Second, void> : incomplete {};

template <template <typename,typename,typename> class Result, typename Second, typename Third, typename ArgRef=void>
struct construct_unary3 {
   typedef ArgRef argument_type;
   typedef Result<ArgRef, Second, Third> result_type;

   template <typename X>
   result_type operator() (X&& x) const
   {
      return result_type(std::forward<X>(x));
   }
};

template <template <typename,typename,typename> class Result, typename Second, typename Third>
struct construct_unary3<Result, Second, Third, void> : incomplete {};

template <template <typename> class Result, typename Second, typename ArgRef=void>
struct construct_unary_with_arg {
protected:
   Second second;
public:
   typedef ArgRef argument_type;
   typedef Result<ArgRef> result_type;

   construct_unary_with_arg() {}
   construct_unary_with_arg(const Second& second_arg) : second(second_arg) {}

   template <typename X>
   result_type operator() (X&& x) const
   {
      return result_type(std::forward<X>(x), second);
   }
};

template <template <typename> class Result, typename Second>
struct construct_unary_with_arg<Result, Second, void> : incomplete {
protected:
   Second second;
public:
   construct_unary_with_arg() {}
   construct_unary_with_arg(const Second& second_arg) : second(second_arg) {}

   operator const Second& () const { return second; }
};

template <template <typename,typename> class Result, typename Second, typename ArgRef=void>
struct construct_unary2_with_arg {
protected:
   Second second;
public:
   typedef ArgRef argument_type;
   typedef Result<ArgRef, Second> result_type;

   construct_unary2_with_arg() {}
   construct_unary2_with_arg(const Second& second_arg) : second(second_arg) {}

   template <typename X>
   result_type operator() (X&& x) const
   {
      return result_type(std::forward<X>(x), second);
   }
};

template <template <typename,typename> class Result, typename Second>
struct construct_unary2_with_arg<Result, Second, void> : incomplete {
protected:
   Second second;
public:
   construct_unary2_with_arg() {}
   construct_unary2_with_arg(const Second& second_arg) : second(second_arg) {}

   operator const Second& () const { return second; }
};

template <template <typename,typename> class Result, typename LeftRef=void, typename RightRef=void>
struct construct_binary {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef Result<LeftRef, RightRef> result_type;

   template <typename L, typename R>
   result_type operator() (L&& l, R&& r) const
   {
      return result_type(std::forward<L>(l), std::forward<R>(r));
   }
};

template <template <typename,typename> class Result>
struct construct_binary<Result, void, void> : incomplete {};

template <template <typename,typename,typename> class Result, typename Third, typename LeftRef=void, typename RightRef=void>
struct construct_binary2 {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef Result<LeftRef, RightRef, Third> result_type;

   template <typename L, typename R>
   result_type operator() (L&& l, R&& r) const
   {
      return result_type(std::forward<L>(l), std::forward<R>(r));
   }
};

template <template <typename,typename,typename> class Result, typename Third>
struct construct_binary2<Result, Third, void, void> : incomplete {};

template <template <typename,typename> class Result, typename Third, typename LeftRef=void, typename RightRef=void>
struct construct_binary_with_arg {
protected:
   Third third;
public:
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef Result<LeftRef, RightRef> result_type;

   construct_binary_with_arg() {}
   construct_binary_with_arg(const Third& third_arg) : third(third_arg) {}

   template <typename L, typename R>
   result_type operator() (L&& l, R&& r) const
   {
      return result_type(std::forward<L>(l), std::forward<R>(r), third);
   }
};

template <template <typename,typename> class Result, typename Third>
struct construct_binary_with_arg<Result, Third, void, void> : incomplete {
protected:
   Third third;
public:
   construct_binary_with_arg() {}
   construct_binary_with_arg(const Third& third_arg) : third(third_arg) {}

   operator const Third& () const { return third; }
};

template <template <typename,typename,typename> class Result, typename Third, typename LeftRef=void, typename RightRef=void>
struct construct_binary2_with_arg {
protected:
   Third third;
public:
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef Result<LeftRef, RightRef, Third> result_type;

   construct_binary2_with_arg() {}
   construct_binary2_with_arg(const Third& third_arg) : third(third_arg) {}

   template <typename L, typename R>
   result_type operator() (L&& l, R&& r) const
   {
      return result_type(std::forward<L>(l), std::forward<R>(r), third);
   }
};

template <template <typename,typename,typename> class Result, typename Third>
struct construct_binary2_with_arg<Result, Third, void, void> : incomplete {
protected:
   Third third;
public:
   construct_binary2_with_arg() {}
   construct_binary2_with_arg(const Third& third_arg) : third(third_arg) {}

   operator const Third& () const { return third; }
};

template <typename IteratorRef>
struct index2element {
   typedef IteratorRef argument_type;
   typedef const int result_type;
   result_type operator() (argument_type it) const { return it.index(); }
};

template <typename IteratorRef>
struct dereference {
   typedef IteratorRef argument_type;
   typedef typename iterator_traits<typename deref<IteratorRef>::type>::reference ref;
   typedef typename std::conditional<attrib<IteratorRef>::is_const, typename attrib<ref>::plus_const, ref>::type result_type;
   result_type operator() (IteratorRef it) const { return *it; }
};

template <typename Ref>
struct identity {
   typedef Ref argument_type;
   typedef Ref result_type;
   Ref operator() (Ref x) const { return x; }
   void assign(typename lvalue_arg<Ref>::type) const {}
};

template <typename Ref>
struct ref2pointer {
   typedef Ref argument_type;
   typedef typename deref<Ref>::minus_ref* result_type;
   result_type operator() (Ref x) const { return &x; }
};

template <typename OrigRef, typename ApparentRef, bool _need_cache=!attrib<OrigRef>::is_reference>
struct reinterpret_impl {
   typedef OrigRef argument_type;
   typedef ApparentRef result_type;
   result_type operator() (OrigRef x) const
   {
      return reinterpret_cast<result_type>(x);
   }
};

template <typename Orig, typename ApparentRef>
struct reinterpret_impl<Orig, ApparentRef, true> {
   typedef Orig argument_type;
   typedef typename attrib<ApparentRef>::plus_ref result_type;
private:
   mutable op_value_cache<typename deref<Orig>::type> cache;
public:
   result_type operator() (argument_type x) const
   {
      cache=x;
      return reinterpret_cast<result_type>(cache.get());
   }
};

template <typename IteratorRef1, typename IteratorRef2>
struct dereference2 {
   typedef IteratorRef1 first_argument_type;
   typedef IteratorRef2 second_argument_type;
   typedef typename iterator_traits<typename deref<IteratorRef2>::type>::reference result_type;

   result_type operator() (IteratorRef1, IteratorRef2 it2) const { return *it2; }
};

template <typename Apparent> struct reinterpret : incomplete {};
template <template <typename> class Masquerade> struct masquerade : incomplete {};
template <template <typename,typename> class Masquerade, typename Second> struct masquerade2 : incomplete {};
template <template <typename,typename,typename> class Masquerade, typename Second, typename Third> struct masquerade3 : incomplete {};

} // end namespace operations

template <template <typename> class Result, typename Iterator, typename Reference>
struct unary_op_builder< operations::construct_unary<Result>, Iterator, Reference>
   : empty_op_builder< operations::construct_unary<Result,Reference> > {};

template <template <typename,typename> class Result, typename Second, typename Iterator, typename Reference>
struct unary_op_builder< operations::construct_unary2<Result,Second>, Iterator, Reference>
   : empty_op_builder< operations::construct_unary2<Result,Second,Reference> > {};

template <template <typename,typename,typename> class Result, typename Second, typename Third, typename Iterator, typename Reference>
struct unary_op_builder< operations::construct_unary3<Result,Second,Third>, Iterator, Reference>
   : empty_op_builder< operations::construct_unary3<Result,Second,Third,Reference> > {};

template <template <typename> class Result, typename Second, typename Iterator, typename Reference>
struct unary_op_builder< operations::construct_unary_with_arg<Result,Second>, Iterator, Reference> {
   typedef operations::construct_unary_with_arg<Result,Second,Reference> operation;

   static operation create(const Second& arg) { return operation(arg); }
};

template <template <typename,typename> class Result, typename Second, typename Iterator, typename Reference>
struct unary_op_builder< operations::construct_unary2_with_arg<Result,Second>, Iterator, Reference> {
   typedef operations::construct_unary2_with_arg<Result,Second,Reference> operation;

   static operation create(const Second& arg) { return operation(arg); }
};

template <template <typename,typename> class Result, typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder< operations::construct_binary<Result>, Iterator1, Iterator2, Reference1, Reference2>
   : empty_op_builder< operations::construct_binary<Result, Reference1, Reference2> > {};

template <template <typename,typename,typename> class Result, typename Third, typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder< operations::construct_binary2<Result,Third>, Iterator1, Iterator2, Reference1, Reference2>
   : empty_op_builder< operations::construct_binary2<Result, Third, Reference1, Reference2> > {};

template <template <typename,typename> class Result, typename Third, typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder< operations::construct_binary_with_arg<Result,Third>, Iterator1, Iterator2, Reference1, Reference2> {
   typedef operations::construct_binary_with_arg<Result, Third, Reference1, Reference2> operation;

   static operation create(const Third& arg) { return operation(arg); }
};

template <template <typename,typename,typename> class Result, typename Third, typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder< operations::construct_binary2_with_arg<Result,Third>, Iterator1, Iterator2, Reference1, Reference2> {
   typedef operations::construct_binary2_with_arg<Result, Third, Reference1, Reference2> operation;

   static operation create(const Third& arg) { return operation(arg); }
};

template <typename Apparent, typename Iterator, typename Reference>
struct unary_op_builder<operations::reinterpret<Apparent>, Iterator, Reference>
   : empty_op_builder< operations::reinterpret_impl<Reference, typename inherit_ref<Apparent,Reference>::type> > {};

template <template <typename> class Masquerade, typename Iterator, typename Reference>
struct unary_op_builder<operations::masquerade<Masquerade>, Iterator, Reference>
   : empty_op_builder< operations::reinterpret_impl<Reference, typename masquerade<Masquerade,Reference>::type> > {};

template <template <typename,typename> class Masquerade, typename Second, typename Iterator, typename Reference>
struct unary_op_builder<operations::masquerade2<Masquerade,Second>, Iterator, Reference>
   : empty_op_builder< operations::reinterpret_impl<Reference, typename masquerade2<Masquerade,Reference,Second>::type> > {};

template <template <typename,typename,typename> class Masquerade, typename Second, typename Third, typename Iterator, typename Reference>
struct unary_op_builder<operations::masquerade3<Masquerade,Second,Third>, Iterator, Reference>
  : empty_op_builder< operations::reinterpret_impl<Reference, typename masquerade3<Masquerade,Reference,Second,Third>::type> > {};

template <typename Iterator>
inline
Iterator&& enforce_movable_values(Iterator&& it,
                                  typename std::enable_if<!std::is_lvalue_reference<typename iterator_traits<Iterator>::reference>::value>::type** = nullptr)
{
   return std::forward<Iterator>(it);
}

template <typename Iterator>
inline
unary_transform_iterator<pointer2iterator_t<Iterator>, BuildUnary<operations::move>>
enforce_movable_values(Iterator&& it,
                       typename std::enable_if<std::is_lvalue_reference<typename iterator_traits<Iterator>::reference>::value>::type** = nullptr)
{
   return pointer2iterator(std::forward<Iterator>(it));
}

} // end namespace pm

namespace polymake {
   using pm::construct_indirect_operation;
   using pm::perform_assign;
   using pm::accumulate;
   using pm::accumulate_in;
   using pm::average;
   using pm::enforce_movable_values;

   namespace operations {
      typedef BuildUnary<pm::operations::neg> neg;
      typedef BuildBinary<pm::operations::add> add;
      typedef BuildBinary<pm::operations::sub> sub;
      typedef BuildBinary<pm::operations::mul> mul;
      typedef BuildBinary<pm::operations::div> div;
      typedef BuildBinary<pm::operations::divexact> divexact;
      typedef BuildUnary<pm::operations::square> square;
      typedef BuildBinary<pm::operations::tensor> tensor;

      typedef BuildBinary<pm::operations::bitwise_xor> bitwise_xor;

      typedef BuildUnary<pm::operations::clear> clear;

      using pm::operations::fix1;
      using pm::operations::fix2;
      using pm::operations::composed11;
      using pm::operations::composed12;
      using pm::operations::composed21;
      using pm::operations::member;
      typedef BuildBinary<pm::operations::swap_op> swap_op;

      using pm::operations::construct_unary;
      using pm::operations::construct_unary2;
      using pm::operations::construct_unary3;
      using pm::operations::construct_binary;
      using pm::operations::construct_binary2;

      typedef BuildUnary<pm::operations::move> move;

      /// these come from pair, but Build*ary is not defined there
      typedef BuildBinary<pm::operations::pair_maker> pair_maker;
      typedef BuildUnary<pm::operations::take_first> take_first;
      typedef BuildUnary<pm::operations::take_second> take_second;
   }
}

#include "polymake/internal/extend_algo.h"

#ifdef __clang__

// FIXME: remove this hack when all composed operations are expelled in favor of lambdas
namespace std {

template <typename InnerOperation, typename OuterOperation>
struct is_default_constructible<pm::operations::composed11<InnerOperation, OuterOperation>>
   : polymake::mlist_and<is_default_constructible<InnerOperation>, is_default_constructible<OuterOperation>> {};

}

#endif // __clang__

#endif // POLYMAKE_INTERNAL_OPERATIONS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
