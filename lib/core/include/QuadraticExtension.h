/* Copyright (c) 1997-2017
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
int sign(const QuadraticExtension<Field>&);

/** @class QuadraticExtension
    @brief Realizes quadratic extensions of fields
 */
template <typename Field=Rational>
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
      : _a()
      , _b()
      , _r() {}

   template <typename T1, typename T2, typename T3,
             typename=typename std::enable_if<can_initialize_particle<T1>::value &&
                                              can_initialize_particle<T2>::value &&
                                              can_initialize_particle<T3>::value>::type>
      QuadraticExtension(T1&& a, T2&& b, T3&& r)
      : _a(std::forward<T1>(a))
      , _b(std::forward<T2>(b))
      , _r(std::forward<T3>(r))
   {
      normalize();
   }

   template <typename T, typename=typename std::enable_if<can_initialize_particle<T>::value>::type>
   explicit QuadraticExtension(T&& a)
      : _a(std::forward<T>(a))
      , _b()
      , _r() {}

   QuadraticExtension(const QuadraticExtension&) = default;
   QuadraticExtension(QuadraticExtension&&) = default;
   QuadraticExtension& operator= (const QuadraticExtension&) = default;
   QuadraticExtension& operator= (QuadraticExtension&&) = default;

   template <typename T,
             typename=typename std::enable_if<can_assign_particle<T>::value>::type>
   QuadraticExtension& operator= (T&& a)
   {
      _a = std::forward<T>(a);
      _b = zero_value<Field>();
      _r = zero_value<Field>();
      return *this;
   }

   template <typename T,
             typename=typename std::enable_if<can_assign_particle<T>::value>::type>
   QuadraticExtension& operator= (std::initializer_list<T> l)
   {
      if (l.size() != 3) throw std::runtime_error("initializer list must have 3 elements");
      const T* val=l.begin();
      _a = val[0];
      _b = val[1];
      _r = val[2];
      normalize();
      return *this;
   }

   const Field& a() const { return _a; }
   const Field& b() const { return _b; }
   const Field& r() const { return _r; }

   void swap (QuadraticExtension& op)
   {
      std::swap(_a, op._a);
      std::swap(_b, op._b);
      std::swap(_r, op._r);
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
      _a.negate();
      _b.negate();
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
      _b.negate();
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
      return _a*_a - _b*_b*_r;
   }

   template <typename T, typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   QuadraticExtension&
   operator+= (const T& x)
   {
      _a += x;
      if (__builtin_expect(!isfinite(x), 0)) {
         _b = zero_value<Field>();
         _r = zero_value<Field>();
      }
      return *this;
   }

   QuadraticExtension& operator+= (const QuadraticExtension& x)
   {
      if (is_zero(x._r)) {
         *this += x._a;
      } else {
         if (is_zero(_r)) {
            if (__builtin_expect(isfinite(_a), 1)) {
               _b = x._b;
               _r = x._r;
            }
         } else {
            if (x._r != _r) throw RootError();
            _b += x._b;
            if (is_zero(_b)) _r=zero_value<Field>();
         }
         _a += x._a; 
      }
      return *this;
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
   friend
   QuadraticExtension operator+ (const QuadraticExtension& x, const T& y)
   {
      QuadraticExtension result(x);
      result += y;
      return result;
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
   friend
   QuadraticExtension&& operator+ (QuadraticExtension&& x, const T& y)
   {
      return std::move(x += y);
   }

   template <typename T, typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   QuadraticExtension operator+ (const T& x, const QuadraticExtension& y)
   {
      return y + x;
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
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


   template <typename T, typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   QuadraticExtension&
   operator-= (const T& x)
   {
      _a -= x;
      if (__builtin_expect(!isfinite(x), 0)) {
         _b = zero_value<Field>();
         _r = zero_value<Field>();
      }
      return *this;
   }

   QuadraticExtension& operator-= (const QuadraticExtension& x)
   {
      if (is_zero(x._r)) {
         *this -= x._a;
      } else {
         if (is_zero(_r)) {
            if (__builtin_expect(isfinite(_a), 1)) {
               _b -= x._b;
               _r = x._r;
            }
         } else {
            if (x._r != _r) throw RootError();
            _b -= x._b;
            if (is_zero(_b)) _r=zero_value<Field>();
         }
         _a -= x._a;
      }
      return *this;
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
   friend
   QuadraticExtension operator- (const QuadraticExtension& x, const T& y)
   {
      QuadraticExtension result(x);
      result -= y;
      return result;
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
   friend
   QuadraticExtension&& operator- (QuadraticExtension&& x, const T& y)
   {
      return std::move(x -= y);
   }

   template <typename T, typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   QuadraticExtension operator- (const T& x, const QuadraticExtension& y)
   {
      QuadraticExtension result(y);
      return (result -= x).negate();
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
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


   template <typename T, typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   QuadraticExtension&
   operator*= (const T& x)
   {
      if (is_zero(_r)) {
         _a *= x;
      } else if (__builtin_expect(isfinite(x), 1)) {
         if (is_zero(x)) {
            *this=x;
         } else {
            _a *= x;
            _b *= x;
         }
      } else {
         *this= sign(*this)<0 ? -x : x;
      }
      return *this;
   }

   QuadraticExtension& operator*= (const QuadraticExtension& x)
   {
      if (is_zero(x._r)) {
         *this *= x._a;
      } else if (is_zero(_r)) {
         if (__builtin_expect(isfinite(_a), 1)) {
            if (!is_zero(_a)) {
               _b = _a * x._b;
               _a *= x._a;
               _r = x._r;
            }
         } else if (sign(x)<0) {
            _a.negate();
         }
      } else {
         if (x._r != _r) throw RootError();
         const Field tmp = _a * x._b;
         _a *= x._a;  _a += _b * x._b * _r;
         _b *= x._a;  _b += tmp;
         if (is_zero(_b)) _r=zero_value<Field>();
      }
      return *this;
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
   friend
   QuadraticExtension operator* (const QuadraticExtension& x, const T& y)
   {
      QuadraticExtension result(x);
      result *= y;
      return result;
   }

   template <typename T, typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   QuadraticExtension operator* (const T& x, const QuadraticExtension& y)
   {
      return y * x;
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
   friend
   QuadraticExtension&& operator* (QuadraticExtension&& x, const T& y)
   {
      return std::move(x *= y);
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
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


   template <typename T, typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   QuadraticExtension& operator/= (const T& x)
   {
      if (std::is_arithmetic<Field>::value && is_zero(x))  // complex numeric types check for zero division themselves
         throw GMP::ZeroDivide();
      _a /= x;
      if (__builtin_expect(isfinite(x), 1)) {
         _b /= x;
      } else if (!is_zero(_r)) {
         _b = zero_value<Field>();
         _r = zero_value<Field>();
      }
      return *this;
   }

   QuadraticExtension& operator/= (const QuadraticExtension& x)
   {
      if (is_zero(x._r)) {
         *this /= x._a;
      } else if (is_zero(_r)) {
         if (__builtin_expect(isfinite(_a), 1)) {
            if (__builtin_expect(!is_zero(_a), 1)) {
               _a /= x.norm();
               _b = -(_a * x._b);
               _a *= x._a;
               _r = x._r;
            }
         } else if (sign(x)<0) {
            _a.negate();
         }
      } else {
         if (x._r != _r) throw RootError();
         // *this = *this * conjugate(x) / x.norm();
         const Field n = x.norm();
         _a /= n;  _b /= n;
         const Field tmp= _a * x._b;
         _a *= x._a;  _a -= _b * x._b * _r;
         _b *= x._a;  _b -= tmp;
         if (is_zero(_b)) _r = zero_value<Field>();
      }
      return *this;
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
   friend
   QuadraticExtension operator/ (const QuadraticExtension& x, const T& y)
   {
      QuadraticExtension result(x);
      result /= y;
      return result;
   }

   template <typename T, typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   QuadraticExtension operator/ (const T& x, const QuadraticExtension& y)
   {
      QuadraticExtension result(x);
      result /= y;
      return result;
   }


   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
   friend
   QuadraticExtension&& operator/ (QuadraticExtension&& x, const T& y)
   {
      return std::move(x /= y);
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
   friend
   QuadraticExtension&& operator/ (const T& x, QuadraticExtension&& y)
   {
      if (is_zero(y))
         throw GMP::ZeroDivide();
      if (__builtin_expect(isfinite(y._a), 1)) {
         if (is_zero(x)) {
            y=x;
         } else {
            y.conjugate();
            y /= y.norm();
            y *= x;
         }
      } else {
         y._a=zero_value<Field>();
      }
      return std::move(y);
   }

   friend
   QuadraticExtension&& operator/ (QuadraticExtension&& x, QuadraticExtension&& y)
   {
      return std::move(x /= y);
   }

   int compare(const QuadraticExtension& x) const
   {
      if (is_zero(_r)) {
         if (is_zero(x._r)) {
            return operations::cmp()(_a, x._a);
         } else {
            return compare(_a, _b, x._a, x._b, x._r);
         }
      } else {
         if (!is_zero(x._r) && x._r != _r)
            throw RootError();
         return compare(_a, _b, x._a, x._b, _r);
      }
   }

   template <typename T, typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   int compare(const T& x) const
   {
      if (is_zero(_r))
         return operations::cmp()(_a, x);
      else
         return compare(_a, _b, x, 0, _r);
   }

   template <typename T, typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   bool operator== (const QuadraticExtension& x, const T& y)
   {
      return is_zero(x._r) && x._a==y;
   }

   template <typename T, typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   bool operator== (const T& x, const QuadraticExtension& y)
   {
      return y==x;
   }

   friend
   bool operator== (const QuadraticExtension& x, const QuadraticExtension& y)
   {
      return x._a==y._a && x._b==y._b && x._r==y._r;
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
   friend
   bool operator!= (const QuadraticExtension& x, const T& y)
   {
      return !(x==y);
   }

   template <typename T, typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   bool operator!= (const T& x, const QuadraticExtension& y)
   {
      return !(y==x);
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
   friend
   bool operator< (const QuadraticExtension& x, const T& y)
   {
      return x.compare(y)<0;
   }

   template <typename T, typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   bool operator< (const T& x, const QuadraticExtension& y)
   {
      return y.compare(x)>0;
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
   friend
   bool operator> (const QuadraticExtension& x, const T& y)
   {
      return y < x;
   }

   template <typename T, typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   bool operator> (const T& x, const QuadraticExtension& y)
   {
      return y < x;
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
   friend
   bool operator<= (const QuadraticExtension& x, const T& y)
   {
      return !(y < x);
   }

   template <typename T, typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   bool operator<= (const T& x, const QuadraticExtension& y)
   {
      return !(y < x);
   }

   template <typename T, typename=typename std::enable_if<fits_as_operand<T>::value>::type>
   friend
   bool operator>= (const QuadraticExtension& x, const T& y)
   {
      return !(x < y);
   }

   template <typename T, typename=typename std::enable_if<fits_as_particle<T>::value>::type>
   friend
   bool operator>= (const T& x, const QuadraticExtension& y)
   {
      return !(x < y);
   }

   field_type to_field_type() const  // FIXME #569
   {
      return _a + field_type(_b * sqrt(AccurateFloat(_r)));
   }

   template <typename T,
             typename=typename std::enable_if<isomorphic_types<T, Field>::value && std::is_constructible<T, Field>::value>::type>
   explicit operator T() const
   {
      return static_cast<T>(to_field_type());
   }

   explicit operator AccurateFloat() const
   {
      return AccurateFloat(_a) + sqrt(AccurateFloat(_r)) * AccurateFloat(_b);
   }

   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& outs, const QuadraticExtension& x)
   {
      Output& os=outs.top();
      if (!is_zero(x._b)) {
         const bool need_parens = has_serialized<Field>::value;
         if (need_parens) os << '(';
         os << x._a;
         if (need_parens) os << ")+(";
         else if (x._b > 0) os << '+';
         os << x._b;
         if (need_parens) os << ')';
         os << 'r' << x._r;
      } else {
         os << x._a;
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
      const int i1=isinf(_a);
      const int i2=isinf(_b);
      if (__builtin_expect(i1 || i2, 0)) {
         if (i1+i2 == 0) throw GMP::NaN();
         if (!i1) _a=_b;
         _b=zero_value<Field>();
         _r=zero_value<Field>();
      } else {
         switch (sign(_r)) {
         case -1:
            throw NonOrderableError();
         case 0:
            _b=zero_value<field_type>();
            break;
         default:
            if (is_zero(_b)) _r=zero_value<field_type>();
            break;
         }
      }
   }

   static
   int compare(const Field& a, const Field& b, const Field& c, const Field& d, const Field& r)
   {
      const int cmp_a = operations::cmp()(a, c),
                cmp_b = operations::cmp()(b, d);
      if (cmp_a == cmp_b || cmp_a + cmp_b != 0)
         return cmp_a ? cmp_a : cmp_b;

      // Else, we transform
      // cmp(a + b sqrt{r}, c + d sqrt{r})   to
      // cmp(a - c, (d - b) sqrt{r}) where sides have sign cmp_a == -cmp_b.
      field_type lhs(a - c), rhs(d - b);
      lhs *= lhs; rhs *= rhs; rhs *= r;
      return operations::cmp()(lhs, rhs) * cmp_a;
   }

protected:
   field_type _a, _b, _r;

   template <typename>
   friend struct spec_object_traits;
};

template <typename Field>
int sign(const QuadraticExtension<Field>& x)
{
   const int sa=sign(x.a()),
             sb=sign(x.b());
   if (sa == sb || sb == 0)
      return sa;
   if (sa == 0)
      return sb;

   Field tmp = x.a() / x.b();
   tmp *= tmp;
   return tmp > x.r() ? sa : sb;
}

template <typename Field, typename T,
          typename=typename std::enable_if<QuadraticExtension<Field>::template fits_as_particle<T>::value>::type>
inline
bool abs_equal(const QuadraticExtension<Field>& x, const T& y)
{
   return is_zero(x.r()) && abs_equal(x.a(), y);
}

template <typename Field, typename T,
          typename=typename std::enable_if<QuadraticExtension<Field>::template fits_as_particle<T>::value>::type>
inline
bool abs_equal(const T& x, const QuadraticExtension<Field>& y)
{
   return abs_equal(y, x);
}

template <typename Field>
inline
bool abs_equal(const QuadraticExtension<Field>& x, const QuadraticExtension<Field>& y)
{
   return x.r()==y.r() && (x.a()==y.a() && x.b()==y.b()) || (x.a()==-y.a() && x.b()==-y.b());
}

template <typename Field>
inline
bool isfinite(const QuadraticExtension<Field>& x) noexcept
{
   return isfinite(x.a());
}

template <typename Field>
inline
int isinf(const QuadraticExtension<Field>& x) noexcept
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

template <typename Field>
struct spec_object_traits< Serialized< QuadraticExtension<Field> > > :
   spec_object_traits<is_composite> {

   typedef QuadraticExtension<Field> masquerade_for;

   typedef cons<Field, cons<Field, Field> > elements;

   template <typename Me, typename Visitor>
   static void visit_elements(Me& me, Visitor& v)
   {
      v << me._a << me._b << me._r;
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

#endif // POLYMAKE_QUADRATIC_EXTENSION_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
