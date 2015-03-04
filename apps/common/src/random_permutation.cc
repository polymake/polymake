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

#include "polymake/client.h"
#include "polymake/RandomSubset.h"
#include "polymake/Array.h"

namespace polymake { namespace common {

      Array<int> rand_perm(const int n, perl::OptionSet options)
{
   const RandomSeed seed(options["seed"]);
   Array<int> perm(n);
   copy(entire(random_permutation(n, seed)), perm.begin());
   return perm;
}


UserFunction4perl("# @category Utilities"
		  "# gives a random permutation matrix"
		  "# @param Int n"
		  "# @return Matrix : random n times n permutation matrix",
		  &rand_perm,"rand_perm($ {seed=> undef})");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
