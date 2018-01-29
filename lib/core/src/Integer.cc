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

#include "polymake/Integer.h"
#include <cstring>
#include <unistd.h>

#pragma GCC diagnostic ignored "-Wswitch"

namespace pm {

Integer& Integer::operator= (long long b)
{
   if (sizeof(long long)==sizeof(long) || (b >= std::numeric_limits<long>::min() && b <= std::numeric_limits<long>::max())) {
      *this=long(b);
   } else {
      typedef std::conditional<sizeof(long long)==sizeof(long), int, long>::type shift_by;
      if (__builtin_expect(!isfinite(*this), 0))
         mpz_init2(this, sizeof(long long)*8);
      mpz_set_si(this, b >> (sizeof(shift_by)*8));
      mpz_mul_2exp(this, this, sizeof(shift_by)*8);
      mpz_add_ui(this, this, b & ULONG_MAX);
   }
   return *this;
}

Integer::operator long long() const
{
   if (sizeof(long long)==sizeof(long) || !isfinite(*this) || mpz_fits_slong_p(this)) {
      return long(*this);
   } else {
      typedef std::conditional<sizeof(long long)==sizeof(long), int, long>::type shift_by;
      Integer tmp= *this >> int(sizeof(long)*8);
      long long result=long(tmp);
      result <<= (sizeof(shift_by)*8);
      result += mpz_get_ui(this);
      return result;
   }
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
      if (isinf(*this)<0)
         strcpy(buf, "-inf");
      else if (flags & std::ios::showpos)
         strcpy(buf, "+inf");
      else
         strcpy(buf, "inf");
      return;
   }
   int base;
   const int plus= flags & std::ios::showpos ? mpz_sgn(this)>0 : 0;
   switch (flags & (std::ios::basefield | std::ios::showbase)) {
   case int(std::ios::hex) | int(std::ios::showbase):
      mpz_get_str(buf+2+plus, 16, this);
      if (mpz_sgn(this)<0) *buf++='-'; else if (plus) *buf++='+';
      *buf++='0';
      *buf='x';
      return;
   case int(std::ios::oct) | int(std::ios::showbase):
      mpz_get_str(buf+1+plus, 8, this);
      if (mpz_sgn(this)<0) *buf++='-'; else if (plus) *buf++='+';
      *buf='0';
      return;
   case std::ios::hex:
      base=16;
      break;
   case std::ios::oct:
      base=8;
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
   int consumed=0, s, total=_mp_alloc * sizeof(mp_limb_t);
   char* d=reinterpret_cast<char*>(_mp_d);
   do {
      s=::read(fd, d+consumed, total-consumed);
      if (s<0) return false;
   } while ((consumed+=s)<total);
   _mp_size=_mp_alloc;
   return true;
}

template <>
Integer
pow(const Integer& base, int exp)
{
   return Integer::pow(base,exp);
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

   BadCast::BadCast() : error("Integer/Rational number is too big for the cast to long/int") {}
}

}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
