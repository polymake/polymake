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

#include "polymake/RandomGenerators.h"
#include "polymake/client.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>

namespace pm {

RandomSeed::RandomSeed(perl::Value v) :
   data(64, Integer::Reserve())
{
   if (!(v >> data)) renew();
}

int RandomSeed::rfd = -2;

void RandomSeed::renew()
{
   if (rfd==-2) rfd=open("/dev/urandom", O_RDONLY);
   if (rfd>=0) {
      if (data.fill_from_file(rfd)) return;
      rfd=-1;
   }
   static unsigned long counter=getpid();
   struct timeval tv;
   gettimeofday(&tv, nullptr);
   data=long(tv.tv_usec + (counter+=1000));
}

#if GMP_LIMB_BITS==32

/* Random functions in MPFR always request one GMP limb of random bits to fill the exponent.
   Because the limb size is equal to sizeof(long) which is different on 32-bit and 64-bit platforms,
   it leads to different random sequences generated for identical seed values.
   This is detrimental for unit testing.

   The remedy consists in intercepting all requests for exactly 32 bits and consuming 64 random bits instead.

   The following function table definition is copied from gmp-impl.h;
   unfortunately, it is classified as "implementation details" and hence not available via public header gmp.h

   Pseudo-random number generator function pointers structure.
*/
extern "C" struct gmp_randfnptr_t {
  void (*randseed_fn)(gmp_randstate_t, mpz_srcptr);
  void (*randget_fn)(gmp_randstate_t, mp_ptr, unsigned long int);
  void (*randclear_fn)(gmp_randstate_t);
  void (*randiset_fn)(gmp_randstate_t, const gmp_randstate_t);
  // added by ourselves:
  void (*randget_orig_fn)(gmp_randstate_t, mp_ptr, unsigned long int);
};

void RandomState::intercept_get_fn(gmp_randstate_t state_arg, mp_ptr limbs, unsigned long int bits)
{
   const gmp_randfnptr_t* const fn=reinterpret_cast<const gmp_randfnptr_t*>(state_arg[0]._mp_algdata._mp_lc);
   fn->randget_orig_fn(state_arg,limbs,bits);
   if (bits==32) {
//      const mp_limb_t ret=limbs[0];
      fn->randget_orig_fn(state_arg,limbs,bits);
//      limbs[0]=ret;
   }
}

void RandomState::fix_for_mpfr()
{
   gmp_randfnptr_t* &fn=reinterpret_cast<gmp_randfnptr_t* &>(state[0]._mp_algdata._mp_lc);
   static gmp_randfnptr_t mod_ftab={ fn->randseed_fn,
                                     &intercept_get_fn,
                                     fn->randclear_fn,
                                     fn->randiset_fn,
                                     fn->randget_fn };
   fn=&mod_ftab;
}

#endif // GMP_LIMB_BITS==32

void DiscreteRandom::normalize()
{
   double acc_sum = 0.;
   for (auto d = entire(distribution); !d.at_end(); ++d)
      *d = (acc_sum += *d);
   for (auto d = entire(distribution); !d.at_end(); ++d)
      *d /= acc_sum;
}

Int DiscreteRandom::get()
{
   const double r = rg.get();
   Vector<double>::const_iterator d = distribution.begin(), e = distribution.end();
   return std::lower_bound(d, e, r) - d;
}

} // end namespace pm

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
