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

#ifndef POLYMAKE_QUADRATIC_EXTENSION_H
#define POLYMAKE_QUADRATIC_EXTENSION_H

#include "polymake/Rational.h"
#include "polymake/AccurateFloat.h"

namespace pm {

namespace {
/// Exception type: "Incompatible roots".
class RootError : public GMP::error {
public:
   RootError() : error("Mismatch in root of extension") {}
};

/// Exception type: "Not totally orderable field".
class NonOrderableError : public GMP::error {
public:
   NonOrderableError() : error("Negative values for the root of the extension yield fields like C that are not totally orderable (which is a Bad Thing).") {}
};

} // end anonymous namespace

/** @file QuadraticExtension.h
    @class QuadraticExtension
    @brief Realizes quadratic extensions of fields
 */
template <typename Field=Rational> 
class QuadraticExtension {
public:
   typedef Field field_type;

protected:
   field_type _a, _b, _r;

public:
   template <typename>
   friend struct spec_object_traits;

   QuadraticExtension()
      : _a(0)
      , _b(0)
      , _r(0) {}

   QuadraticExtension(const field_type& a, const field_type& b, const field_type& r)
      : _a(a)
      , _b(b)
      , _r(r)
   {
      switch (sign(r)) {
      case cmp_lt:
         throw NonOrderableError();
      case cmp_eq:
         _b=0; break;
      default:
         break;
      }
   }

   explicit QuadraticExtension(const field_type& j)
      : _a(j)
      , _b(0)
      , _r(0) {}

   explicit QuadraticExtension(const double j)
      : _a(j)
      , _b(0)
      , _r(0) {}

   explicit QuadraticExtension(const int j)
      : _a(j)
      , _b(0)
      , _r(0) {}

   explicit QuadraticExtension(const long j)
      : _a(j)
      , _b(0)
      , _r(0) {}

   template <typename T>
   explicit QuadraticExtension(const T& j,
                               typename enable_if<void**, explicitly_convertible_to<T, Field>::value>::type =0)
      : _a(conv<T, Field>()(j))
      , _b(0)
      , _r(0) {}

   template <typename T>
   typename enable_if<QuadraticExtension&, assignable_to<T, Field>::value>::type
   operator= (const T& i)
   {
      _a = i;
      _b = 0;
      _r = 0;
      return *this;
   }

   const Field& a() const { return _a; }
   const Field& b() const { return _b; }
   const Field& r() const { return _r; }

   ///
   void swap (QuadraticExtension& op)
   {
      _a.swap(op._a);
      _b.swap(op._b);
   }

   ///
   QuadraticExtension& negate() 
   {
      _a.negate(); 
      _b.negate(); 
      return *this;
   }

   ///
   QuadraticExtension& conjugate() 
   {
      _b.negate();
      return *this;
   }

   ///
   field_type norm() const
   {
      return _a*_a - _b*_b*_r;
   }

   template <typename T>
   typename enable_if<QuadraticExtension&, convertible_to<T, Field>::value>::type
   operator+= (const T& op)
   {
      _a += op;
      return *this;
   }

   /// add an element
   QuadraticExtension& operator+= (const QuadraticExtension<Field>& op)
   {
      if (is_zero(_r)) {
         _r = op.r(); 
      } else if (!is_zero(op.r()) && op.r()!=_r) {
         throw RootError();
      }
      _a += op.a(); _b += op.b(); 
      return *this;
   }




   // subtraction from self
   template <typename T>
   typename enable_if<QuadraticExtension&, convertible_to<T, Field>::value>::type
   operator-= (const T& op)
   {
      _a -= op;
      return *this;
   }

   /// substract an element
   QuadraticExtension& operator-= (const QuadraticExtension& op)
   {
      if (is_zero(_r)) {
         _r = op.r(); 
      } else if (!is_zero(op.r()) && op.r()!=_r) {
         throw RootError();
      }
      _a -= op.a(); _b -= op.b(); 
      return *this;
   }


