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
#include "polymake/Array.h"
#include "polymake/group/named_groups.h"
#include "polymake/AccurateFloat.h"

namespace polymake { namespace group {

BigObject symmetric_group(Int n)
{
   return symmetric_group_impl(n);
}

BigObject alternating_group(Int n)
{
   if (n < 1) 
      throw std::runtime_error("alternating_group: the degree must be greater or equal than 1");

   BigObject pa("PermutationAction");

   if (n <= 3) {
      Array<Array<Int>> gens(1);
      Array<Int> gen(n);     
      for (Int j = 0; j < n-1; ++j) {
         gen[j] = j+1;
      }
      gen[n-1] = 0;
      gens[0] = gen; 
      pa.take("GENERATORS") << gens;

   } else {

      Array<Array<Int>> gens(2);
      Array<Int> gen0(n);
      for (Int j = 0; j < n; ++j)
         gen0[j] = j;
      gen0[0] = 1;
      gen0[1] = 2;
      gen0[2] = 0;
      gens[0] = gen0;

      Array<Int> gen1(n);
      Int mod = n%2;
      for (Int j = 1 - mod; j < n-1; ++j)
         gen1[j] = j+1;
      gen1[n-1] = 1-mod;
      gens[1] = gen1;
      pa.take("GENERATORS") << gens;
   }
   BigObject g("Group");
   g.take("PERMUTATION_ACTION") << pa;
   g.set_description() << "Alternating group of degree " << n << endl;
   return g;
}

BigObject cyclic_group(Int n)
{
   Array<Array<Int>> sgs(1);
   Array<Int> gen(n);
   for (Int j = 0; j < n; ++j)
      gen[j] = (j+1)%n;
   sgs[0] = gen;
   BigObject pa("PermutationAction");
   pa.take("GENERATORS") << sgs;

   BigObject g("Group");
   g.take("PERMUTATION_ACTION") << pa;
   g.set_description() << "Cyclic group of order " << n << endl;
   return g;
}

BigObject dihedral_group(Int n2)
{   
   return dihedral_group_impl(n2);
}


/****************************************************************
user functions
****************************************************************/

UserFunction4perl("# @category Producing a group"
		  "# Constructs a __symmetric group__ of given degree //d//."
		  "# @param Int d degree of the symmetric group"
                  "# @return Group",
                  &symmetric_group,"symmetric_group($)");

UserFunction4perl("# @category Producing a group"
		  "# Constructs an __alternating group__ of given degree //d//."
		  "# @param Int d degree of the alternating group"
                  "# @return Group",
                  &alternating_group,"alternating_group($)");


UserFunction4perl("# @category Producing a group"
		  "# Constructs a __cyclic group__ of given degree //d//."
		  "# @param Int d degree of the cyclic group"
                  "# @return Group",
                  &cyclic_group,"cyclic_group($)");

UserFunction4perl("# @category Producing a group"
		  "# Constructs a __dihedral group__ of a given order //o//."
                  "# If the order is 2, 4, 6, 8, 10, 12, 16, 20 or 24, the character table is exact,"
                  "# otherwise some entries are mutilated rational approximations of algebraic numbers."
		  "# @param Int o order of the dihedral group that acts on a regular //(o/2)//-gon"
                  "# @return Group",
                  &dihedral_group, "dihedral_group($)");

}
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

