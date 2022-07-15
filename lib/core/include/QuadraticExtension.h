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

#include "polymake/Rational.h"
#include "polymake/AccurateFloat.h"
#include <initializer_list>

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

template <typename Field>
class QuadraticExtension;

template <typename Field>
Int sign(const QuadraticExtension<Field>&);

/** @class QuadraticExtension
    @brief Realizes quadratic extensions of fields
 */
template <typename Field = Rational>
class QuadraticExtension {
public:
   static_assert(is_field<Field>::value, "base number type is not a field");

   typedef Field field_type;

   template <typename T>
   struct can_initialize_particle
      : bool_constant<can_initialize<pure_type_t<T>, Field>::value && !std::is_same<pure_type_t<T>, QuadraticExtension>::value> {};

   template <typename T>
   struct can_assign_particle
      : bool_constant<isomorphic_types<pure_type_t<T>, Field>::value && can_assign_to<pure_type_t<T>, Field>::value && !std::is_same<pure_type_t<T>, QuadraticExtension>::value> {};

   template <typename T>
   struct fits_as_particle
      : bool_constant<can_upgrade<pure_type_t<T>, Field>::value && !std::is_same<pure_type_t<T>, QuadraticExtension>::value> {};

   template <typename T>
   struct fits_as_operand
      : bool_constant<can_upgrade<pure_type_t<T>, Field>::value || std::is_same<pure_type_t<T>, QuadraticExtension>::value> {};

   QuadraticExtension()
      : a_()
      , b_()
      , r_() {}

   template <typename T1, typename T2, typename T3,
             typename = std::enable_if_t<can_initialize_particle<T1>::value &&
                                         can_initialize_particle<T2>::value &&
                                         can_initialize_particle<T3>::value>>
      QuadraticExtension(T1&& a, T2&& b, T3&& r)
      : a_(std::forward<T1>(a))
      , b_(std::forward<T2>(b))
      , r_(std::forward<T3>(r))
   {
      normalize();
   }

   template <typename T, typename = std::enable_if_t<can_initialize_particle<T>::value>>
   explicit QuadraticExtension(T&& a)
      : a_(std::forward<T>(a))
      , b_()
      , r_() {}

   QuadraticExtension(const QuadraticExtension&) = default;
   QuadraticExtension(QuadraticExtension&&) = default;
   QuadraticExtension& operator= (const QuadraticExtension&) = default;
   QuadraticExtension& operator= (QuadraticExtension&&) = default;

   template <typename T,
             typename = std::enable_if_t<can_assign_particle<T>::value>>
   QuadraticExtension& operator= (T&& a)
   {
      a_ = std::forward<T>(a);
      b_ = zero_value<Field>();
      r_ = zero_value<Field>();
      return *this;
   }

   template <typename T,
             typename = std::enable_if_t<can_assign_particle<T>::value>>
   QuadraticExtension& operator= (std::initializer_list<T> l)
   {
      if (l.size() != 3) throw std::runtime_error("initializer list must have 3 elements");
      const T* val=l.begin();
      a_ = val[0];
      b_ = val[1];
      r_ = val[2];
      normalize();
      return *this;
   }

   const Field& a() const { return a_; }
   const Field& b() const { return b_; }
   const Field& r() const { return r_; }

   void swap (QuadraticExtension& op)
   {
      std::swap(a_, op.a_);
      std::swap(b_, op.b_);
      std::swap(r_, op.r_);
   }

   friend
   QuadraticExtension operator+ (const QuadraticExtension& a)
   {
      return a;
   }

   friend
   QuadraticExtension&& operator+ (QuadraticExtension&& a)
   {
      return std::move(a);
   }

   QuadraticExtension& negate()
   {
      a_.negate();
      b_.negate();
      return *this;
   }

   friend
   QuadraticExtension operator- (const QuadraticExtension& a)
   {
      QuadraticExtension result(a);
      result.negate();
      return result;
   }

   friend
   QuadraticExtension&& operator- (QuadraticExtension&& a)
   {
      return std::move(a.negate());
   }

   QuadraticExtension& conjugate()
   {
      b_.negate();
      return *this;
   }

