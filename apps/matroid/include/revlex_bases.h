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

#ifndef __POLYMAKE_MATROID_REVLEX_BASES_H__
#define __POLYMAKE_MATROID_REVLEX_BASES_H__

#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/Integer.h"
#include "polymake/matroid/check_axioms.h"

namespace polymake { namespace matroid {

namespace {

template<typename Container>
bool revlex(const Container& a, const Container& b)
{
   assert (a.size() == b.size());
   for (typename Entire<Container>::const_reverse_iterator ait = rentire(a), bit = rentire(b); !ait.at_end(); ++ait, ++bit) {
      if (*ait < *bit) return true;
      if (*ait > *bit) return false;
   }
   return false;
}

} // end anonymous namespace

inline
Array<Set<int> > make_revlex_bases(int n, int r)
{
   Array<Set<int> > revlex_bases(Integer::binom(n,r).to_int());
   Entire<Array<Set<int> > >::iterator rit = entire(revlex_bases);
   for (Entire<Subsets_of_k<const sequence&> >::const_iterator bit(entire(all_subsets_of_k(sequence(0,n), r))); !bit.at_end(); ++bit, ++rit) 
      *rit = Set<int>(*bit);
   std::sort(revlex_bases.begin(), revlex_bases.end(), revlex<Set<int> >);
   return revlex_bases;
}

template<typename Container>
std::string bases_to_revlex_encoding_impl(const Container& bases,
                                          int rank,
                                          int n_elements)
{
   Set<Set<int> > bases_set;
   for (typename Entire<Container>::const_iterator ait = entire(bases); !ait.at_end(); ++ait)
      bases_set += *ait; // must do this instead of "Set<Set<int> > bases_set(entire(bases));" to get the Set tree right and enable searching

   const Array<Set<int> > revlex_bases = make_revlex_bases(n_elements, rank);
   std::string encoding;
   for (Entire<Array<Set<int> > >::const_iterator bit = entire(revlex_bases); !bit.at_end(); ++bit) 
      encoding += (bases_set.contains(*bit) 
                   ? '*' 
                   : '0');

   return encoding;
}

template<typename Container>
Array<Set<int> > bases_from_revlex_encoding_impl(const Container& revlex_encoding,
                                                 int rank,
                                                 int n_elements,
                                                 bool dual  = false,
                                                 bool check = false)
{
   const Array<Set<int> > revlex_bases = make_revlex_bases(n_elements, rank);
   Array<Set<int> > matroid_bases(std::count(revlex_encoding.begin(), revlex_encoding.end(), '*') +
                                  std::count(revlex_encoding.begin(), revlex_encoding.end(), '1'));

   Entire<Array<Set<int> > >::iterator mbit = entire(matroid_bases);
   Entire<Array<Set<int> > >::const_iterator rbit = entire(revlex_bases);

   for (typename Entire<Container>::const_iterator sit = entire(revlex_encoding); !sit.at_end(); ++sit, ++rbit) {
      if (*sit == '*' || *sit == '1') {
         *mbit = (dual 
                  ? Set<int>(sequence(0,n_elements) - *rbit)
                  : *rbit);
         ++mbit;
      }
   }

   if (check && !check_basis_exchange_axiom_impl(matroid_bases, true))
      throw std::runtime_error("The given revlex string did not correspond to a matroid.");

   return matroid_bases;
}

} }

#endif // __POLYMAKE_MATROID_REVLEX_BASES_H__

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