   // multiplication with self
   template <typename T> 
   typename enable_if<QuadraticExtension&, convertible_to<T, Field>::value>::type
   operator*= (const T& op)
   {
      _a *= op; _b *= op;
      return *this;
   }

   /// multiply with an element
   QuadraticExtension& operator*= (const QuadraticExtension<Field>& op)
   {
      if (is_zero(_r)) {
         _r = op.r(); 
      } else if (!is_zero(op.r()) && op.r()!=_r) {
         throw RootError();
      }
      // const field_type tmp(_a);
      // _a = _a * op.a() + _b * (op.b() * _r);
      // _b = _b * op.a() + tmp * op.b();
      field_type tmp_a(_a), tmp_b(_b);
      tmp_a *= op.a(); tmp_b *= op.b(); tmp_b *= _r; tmp_a += tmp_b;
      _a.swap(tmp_a);
      tmp_a *= op.b(); tmp_b.swap(_b); tmp_b *= op.a(); tmp_a += tmp_b; 
      _b.swap(tmp_a);
      return *this;
   }


   // division into self
   template <typename T>
   typename enable_if<QuadraticExtension&, convertible_to<T, Field>::value>::type
   operator/= (const T& op)
   {
      if (is_zero(op)) throw GMP::ZeroDivide();
      _a /= op; _b /= op;
      return *this;
   }

   /// divide by an element
   QuadraticExtension& operator/= (const QuadraticExtension<Field>& op)
   {
      if (is_zero(op)) throw GMP::ZeroDivide();
      if (is_zero(_r)) {
         _r = op.r(); 
      } else if (!is_zero(op.r()) && op.r()!=_r) {
         throw RootError();
      }
      /*
      QuadraticExtension f(op);
      f.conjugate();
      *this *= f;
      *this /= f.norm();
      */
      field_type tmp_a(_a), tmp_b(_b);  // *this == a + b sqrt{r},  op == c + d sqrt{r}
      // we now calculate ((ac-bdr) + (bc-ad)sqrt{r})/(c^2-d^2 r)
      tmp_a *= op.a(); _a.swap(tmp_a);  // _a now holds a*c, tmp_a holds _a
      tmp_a *= op.b(); // tmp_a holds a * d
      tmp_b *= op.a(); tmp_b -= tmp_a; _b.swap(tmp_b); // _b holds b * c - a * d; tmp_b holds b
      tmp_b *= op.b(); tmp_b *= _r; // tmp_b holds b*d*r
      _a -= tmp_b; // _a holds a*c - b*d*r
      tmp_a = op.a(); tmp_b = op.b();
      tmp_a *= op.a(); tmp_b *= op.b(); tmp_b *= _r; tmp_a -= tmp_b; // tmp_a holds c^2-d^2r
      _a /= tmp_a; _b /= tmp_a;
      return *this;
   }

   // Addition with others
   template <typename T> friend
   typename enable_if<QuadraticExtension, convertible_to<T, Field>::value>::type
   operator+ (const T& e, const QuadraticExtension& f)
   {
      QuadraticExtension g(f);
      return g += e;
   }

   template <typename T> friend
   typename enable_if<QuadraticExtension, convertible_to<T, Field>::value>::type
   operator+ (const QuadraticExtension& e, const T& f)
   {
      QuadraticExtension g(e);
      return g += f;
   }

   friend
   QuadraticExtension operator+ (const QuadraticExtension& e, const QuadraticExtension& f)
   {
      QuadraticExtension g(e);
      g += f;
      return g;
   }


   // Subtraction with others
   template <typename T> friend
   typename enable_if<QuadraticExtension, convertible_to<T, Field>::value>::type
   operator- (const QuadraticExtension& e, const T& f)
   {
      QuadraticExtension g(e);
      return g -= f;
   }

   template <typename T> friend
   typename enable_if<QuadraticExtension, convertible_to<T, Field>::value>::type
   operator- (const T& e, const QuadraticExtension& f)
   {
      return (f-e).negate();
   }