   friend
   QuadraticExtension conjugate(const QuadraticExtension& a)
   {
      QuadraticExtension result(a);
      result.conjugate();
      return result;
   }

   friend
   QuadraticExtension&& conjugate(QuadraticExtension&& a)
   {
      return std::move(a.conjugate());
   }

   field_type norm() const
   {
      return a_*a_ - b_*b_*r_;
   }

   template <typename T, typename = std::enable_if_t<fits_as_particle<T>::value>>
   QuadraticExtension&
   operator+= (const T& x)
   {
      a_ += x;
      if (__builtin_expect(!isfinite(x), 0)) {
         b_ = zero_value<Field>();
         r_ = zero_value<Field>();
      }
      return *this;
   }

   QuadraticExtension& operator+= (const QuadraticExtension& x)
   {
      if (is_zero(x.r_)) {
         *this += x.a_;
      } else {
         if (is_zero(r_)) {
            if (__builtin_expect(isfinite(a_), 1)) {
               b_ = x.b_;
               r_ = x.r_;
            }
         } else {
            if (x.r_ != r_) throw RootError();
            b_ += x.b_;
            if (is_zero(b_)) r_=zero_value<Field>();
         }
         a_ += x.a_; 
      }
      return *this;
   }

   template <typename T, typename = std::enable_if_t<fits_as_operand<T>::value>>
   friend
   QuadraticExtension operator+ (const QuadraticExtension& x, const T& y)
   {
      QuadraticExtension result(x);
      result += y;
      return result;
   }

   template <typename T, typename = std::enable_if_t<fits_as_operand<T>::value>>
   friend
   QuadraticExtension&& operator+ (QuadraticExtension&& x, const T& y)
   {
      return std::move(x += y);
   }

   template <typename T, typename = std::enable_if_t<fits_as_particle<T>::value>>
   friend
   QuadraticExtension operator+ (const T& x, const QuadraticExtension& y)
   {
      return y+x;
   }

   template <typename T, typename = std::enable_if_t<fits_as_operand<T>::value>>
   friend
   QuadraticExtension&& operator+ (const T& x, QuadraticExtension&& y)
   {
      return std::move(y += x);
   }

   friend
   QuadraticExtension&& operator+ (QuadraticExtension&& x, QuadraticExtension&& y)
   {
      return std::move(x += y);
   }


   template <typename T, typename = std::enable_if_t<fits_as_particle<T>::value>>
   QuadraticExtension&
   operator-= (const T& x)
   {
      a_ -= x;
      if (__builtin_expect(!isfinite(x), 0)) {
         b_ = zero_value<Field>();
         r_ = zero_value<Field>();
      }
      return *this;
   }

   QuadraticExtension& operator-= (const QuadraticExtension& x)
   {
      if (is_zero(x.r_)) {
         *this -= x.a_;
      } else {
         if (is_zero(r_)) {
            if (__builtin_expect(isfinite(a_), 1)) {
               b_ -= x.b_;
               r_ = x.r_;
            }
         } else {
            if (x.r_ != r_) throw RootError();
            b_ -= x.b_;
            if (is_zero(b_)) r_=zero_value<Field>();
         }
         a_ -= x.a_;
      }
      return *this;
   }

   template <typename T, typename = std::enable_if_t<fits_as_operand<T>::value>>
   friend
   QuadraticExtension operator- (const QuadraticExtension& x, const T& y)
   {
      QuadraticExtension result(x);
      result -= y;
      return result;
   }

   template <typename T, typename = std::enable_if_t<fits_as_operand<T>::value>>
   friend
   QuadraticExtension&& operator- (QuadraticExtension&& x, const T& y)
   {
      return std::move(x -= y);
   }

   template <typename T, typename = std::enable_if_t<fits_as_particle<T>::value>>
   friend
   QuadraticExtension operator- (const T& x, const QuadraticExtension& y)
   {
      QuadraticExtension result(y);
      return (result -= x).negate();
   }

   template <typename T, typename = std::enable_if_t<fits_as_operand<T>::value>>
   friend
   QuadraticExtension&& operator- (const T& x, QuadraticExtension&& y)
   {
      return std::move((y -= x).negate());
   }

