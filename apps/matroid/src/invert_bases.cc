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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Integer.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace matroid {

// bases must not be empty
Array<Set <int> >invert_bases(const Array< Set<int> >& bases, int n_elements)
{
  const int rank=bases[0].size();
  Set< Set<int> > bases_set;
  for (auto i = entire(bases); !i.at_end(); ++i)
     bases_set.insert(*i);

  Array<Set<int>> inverted(int(Integer::binom(n_elements,rank))-bases.size());
  int j=0;
  for (Entire< Subsets_of_k<const sequence&> >::const_iterator i=entire(all_subsets_of_k(sequence(0,n_elements),rank)); !i.at_end(); ++i) {
     const Set<int> base = *i;
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