   friend
   QuadraticExtension operator- (const QuadraticExtension& e, const QuadraticExtension& f)
   {
      QuadraticExtension g(e);
      return g -= f;
   }

   friend
   QuadraticExtension operator- (const QuadraticExtension& e)
   {
      QuadraticExtension f(e);
      f.negate();
      return f;
   }

   template <typename T> friend
   typename enable_if<QuadraticExtension, convertible_to<T, Field>::value>::type
   operator* (const QuadraticExtension& e, const T& f)
   {
      QuadraticExtension g(e);
      return g *= f;
   }

   template <typename T> friend
   typename enable_if<QuadraticExtension, convertible_to<T, Field>::value>::type
   operator* (const T& e, const QuadraticExtension& f)
   {
      QuadraticExtension g(f);
      return g *= e;
   }

   friend
   QuadraticExtension operator* (const QuadraticExtension& e, const QuadraticExtension& f)
   {
      QuadraticExtension g(e);
      return g *= f;
   }

   template <typename T> friend
   typename enable_if<QuadraticExtension, convertible_to<T, Field>::value>::type
   operator/ (const QuadraticExtension& e, const T& f)
   {
      QuadraticExtension g(e);
      return g /= f;
   }

   template <typename T> friend
   typename enable_if<QuadraticExtension, convertible_to<T, Field>::value>::type
   operator/ (const T& e, const QuadraticExtension& f)
   {
      QuadraticExtension g(e);
      return g /= f;
   }

   friend
   QuadraticExtension operator/ (const QuadraticExtension& e, const QuadraticExtension& f)
   {
      QuadraticExtension g(e);
      return g /= f;
   }

   // end of arithmetic operations


   cmp_value compare(const QuadraticExtension<Field>& op) const
   {
      if (!is_zero(_r) && !is_zero(op.r()) && op.r()!=_r) {
         throw RootError();
      }
      operations::cmp cmp;
      const cmp_value
         cmp_a = cmp(_a, op.a()),
         cmp_b = cmp(_b, op.b());
      if ((is_zero(_r) && is_zero(op.r()))
          || cmp_a == cmp_b)
         return cmp_a;
      if (cmp_a==cmp_eq) return cmp_b;
      if (cmp_b==cmp_eq) return cmp_a;

      // Else, we transform 
      // cmp(_a + _b sqrt{r}, op.a() + op.b() sqrt{r})   to
      // cmp( _a - op.a(), (op.b() - _b) sqrt{r}) where sides have sign cmp_a, resp. cmp_b.
      field_type lhs(_a - op.a()), rhs(op.b() - _b);
      lhs *= lhs; rhs *= rhs; rhs *= (is_zero(_r) ? op.r() : _r);
      return cmp_value(cmp(lhs, rhs) * cmp_a);
   }

   template <typename T>
   typename enable_if<cmp_value, convertible_to<T, Field>::value>::type
   compare(const T& op) const 
   {
      return compare(QuadraticExtension(op));
   }

   ///
   template <typename T> friend
   typename enable_if<bool, convertible_to<T, Field>::value>::type
   operator== (const QuadraticExtension& e, const T& f)
   {
      return is_zero(f) ? is_zero(e) : e.compare(f) == cmp_eq;
   }

   ///
   template <typename T> friend
   typename enable_if<bool, convertible_to<T, Field>::value>::type
   operator!= (const QuadraticExtension& e, const T& f)
   {
      return is_zero(f) ? !is_zero(e) : e.compare(f) != cmp_eq;
   }

   template <typename T> friend
   typename enable_if<bool, convertible_to<T, Field>::value>::type
   operator< (const QuadraticExtension& e, const T& f)
   {
      return e.compare(f) == cmp_lt;
   }

   ///
   template <typename T> friend
   typename enable_if<bool, convertible_to<T, Field>::value>::type
   operator<= (const QuadraticExtension& e, const T& f)
   {
      return e.compare(f) != cmp_gt;
   }

   ///
   template <typename T> friend
   typename enable_if<bool, convertible_to<T, Field>::value>::type
   operator> (const QuadraticExtension& e, const T& f)
   {
      return e.compare(f) == cmp_gt;
   }