   friend
   QuadraticExtension&& operator- (QuadraticExtension&& x, QuadraticExtension&& y)
   {
      return std::move(x -= y);
   }


   template <typename T, typename = std::enable_if_t<fits_as_particle<T>::value>>
   QuadraticExtension&
   operator*= (const T& x)
   {
      if (is_zero(r_)) {
         a_ *= x;
      } else if (__builtin_expect(isfinite(x), 1)) {
         if (is_zero(x)) {
            *this = x;
         } else {
            a_ *= x;
            b_ *= x;
         }
      } else {
         *this = sign(*this) < 0 ? -x : x;
      }
      return *this;
   }

   QuadraticExtension& operator*= (const QuadraticExtension& x)
   {
      if (is_zero(x.r_)) {
         *this *= x.a_;
      } else if (is_zero(r_)) {
         if (__builtin_expect(isfinite(a_), 1)) {
            if (!is_zero(a_)) {
               b_ = a_ * x.b_;
               a_ *= x.a_;
               r_ = x.r_;
            }
         } else if (sign(x) < 0) {
            a_.negate();
         }
      } else {
         if (x.r_ != r_) throw RootError();
         const Field tmp = a_ * x.b_;
         a_ *= x.a_;  a_ += b_ * x.b_ * r_;
         b_ *= x.a_;  b_ += tmp;
         if (is_zero(b_)) r_=zero_value<Field>();
      }
      return *this;
   }

   template <typename T, typename = std::enable_if_t<fits_as_operand<T>::value>>
   friend
   QuadraticExtension operator* (const QuadraticExtension& x, const T& y)
   {
      QuadraticExtension result(x);
      result *= y;
      return result;
   }

   template <typename T, typename = std::enable_if_t<fits_as_particle<T>::value>>
   friend
   QuadraticExtension operator* (const T& x, const QuadraticExtension& y)
   {
      return y*x;
   }

   template <typename T, typename = std::enable_if_t<fits_as_operand<T>::value>>
   friend
   QuadraticExtension&& operator* (QuadraticExtension&& x, const T& y)
   {
      return std::move(x *= y);
   }

   template <typename T, typename = std::enable_if_t<fits_as_operand<T>::value>>
   friend
   QuadraticExtension&& operator* (const T& x, QuadraticExtension&& y)
   {
      return std::move(y *= x);
   }

   friend
   QuadraticExtension&& operator* (QuadraticExtension&& x, QuadraticExtension&& y)
   {
      return std::move(x *= y);
   }


   template <typename T, typename = std::enable_if_t<fits_as_particle<T>::value>>
   QuadraticExtension& operator/= (const T& x)
   {
      if (std::is_arithmetic<Field>::value && is_zero(x))  // complex numeric types check for zero division themselves
         throw GMP::ZeroDivide();
      a_ /= x;
      if (__builtin_expect(isfinite(x), 1)) {
         b_ /= x;
      } else if (!is_zero(r_)) {
         b_ = zero_value<Field>();
         r_ = zero_value<Field>();
      }
      return *this;
   }

   QuadraticExtension& operator/= (const QuadraticExtension& x)
   {
      if (is_zero(x.r_)) {
         *this /= x.a_;
      } else if (is_zero(r_)) {
         if (__builtin_expect(isfinite(a_), 1)) {
            if (__builtin_expect(!is_zero(a_), 1)) {
               a_ /= x.norm();
               b_ = -(a_ * x.b_);
               a_ *= x.a_;
               r_ = x.r_;
            }
         } else if (sign(x) < 0) {
            a_.negate();
         }
      } else {
         if (x.r_ != r_) throw RootError();
         // *this = *this * conjugate(x) / x.norm();
         const Field n = x.norm();
         a_ /= n;  b_ /= n;
         const Field tmp= a_ * x.b_;
         a_ *= x.a_;  a_ -= b_ * x.b_ * r_;
         b_ *= x.a_;  b_ -= tmp;
         if (is_zero(b_)) r_ = zero_value<Field>();
      }
      return *this;
   }

