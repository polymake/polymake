/* Copyright (c) 1997-2021
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

#include "polymake/Bitset.h"

namespace pm {

mp_bitcnt_t Bitset_iterator_base::last_pos(mpz_srcptr bits)
{
   const int n = bits[0]._mp_size;
   return n == 0 ? end_pos : (n-1)*bits_per_limb + log2_floor(mpz_getlimbn(bits, n-1));
}

void Bitset_iterator_base::prev_pos()
{
   if (cur == end_pos) {
      cur = first_pos(bits);
   } else if (cur == 0) {
      cur = end_pos;
   } else {
      --cur;
      mp_bitcnt_t n = cur / bits_per_limb;
      const unsigned int skip = bits_per_limb-1-cur%bits_per_limb;
      mp_limb_t limb = (mpz_getlimbn(bits, n) << skip) >> skip;
      for (;;) {
         if (limb != 0) {
            cur = n*bits_per_limb + log2_floor(limb);
            break;
         }
         if (n == 0) {
            cur = end_pos;
            break;
         }
         --n;
         limb = mpz_getlimbn(bits, n);
      }
   }
}

void Bitset::difference(mpz_ptr dst, mpz_srcptr src1, mpz_srcptr src2)
{
   const mp_limb_t* s2 = src2->_mp_d;
   if (dst == src1) {
      // assignment
      mp_limb_t* d = dst->_mp_d;

      if (dst->_mp_size <= src2->_mp_size) {
         // lhs can get shortened
         mp_limb_t* const end = d + dst->_mp_size;
         mp_limb_t* non0 = d;
         while (d < end)
            if ((*d++ &= ~(*s2++))) non0 = d;
         dst->_mp_size = static_cast<int>(non0 - dst->_mp_d);
      } else {
         const mp_limb_t* const end = s2 + src2->_mp_size;
         while (s2 < end)
            (*d++) &= ~(*s2++);
      }
   } else {
      _mpz_realloc(dst, src1->_mp_size);
      mp_limb_t* d = dst->_mp_d;
      const mp_limb_t* s1 = src1->_mp_d;

      if (src1->_mp_size <= src2->_mp_size) {
         const mp_limb_t* const end = s1 + src1->_mp_size;
         mp_limb_t* non0 = d;
         while (s1 < end)
            if ((*d++ = (*s1++) & ~(*s2++))) non0 = d;
         dst->_mp_size = static_cast<int>(non0 - dst->_mp_d);
      } else {
         const mp_limb_t* const end2 = s2 + src2->_mp_size;
         mp_limb_t* const end = d + (dst->_mp_size = src1->_mp_size);
         while (s2 < end2)
            *d++ = (*s1++) & ~(*s2++);
         while (d < end)
            *d++ = *s1++;
      }
   }
}

void Bitset::fill1s(size_t n)
{
   mpz_ptr dst = rep;
   const size_t n_limbs = (n+iterator::bits_per_limb-1) / iterator::bits_per_limb;
   dst->_mp_size = static_cast<int>(n_limbs);
   mp_limb_t* l = dst->_mp_d;
   mp_limb_t* l_top = l-1+n_limbs;
   mp_limb_t ones = mp_limb_t(mp_limb_signed_t(-1));      // take care of sign expansion
#if GMP_NAIL_BITS
   ones &= GMP_NUMB_MASK;
#endif
   for (; l < l_top; ++l)
      *l = ones;
   *l = ones >> (n_limbs * iterator::bits_per_limb - n);
}

void Bitset::fill1s(const sequence& s)
{
   if (!s.empty()) {
      reserve(s.back()+1);
      fill1s(s.size());
      if (s.front() > 0)
         mpz_mul_2exp(rep, rep, s.front());
   }
}

Int incl(const Bitset& s1, const Bitset& s2) noexcept
{
   mpz_srcptr rep1 = s1.get_rep(), rep2 = s2.get_rep();
   const int size1 = rep1->_mp_size;
   const int size2 = rep2->_mp_size;
   Int result = sign(size1 - size2);
   for (const mp_limb_t *e1 = rep1->_mp_d, *e2 = rep2->_mp_d, * const stop = e1 + std::min(size1, size2);
        e1 != stop;
        ++e1, ++e2) {
      const mp_limb_t intersect = *e1 & *e2;
      if (*e1 != intersect) {
         if (result < 0) return 2;
         result = 1;
      }
      if (*e2 != intersect) {
         if (result > 0) return 2;
         result = -1;
      }
   }
   return result;
}

}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