   ///
   template <typename T> friend
   typename enable_if<bool, convertible_to<T, Field>::value>::type
   operator>= (const QuadraticExtension& e, const T& f)
   {
      return e.compare(f) != cmp_lt;
   }

   ///
   template <typename T> friend
   typename enable_if<bool, convertible_to<T, Field>::value>::type
   operator== (const T& e, const QuadraticExtension& f)
   {
      return f==e;
   }

   template <typename T> friend
   typename enable_if<bool, convertible_to<T, Field>::value>::type
   operator!= (const T& e, const QuadraticExtension& f)
   {
      return f!=e;
   }

   ///
   template <typename T> friend
   typename enable_if<bool, convertible_to<T, Field>::value>::type
   operator< (const T& e, const QuadraticExtension& f)
   {
      return f>e;
   }

   ///
   template <typename T> friend
   typename enable_if<bool, convertible_to<T, Field>::value>::type
   operator<= (const T& e, const QuadraticExtension& f)
   {
      return f>=e;
   }

   template <typename T> friend
   typename enable_if<bool, convertible_to<T, Field>::value>::type
   operator> (const T& e, const QuadraticExtension& f)
   {
      return f<e;
   }

   ///
   template <typename T> friend
   typename enable_if<bool, convertible_to<T, Field>::value>::type
   operator>= (const T& e, const QuadraticExtension& f)
   {
      return f<=e;
   }

   friend
   bool operator== (const QuadraticExtension& e, const QuadraticExtension& f)
   {
      return e.compare(f) == cmp_eq;
   }

   friend
   bool operator!= (const QuadraticExtension& e, const QuadraticExtension& f)
   {
      return e.compare(f) != cmp_eq;
   }

   friend
   bool operator< (const QuadraticExtension& e, const QuadraticExtension& f)
   {
      return e.compare(f) == cmp_lt;
   }

   friend
   bool operator<= (const QuadraticExtension& e, const QuadraticExtension& f)
   {
      return e.compare(f) != cmp_gt;
   }

   friend
   bool operator> (const QuadraticExtension& e, const QuadraticExtension& f)
   {
      return e.compare(f) == cmp_gt;
   }

   friend
   bool operator>= (const QuadraticExtension& e, const QuadraticExtension& f)
   {
      return e.compare(f) != cmp_lt;
   }

   bool non_zero() const
   {
      return !is_zero(_a) || !is_zero(_b);
   }

   field_type to_field_type() const  // FIXME #569
   {
      return _a + field_type(_b * sqrt(AccurateFloat(_r)));
   }

   double to_double() const // FIXME #569
   {
      return to_field_type().to_double();
   }

   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& outs, const QuadraticExtension& e)
   {
      Output& os=outs.top();
      if (!is_zero(e.b())) {
         const bool need_parens = has_serialized<Field>::value;
         if (need_parens) os << '(';
         os << e.a();
         if (need_parens) os << ")+(";
         else if (e.b() > 0) os << '+';
         os << e.b();
         if (need_parens) os << ')';
         os << 'r' << e.r();
      } else {
         os << e.a();
      }
      return os;
   }

   /// the absolute value
   friend QuadraticExtension abs(const QuadraticExtension& e)
   {
      return (e>=0) ? e : -e;
   }
};

template <typename Field, typename T> inline
typename enable_if<bool, convertible_to<T, Field>::value>::type
abs_equal(const QuadraticExtension<Field>& e, const T& f)
{
   return is_zero(e.b()) && abs_equal(e.a(), f);
}

template <typename Field, typename T> inline
typename enable_if<bool, convertible_to<T, Field>::value>::type
abs_equal(const T& e, const QuadraticExtension<Field>& f)
{
   return abs_equal(f, e);
}

template <typename Field> inline
bool abs_equal(const QuadraticExtension<Field>& e, const QuadraticExtension<Field>& f)
{
   return abs_equal(e.a(), f.a()) && abs_equal(e.b(), f.b());
}