   template <typename T, typename = std::enable_if_t<fits_as_operand<T>::value>>
   friend
   QuadraticExtension operator/ (const QuadraticExtension& x, const T& y)
   {
      QuadraticExtension result(x);
      result /= y;
      return result;
   }

   template <typename T, typename = std::enable_if_t<fits_as_particle<T>::value>>
   friend
   QuadraticExtension operator/ (const T& x, const QuadraticExtension& y)
   {
      QuadraticExtension result(x);
      result /= y;
      return result;
   }


   template <typename T, typename = std::enable_if_t<fits_as_operand<T>::value>>
   friend
   QuadraticExtension&& operator/ (QuadraticExtension&& x, const T& y)
   {
      return std::move(x /= y);
   }

   template <typename T, typename = std::enable_if_t<fits_as_operand<T>::value>>
   friend
   QuadraticExtension&& operator/ (const T& x, QuadraticExtension&& y)
   {
      if (is_zero(y))
         throw GMP::ZeroDivide();
      if (__builtin_expect(isfinite(y.a_), 1)) {
         if (is_zero(x)) {
            y=x;
         } else {
            y.conjugate();
            y /= y.norm();
            y *= x;
         }
      } else {
         y.a_=zero_value<Field>();
      }
      return std::move(y);
   }

   friend
   QuadraticExtension&& operator/ (QuadraticExtension&& x, QuadraticExtension&& y)
   {
      return std::move(x /= y);
   }

   Int compare(const QuadraticExtension& x) const
   {
      if (is_zero(r_)) {
         if (is_zero(x.r_)) {
            return operations::cmp()(a_, x.a_);
         } else {
            return compare(a_, b_, x.a_, x.b_, x.r_);
         }
      } else {
         if (!is_zero(x.r_) && x.r_ != r_)
            throw RootError();
         return compare(a_, b_, x.a_, x.b_, r_);
      }
   }

   template <typename T>
   std::enable_if_t<fits_as_particle<T>::value, Int>
   compare(const T& x) const
   {
      if (is_zero(r_))
         return operations::cmp()(a_, x);
      else
         return compare(a_, b_, x, 0, r_);
   }

   template <typename T>
   friend
   std::enable_if_t<fits_as_particle<T>::value, bool>
   operator== (const QuadraticExtension& x, const T& y)
   {
      return is_zero(x.r_) && x.a_ == y;
   }

   template <typename T>
   friend
   std::enable_if_t<fits_as_particle<T>::value, bool>
   operator== (const T& x, const QuadraticExtension& y)
   {
      return y == x;
   }

   friend
   bool operator== (const QuadraticExtension& x, const QuadraticExtension& y)
   {
      return x.a_ == y.a_ && x.b_ == y.b_ && x.r_ == y.r_;
   }

   template <typename T>
   friend
   std::enable_if_t<fits_as_operand<T>::value, bool>
   operator!= (const QuadraticExtension& x, const T& y)
   {
      return !(x == y);
   }

   template <typename T>
   friend
   std::enable_if_t<fits_as_particle<T>::value, bool>
   operator!= (const T& x, const QuadraticExtension& y)
   {
      return !(y == x);
   }

   template <typename T>
   friend
   std::enable_if_t<fits_as_operand<T>::value, bool>
   operator< (const QuadraticExtension& x, const T& y)
   {
      return x.compare(y) < 0;
   }

   template <typename T>
   friend
   std::enable_if_t<fits_as_particle<T>::value, bool>
   operator< (const T& x, const QuadraticExtension& y)
   {
      return y.compare(x) > 0;
   }

   template <typename T>
   friend
   std::enable_if_t<fits_as_operand<T>::value, bool>
   operator> (const QuadraticExtension& x, const T& y)
   {
      return y < x;
   }

   template <typename T>
   friend
   std::enable_if_t<fits_as_particle<T>::value, bool>
   operator> (const T& x, const QuadraticExtension& y)
   {
      return y < x;
   }

   template <typename T>
   friend
   std::enable_if_t<fits_as_operand<T>::value, bool>
   operator<= (const QuadraticExtension& x, const T& y)
   {
      return !(y < x);
   }

