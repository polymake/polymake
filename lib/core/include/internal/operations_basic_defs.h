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

#ifndef POLYMAKE_INTERNAL_OPERATIONS_BASIC_DEFS_H
#define POLYMAKE_INTERNAL_OPERATIONS_BASIC_DEFS_H

#include "polymake/internal/type_manip.h"
#include <cmath>
#include <string>

namespace pm {

template <typename T> inline
typename std::enable_if<std::is_arithmetic<T>::value, T&>::type
negate(T& x)
{
   x=-x;
   return x;
}

template <typename T> inline
typename std::enable_if<is_class_or_union<pure_type_t<T>>::value &&
                        !std::is_const<std::remove_reference_t<T>>::value, T&&>::type
negate(T&& x)
{
   return std::forward<T>(x.negate());
}

namespace operations {

struct partial {};
struct partial_left : partial {};
struct partial_right : partial {};

namespace analyzer {
   template <typename T1, typename T2>
   derivation::yes test_f(const T1&, const T2&, const T1&);

   template <typename T1, typename T2>
   derivation::no test_f(const T1&, const T2&, const T2&);

   template <typename T1>
   derivation::yes test_f(const T1&, const T1&, const T1&);
}

#define GuessResultType(name,sign) \
template <typename T1, typename T2> \
struct name##_result { \
   static const T1& op1();  static const T2& op2(); \
   static const bool first=sizeof(analyzer::test_f(op1(),op2(),op1() sign op2()))==sizeof(derivation::yes); \
   typedef typename std::conditional<first, T1, T2>::type type; \
}

GuessResultType(add,+);
GuessResultType(sub,-);
GuessResultType(mul,*);
GuessResultType(div,/);
GuessResultType(mod,%);
GuessResultType(or,|);
GuessResultType(and,&);
GuessResultType(xor,^);

template <typename Op, typename Result>
struct neg_scalar {
   typedef Op argument_type;
   typedef const Result result_type;
   result_type operator() (typename function_argument<Op>::type a) const { return -a; }
   void assign(Op& a) const { negate(a); }
};

template <typename Op>
struct neg_scalar<Op,void> {
   typedef void result_type;
};

template <typename Left, typename Right, typename Result>
struct add_scalar {
   typedef Left first_argument_type;
   typedef Right second_argument_type;
   typedef const Result result_type;

   result_type operator() (typename function_argument<Left>::type a, typename function_argument<Right>::type b) const { return a+b; }
   template <typename Iterator2>
   const Left& operator() (partial_left, const Left& a, const Iterator2&) const { return a; }
   template <typename Iterator1>
   const Right& operator() (partial_right, const Iterator1&, const Right& b) const { return b; }
   void assign(Left& a, typename function_argument<Right>::type b) const { a+=b; }
};

template <typename Left, typename Right, typename Result>
struct sub_scalar {
   typedef Left first_argument_type;
   typedef Right second_argument_type;
   typedef const Result result_type;

   result_type operator() (typename function_argument<Left>::type a, typename function_argument<Right>::type b) const { return a-b; }
   template <typename Iterator2>
   const Left& operator() (partial_left, const Left& a, const Iterator2&) const { return a; }
   template <typename Iterator1>
   result_type operator() (partial_right, const Iterator1&, typename function_argument<Right>::type b) const { return -b; }
   void assign(Left& a, typename function_argument<Right>::type b) const { a-=b; }
};

template <typename Left, typename Right, typename Result>
struct mul_scalar {
   typedef Left first_argument_type;
   typedef Right second_argument_type;
   typedef const Result result_type;

   result_type operator() (typename function_argument<Left>::type a, typename function_argument<Right>::type b) const { return a*b; }
   void assign(Left& a, typename function_argument<Right>::type b) const { a*=b; }
};

template <typename Left, typename Right>
struct mul_scalar<Left,Right,void> {
   typedef void result_type;
};

template <typename Left, typename Right, typename Result>
struct div_scalar {
   typedef Left first_argument_type;
   typedef Right second_argument_type;
   typedef const Result result_type;

   result_type operator() (typename function_argument<Left>::type a, typename function_argument<Right>::type b) const { return a/b; }
   void assign(Left& a, typename function_argument<Right>::type b) const { a/=b; }
};

template <typename Left, typename Right>
struct div_scalar<Left,Right,void> {
   typedef void result_type;
};

template <typename Left, typename Right, typename Result>
struct divexact_scalar : div_scalar <Left, Right, Result> {};

template <typename Left, typename Right, typename Result>
struct mod_scalar {
   typedef Left first_argument_type;
   typedef Right second_argument_type;
   typedef const Result result_type;

   result_type operator() (typename function_argument<Left>::type a, typename function_argument<Right>::type b) const { return a%b; }
   void assign(Left& a, typename function_argument<Right>::type b) const { a%=b; }
};

template <typename Left, typename Right>
struct mod_scalar<Left,Right,void> {
   typedef void result_type;
};

template <typename Op, typename Result>
struct inv_scalar {
   typedef Op argument_type;
   typedef const Result result_type;
   result_type operator() (typename function_argument<Op>::type a) const { return ~a; }
   void assign(Op& a) const { a=~a; }
};

template <typename Op>
struct inv_scalar<Op,void> {
   typedef void result_type;
};

template <typename Left, typename Right, typename Result>
struct or_scalar {
   typedef Left first_argument_type;
   typedef Right second_argument_type;
   typedef const Result result_type;

   result_type operator() (typename function_argument<Left>::type a, typename function_argument<Right>::type b) const { return a|b; }
   template <typename Iterator2>
   const Left& operator() (partial_left, const Left& a, const Iterator2&) const { return a; }
   template <typename Iterator1>
   const Right& operator() (partial_right, const Iterator1&, const Right& b) const { return b; }
   void assign(Left& a, typename function_argument<Right>::type b) const { a|=b; }
};

template <typename Left, typename Right>
struct or_scalar<Left,Right,void> {
   typedef void result_type;
};

template <typename Left, typename Right, typename Result>
struct and_scalar {
   typedef Left first_argument_type;
   typedef Right second_argument_type;
   typedef const Result result_type;

   result_type operator() (typename function_argument<Left>::type a, typename function_argument<Right>::type b) const { return a&b; }
   void assign(Left& a, typename function_argument<Right>::type b) const { a&=b; }
};

template <typename Left, typename Right>
struct and_scalar<Left,Right,void> {
   typedef void result_type;
};

template <typename Left, typename Right, typename Result>
struct xor_scalar {
   typedef Left first_argument_type;
   typedef Right second_argument_type;
   typedef const Result result_type;

   result_type operator() (typename function_argument<Left>::type a, typename function_argument<Right>::type b) const { return a^b; }
   void assign(Left& a, typename function_argument<Right>::type b) const { a^=b; }
};

template <typename Left, typename Right>
struct xor_scalar<Left,Right,void> {
   typedef void result_type;
};

} // end namespace operations

template <typename Char, typename Traits, typename Alloc>
struct spec_object_traits< std::basic_string<Char, Traits, Alloc> >
   : spec_object_traits<is_opaque> {};

using std::sqrt;

}
namespace polymake {

using pm::negate;
using pm::sqrt;

}

#endif // POLYMAKE_INTERNAL_OPERATIONS_BASIC_DEFS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