template <typename Field> inline
bool isfinite(const QuadraticExtension<Field>& op) 
{ 
   return isfinite(op.a()) && isfinite(op.b()); 
}

template <typename Field> inline
QuadraticExtension<Field> conjugate(const QuadraticExtension<Field>& f)
{
   QuadraticExtension<Field> q(f);
   return q.conjugate();
}

template <typename Field> inline
cmp_value sign(const QuadraticExtension<Field>& f)
{
   const cmp_value sa=sign(f.a()),
                   sb=sign(f.b());
   if (sa == sb || sb == cmp_eq)
      return sa;
   if (sa == cmp_eq)
      return sb;

   Field tmp = f.a() / f.b();
   tmp *= tmp;
   return tmp > f.r() ? sa : sb;
}

template <typename Field>
class conv<QuadraticExtension<Field>, Field> {
public:
   typedef QuadraticExtension<Field> argument_type;
   typedef Field result_type;
   result_type operator() (const argument_type& f) const { return f.to_field_type(); }
};

template <typename Field, typename Scalar, typename FinalConv>
class QuadraticExtension_conv_helper : public FinalConv {
public:
   typedef QuadraticExtension<Field> argument_type;
   typename FinalConv::result_type operator() (const argument_type& f) const { return FinalConv::operator()(f.to_field_type()); }
};

template <typename Field, typename Scalar>
class QuadraticExtension_conv_helper<Field, Scalar, nothing> : public nothing {};

template <typename Field, typename Scalar>
class conv<QuadraticExtension<Field>, Scalar>
   : public QuadraticExtension_conv_helper<Field, Scalar, typename if_else< convertible_to<Field, Scalar>::value, conv_by_cast<Field, Scalar>,
                                                          typename if_else< explicitly_convertible_to<Field, Scalar>::value, conv<Field, Scalar>,
                                                                            nothing >::type >::type> {};
template <typename Field>
class conv<Field, QuadraticExtension<Field> > : public conv_by_cast<Field, QuadraticExtension<Field> > {};

template <typename Scalar, typename Field>
class conv<Scalar, QuadraticExtension<Field> > : public if_else< (convertible_to<Scalar, Field>::value ||
                                                                  explicitly_convertible_to<Scalar, Field>::value),
                                                                 conv_by_cast<Scalar, QuadraticExtension<Field> >,
                                                                 nothing >::type {};

// disambiguation
template <typename Field>
class conv<QuadraticExtension<Field>, QuadraticExtension<Field> > : public trivial_conv< QuadraticExtension<Field> > {};

template <typename Field>
class conv<QuadraticExtension<Field>, AccurateFloat> {
public:
   typedef QuadraticExtension<Field> argument_type;
   typedef AccurateFloat result_type;
   result_type operator() (const QuadraticExtension<Field>& q) const { return AccurateFloat(q.a()) + sqrt(AccurateFloat(q.r())) * AccurateFloat(q.b()); }
};



template <typename Field>
struct choose_generic_object_traits<QuadraticExtension<Field>, false, false> :
   spec_object_traits< QuadraticExtension<Field> > {
   typedef void generic_type;
   typedef is_scalar generic_tag;
   typedef QuadraticExtension<Field> persistent_type;

   static
   bool is_zero(const QuadraticExtension<Field>& e)
   {
      return !e.non_zero();
   }

   static
   bool is_one(const QuadraticExtension<Field>& e)
   {
      return spec_object_traits<Field>::is_one(e.a()) && spec_object_traits<Field>::is_zero(e.b());
   }

   static const persistent_type& zero() 
   {
      static const QuadraticExtension<Field> qe_zero(0, 0, 0); 
      return qe_zero;
   }

   static const persistent_type& one() 
   {
      static const QuadraticExtension<Field> qe_one (1, 0, 0); 
      return qe_one;
   }
};