   template <typename T>
   friend
   std::enable_if_t<fits_as_particle<T>::value, bool>
   operator<= (const T& x, const QuadraticExtension& y)
   {
      return !(y < x);
   }

   template <typename T>
   friend
   std::enable_if_t<fits_as_operand<T>::value, bool>
   operator>= (const QuadraticExtension& x, const T& y)
   {
      return !(x < y);
   }

   template <typename T>
   friend
   std::enable_if_t<fits_as_particle<T>::value, bool>
   operator>= (const T& x, const QuadraticExtension& y)
   {
      return !(x < y);
   }

   field_type to_field_type() const  // FIXME #569
   {
      return a_ + field_type(b_ * sqrt(AccurateFloat(r_)));
   }

   template <typename T,
             typename = std::enable_if_t<isomorphic_types<T, Field>::value && std::is_constructible<T, Field>::value>>
   explicit operator T() const
   {
      return static_cast<T>(to_field_type());
   }

   explicit operator AccurateFloat() const
   {
      return AccurateFloat(a_) + sqrt(AccurateFloat(r_)) * AccurateFloat(b_);
   }

   template <typename Output>
   friend
   Output& operator<< (GenericOutput<Output>& outs, const QuadraticExtension& x)
   {
      Output& os=outs.top();
      if (!is_zero(x.b_)) {
         const bool need_parens = has_serialized<Field>::value;
         if (need_parens) os << '(';
         os << x.a_;
         if (need_parens) os << ")+(";
         else if (x.b_ > 0) os << '+';
         os << x.b_;
         if (need_parens) os << ')';
         os << 'r' << x.r_;
      } else {
         os << x.a_;
      }
      return os;
   }

   /// the absolute value
   friend
   QuadraticExtension abs(const QuadraticExtension& x)
   {
      return x>=0 ? x : -x;
   }

   friend
   QuadraticExtension&& abs(QuadraticExtension&& x)
   {
      if (x<0) x.negate();
      return std::move(x);
   }

#if POLYMAKE_DEBUG
   void dump() const __attribute__((used)) { cerr << *this << std::flush; }
#endif

protected:
   void normalize()
   {
      const Int i1 = isinf(a_);
      const Int i2 = isinf(b_);
      if (__builtin_expect(i1 || i2, 0)) {
         if (i1+i2 == 0) throw GMP::NaN();
         if (!i1) a_ = b_;
         b_ = zero_value<Field>();
         r_ = zero_value<Field>();
      } else {
         const Int s = sign(r_);
         if (s < 0)
            throw NonOrderableError();
         if (s == 0)
            b_ = zero_value<field_type>();
         else if (is_zero(b_))
            r_ = zero_value<field_type>();
      }
   }


   static
   Int compare(const Field& a, const Field& b, const Field& c, const Field& d, const Field& r)
   {
      const int cmp_a = operations::cmp()(a, c),
                cmp_b = operations::cmp()(b, d);
      if (cmp_a == cmp_b || cmp_a + cmp_b != 0)
         return cmp_a ? cmp_a : cmp_b;

      // Else, we transform
      // cmp(a + b sqrt{r}, c + d sqrt{r})   to
      // cmp(a - c, (d - b) sqrt{r}) where sides have sign cmp_a == -cmp_b.
      field_type lhs(a-c), rhs(d-b);
      lhs *= lhs; rhs *= rhs; rhs *= r;
      return operations::cmp()(lhs, rhs) * cmp_a;
   }

protected:
   field_type a_, b_, r_;

   template <typename>
   friend struct spec_object_traits;
};

template <typename Field>
Int sign(const QuadraticExtension<Field>& x)
{
   const Int sa = sign(x.a()),
             sb = sign(x.b());
   if (sa == sb || sb == 0)
      return sa;
   if (sa == 0)
      return sb;

   Field tmp = x.a() / x.b();
   tmp *= tmp;
   return tmp > x.r() ? sa : sb;
}

template <typename Field>
Integer floor(const QuadraticExtension<Field>& x)
{
   AccurateFloat f(sqrt(AccurateFloat(x.r())));
   f *= x.b(); f += x.a();
   return Integer(floor(f));
}

