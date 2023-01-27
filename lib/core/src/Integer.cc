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

#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include <cstring>
#include <unistd.h>

#pragma GCC diagnostic ignored "-Wswitch"

namespace pm {

#if POLYMAKE_NEED_LONG_LONG_GMP_WRAPPERS

namespace {

int set_limbs(mp_limb_t* limbs, long long x)
{
   if (x >= 0) {
      limbs[0] = x & ULONG_MAX;
      limbs[1] = (x >> 32) & ULONG_MAX;
      return 1 + (limbs[1] != 0);
   } else {
      x = -x;
      limbs[0] = x & ULONG_MAX;
      limbs[1] = (x >> 32) & ULONG_MAX;
      return -1 - (limbs[1] != 0);
   }
}

}

Integer::Integer(long long b)
{
   if (b < LONG_MIN || b > LONG_MAX) {
      mpz_init2(this, sizeof(b)*8);
      _mp_size = set_limbs(_mp_d, b);
   } else {
      mpz_init_set_si(this, static_cast<long>(b));
   }
}

Integer& Integer::operator= (long long b)
{
   if (b < LONG_MIN || b > LONG_MAX) {
      if (__builtin_expect(_mp_d != nullptr, 1)) {
         if (_mp_size >= -1 && _mp_size <= 1)
            mpz_realloc2(this, sizeof(b)*8);
      } else {
         mpz_init2(this, sizeof(b)*8);
      }
      _mp_size = set_limbs(_mp_d, b);
   } else {
      operator=(static_cast<long>(b));
   }
   return *this;
}

Integer::operator long long() const
{
   if (isfinite(*this)) {
      if (mpz_fits_slong_p(this))
         return mpz_get_si(this);
      if (_mp_size == 1 || _mp_size == -1) {
         long long result = _mp_d[0];
         return _mp_size > 0 ? result : -result;
      }
      if (_mp_size == 2 || _mp_size == -2) {
         long long result = _mp_d[1];
         if (!(result & (1UL << 31))) {
            result <<= 32;
            result |= _mp_d[0];
            return _mp_size > 0 ? result : -result;
         }
      }
   }
   throw GMP::BadCast();
}

bool Integer::fits_into_long_long() const
{
   if (isfinite(*this)) {
      const int s = std::abs(_mp_size);
      return s <= 1 || (s == 2 && !(_mp_d[1] & (1UL << 31)));
   }
   return false;
}

#endif

Integer Integer::pow(const Integer& a, long k)
{
   if (__builtin_expect(k < 0, 0))
      throw GMP::NaN();
   Integer result;
   if (__builtin_expect(isfinite(a), 1))
      mpz_pow_ui(&result, &a, k);
   else if (k != 0)
      set_inf(&result, k%2 ? mpz_sgn(&a) : 1);
   else
      throw GMP::NaN();
   return result;
}

Integer Integer::pow(long a, long k)
{
   if (__builtin_expect(k < 0, 0))
      throw GMP::NaN();
   Integer result;
   if (a >= 0) {
      mpz_ui_pow_ui(&result, a, k);
   } else {
      mpz_ui_pow_ui(&result, -a, k);
      result.negate();
   }
   return result;
}


// fragments borrowed from read_int() of GNU libio
size_t Integer::strsize(const std::ios::fmtflags flags) const
{
   size_t s=1 + ((flags & std::ios::showpos) || (mpz_sgn(this)<0));      // terminating '\0' and possible sign
   if (__builtin_expect(!isfinite(*this), 0))
      return s+3;
   int base;
   switch (flags & (std::ios::basefield | std::ios::showbase)) {
   case int(std::ios::hex) | int(std::ios::showbase):
      s+=2;
   case std::ios::hex:
      base=16;
      break;
   case int(std::ios::oct) | int(std::ios::showbase):
      s+=1;
   case std::ios::oct:
      base=8;
      break;
   default:
      base=10;
   }
   return s+mpz_sizeinbase(this, base);
}

void Integer::putstr(std::ios::fmtflags flags, char* buf) const
{
   if (__builtin_expect(!isfinite(*this), 0)) {
      if (isinf(*this) < 0)
         strcpy(buf, "-inf");
      else if (flags & std::ios::showpos)
         strcpy(buf, "+inf");
      else
         strcpy(buf, "inf");
      return;
   }
   int base;
   const int plus = flags & std::ios::showpos ? mpz_sgn(this) > 0 : 0;
   switch (flags & (std::ios::basefield | std::ios::showbase)) {
   case int(std::ios::hex) | int(std::ios::showbase):
      mpz_get_str(buf+2+plus, 16, this);
      if (mpz_sgn(this)<0) *buf++='-'; else if (plus) *buf++='+';
      *buf++ = '0';
      *buf = 'x';
      return;
   case int(std::ios::oct) | int(std::ios::showbase):
      mpz_get_str(buf+1+plus, 8, this);
      if (mpz_sgn(this)<0) *buf++='-'; else if (plus) *buf++='+';
      *buf = '0';
      return;
   case std::ios::hex:
      base = 16;
      break;
   case std::ios::oct:
      base = 8;
      break;
   default:
      base=10;
   }
   if (plus) *buf++='+';
   mpz_get_str(buf, base, this);
}

Integer::Integer(const char *s)
{
   mpz_init(this);
   try {
      parse(s);
   }
   catch (const GMP::error&) {
      mpz_clear(this);
      throw;
   }
}

void Integer::parse(const char *s)
{
   if (mpz_set_str(this, s, 0) < 0) {
      if (s[0]=='+' ? !strcmp(s+1, "inf") : !strcmp(s, "inf"))
         set_inf(this, 1);
      else if (s[0]=='-' && !strcmp(s+1, "inf"))
         set_inf(this, -1);
      else
         throw GMP::error("Integer: syntax error");
   }
}

void Integer::read(std::istream& is, bool allow_sign)
{
   std::ios::iostate exc=is.exceptions();
   is.exceptions(std::ios::goodbit);

   std::string text;
   char c=0;
   int sgn=1;

   if (allow_sign) {
      is >> c;          // skip leading whitespaces, consume the sign or the first digit
      switch (c) {
      case '-':
         text += c;
         sgn=-1;
         /* NOBREAK */
      case '+':
         is >> c;       // there may be some spaces after the sign, too
      }
   } else {
      is.get(c);
   }
   if (is.eof()) {
      is.setstate(std::ios::failbit);
   } else {
      bool valid=false;
      int base=0;
      switch (is.flags() & std::ios::basefield) {
      case std::ios::hex:
         base=16; break;
      case std::ios::oct:
         base=8;  break;
      case std::ios::dec:
         base=10; break;
      default:          // the base is to be guessed from the prefix
         if (c == '0') {
            is.get(c);
            if (c == 'x' || c == 'X') {
               base=16;
               is.get(c);
            } else {
               text += '0';
               valid=true;
               base=8;
            }
         } else {
            base=10;
         }
      }

      // check for infinity value
      if (c=='i') {
         if (is.peek()=='n') {
            is.get();
            if (is.peek()=='f') {
               is.get();
               set_inf(this, sgn);
               valid=true;
            } else {
               is.unget();
            }
         } else {
            is.unget();
         }
      } else {
         // gather all feasible characters
         while (!is.eof()) {
            if (isdigit(c)
                ? (base==8 && c > '7')
                : !isalpha(c) || base != 16 || (isupper(c) ? c > 'F' : c > 'f')) {
               is.unget();
               break;
            }
            text += c;
            valid=true;
            is.get(c);
         }

         if (valid)
            mpz_set_str(this, text.c_str(), base);
      }

      if (valid)
         is.clear(is.rdstate() & std::ios::eofbit);
      else
         is.setstate(std::ios::failbit);
   }
   is.exceptions(exc);
}

Integer Integer::binom(const Integer& n, long k)
{
   Integer result;
   if (__builtin_expect(k>=0, 1)) {
      if (__builtin_expect(isfinite(n), 1)) {
         if (__builtin_expect(n>=0, 1)) {
            mpz_bin_ui(&result, &n, k);
         } else {
            const Integer nn=(k-1)-n;
            mpz_bin_ui(&result, &nn, k);
            if (k%2) result.negate();
         }
      } else {
         set_inf(&result, n);
      }
   }
   return result;
}

Integer Integer::binom(long n, long k)
{
   Integer result;
   if (__builtin_expect(k>=0, 1)) {
      if (__builtin_expect(n>=0, 1)) {
         mpz_bin_uiui(&result, n, k);
      } else {
         const long nn=k-1-n;
         mpz_bin_uiui(&result, nn, k);
         if (k%2) result.negate();
      }
   }
   return result;
}

bool Integer::fill_from_file(int fd)
{
   std::streamsize consumed = 0, s, total = _mp_alloc * sizeof(mp_limb_t);
   char* d = reinterpret_cast<char*>(_mp_d);
   do {
      s = ::read(fd, d+consumed, total-consumed);
      if (s < 0)
         return false;
   } while ((consumed += s) < total);
   _mp_size = _mp_alloc;
   return true;
}

Integer pow(const Integer& base, long exp)
{
   if (exp >= 0)
      return Integer::pow(base,exp);
   else
      return Integer(Rational::pow(base,exp));
}

namespace {

mp_limb_t limb0=0, limb1=1;
const __mpz_struct mpz_zero_c{1, 0, &limb0};
const __mpz_struct mpz_one_c {1, 1, &limb1};

}

const Integer& spec_object_traits<Integer>::zero()
{
   return static_cast<const Integer&>(mpz_zero_c);
}

const Integer& spec_object_traits<Integer>::one()
{
   return static_cast<const Integer&>(mpz_one_c);
}

namespace GMP {
   NaN::NaN() : error("Integer/Rational NaN") {}

   ZeroDivide::ZeroDivide() : error("Integer/Rational zero division") {}

   BadCast::BadCast() : error("Integer/Rational number is too big for the cast to Int") {}
}

}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