template <typename Field>
struct spec_object_traits< Serialized< QuadraticExtension<Field> > > :
   spec_object_traits<is_composite> {

   typedef QuadraticExtension<Field> masquerade_for;

   typedef cons<Field, cons<Field, Field> > elements;

   template <typename Me, typename Visitor>
   static void visit_elements(Me& me, Visitor& v)
   {
      v << me._a << me._b << me._r;
   }
};

// some magic needed for operator construction
struct is_quadratic_extension {};

namespace operations {

// these operations will be required e.g. for Vector<QuadraticExtension> or Matrix<QuadraticExtension>

template <typename OpRef>
struct neg_impl<OpRef, is_quadratic_extension> {
   typedef OpRef argument_type;
   typedef typename deref<OpRef>::type result_type;

   result_type operator() (typename function_argument<OpRef>::const_type x) const
   {
      return -x;
   }

   void assign(typename lvalue_arg<OpRef>::type x) const
   {
      x.negate();
   }
};

template <typename LeftRef, class RightRef>
struct add_impl<LeftRef, RightRef, cons<is_quadratic_extension, is_quadratic_extension> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l+r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l+=r;
   }
};

template <typename LeftRef, class RightRef>
struct add_impl<LeftRef, RightRef, cons<is_quadratic_extension, is_scalar> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l+r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l+=r;
   }
};

template <typename LeftRef, class RightRef>
struct add_impl<LeftRef, RightRef, cons<is_scalar, is_quadratic_extension> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<RightRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l+r;
   }
};

template <typename LeftRef, class RightRef>
struct sub_impl<LeftRef, RightRef, cons<is_quadratic_extension, is_quadratic_extension> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l-r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l-=r;
   }
};

template <typename LeftRef, class RightRef>
struct sub_impl<LeftRef, RightRef, cons<is_quadratic_extension, is_scalar> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l-r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l-=r;
   }
};

template <typename LeftRef, class RightRef>
struct sub_impl<LeftRef, RightRef, cons<is_scalar, is_quadratic_extension> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<RightRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l-r;
   }
};

template <typename LeftRef, class RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_quadratic_extension, is_quadratic_extension> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l*r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l*=r;
   }
};

template <typename LeftRef, class RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_quadratic_extension, is_scalar> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l*r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l*=r;
   }
};

template <typename LeftRef, class RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_scalar, is_quadratic_extension> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<RightRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l*r;
   }
};

template <typename LeftRef, class RightRef>
struct div_impl<LeftRef, RightRef, cons<is_quadratic_extension, is_scalar> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return l/r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l/=r;
   }
};

} // end namespace operations

template <typename Field>
struct hash_func<QuadraticExtension<Field>, is_scalar> : hash_func<MP_RAT> {
protected:
   size_t _do(const QuadraticExtension<Field>& e) const
   {
      return hash_func<MP_RAT>::_do(e.a().get_rep()) + hash_func<MP_RAT>::_do(e.b().get_rep());
   }
public:
   size_t operator() (const QuadraticExtension<Field>& e) const { return isfinite(e) ? _do(e) : 0; }
};


template <typename Field>
struct algebraic_traits<QuadraticExtension<Field> > {
   typedef QuadraticExtension<Field> field_type;
};

} // end namespace pm

namespace polymake {
   using pm::QuadraticExtension;
}

namespace std {
   template <typename Field>
   void swap(pm::QuadraticExtension<Field>& x1, pm::QuadraticExtension<Field>& x2) { x1.swap(x2); }

   template <typename Field>
   struct numeric_limits<pm::QuadraticExtension<Field> > : numeric_limits<Field> {
      static const bool is_integer=false;
      static pm::QuadraticExtension<Field> min() throw() { return pm::QuadraticExtension<Field>(numeric_limits<Field>::min()); }
      static pm::QuadraticExtension<Field> infinity() throw() { return pm::QuadraticExtension<Field>(numeric_limits<Field>::infinity()); }
      static pm::QuadraticExtension<Field> max() throw() { return pm::QuadraticExtension<Field>(numeric_limits<Field>::max()); }
   };

}



#endif // POLYMAKE_QUADRATIC_EXTENSION_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
