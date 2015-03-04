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

#ifndef __POLYMAKE_MATROID_CHECK_BASIS_EXCHANGE_AXIOM_H__
#define __POLYMAKE_MATROID_CHECK_BASIS_EXCHANGE_AXIOM_H__

#include "polymake/Set.h"

namespace polymake { namespace matroid {

template<typename Container>
bool check_basis_exchange_axiom_impl(const Container& bases, bool verbose=false)
{
   Set<Set<int> > basis_set;
   for (typename Entire<Container>::const_iterator bit = entire(bases); !bit.at_end(); ++bit)
      basis_set += *bit; // have to do it like this so that the comparison tree gets built properly
   
   for (typename Entire<Container>::const_iterator bit1 = entire(bases); !bit1.at_end(); ++bit1) {
      typename Entire<Container>::const_iterator bit2 = bit1;
      for (++bit2; !bit2.at_end(); ++bit2) {
         const Set<int> 
            AmB = *bit1 - *bit2,
            BmA = *bit2 - *bit1;
         for (Entire<Set<int> >::const_iterator ambit = entire(AmB); !ambit.at_end(); ++ambit) {
            bool verified (false);
            for (Entire<Set<int> >::const_iterator bmait = entire(BmA); !verified && !bmait.at_end(); ++bmait) {
               verified = basis_set.contains(*bit1 - *ambit + *bmait);
            }
            if (!verified) {
               if (verbose) {
                  cout << "The given set of bases\n" << basis_set
                       << "\nis not a matroid.\nProof: A=" << *bit1 << ", B=" << *bit2 << "; A-B contains " << *ambit << ", B-A=" << BmA 
                       << "; but A - " << *ambit << " + b is not a basis for any b in " << BmA << endl;
               }
               return false;
            }
         }
      }
   }
   return true;
}



} }

#endif // __POLYMAKE_MATROID_CHECK_BASIS_EXCHANGE_AXIOM_H__

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
