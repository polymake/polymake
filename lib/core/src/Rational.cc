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

#include "polymake/Rational.h"

namespace pm {

void Rational::_set(const char* s)
{
   if (const char* den=strchr(s,'/')) {
      const int numerator_digits=den-s;
      if (!numerator_digits)
         throw GMP::error("Rational: empty numerator");
      ++den;
      if (!isdigit(*den))
         throw GMP::error("Rational: syntax error in denominator");

#ifdef __gnu_linux__
      char *num=strndup(s,numerator_digits);
      if (!num) throw std::bad_alloc();
#else
      char *num=(char*)malloc(numerator_digits+1);
      if (!num) throw std::bad_alloc();
      std::memcpy(num,s,numerator_digits);
      num[numerator_digits]=0;
#endif
      if (mpz_set_str(mpq_numref(rep),num,0) < 0) {
         free(num);
         throw GMP::error("Rational: syntax error in numerator");
      }
      free(num);
      if (mpz_set_str(mpq_denref(rep),den,0) < 0)
         throw GMP::error("Rational: syntax error in denominator");
      canonicalize();

   } else if (const char* point=strchr(s,'.')) {
      const int before_pt=point-s;
      int after_pt=0;
      ++point;
      int trailing=0;
      while (isdigit(point[after_pt])) {
         if (point[after_pt]!='0') trailing=after_pt+1;
         ++after_pt;
      }
      char *num=(char*)malloc(before_pt+trailing+1);
      if (!num) throw std::bad_alloc();
      if (before_pt) std::memcpy(num,s,before_pt);
      if (trailing) std::memcpy(num+before_pt,point,trailing);
      num[before_pt+trailing]=0;
      if (mpz_set_str(mpq_numref(rep),num,10) < 0) {
         free(num);
         throw GMP::error("Rational: syntax error");
      }
      free(num);
      if (trailing) {
         mpz_ui_pow_ui(mpq_denref(rep),10,trailing);
         canonicalize();
      } else {
         mpz_set_ui(mpq_denref(rep),1);
      }

   } else {
      if (mpz_set_str(mpq_numref(rep),s,0) < 0) {
         if (s[0]=='+' ? !strcmp(s+1,"inf") : !strcmp(s,"inf"))
            _set_inf(rep,1);
         else if (s[0]=='-' && !strcmp(s+1,"inf"))
            _set_inf(rep,-1);
         else
            throw GMP::error("Rational: syntax error");
      }
      mpz_set_ui(mpq_denref(rep),1);
   }
}

std::string Rational::to_string(int base) const
{
   if (!mpz_cmp_ui(mpq_denref(rep),1))
      return numerator(*this).to_string();
   std::string s(mpz_sizeinbase(mpq_numref(rep), base)+2 +      // numerator with possible sign and slash
                 mpz_sizeinbase(mpq_denref(rep), base)+1,       // denominator and terminating '\0'
                 '\0');
   char *buf=const_cast<char*>(s.data());
   mpz_get_str(buf,base,mpq_numref(rep));
   buf+=strlen(buf);
   *buf++='/';
   mpz_get_str(buf,base,mpq_denref(rep));
   s.resize(s.find('\0'));
   return s;
}

void Rational::putstr(std::ios::fmtflags flags, char *buf, bool show_den) const
{
   numerator(*this).putstr(flags, buf);
   if (show_den) {
      buf+=strlen(buf);
      *buf++ = '/';
      denominator(*this).putstr(flags & ~std::ios::showpos, buf);
   }
}

namespace {

mp_limb_t limb0=0, limb1=1;
const mpq_t mpq_zero_c={{{1, 0, &limb0}, {1, 1, &limb1}}};
const mpq_t mpq_one_c ={{{1, 1, &limb1}, {1, 1, &limb1}}};

}

const Rational& spec_object_traits<Rational>::zero()
{
   return *reverse_cast(mpq_zero_c, 0, &Rational::rep);
}

const Rational& spec_object_traits<Rational>::one()
{
   return *reverse_cast(mpq_one_c, 0, &Rational::rep);
}

}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