template <typename Field>
Integer ceil(const QuadraticExtension<Field>& x)
{
   AccurateFloat f(sqrt(AccurateFloat(x.r())));
   f *= x.b(); f += x.a();
   return Integer(ceil(f));
}
   
template <typename Field, typename T,
          typename = std::enable_if_t<QuadraticExtension<Field>::template fits_as_particle<T>::value>>
bool abs_equal(const QuadraticExtension<Field>& x, const T& y)
{
   return is_zero(x.r()) && abs_equal(x.a(), y);
}

template <typename Field, typename T,
          typename = std::enable_if_t<QuadraticExtension<Field>::template fits_as_particle<T>::value>>
bool abs_equal(const T& x, const QuadraticExtension<Field>& y)
{
   return abs_equal(y, x);
}

template <typename Field>
bool abs_equal(const QuadraticExtension<Field>& x, const QuadraticExtension<Field>& y)
{
   return x.r()==y.r() && (x.a()==y.a() && x.b()==y.b()) || (x.a()==-y.a() && x.b()==-y.b());
}

template <typename Field>
bool isfinite(const QuadraticExtension<Field>& x) noexcept
{
   return isfinite(x.a());
}

template <typename Field>
Int isinf(const QuadraticExtension<Field>& x) noexcept
{
   return isinf(x.a());
}

template <typename Field>
struct spec_object_traits<QuadraticExtension<Field>>
   : spec_object_traits<is_scalar> {

   static
   bool is_zero(const QuadraticExtension<Field>& x)
   {
      return pm::is_zero(x.a()) && pm::is_zero(x.r());
   }

   static
   bool is_one(const QuadraticExtension<Field>& x)
   {
      return pm::is_one(x.a()) && pm::is_zero(x.r());
   }

   static const QuadraticExtension<Field>& zero()
   {
      static const QuadraticExtension<Field> qe_zero(0);
      return qe_zero;
   }

   static const QuadraticExtension<Field>& one()
   {
      static const QuadraticExtension<Field> qe_one(1);
      return qe_one;
   }
};

// fast prime factorization via bundled flint
#ifdef POLYMAKE_WITH_FLINT
template < > 
	void QuadraticExtension<Rational>::normalize();
#endif

template <typename Field>
struct spec_object_traits< Serialized< QuadraticExtension<Field> > > :
   spec_object_traits<is_composite> {

   typedef QuadraticExtension<Field> masquerade_for;

   typedef cons<Field, cons<Field, Field> > elements;

   template <typename Me, typename Visitor>
   static void visit_elements(Me& me, Visitor& v)
   {
      v << me.a_ << me.b_ << me.r_;
      normalize(me);
   }
private:
   static void normalize(const QuadraticExtension<Field>&) {}
   static void normalize(QuadraticExtension<Field>& me) { me.normalize(); }
};

template <typename Field>
struct hash_func<QuadraticExtension<Field>, is_scalar> : hash_func<Field> {
   typedef hash_func<Field> base_t;
protected:
   size_t impl(const QuadraticExtension<Field>& x) const
   {
      size_t h=base_t::operator()(x.a());
      hash_combine(h, base_t::operator()(x.b()));
      return h;
   }
public:
   size_t operator() (const QuadraticExtension<Field>& x) const { return isfinite(x) ? impl(x) : 0; }
};


template <typename Field>
struct algebraic_traits< QuadraticExtension<Field> > {
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
      static pm::QuadraticExtension<Field> min() noexcept(noexcept(numeric_limits<Field>::min()))
      {
         return pm::QuadraticExtension<Field>(numeric_limits<Field>::min());
      }
      static pm::QuadraticExtension<Field> max() noexcept(noexcept(numeric_limits<Field>::max()))
      {
         return pm::QuadraticExtension<Field>(numeric_limits<Field>::max());
      }
      static pm::QuadraticExtension<Field> infinity() noexcept(noexcept(numeric_limits<Field>::infinity()))
      {
         return pm::QuadraticExtension<Field>(numeric_limits<Field>::infinity());
      }
   };
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
