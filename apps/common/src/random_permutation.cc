/* Copyright (c) 1997-2020
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
#include "polymake/RandomSubset.h"
#include "polymake/Array.h"

namespace polymake { namespace common {

Array<Int> rand_perm(const Int n, OptionSet options)
{
   const RandomSeed seed(options["seed"]);
   Array<Int> perm(n, random_permutation(n, seed).begin());
   return perm;
}


UserFunction4perl("# @category Utilities"
		  "# gives a random permutation"
		  "# @param Int n"
                  "# @option Int Seed"
		  "# @return Array<Int> random permutation",
		  &rand_perm,"rand_perm($ {seed=> undef})");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
