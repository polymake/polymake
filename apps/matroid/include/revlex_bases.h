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

#pragma once

#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/Integer.h"
#include "polymake/matroid/check_axioms.h"

namespace polymake { namespace matroid {

namespace {

template <typename Container>
bool revlex(const Container& a, const Container& b)
{
   assert(a.size() == b.size());
   for (auto ait = entire<reversed>(a), bit = entire<reversed>(b); !ait.at_end(); ++ait, ++bit) {
      if (*ait < *bit) return true;
      if (*ait > *bit) return false;
   }
   return false;
}

} // end anonymous namespace

// TODO: replace with backward traversal of Subsets_of_k when implemented
inline
Array<Set<Int>> make_revlex_bases(Int n, Int r)
{
   Array<Set<Int>> revlex_bases(Int(Integer::binom(n,r)));
   auto rit = entire(revlex_bases);
   for (auto bit(entire(all_subsets_of_k(sequence(0,n), r))); !bit.at_end(); ++bit, ++rit)
      *rit = Set<Int>(*bit);
   std::sort(revlex_bases.begin(), revlex_bases.end(), revlex<Set<Int>>);
   return revlex_bases;
}

template <typename Container>
std::string bases_to_revlex_encoding_impl(const Container& bases,
                                          Int rank,
                                          Int n_elements)
{
   Set<Set<Int>> bases_set;
   for (auto ait = entire(bases); !ait.at_end(); ++ait)
      bases_set += *ait; // must do this instead of "Set<Set<Int> > bases_set(entire(bases));" to get the Set tree right and enable searching

   const Array<Set<Int>> revlex_bases = make_revlex_bases(n_elements, rank);
   std::string encoding;
   for (auto bit = entire(revlex_bases); !bit.at_end(); ++bit)
      encoding += (bases_set.contains(*bit) ? '*' : '0');

   return encoding;
}

template <typename Container>
Array<Set<Int>> bases_from_revlex_encoding_impl(const Container& revlex_encoding,
                                                Int rank,
                                                Int n_elements,
                                                bool dual  = false,
                                                bool check = false)
{
   const Array<Set<Int>> revlex_bases = make_revlex_bases(n_elements, rank);
   Array<Set<Int>> matroid_bases(std::count(revlex_encoding.begin(), revlex_encoding.end(), '*') +
                                 std::count(revlex_encoding.begin(), revlex_encoding.end(), '1'));

   auto mbit = entire(matroid_bases);
   auto rbit = entire(revlex_bases);

   for (auto sit = entire(revlex_encoding); !sit.at_end(); ++sit, ++rbit) {
      if (*sit == '*' || *sit == '1') {
         *mbit = (dual
                  ? Set<Int>(sequence(0,n_elements) - *rbit)
                  : *rbit);
         ++mbit;
      }
   }

   if (check && !check_basis_exchange_axiom_impl(matroid_bases, true))
      throw std::runtime_error("The given revlex string did not correspond to a matroid.");

   return matroid_bases;
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
