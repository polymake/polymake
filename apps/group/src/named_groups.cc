/* Copyright (c) 1997-2014
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

namespace polymake { namespace group {

perl::Object symmetric_group(int n){
   perl::Object g("Group");
   if ( n < 1) 
      throw std::runtime_error("symmetric_group: the degree must be greater or equal than 1");
   
   Array< Array<int> > sgs(n-1);
   for (int i = 0; i < n-1; ++i) {
      Array<int> gen(n);
      for (int j = 0; j < n; ++j)
         gen[j] = j;
      std::swap(gen[i], gen[i+1]);
      sgs[i] = gen;
   }
   g.take("GENERATORS") << sgs;
   g.set_description() << "Symmetric group of degree " << n << endl;
   return g;
}

perl::Object alternating_group(int n){
   perl::Object g("Group");
   if ( n < 1) 
      throw std::runtime_error("alternating_group: the degree must be greater or equal than 1");

   if ( n <= 3) {

      Array< Array<int> > gens(1);
      Array<int> gen(n);     
      for (int j = 0; j < n-1; ++j) {
         gen[j] = j+1;
      }
      gen[n-1] = 0;
      gens[0] = gen; 
      g.take("GENERATORS") << gens;

   } else {

      Array< Array<int> > gens(2);
      Array<int> gen0(n);
      for (int j = 0; j < n; ++j)
         gen0[j] = j;
      gen0[0] = 1;
      gen0[1] = 2;
      gen0[2] = 0;
      gens[0] = gen0;

      Array<int> gen1(n);
      int mod = n % 2;
      for (int j = 1-mod; j < n-1; ++j)
         gen1[j] = j+1;
      gen1[n-1] = 1-mod;
      gens[1] = gen1;
      g.take("GENERATORS") << gens;
   }
   g.set_description() << "Alternating group of degree " << n << endl;
   return g;
}

perl::Object cyclic_group(int n){
   perl::Object g("Group");
   Array< Array<int> > sgs(1);
   Array<int> gen(n);
   for (int j = 0; j < n; ++j)
      gen[j] = (j + 1) % n;
   sgs[0] = gen;
   g.take("GENERATORS") << sgs;
   g.set_description() << "Cyclic group of order " << n << endl;
   return g;
}


/****************************************************************
user functions
****************************************************************/

UserFunction4perl("# @category Symmetry"
		  "# Constructs a symmetric group of given //degree//."
		  "# @param int degree of the symmetric group"
                  "# @return Group",
                  &symmetric_group,"symmetric_group($)");

UserFunction4perl("# @category Symmetry"
		  "# Constructs an alternating group of given //degree//."
		  "# @param int degree of the alternating group"
                  "# @return Group",
                  &alternating_group,"alternating_group($)");


UserFunction4perl("# @category Symmetry"
		  "# Constructs a cyclic group of given //degree//."
		  "# @param int degree of the cyclic group"
                  "# @return Group",
                  &cyclic_group,"cyclic_group($)");


}
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

