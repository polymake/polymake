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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Integer.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace matroid {

// bases must not be empty
Array<Set<Int>> invert_bases(const Array<Set<Int>>& bases, Int n_elements)
{
  const Int rank = bases[0].size();
  Set<Set<Int>> bases_set;
  for (auto i = entire(bases); !i.at_end(); ++i)
     bases_set.insert(*i);

  Array<Set<Int>> inverted(Int(Integer::binom(n_elements,rank)) - bases.size());
  Int j = 0;
  for (auto i=entire(all_subsets_of_k(sequence(0,n_elements), rank)); !i.at_end(); ++i) {
     const Set<Int> base = *i;
     if (!bases_set.contains(base)) inverted[j++]=base;
  }

  return inverted;
}

Function4perl(&invert_bases, "invert_bases");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
