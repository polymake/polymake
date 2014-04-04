/* Copyright (c) 1997-2014
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

// fragments borrowed from read_int() of GNU libio
size_t Integer::strsize(const std::ios::fmtflags flags) const
{
   size_t s=1 + ((flags & std::ios::showpos) || (mpz_sgn(rep)<0));      // terminating '\0' and possible sign
   if (__builtin_expect(!isfinite(*this),0)) return s+3;
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
   return s+mpz_sizeinbase(rep, base);
}

void Integer::putstr(std::ios::fmtflags flags, char* buf) const
{
   const int i=isinf(*this);
   if (__builtin_expect(i,0)) {
      if (i<0)
         strcpy(buf,"-inf");
      else if (flags & std::ios::showpos)
         strcpy(buf,"+inf");
      else
         strcpy(buf,"inf");
      return;
   }
   int base;
   const int plus= flags & std::ios::showpos ? mpz_sgn(rep)>0 : 0;
   switch (flags & (std::ios::basefield | std::ios::showbase)) {
   case int(std::ios::hex) | int(std::ios::showbase):
      mpz_get_str(buf+2+plus, 16, rep);
      if (mpz_sgn(rep)<0) *buf++='-'; else if (plus) *buf++='+';
      *buf++='0';
      *buf='x';
      return;
   case int(std::ios::oct) | int(std::ios::showbase):
      mpz_get_str(buf+1+plus, 8, rep);
      if (mpz_sgn(rep)<0) *buf++='-'; else if (plus) *buf++='+';
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
   mpz_get_str(buf, base, rep);
}

std::string Integer::to_string(int base) const
{
   const int i=isinf(*this);
   if (__builtin_expect(i,0)) {
      return i>0 ? "inf" : "-inf";
   }
   std::string s(mpz_sizeinbase(rep,base)+2, '\0');
   mpz_get_str(const_cast<char*>(s.data()), base, rep); // a nasty cast, I know...
   s.resize(s.find('\0'));
   return s;
}

void Integer::_set(const char *s)
{
   if (mpz_set_str(rep,s,0) < 0) {
      if (s[0]=='+' ? !strcmp(s+1,"inf") : !strcmp(s,"inf"))
         _set_inf(rep,1);
      else if (s[0]=='-' && !strcmp(s+1,"inf"))
         _set_inf(rep,-1);
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
               _set_inf(rep,sgn);
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
            mpz_set_str(rep, text.c_str(), base);
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
   if (k<0) return Integer(0);
   if (n<0) {
      const Integer nn=(k-1)-n;
      if (k%2==0) {
         return Integer(mpz_bin_ui, nn.rep, (unsigned long)k);
      } else {
         return -Integer(mpz_bin_ui, nn.rep, (unsigned long)k);
      }
   }
   if (__builtin_expect(isfinite(n),1))
      return Integer(mpz_bin_ui, n.rep, (unsigned long)k);
   return Integer(maximal<Integer>(), 1);
}

Integer Integer::binom(long n, long k)
{
   if (k<0) return Integer(0);
   if (n<0) {
      const long nn=k-1-n;
      if (k%2==0) {
         return Integer(mpz_bin_uiui, (unsigned long)nn, (unsigned long)k);
      } else {
         return -Integer(mpz_bin_uiui, (unsigned long)nn, (unsigned long)k);
      }
   }
   return Integer(mpz_bin_uiui, (unsigned long)n, (unsigned long)k);
}

bool Integer::fill_from_file(int fd)
{
   int consumed=0, s, total=rep[0]._mp_alloc * sizeof(mp_limb_t);
   char* d=reinterpret_cast<char*>(const_cast<mp_limb_t*>(rep[0]._mp_d));
   do {
      s=::read(fd, d+consumed, total-consumed);
      if (s<0) return false;
   } while ((consumed+=s)<total);
   rep[0]._mp_size=rep[0]._mp_alloc;
   return true;
}

namespace {

mp_limb_t limb0=0, limb1=1;
const mpz_t mpz_zero_c={{1, 0, &limb0}};
const mpz_t mpz_one_c ={{1, 1, &limb1}};

}

const Integer& spec_object_traits<Integer>::zero()
{
   return *reverse_cast(mpz_zero_c, 0, &Integer::rep);
}

const Integer& spec_object_traits<Integer>::one()
{
   return *reverse_cast(mpz_one_c, 0, &Integer::rep);
}

namespace GMP {
   NaN::NaN() : error("Integer/Rational NaN") {}

   ZeroDivide::ZeroDivide() : error("Integer/Rational zero division") {}
}

}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
